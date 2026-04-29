/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sortingrenderer.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "dx8wrapper.h"
#include "bgfxrenderer.h"
#include "vertmaterial.h"
#include "texture.h"
#include "matrix4.h"
#include "statistics.h"
#include "wwperfmon.h"
#include <wwprofile.h>

#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>

bool SortingRendererClass::_EnableTriangleDraw=true;

struct ShortVectorIStruct
{
	unsigned short i;
	unsigned short j;
	unsigned short k;

	ShortVectorIStruct(unsigned short i_,unsigned short j_,unsigned short k_) : i(i_),j(j_),k(k_) {}
	ShortVectorIStruct() {}
};

struct TempIndexStruct
{
	ShortVectorIStruct tri;
	unsigned short idx;

	TempIndexStruct() {}
	TempIndexStruct(const ShortVectorIStruct& tri_, unsigned short idx_)
		:
		tri(tri_),
		idx(idx_)
	{
	}
};

// ----------------------------------------------------------------------------
//
// InsertionSort (T* array, K *keys, int l, int r)
// Performs insertion sort on array 'array' elements [l-r]. Uses values from array
// 'keys' as sort keys.
//
// ----------------------------------------------------------------------------

template <class T, class K>
void InsertionSort (
	T* array,		// array to sort
	K* keys,			// sort keys
	int l,			//	first item
	int r)			//	last item
{
	for (int i = l+1; i < r; i++) {
		K v=keys[i];
		T tv=array[i];
		int j=i;

		while (keys[j-1] > v) {
			keys[j]=keys[j-1];
			array[j]=array[j-1];
			j--;
			if (j == l) break;
		};
		keys[j]=v;
		array[j]=tv;
	}
}

// ----------------------------------------------------------------------------
//
//	QuickSort (T* array, K* a, int l, int r)
//
//	Performs quicksort on array 'array'. Uses values from array 'keys' as sort keys.
//
// Once the length of the array to be sorted is less than 8, the routine calls
// InsertionSort() to perform the actual sorting work.
//
// ----------------------------------------------------------------------------

template <class T, class K>
void QuickSort (
	T* array,		//	array to sort
	K* keys,			// sort keys
	int l,			// first element
	int r)			// last element
{
	if (r-l <= 8) {
		InsertionSort(array,keys,l,r+1);
		return;
	}

	K t;
	K v=keys[r];
	T ttemp;
	int i=l-1;
	int j=r;

	do {
		do { i++; } while (i<r && keys[i]<v);
		do { j--; } while (j>0 && keys[j]>v);
		
		WWASSERT(j>=0);
		WWASSERT(i<=r);

		ttemp=array[i]; array[i]=array[j]; array[j]=ttemp;
		t=keys[i]; keys[i]=keys[j]; keys[j]=t;
	} while (j>i);

	array[j]=array[i];
	array[i]=array[r];
	array[r]=ttemp;
	keys[j]=keys[i];
	keys[i]=keys[r];
	keys[r]=t;
	
	if (i-1>l) QuickSort(array,keys,l,i-1);
	if (r>i+1) QuickSort(array,keys,i+1,r);
}

// ----------------------------------------------------------------------------
//
// Sorts and array. Uses values from array 'keys' as sort keys.
//
// ----------------------------------------------------------------------------

template <class T, class K>
void Sort (
	T* array,		// array to sort
	K *keys,	// sort keys
	int count)		// array element count
{
	bool do_insertion = false;

	if (count<=1) return;									// only one element.. return..

	int c=0;														// count number of rise pairs
	int i;
	for (i = 1; i < count; i++)
	if (keys[i] >= keys[i-1]) c++;

	if (c+1 == count) return;								// array already sorted
	if (c<50) do_insertion=true;							// array smaller than 50 should use insertion sort

	if (c<count/3) {											// if array is not rising
		T tmp;
		K tval;

		for (i=0;i<count/2;i++) {
			int neg = count-1-i;

			tmp=array[i]; array[i]=array[neg]; array[neg]=tmp;
			tval=keys[i]; keys[i] = keys[neg]; keys[neg]=tval;
		}

		if (!c) return;

		do_insertion = true;
	}
	if (do_insertion) InsertionSort(array,keys,0,count);
	else QuickSort(array,keys,0,count-1);			// quick sort
}

// ----------------------------------------------------------------------------

struct SortingNodeStruct : DLNodeClass<SortingNodeStruct>
{
	WW3D::FixedFunctionSubmitDesc Submission;
	WW3D::FixedFunctionStateDesc FixedFunctionState;
	WW3D::LightingSubmitDesc CapturedLighting;
	WW3D::SubmitLightDesc CapturedLights[WW3D::MAX_SUBMIT_LIGHTS];

	SphereClass bounding_sphere;

	Vector3 transformed_center;
	float transformed_depth_radius;
	unsigned short start_index;			// First index used in the ib
	unsigned short polygon_count;			// Polygon count to process (3 indices = one polygon)
	unsigned short min_vertex_index;		// First index used in the vb
	unsigned short vertex_count;			// Number of vertices used in vb
};

static DLListClass<SortingNodeStruct> sorted_list;
static DLListClass<SortingNodeStruct> clean_list;
static unsigned total_sorting_vertices;

static SortingNodeStruct* Get_Sorting_Struct()
{

	SortingNodeStruct* state=clean_list.Head();
	if (state) {
		state->Remove();
		return state;
	}
	state=new SortingNodeStruct();
	return state;
}

static void Add_Submission_Refs(const WW3D::FixedFunctionSubmitDesc &submission)
{
	if (submission.VertexBuffer != NULL) {
		submission.VertexBuffer->Add_Engine_Ref();
	}
	if (submission.IndexBuffer != NULL) {
		submission.IndexBuffer->Add_Engine_Ref();
	}
	if (submission.Material != NULL) {
		submission.Material->Add_Ref();
	}
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		if (submission.Textures[i] != NULL) {
			submission.Textures[i]->Add_Ref();
		}
	}
}

static void Release_Submission_Refs(const WW3D::FixedFunctionSubmitDesc &submission)
{
	if (submission.VertexBuffer != NULL) {
		submission.VertexBuffer->Release_Engine_Ref();
	}
	if (submission.IndexBuffer != NULL) {
		submission.IndexBuffer->Release_Engine_Ref();
	}
	if (submission.Material != NULL) {
		submission.Material->Release_Ref();
	}
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		if (submission.Textures[i] != NULL) {
			submission.Textures[i]->Release_Ref();
		}
	}
}

// ----------------------------------------------------------------------------
//
// Temporary arrays for the sorting system
//
// ----------------------------------------------------------------------------

static float* vertex_z_array;
static float* polygon_z_array;
static unsigned * node_id_array;
static unsigned * sorted_node_id_array;
static ShortVectorIStruct* polygon_index_array;
static unsigned vertex_z_array_count;
static unsigned polygon_z_array_count;
static unsigned node_id_array_count;
static unsigned sorted_node_id_array_count;
static unsigned polygon_index_array_count;
TempIndexStruct* temp_index_array;
unsigned temp_index_array_count;

static float Compute_Depth_Radius(const SphereClass& bounding_sphere, const Matrix4& world_view, const Vector3& center);

static TempIndexStruct* Get_Temp_Index_Array(unsigned count)
{
	if (count>temp_index_array_count) {
		delete[] temp_index_array;
		temp_index_array=new TempIndexStruct[count];
		temp_index_array_count=count;
	}
	return temp_index_array;
}

static float* Get_Vertex_Z_Array(unsigned count)
{
	if (count>vertex_z_array_count) {
		delete[] vertex_z_array;
		vertex_z_array=new float[count];
		vertex_z_array_count=count;
	}
	return vertex_z_array;
}

static float* Get_Polygon_Z_Array(unsigned count)
{
	if (count>polygon_z_array_count) {
		delete[] polygon_z_array;
		polygon_z_array=new float[count];
		polygon_z_array_count=count;
	}
	return polygon_z_array;
}

static unsigned * Get_Node_Id_Array(unsigned count)
{
	if (count>node_id_array_count) {
		delete[] node_id_array;
		node_id_array=new unsigned[count];
		node_id_array_count=count;
	}
	return node_id_array;
}

static unsigned * Get_Sorted_Node_Id_Array(unsigned count)
{
	if (count>sorted_node_id_array_count) {
		delete[] sorted_node_id_array;
		sorted_node_id_array=new unsigned[count];
		sorted_node_id_array_count=count;
	}
	return sorted_node_id_array;
}

static ShortVectorIStruct* Get_Polygon_Index_Array(unsigned count)
{
	if (count>polygon_index_array_count) {
		delete[] polygon_index_array;
		polygon_index_array=new ShortVectorIStruct[count];
		polygon_index_array_count=count;
	}
	return polygon_index_array;
}


// ----------------------------------------------------------------------------
//
// Insert triangles to the sorting system.
//
// ----------------------------------------------------------------------------

void SortingRendererClass::Insert_Triangles_Internal(
	const SphereClass& bounding_sphere,
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count,
	bool use_explicit_shadow_flags,
	bool receive_shadows,
	bool cast_shadows)
{
	if (!WW3D::Is_Sorting_Enabled()) {
		if (use_explicit_shadow_flags) {
			BgfxRenderer::Submit_Current_Fixed_Function_Triangles(
				start_index,
				polygon_count,
				min_vertex_index,
				vertex_count,
				receive_shadows,
				cast_shadows);
		} else {
			BgfxRenderer::Submit_Current_Fixed_Function_Triangles(
				start_index,
				polygon_count,
				min_vertex_index,
				vertex_count);
		}
		return;
	}

	DX8_RECORD_SORTING_RENDER(polygon_count,vertex_count);

	RenderStateStruct render_state;
	DX8Wrapper::Get_Render_State(render_state);
	if (render_state.vertex_buffer == NULL || render_state.index_buffer == NULL) {
		return;
	}

	Matrix4 projection(true);
	DX8Wrapper::Get_Transform(D3DTS_PROJECTION, projection);

	WW3D::FixedFunctionSubmitDesc submission;
	submission.VertexBuffer = render_state.vertex_buffer;
	submission.VertexBufferOffset = render_state.vba_offset;
	submission.IndexBuffer = render_state.index_buffer;
	submission.IndexBufferOffset = render_state.iba_offset;
	submission.IndexBaseOffset = render_state.index_base_offset;
	submission.StartIndex = start_index;
	submission.PolygonCount = polygon_count;
	submission.MinVertexIndex = min_vertex_index;
	submission.VertexCount = vertex_count;
	submission.Textures[0] = render_state.Textures[0];
	submission.Textures[1] = render_state.Textures[1];
	submission.Material = render_state.material;
	submission.Shader = render_state.shader;
	submission.WorldTransform = render_state.world;
	submission.ViewTransform = render_state.view;
	submission.ProjectionTransform = projection;
	submission.ReceiveShadows =
		use_explicit_shadow_flags
			? receive_shadows
			: render_state.shader.Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_ZERO;
	submission.CastShadows = use_explicit_shadow_flags ? cast_shadows : false;
	submission.Strip = false;
	Insert_Fixed_Function_Draw(bounding_sphere, submission);
}

void SortingRendererClass::Insert_Triangles(
	const SphereClass& bounding_sphere,
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	Insert_Triangles_Internal(
		bounding_sphere,
		start_index,
		polygon_count,
		min_vertex_index,
		vertex_count,
		false,
		false,
		false);
}

void SortingRendererClass::Insert_Triangles(
	const SphereClass& bounding_sphere,
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count,
	bool receive_shadows,
	bool cast_shadows)
{
	Insert_Triangles_Internal(
		bounding_sphere,
		start_index,
		polygon_count,
		min_vertex_index,
		vertex_count,
		true,
		receive_shadows,
		cast_shadows);
}

// ----------------------------------------------------------------------------
//
// Insert triangles to the sorting system, with no bounding information.
//
// ----------------------------------------------------------------------------

void SortingRendererClass::Insert_Triangles(
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	SphereClass sphere(Vector3(0.0f,0.0f,0.0f),0.0f);
	Insert_Triangles(sphere,start_index,polygon_count,min_vertex_index,vertex_count);
}

void SortingRendererClass::Insert_Triangles(
	unsigned short start_index, 
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count,
	bool receive_shadows,
	bool cast_shadows)
{
	SphereClass sphere(Vector3(0.0f,0.0f,0.0f),0.0f);
	Insert_Triangles(
		sphere,
		start_index,
		polygon_count,
		min_vertex_index,
		vertex_count,
		receive_shadows,
		cast_shadows);
}

void SortingRendererClass::Insert_Fixed_Function_Draw(
	const SphereClass& bounding_sphere,
	const WW3D::FixedFunctionSubmitDesc &submission)
{
	if (submission.VertexBuffer == NULL || submission.IndexBuffer == NULL) {
		return;
	}

	SortingNodeStruct *state = Get_Sorting_Struct();
	state->Submission = submission;
	state->FixedFunctionState =
		submission.RenderState != NULL
			? *submission.RenderState
			: WW3D::FixedFunctionStateDesc();
	if (submission.RenderState == NULL) {
		WW3D::Capture_Current_Fixed_Function_State(state->FixedFunctionState, submission.Material);
	}
	state->Submission.RenderState = &state->FixedFunctionState;

	if (submission.Lighting != NULL) {
		state->Submission.Lighting = submission.Lighting;
	} else {
		WW3D::Capture_Current_Lighting_Submission(
			state->CapturedLighting,
			state->CapturedLights,
			WW3D::MAX_SUBMIT_LIGHTS);
		state->Submission.Lighting = &state->CapturedLighting;
	}

	Add_Submission_Refs(state->Submission);

	state->bounding_sphere = bounding_sphere;
	state->start_index = state->Submission.StartIndex;
	state->polygon_count = state->Submission.PolygonCount;
	state->min_vertex_index = state->Submission.MinVertexIndex;
	state->vertex_count = state->Submission.VertexCount;

	const VertexBufferClass *vertex_buffer = state->Submission.VertexBuffer;
	WWASSERT(vertex_buffer != NULL);
	WWASSERT(state->vertex_count <= vertex_buffer->Get_Vertex_Count());

	Matrix4 mtx = state->Submission.WorldTransform * state->Submission.ViewTransform;
	Vector4 transformed_vec;
	Matrix4::Transform_Vector(mtx, state->bounding_sphere.Center, &transformed_vec);
	state->transformed_center = Vector3(transformed_vec[0], transformed_vec[1], transformed_vec[2]);
	state->transformed_depth_radius = Compute_Depth_Radius(state->bounding_sphere, mtx, state->transformed_center);

	SortingNodeStruct* node = sorted_list.Head();
	while (node) {
		if (state->transformed_center.Z > node->transformed_center.Z) {
			if (sorted_list.Head() == sorted_list.Tail())
				sorted_list.Add_Head(state);
			else
				state->Insert_Before(node);
			break;
		}
		node = node->Succ();
	}
	if (!node) {
		sorted_list.Add_Tail(state);
	}
}

// ----------------------------------------------------------------------------
//
// Flush all sorting polygons.
//
// ----------------------------------------------------------------------------

void Release_Refs(SortingNodeStruct* state)
{
	Release_Submission_Refs(state->Submission);
}

static unsigned overlapping_node_count;
static unsigned overlapping_polygon_count;
static unsigned overlapping_vertex_count;
const unsigned MAX_OVERLAPPING_NODES=4096;
static SortingNodeStruct* overlapping_nodes[MAX_OVERLAPPING_NODES];
static bool sorting_pool_depth_valid;
static float sorting_pool_depth_min;
static float sorting_pool_depth_max;

// ----------------------------------------------------------------------------

static double Perf_Ticks_To_Milliseconds(Uint64 ticks)
{
	const Uint64 frequency = SDL_GetPerformanceFrequency();
	if (ticks == 0 || frequency == 0) {
		return 0.0;
	}

	return static_cast<double>(ticks) * 1000.0 / static_cast<double>(frequency);
}

static float Compute_Depth_Radius(const SphereClass& bounding_sphere, const Matrix4& world_view, const Vector3& center)
{
	Vector4 transformed_point;
	float depth_radius = 0.0f;
	const float radius = bounding_sphere.Radius;

	Matrix4::Transform_Vector(world_view, bounding_sphere.Center + Vector3(radius, 0.0f, 0.0f), &transformed_point);
	depth_radius = std::max(depth_radius, static_cast<float>(std::fabs(transformed_point[2] - center.Z)));

	Matrix4::Transform_Vector(world_view, bounding_sphere.Center + Vector3(0.0f, radius, 0.0f), &transformed_point);
	depth_radius = std::max(depth_radius, static_cast<float>(std::fabs(transformed_point[2] - center.Z)));

	Matrix4::Transform_Vector(world_view, bounding_sphere.Center + Vector3(0.0f, 0.0f, radius), &transformed_point);
	depth_radius = std::max(depth_radius, static_cast<float>(std::fabs(transformed_point[2] - center.Z)));

	return depth_radius;
}

static bool Uses_Sorting_Pool(const SortingNodeStruct* state)
{
	return
		(state->Submission.IndexBuffer != NULL) &&
		(state->Submission.VertexBuffer != NULL) &&
		(state->Submission.IndexBuffer->Type() == BUFFER_TYPE_SORTING || state->Submission.IndexBuffer->Type() == BUFFER_TYPE_DYNAMIC_SORTING) &&
		(state->Submission.VertexBuffer->Type() == BUFFER_TYPE_SORTING || state->Submission.VertexBuffer->Type() == BUFFER_TYPE_DYNAMIC_SORTING);
}

static bool Overlaps_Current_Sorting_Pool(const SortingNodeStruct* state)
{
	if (!sorting_pool_depth_valid || overlapping_node_count == 0) {
		return false;
	}

	const float state_depth_max = state->transformed_center.Z + state->transformed_depth_radius;
	return state_depth_max >= sorting_pool_depth_min;
}

// ----------------------------------------------------------------------------

void SortingRendererClass::Insert_To_Sorting_Pool(SortingNodeStruct* state)
{
	if (overlapping_node_count>=MAX_OVERLAPPING_NODES) {
		Release_Refs(state);
		WWASSERT(0);
		return;
	}

	overlapping_nodes[overlapping_node_count]=state;
	overlapping_vertex_count+=state->vertex_count;
	overlapping_polygon_count+=state->polygon_count;
	overlapping_node_count++;

	const float state_depth_min = state->transformed_center.Z - state->transformed_depth_radius;
	const float state_depth_max = state->transformed_center.Z + state->transformed_depth_radius;
	if (!sorting_pool_depth_valid) {
		sorting_pool_depth_valid = true;
		sorting_pool_depth_min = state_depth_min;
		sorting_pool_depth_max = state_depth_max;
	} else {
		sorting_pool_depth_min = std::min(sorting_pool_depth_min, state_depth_min);
		sorting_pool_depth_max = std::max(sorting_pool_depth_max, state_depth_max);
	}
}

void SortingRendererClass::Flush_Sorting_Pool()
{
	if (!overlapping_node_count) return;

	SNAPSHOT_SAY(("SortingSystem - Flush \n"));

	const unsigned batch_node_count = overlapping_node_count;
	const unsigned batch_polygon_count = overlapping_polygon_count;
	const unsigned batch_vertex_count = overlapping_vertex_count;
	const Uint64 vertex_copy_start = SDL_GetPerformanceCounter();
	unsigned node_id;
	// Fill dynamic index buffer with sorting index buffer vertices
	unsigned * node_id_array=Get_Node_Id_Array(overlapping_polygon_count);
	float* polygon_z_array=Get_Polygon_Z_Array(overlapping_polygon_count);
	ShortVectorIStruct* polygon_idx_array=(ShortVectorIStruct*)Get_Polygon_Index_Array(overlapping_polygon_count);

	DynamicVBAccessClass dyn_vb_access(BUFFER_TYPE_DYNAMIC_RENDER,dynamic_vertex_format,overlapping_vertex_count);
	{
		DynamicVBAccessClass::WriteLockClass lock(&dyn_vb_access);
		VertexFormatXYZNDUV2* dest_verts=lock.Get_Formatted_Vertex_Array();

		unsigned polygon_array_offset=0;
		unsigned vertex_array_offset=0;
		for (node_id=0;node_id<overlapping_node_count;++node_id) {
			SortingNodeStruct* state=overlapping_nodes[node_id];
			float* vertex_z_array=Get_Vertex_Z_Array(state->vertex_count);

			VertexFormatXYZNDUV2* src_verts=NULL;
			SortingVertexBufferClass* vertex_buffer=static_cast<SortingVertexBufferClass*>(const_cast<VertexBufferClass*>(state->Submission.VertexBuffer));
			WWASSERT(vertex_buffer);
			src_verts=vertex_buffer->VertexBuffer;
			WWASSERT(src_verts);
			src_verts+=state->Submission.VertexBufferOffset;
			src_verts+=state->Submission.IndexBaseOffset;
			src_verts+=state->min_vertex_index;

			const Matrix4 mtx = (state->Submission.WorldTransform * state->Submission.ViewTransform).Transpose();
			for (unsigned i=0;i<state->vertex_count;++i,++src_verts) {
				vertex_z_array[i] = (mtx[2][0] * src_verts->x + mtx[2][1] * src_verts->y + mtx[2][2] * src_verts->z + mtx[2][3]);

				//
				// If you have a crash in here and "dest_verts" points to illegal memory area,
				// it is because D3D is in illegal state, and the only known cure is rebooting.
				// This illegal state is usually caused by Quake3-engine powered games such as MOHAA.

				*dest_verts++=*src_verts;
			}

			unsigned short* indices=NULL;
			SortingIndexBufferClass* index_buffer=static_cast<SortingIndexBufferClass*>(const_cast<IndexBufferClass*>(state->Submission.IndexBuffer));
			WWASSERT(index_buffer);
			indices=index_buffer->index_buffer;
			WWASSERT(indices);
			indices+=state->start_index;
			indices+=state->Submission.IndexBufferOffset;

			for (unsigned i = 0; i < state->polygon_count; ++i) {
				unsigned short idx1=indices[i*3]-state->min_vertex_index;
				unsigned short idx2=indices[i*3+1]-state->min_vertex_index;
				unsigned short idx3=indices[i*3+2]-state->min_vertex_index;
				WWASSERT(idx1<state->vertex_count);
				WWASSERT(idx2<state->vertex_count);
				WWASSERT(idx3<state->vertex_count);
				float z1=vertex_z_array[idx1];
				float z2=vertex_z_array[idx2];
				float z3=vertex_z_array[idx3];
				float z=(z1+z2+z3)/3.0f;
				unsigned array_index=i+polygon_array_offset;
				WWASSERT(array_index<overlapping_polygon_count);
				polygon_z_array[array_index]=z;
				node_id_array[array_index]=node_id;
				polygon_idx_array[array_index]=ShortVectorIStruct(
					idx1+vertex_array_offset,
					idx2+vertex_array_offset,
					idx3+vertex_array_offset);
			}

			state->min_vertex_index=vertex_array_offset;
			state->Submission.MinVertexIndex=static_cast<unsigned short>(vertex_array_offset);

			polygon_array_offset+=state->polygon_count;
			vertex_array_offset+=state->vertex_count;

		}
	}
	const double vertex_copy_ms = Perf_Ticks_To_Milliseconds(SDL_GetPerformanceCounter() - vertex_copy_start);

	TempIndexStruct* tis=Get_Temp_Index_Array(overlapping_polygon_count);
	for (unsigned a=0;a<overlapping_polygon_count;++a) {
		tis[a]=TempIndexStruct(polygon_idx_array[a],node_id_array[a]);
	}
	const Uint64 sort_start = SDL_GetPerformanceCounter();
	Sort<TempIndexStruct,float>(tis,polygon_z_array,overlapping_polygon_count);
	const double sort_ms = Perf_Ticks_To_Milliseconds(SDL_GetPerformanceCounter() - sort_start);

	const Uint64 index_copy_start = SDL_GetPerformanceCounter();
	DynamicIBAccessClass dyn_ib_access(BUFFER_TYPE_DYNAMIC_RENDER,overlapping_polygon_count*3);
	{
		DynamicIBAccessClass::WriteLockClass lock(&dyn_ib_access);
		ShortVectorIStruct* sorted_polygon_index_array=(ShortVectorIStruct*)lock.Get_Index_Array();

		for (unsigned a = 0; a < overlapping_polygon_count; ++a) {
			sorted_polygon_index_array[a]=tis[a].tri;
		}
	}
	const double index_copy_ms = Perf_Ticks_To_Milliseconds(SDL_GetPerformanceCounter() - index_copy_start);
	WWPerfMonClass::Record_Sorting_Pool_Batch(
		batch_node_count,
		batch_polygon_count,
		batch_vertex_count,
		vertex_copy_ms,
		index_copy_ms,
		sort_ms);

	unsigned count_to_render=1;
	unsigned start_index=0;
	node_id=tis[0].idx;
	for (unsigned i=1;i<overlapping_polygon_count;++i) {
		if (node_id!=tis[i].idx) {
			SortingNodeStruct* state=overlapping_nodes[node_id];
			WW3D::FixedFunctionSubmitDesc submission = state->Submission;
			submission.VertexBuffer = dyn_vb_access.Peek_Vertex_Buffer();
			submission.VertexBufferOffset = dyn_vb_access.Get_Vertex_Buffer_Offset();
			submission.IndexBuffer = dyn_ib_access.Peek_Index_Buffer();
			submission.IndexBufferOffset = dyn_ib_access.Get_Index_Buffer_Offset();
			submission.IndexBaseOffset = 0;
			submission.StartIndex = static_cast<unsigned short>(start_index * 3);
			submission.PolygonCount = static_cast<unsigned short>(count_to_render);
			submission.MinVertexIndex = state->min_vertex_index;
			submission.VertexCount = state->vertex_count;
			WW3D::Submit_Fixed_Function_Draw(submission);

			count_to_render=0;
			start_index=i;
			node_id=tis[i].idx;
		}
		count_to_render++;
	}

	// Render any remaining polygons...
	if (count_to_render) {
		SortingNodeStruct* state=overlapping_nodes[node_id];
		WW3D::FixedFunctionSubmitDesc submission = state->Submission;
		submission.VertexBuffer = dyn_vb_access.Peek_Vertex_Buffer();
		submission.VertexBufferOffset = dyn_vb_access.Get_Vertex_Buffer_Offset();
		submission.IndexBuffer = dyn_ib_access.Peek_Index_Buffer();
		submission.IndexBufferOffset = dyn_ib_access.Get_Index_Buffer_Offset();
		submission.IndexBaseOffset = 0;
		submission.StartIndex = static_cast<unsigned short>(start_index * 3);
		submission.PolygonCount = static_cast<unsigned short>(count_to_render);
		submission.MinVertexIndex = state->min_vertex_index;
		submission.VertexCount = state->vertex_count;
		WW3D::Submit_Fixed_Function_Draw(submission);
	}

	// Release all references and return nodes back to the clean list for the frame...
	for (node_id=0;node_id<overlapping_node_count;++node_id) {
		SortingNodeStruct* state=overlapping_nodes[node_id];
		Release_Refs(state);
		clean_list.Add_Head(state);
	}
	overlapping_node_count=0;
	overlapping_polygon_count=0;
	overlapping_vertex_count=0;
	sorting_pool_depth_valid = false;
	sorting_pool_depth_min = 0.0f;
	sorting_pool_depth_max = 0.0f;
	SNAPSHOT_SAY(("SortingSystem - Done flushing\n"));

}

// ----------------------------------------------------------------------------

void SortingRendererClass::Flush()
{
	WWPROFILE("SortingRenderer::Flush");

	while (SortingNodeStruct* state=sorted_list.Head()) {
		state->Remove();
		
		if (Uses_Sorting_Pool(state)) {
			if (overlapping_node_count != 0 && !Overlaps_Current_Sorting_Pool(state)) {
				Flush_Sorting_Pool();
			}
			Insert_To_Sorting_Pool(state);
		}
		else {
			Flush_Sorting_Pool();
			WW3D::Submit_Fixed_Function_Draw(state->Submission);
			Release_Refs(state);
			clean_list.Add_Head(state);
		}
	}

	Flush_Sorting_Pool();

	DX8Wrapper::Set_Index_Buffer(0,0);
	DX8Wrapper::Set_Vertex_Buffer(0);
	total_sorting_vertices=0;

	DynamicIBAccessClass::_Reset(false);
	DynamicVBAccessClass::_Reset(false);
}

// ----------------------------------------------------------------------------

void SortingRendererClass::Deinit()
{
	SortingNodeStruct *head = NULL;

	//
	//	Flush the sorted list
	//
	while ((head = sorted_list.Head ()) != NULL) {
		sorted_list.Remove_Head ();
		delete head;
	}

	//
	//	Flush the clean list
	//
	while ((head = clean_list.Head ()) != NULL) {
		clean_list.Remove_Head ();
		delete head;
	}

	delete[] vertex_z_array;
	vertex_z_array=NULL;
	vertex_z_array_count=0;
	delete[] polygon_z_array;
	polygon_z_array=NULL;
	polygon_z_array_count=0;
	delete[] node_id_array;
	node_id_array=NULL;
	node_id_array_count=0;
	delete[] sorted_node_id_array;
	sorted_node_id_array=NULL;
	sorted_node_id_array_count=0;
	delete[] polygon_index_array;
	polygon_index_array=NULL;
	polygon_index_array_count=0;
	delete[] temp_index_array;
	temp_index_array=NULL;
	temp_index_array_count=0;
}

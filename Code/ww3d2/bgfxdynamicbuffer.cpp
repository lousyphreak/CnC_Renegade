#include "dx8wrapper.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <bgfx/bgfx.h>

#include "bgfxrenderer.h"

namespace
{
const unsigned kDefaultDynamicVertexCount = 5000;
static const FVFInfoClass kDynamicFVFInfo(dynamic_fvf_type);

struct SubmissionVertex
{
	float x;
	float y;
	float z;
	uint32_t color;
	float u;
	float v;
};

SortingVertexBufferClass *g_dynamic_sorting_vertex_buffer = nullptr;
bool g_dynamic_sorting_vertex_buffer_in_use = false;
unsigned short g_dynamic_sorting_vertex_buffer_size = 0;
unsigned short g_dynamic_sorting_vertex_buffer_offset = 0;

unsigned g_vertex_buffer_count = 0;
unsigned g_vertex_buffer_total_vertices = 0;
unsigned g_vertex_buffer_total_size = 0;

DX8Caps *g_stub_caps = nullptr;
std::unordered_map<const DX8VertexBufferClass *, std::vector<unsigned char>> g_dx8_vertex_buffers;

bgfx::TextureHandle Resolve_Texture_Handle(TextureClass *texture)
{
	if (texture != nullptr) {
		bgfx::TextureHandle handle = texture->Get_Bgfx_Texture();
		if (bgfx::isValid(handle)) {
			return handle;
		}
	}

	return BgfxRenderer::Get_White_Texture();
}

uint32_t Resolve_Sampler_Flags(TextureClass *texture)
{
	return texture != nullptr ? texture->Get_Bgfx_Sampler_Flags() : 0u;
}

std::vector<unsigned char> &Get_DX8_Vertex_Data(const DX8VertexBufferClass *buffer)
{
	return g_dx8_vertex_buffers[buffer];
}
}

unsigned number_of_DX8_calls = 0;
bool _DX8SingleThreaded = false;

void DX8_Assert()
{
}

void Log_DX8_ErrorCode(unsigned)
{
}

bool DX8Wrapper::IsInitted = false;
bool DX8Wrapper::_EnableTriangleDraw = true;
bool DX8Wrapper::IsDeviceLost = false;
void *DX8Wrapper::Hwnd = nullptr;
unsigned DX8Wrapper::_MainThreadID = 0;
int DX8Wrapper::CurRenderDevice = -1;
int DX8Wrapper::ResolutionWidth = 800;
int DX8Wrapper::ResolutionHeight = 600;
int DX8Wrapper::BitDepth = 32;
int DX8Wrapper::TextureBitDepth = 16;
bool DX8Wrapper::IsWindowed = true;
D3DMATRIX DX8Wrapper::old_world = {};
D3DMATRIX DX8Wrapper::old_view = {};
D3DMATRIX DX8Wrapper::old_prj = {};
bool DX8Wrapper::world_identity = true;
unsigned DX8Wrapper::RenderStates[256] = {};
unsigned DX8Wrapper::TextureStageStates[MAX_TEXTURE_STAGES][32] = {};
IDirect3DBaseTexture8 *DX8Wrapper::Textures[MAX_TEXTURE_STAGES] = {};
bool DX8Wrapper::FogEnable = false;
D3DCOLOR DX8Wrapper::FogColor = 0;
unsigned DX8Wrapper::matrix_changes = 0;
unsigned DX8Wrapper::material_changes = 0;
unsigned DX8Wrapper::vertex_buffer_changes = 0;
unsigned DX8Wrapper::index_buffer_changes = 0;
unsigned DX8Wrapper::light_changes = 0;
unsigned DX8Wrapper::texture_changes = 0;
unsigned DX8Wrapper::render_state_changes = 0;
unsigned DX8Wrapper::texture_stage_state_changes = 0;
bool DX8Wrapper::CurrentDX8LightEnables[4] = {};
unsigned long DX8Wrapper::FrameCount = 0;
DX8Caps *DX8Wrapper::CurrentCaps = g_stub_caps;
D3DADAPTER_IDENTIFIER8 DX8Wrapper::CurrentAdapterIdentifier = {};
IDirect3D8 *DX8Wrapper::D3DInterface = nullptr;
IDirect3DDevice8 *DX8Wrapper::D3DDevice = nullptr;
IDirect3DSurface8 *DX8Wrapper::CurrentRenderTarget = nullptr;
IDirect3DSurface8 *DX8Wrapper::DefaultRenderTarget = nullptr;
IDirect3DSurface8 *DX8Wrapper::DefaultDepthBuffer = nullptr;
bool DX8Wrapper::IsRenderToTexture = false;
int DX8Wrapper::ZBias = 0;
float DX8Wrapper::ZNear = 0.0f;
float DX8Wrapper::ZFar = 1.0f;
Matrix4 DX8Wrapper::ProjectionMatrix(true);
RenderStateStruct DX8Wrapper::render_state;
unsigned DX8Wrapper::render_state_changed = 0;
static unsigned g_last_frame_matrix_changes = 0;
static unsigned g_last_frame_material_changes = 0;
static unsigned g_last_frame_vertex_buffer_changes = 0;
static unsigned g_last_frame_index_buffer_changes = 0;
static unsigned g_last_frame_light_changes = 0;
static unsigned g_last_frame_texture_changes = 0;
static unsigned g_last_frame_render_state_changes = 0;
static unsigned g_last_frame_texture_stage_state_changes = 0;
static unsigned g_last_frame_dx8_calls = 0;

VertexBufferClass::VertexBufferClass(unsigned type_, unsigned FVF, unsigned short vertex_count_)
	: type(type_), VertexCount(vertex_count_), engine_refs(0), fvf_info(new FVFInfoClass(FVF))
{
	++g_vertex_buffer_count;
	g_vertex_buffer_total_vertices += VertexCount;
	g_vertex_buffer_total_size += VertexCount * fvf_info->Get_FVF_Size();
}

VertexBufferClass::~VertexBufferClass()
{
	--g_vertex_buffer_count;
	g_vertex_buffer_total_vertices -= VertexCount;
	g_vertex_buffer_total_size -= VertexCount * fvf_info->Get_FVF_Size();
	delete fvf_info;
}

void VertexBufferClass::Add_Engine_Ref() const
{
	++engine_refs;
}

void VertexBufferClass::Release_Engine_Ref() const
{
	--engine_refs;
	WWASSERT(engine_refs >= 0);
}

unsigned VertexBufferClass::Get_Total_Buffer_Count()
{
	return g_vertex_buffer_count;
}

unsigned VertexBufferClass::Get_Total_Allocated_Vertices()
{
	return g_vertex_buffer_total_vertices;
}

unsigned VertexBufferClass::Get_Total_Allocated_Memory()
{
	return g_vertex_buffer_total_size;
}

VertexBufferClass::WriteLockClass::WriteLockClass(VertexBufferClass *vertex_buffer) : VertexBufferLockClass(vertex_buffer)
{
	WWASSERT(vertex_buffer != nullptr);
	vertex_buffer->Add_Ref();
	switch (vertex_buffer->Type()) {
	case BUFFER_TYPE_DX8:
		Vertices = Get_DX8_Vertex_Data(static_cast<DX8VertexBufferClass *>(vertex_buffer)).data();
		break;
	case BUFFER_TYPE_SORTING:
		Vertices = static_cast<SortingVertexBufferClass *>(vertex_buffer)->VertexBuffer;
		break;
	default:
		Vertices = nullptr;
		WWASSERT(0);
		break;
	}
}

VertexBufferClass::WriteLockClass::~WriteLockClass()
{
	VertexBuffer->Release_Ref();
}

VertexBufferClass::AppendLockClass::AppendLockClass(VertexBufferClass *vertex_buffer, unsigned start_index, unsigned)
	: VertexBufferLockClass(vertex_buffer)
{
	WWASSERT(vertex_buffer != nullptr);
	vertex_buffer->Add_Ref();
	switch (vertex_buffer->Type()) {
	case BUFFER_TYPE_DX8:
		Vertices = Get_DX8_Vertex_Data(static_cast<DX8VertexBufferClass *>(vertex_buffer)).data() + start_index * vertex_buffer->FVF_Info().Get_FVF_Size();
		break;
	case BUFFER_TYPE_SORTING:
		Vertices = static_cast<SortingVertexBufferClass *>(vertex_buffer)->VertexBuffer + start_index;
		break;
	default:
		Vertices = nullptr;
		WWASSERT(0);
		break;
	}
}

VertexBufferClass::AppendLockClass::~AppendLockClass()
{
	VertexBuffer->Release_Ref();
}

SortingVertexBufferClass::SortingVertexBufferClass(unsigned short vertex_count)
	: VertexBufferClass(BUFFER_TYPE_SORTING, dynamic_fvf_type, vertex_count), VertexBuffer(new VertexFormatXYZNDUV2[vertex_count])
{
}

SortingVertexBufferClass::~SortingVertexBufferClass()
{
	delete[] VertexBuffer;
}

DX8VertexBufferClass::DX8VertexBufferClass(unsigned FVF, unsigned short vertex_count_, UsageType)
	: VertexBufferClass(BUFFER_TYPE_DX8, FVF, vertex_count_), VertexBuffer(nullptr)
{
	g_dx8_vertex_buffers[this].resize(static_cast<size_t>(FVF_Info().Get_FVF_Size()) * vertex_count_);
}

DX8VertexBufferClass::DX8VertexBufferClass(const Vector3 *vertices, const Vector3 *normals, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: DX8VertexBufferClass(DX8_FVF_FLAG_XYZ | DX8_FVF_FLAG_TEX1 | DX8_FVF_FLAG_NORMAL, vertex_count_, usage)
{
	Copy(vertices, normals, tex_coords, 0, vertex_count_);
}

DX8VertexBufferClass::DX8VertexBufferClass(const Vector3 *vertices, const Vector3 *normals, const Vector4 *diffuse, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: DX8VertexBufferClass(DX8_FVF_FLAG_XYZ | DX8_FVF_FLAG_TEX1 | DX8_FVF_FLAG_NORMAL | DX8_FVF_FLAG_DIFFUSE, vertex_count_, usage)
{
	Copy(vertices, normals, tex_coords, diffuse, 0, vertex_count_);
}

DX8VertexBufferClass::DX8VertexBufferClass(const Vector3 *vertices, const Vector4 *diffuse, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: DX8VertexBufferClass(DX8_FVF_FLAG_XYZ | DX8_FVF_FLAG_TEX1 | DX8_FVF_FLAG_DIFFUSE, vertex_count_, usage)
{
	Copy(vertices, tex_coords, diffuse, 0, vertex_count_);
}

DX8VertexBufferClass::DX8VertexBufferClass(const Vector3 *vertices, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: DX8VertexBufferClass(DX8_FVF_FLAG_XYZ | DX8_FVF_FLAG_TEX1, vertex_count_, usage)
{
	Copy(vertices, tex_coords, 0, vertex_count_);
}

DX8VertexBufferClass::~DX8VertexBufferClass()
{
	g_dx8_vertex_buffers.erase(this);
}

void DX8VertexBufferClass::Create_Vertex_Buffer(UsageType)
{
}

void DX8VertexBufferClass::Copy(const Vector3 *loc, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_DX8_Vertex_Data(this).data() + first_vertex * FVF_Info().Get_FVF_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Location_Offset()) = loc[i];
		vertices += FVF_Info().Get_FVF_Size();
	}
}

void DX8VertexBufferClass::Copy(const Vector3 *loc, const Vector2 *uv, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_DX8_Vertex_Data(this).data() + first_vertex * FVF_Info().Get_FVF_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector2 *>(vertices + FVF_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += FVF_Info().Get_FVF_Size();
	}
}

void DX8VertexBufferClass::Copy(const Vector3 *loc, const Vector3 *norm, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_DX8_Vertex_Data(this).data() + first_vertex * FVF_Info().Get_FVF_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Normal_Offset()) = norm[i];
		vertices += FVF_Info().Get_FVF_Size();
	}
}

void DX8VertexBufferClass::Copy(const Vector3 *loc, const Vector3 *norm, const Vector2 *uv, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_DX8_Vertex_Data(this).data() + first_vertex * FVF_Info().Get_FVF_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Normal_Offset()) = norm[i];
		*reinterpret_cast<Vector2 *>(vertices + FVF_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += FVF_Info().Get_FVF_Size();
	}
}

void DX8VertexBufferClass::Copy(const Vector3 *loc, const Vector3 *norm, const Vector2 *uv, const Vector4 *diffuse, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_DX8_Vertex_Data(this).data() + first_vertex * FVF_Info().Get_FVF_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Normal_Offset()) = norm[i];
		*reinterpret_cast<unsigned *>(vertices + FVF_Info().Get_Diffuse_Offset()) = DX8Wrapper::Convert_Color(diffuse[i]);
		*reinterpret_cast<Vector2 *>(vertices + FVF_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += FVF_Info().Get_FVF_Size();
	}
}

void DX8VertexBufferClass::Copy(const Vector3 *loc, const Vector2 *uv, const Vector4 *diffuse, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_DX8_Vertex_Data(this).data() + first_vertex * FVF_Info().Get_FVF_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + FVF_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<unsigned *>(vertices + FVF_Info().Get_Diffuse_Offset()) = DX8Wrapper::Convert_Color(diffuse[i]);
		*reinterpret_cast<Vector2 *>(vertices + FVF_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += FVF_Info().Get_FVF_Size();
	}
}

DynamicVBAccessClass::DynamicVBAccessClass(unsigned type, unsigned fvf, unsigned short vertex_count)
	: FVFInfo(kDynamicFVFInfo), Type(type == BUFFER_TYPE_DYNAMIC_DX8 ? BUFFER_TYPE_DYNAMIC_SORTING : type), VertexCount(vertex_count), VertexBufferOffset(0), VertexBuffer(nullptr)
{
	WWASSERT(fvf == dynamic_fvf_type);
	WWASSERT(Type == BUFFER_TYPE_DYNAMIC_SORTING);
	Allocate_Sorting_Dynamic_Buffer();
}

DynamicVBAccessClass::~DynamicVBAccessClass()
{
	REF_PTR_RELEASE(VertexBuffer);
	g_dynamic_sorting_vertex_buffer_in_use = false;
	g_dynamic_sorting_vertex_buffer_offset += VertexCount;
}

void DynamicVBAccessClass::_Deinit()
{
	REF_PTR_RELEASE(g_dynamic_sorting_vertex_buffer);
	g_dynamic_sorting_vertex_buffer_in_use = false;
	g_dynamic_sorting_vertex_buffer_size = 0;
	g_dynamic_sorting_vertex_buffer_offset = 0;
}

void DynamicVBAccessClass::_Reset(bool)
{
	g_dynamic_sorting_vertex_buffer_offset = 0;
}

DynamicVBAccessClass::WriteLockClass::WriteLockClass(DynamicVBAccessClass *vb_access)
	: DynamicVBAccess(vb_access), Vertices(nullptr)
{
	WWASSERT(vb_access != nullptr);
	DynamicVBAccess->VertexBuffer->Add_Ref();
	Vertices = static_cast<SortingVertexBufferClass *>(DynamicVBAccess->VertexBuffer)->VertexBuffer + DynamicVBAccess->VertexBufferOffset;
}

DynamicVBAccessClass::WriteLockClass::~WriteLockClass()
{
	DynamicVBAccess->VertexBuffer->Release_Ref();
}

void DynamicVBAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	WWASSERT(!g_dynamic_sorting_vertex_buffer_in_use);
	g_dynamic_sorting_vertex_buffer_in_use = true;

	const unsigned required_vertex_count = g_dynamic_sorting_vertex_buffer_offset + VertexCount;
	WWASSERT(required_vertex_count < 65536);
	if (required_vertex_count > g_dynamic_sorting_vertex_buffer_size) {
		REF_PTR_RELEASE(g_dynamic_sorting_vertex_buffer);
		g_dynamic_sorting_vertex_buffer_size = std::max<unsigned short>(static_cast<unsigned short>(required_vertex_count), static_cast<unsigned short>(kDefaultDynamicVertexCount));
	}

	if (g_dynamic_sorting_vertex_buffer == nullptr) {
		g_dynamic_sorting_vertex_buffer = NEW_REF(SortingVertexBufferClass, (g_dynamic_sorting_vertex_buffer_size));
		g_dynamic_sorting_vertex_buffer_offset = 0;
	}

	REF_PTR_SET(VertexBuffer, g_dynamic_sorting_vertex_buffer);
	VertexBufferOffset = g_dynamic_sorting_vertex_buffer_offset;
}

void DynamicVBAccessClass::Allocate_DX8_Dynamic_Buffer()
{
	Allocate_Sorting_Dynamic_Buffer();
}

bool DX8Wrapper::Init(void *hwnd, bool lite)
{
	Hwnd = hwnd;
	IsInitted = BgfxRenderer::Init(hwnd, lite);
	if (!IsInitted) {
		return false;
	}

	Do_Onetime_Device_Dependent_Inits();
	Get_Device_Resolution(ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed);
	return true;
}

void DX8Wrapper::Shutdown(void)
{
	Do_Onetime_Device_Dependent_Shutdowns();
	BgfxRenderer::Shutdown();
	IsInitted = false;
	Hwnd = nullptr;
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Inits(void)
{
	render_state.view = BgfxRenderer::Get_Current_View_Matrix();
	ProjectionMatrix = BgfxRenderer::Get_Current_Projection_Matrix();
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns(void)
{
	DynamicVBAccessClass::_Deinit();
	DynamicIBAccessClass::_Deinit();
}

void DX8Wrapper::Begin_Scene(void)
{
	render_state.view = BgfxRenderer::Get_Current_View_Matrix();
	ProjectionMatrix = BgfxRenderer::Get_Current_Projection_Matrix();
}

void DX8Wrapper::End_Scene(bool flip_frame)
{
	if (flip_frame) {
		BgfxRenderer::End_Frame();
		g_last_frame_matrix_changes = matrix_changes;
		g_last_frame_material_changes = material_changes;
		g_last_frame_vertex_buffer_changes = vertex_buffer_changes;
		g_last_frame_index_buffer_changes = index_buffer_changes;
		g_last_frame_light_changes = light_changes;
		g_last_frame_texture_changes = texture_changes;
		g_last_frame_render_state_changes = render_state_changes;
		g_last_frame_texture_stage_state_changes = texture_stage_state_changes;
		g_last_frame_dx8_calls = number_of_DX8_calls;
		matrix_changes = material_changes = vertex_buffer_changes = index_buffer_changes = 0;
		light_changes = texture_changes = render_state_changes = texture_stage_state_changes = 0;
		number_of_DX8_calls = 0;
		++FrameCount;
	}
}

void DX8Wrapper::Flip_To_Primary(void)
{
}

void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float, unsigned int)
{
	BgfxRenderer::Clear_View(clear_color, clear_z_stencil, color);
}

void DX8Wrapper::Set_Viewport(const D3DVIEWPORT8 *viewport)
{
	if (viewport != nullptr) {
		BgfxRenderer::Set_Viewport(viewport->X, viewport->Y, viewport->Width, viewport->Height);
	}
}

void DX8Wrapper::Set_Projection_Transform_With_Z_Bias(const Matrix4 &matrix, float znear, float zfar)
{
	ProjectionMatrix = matrix;
	ZNear = znear;
	ZFar = zfar;
}

void DX8Wrapper::Set_DX8_ZBias(int zbias)
{
	ZBias = zbias;
}

void DX8Wrapper::Set_Pseudo_ZBias(int zbias)
{
	ZBias = zbias;
}

void DX8Wrapper::Set_Gamma(float, float, float, bool, bool)
{
}

void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE transform, const Matrix4 &m)
{
	switch (transform) {
	case D3DTS_WORLD:
		render_state.world = m;
		render_state_changed |= WORLD_CHANGED;
		render_state_changed &= ~WORLD_IDENTITY;
		break;
	case D3DTS_VIEW:
		render_state.view = m;
		render_state_changed |= VIEW_CHANGED;
		render_state_changed &= ~VIEW_IDENTITY;
		break;
	case D3DTS_PROJECTION:
		ProjectionMatrix = m;
		break;
	default:
		break;
	}
}

void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE transform, const Matrix3D &m)
{
	Set_Transform(transform, Matrix4(m));
}

void DX8Wrapper::Get_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4 &m)
{
	switch (transform) {
	case D3DTS_WORLD:
		m = render_state.world;
		break;
	case D3DTS_VIEW:
		m = render_state.view;
		break;
	case D3DTS_PROJECTION:
		m = ProjectionMatrix;
		break;
	default:
		m.Make_Identity();
		break;
	}
}

void DX8Wrapper::_Set_DX8_Transform(D3DTRANSFORMSTATETYPE transform, const Matrix4 &m)
{
	Set_Transform(transform, m);
}

void DX8Wrapper::_Set_DX8_Transform(D3DTRANSFORMSTATETYPE transform, const Matrix3D &m)
{
	Set_Transform(transform, m);
}

void DX8Wrapper::_Get_DX8_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4 &m)
{
	Get_Transform(transform, m);
}

void DX8Wrapper::Set_DX8_Render_State(D3DRENDERSTATETYPE state, unsigned value)
{
	if (state < 256) {
		RenderStates[state] = value;
	}
	++render_state_changes;
}

void DX8Wrapper::Set_Light_Environment(LightEnvironmentClass *)
{
}

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass *vb)
{
	render_state.vba_offset = 0;
	render_state.vba_count = 0;
	if (render_state.vertex_buffer != nullptr) {
		render_state.vertex_buffer->Release_Engine_Ref();
	}
	render_state.vertex_buffer = const_cast<VertexBufferClass *>(vb);
	if (render_state.vertex_buffer != nullptr) {
		render_state.vertex_buffer->Add_Engine_Ref();
		render_state.vertex_buffer_type = render_state.vertex_buffer->Type();
	} else {
		render_state.vertex_buffer_type = BUFFER_TYPE_INVALID;
	}
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass *ib, unsigned short index_base_offset)
{
	render_state.iba_offset = 0;
	if (render_state.index_buffer != nullptr) {
		render_state.index_buffer->Release_Engine_Ref();
	}
	render_state.index_buffer = const_cast<IndexBufferClass *>(ib);
	if (render_state.index_buffer != nullptr) {
		render_state.index_buffer->Add_Engine_Ref();
		render_state.index_buffer_type = render_state.index_buffer->Type();
	} else {
		render_state.index_buffer_type = BUFFER_TYPE_INVALID;
	}
	render_state.index_base_offset = index_base_offset;
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass &vba)
{
	Set_Vertex_Buffer(vba.VertexBuffer);
	render_state.vba_offset = vba.VertexBufferOffset;
	render_state.vba_count = vba.VertexCount;
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass &iba, unsigned short index_base_offset)
{
	Set_Index_Buffer(iba.IndexBuffer, index_base_offset);
	render_state.iba_offset = iba.IndexBufferOffset;
}

void DX8Wrapper::Apply_Render_State_Changes()
{
	render_state_changed = 0;
}

void DX8Wrapper::Draw_Triangles(unsigned, unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	Draw_Triangles(start_index, polygon_count, min_vertex_index, vertex_count);
}

void DX8Wrapper::Draw_Triangles(unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	if (!BgfxRenderer::Is_Initted() || !_EnableTriangleDraw || render_state.vertex_buffer == nullptr || render_state.index_buffer == nullptr) {
		return;
	}

	if ((render_state.vertex_buffer->Type() != BUFFER_TYPE_SORTING && render_state.vertex_buffer->Type() != BUFFER_TYPE_DX8) ||
		(render_state.index_buffer->Type() != BUFFER_TYPE_SORTING && render_state.index_buffer->Type() != BUFFER_TYPE_DX8)) {
		return;
	}

	if (vertex_count == 0 || polygon_count == 0) {
		return;
	}

	const uint32_t triangle_index_count = static_cast<uint32_t>(polygon_count) * 3u;
	const bgfx::VertexLayout &layout = BgfxRenderer::Get_Pos_Color_Texcoord_Layout();
	if (bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count ||
		bgfx::getAvailTransientIndexBuffer(triangle_index_count) < triangle_index_count) {
		return;
	}

	bgfx::TransientVertexBuffer transient_vertex_buffer;
	bgfx::TransientIndexBuffer transient_index_buffer;
	bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, layout);
	bgfx::allocTransientIndexBuffer(&transient_index_buffer, triangle_index_count);

	SubmissionVertex *submission_vertices = reinterpret_cast<SubmissionVertex *>(transient_vertex_buffer.data);
	VertexBufferClass::AppendLockClass vertex_lock(render_state.vertex_buffer, render_state.vba_offset + min_vertex_index, vertex_count);
	const unsigned char *source_vertices = reinterpret_cast<const unsigned char *>(vertex_lock.Get_Vertex_Array());
	for (unsigned short vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
		const unsigned char *vertex = source_vertices + vertex_index * render_state.vertex_buffer->FVF_Info().Get_FVF_Size();
		submission_vertices[vertex_index].x = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[0];
		submission_vertices[vertex_index].y = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[1];
		submission_vertices[vertex_index].z = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[2];
		submission_vertices[vertex_index].color = render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset() < render_state.vertex_buffer->FVF_Info().Get_FVF_Size()
			? *reinterpret_cast<const unsigned *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset())
			: 0xffffffffu;
		submission_vertices[vertex_index].u = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[0];
		submission_vertices[vertex_index].v = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[1];
	}

	uint16_t *submission_indices = reinterpret_cast<uint16_t *>(transient_index_buffer.data);
	IndexBufferClass::AppendLockClass index_lock(render_state.index_buffer, render_state.iba_offset + start_index, triangle_index_count);
	const unsigned short *source_indices = index_lock.Get_Index_Array();
	for (uint32_t index = 0; index < triangle_index_count; ++index) {
		const unsigned short resolved_index = static_cast<unsigned short>(source_indices[index] + render_state.index_base_offset);
		submission_indices[index] = static_cast<uint16_t>(resolved_index - min_vertex_index);
	}

	bgfx::setViewTransform(BgfxRenderer::Get_Main_View_Id(), &render_state.view[0][0], &ProjectionMatrix[0][0]);
	bgfx::setTransform(&render_state.world[0][0]);
	bgfx::setVertexBuffer(0, &transient_vertex_buffer);
	bgfx::setIndexBuffer(&transient_index_buffer);
	bgfx::setTexture(0, BgfxRenderer::Get_Color_Texture_Uniform(), Resolve_Texture_Handle(render_state.Textures[0]), Resolve_Sampler_Flags(render_state.Textures[0]));
	bgfx::setState(BgfxRenderer::Build_Render_State(render_state.shader));
	bgfx::submit(BgfxRenderer::Get_Main_View_Id(), BgfxRenderer::Get_Color_Texture_Program());
}

void DX8Wrapper::Draw_Strip(unsigned short start_index, unsigned short index_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	if (index_count < 3 || render_state.index_buffer == nullptr || render_state.vertex_buffer == nullptr) {
		return;
	}

	if ((render_state.index_buffer->Type() != BUFFER_TYPE_SORTING && render_state.index_buffer->Type() != BUFFER_TYPE_DX8) ||
		(render_state.vertex_buffer->Type() != BUFFER_TYPE_SORTING && render_state.vertex_buffer->Type() != BUFFER_TYPE_DX8)) {
		return;
	}
	const unsigned short polygon_count = static_cast<unsigned short>(index_count - 2);
	const uint32_t triangle_index_count = static_cast<uint32_t>(polygon_count) * 3u;

	const bgfx::VertexLayout &layout = BgfxRenderer::Get_Pos_Color_Texcoord_Layout();
	if (bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count ||
		bgfx::getAvailTransientIndexBuffer(triangle_index_count) < triangle_index_count) {
		return;
	}

	bgfx::TransientVertexBuffer transient_vertex_buffer;
	bgfx::TransientIndexBuffer transient_index_buffer;
	bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, layout);
	bgfx::allocTransientIndexBuffer(&transient_index_buffer, triangle_index_count);

	SubmissionVertex *submission_vertices = reinterpret_cast<SubmissionVertex *>(transient_vertex_buffer.data);
	VertexBufferClass::AppendLockClass vertex_lock(render_state.vertex_buffer, render_state.vba_offset + min_vertex_index, vertex_count);
	const unsigned char *source_vertices = reinterpret_cast<const unsigned char *>(vertex_lock.Get_Vertex_Array());
	for (unsigned short vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
		const unsigned char *vertex = source_vertices + vertex_index * render_state.vertex_buffer->FVF_Info().Get_FVF_Size();
		submission_vertices[vertex_index].x = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[0];
		submission_vertices[vertex_index].y = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[1];
		submission_vertices[vertex_index].z = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[2];
		submission_vertices[vertex_index].color = render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset() < render_state.vertex_buffer->FVF_Info().Get_FVF_Size()
			? *reinterpret_cast<const unsigned *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset())
			: 0xffffffffu;
		submission_vertices[vertex_index].u = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[0];
		submission_vertices[vertex_index].v = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[1];
	}

	uint16_t *submission_indices = reinterpret_cast<uint16_t *>(transient_index_buffer.data);
	IndexBufferClass::AppendLockClass index_lock(render_state.index_buffer, render_state.iba_offset + start_index, index_count);
	const unsigned short *strip_indices = index_lock.Get_Index_Array();
	for (unsigned short triangle = 0; triangle < polygon_count; ++triangle) {
		const bool odd_triangle = (triangle & 1u) != 0u;
		const unsigned short a = strip_indices[triangle + (odd_triangle ? 1 : 0)];
		const unsigned short b = strip_indices[triangle + (odd_triangle ? 0 : 1)];
		const unsigned short c = strip_indices[triangle + 2];
		submission_indices[triangle * 3 + 0] = static_cast<uint16_t>(a + render_state.index_base_offset - min_vertex_index);
		submission_indices[triangle * 3 + 1] = static_cast<uint16_t>(b + render_state.index_base_offset - min_vertex_index);
		submission_indices[triangle * 3 + 2] = static_cast<uint16_t>(c + render_state.index_base_offset - min_vertex_index);
	}

	bgfx::setViewTransform(
		BgfxRenderer::Get_Main_View_Id(),
		&render_state.view[0][0],
		&ProjectionMatrix[0][0]);
	bgfx::setTransform(&render_state.world[0][0]);
	bgfx::setVertexBuffer(0, &transient_vertex_buffer);
	bgfx::setIndexBuffer(&transient_index_buffer);
	bgfx::setTexture(0, BgfxRenderer::Get_Color_Texture_Uniform(), Resolve_Texture_Handle(render_state.Textures[0]), Resolve_Sampler_Flags(render_state.Textures[0]));
	bgfx::setState(BgfxRenderer::Build_Render_State(render_state.shader));
	bgfx::submit(BgfxRenderer::Get_Main_View_Id(), BgfxRenderer::Get_Color_Texture_Program());
}

TextureClass *DX8Wrapper::Create_Render_Target(int width, int height, WW3DFormat format)
{
	if (format == WW3D_FORMAT_UNKNOWN) {
		format = WW3D_FORMAT_A8R8G8B8;
	}
	return NEW_REF(TextureClass, (static_cast<unsigned>(width), static_cast<unsigned>(height), format, TextureClass::MIP_LEVELS_1, TextureClass::POOL_DEFAULT, true));
}

void DX8Wrapper::Set_Render_Target(TextureClass *texture)
{
	IsRenderToTexture = texture != nullptr;
	if (texture != nullptr) {
		BgfxRenderer::Set_Render_Target(*texture);
	} else {
		BgfxRenderer::Reset_Render_Target();
	}
}

void DX8Wrapper::Set_Render_Target(IDirect3DSurface8 *, bool)
{
	BgfxRenderer::Reset_Render_Target();
	IsRenderToTexture = false;
}

void DX8Wrapper::Set_Render_Target(IDirect3DSwapChain8 *)
{
	BgfxRenderer::Reset_Render_Target();
	IsRenderToTexture = false;
}

void DX8Wrapper::Get_Device_Resolution(int &set_w, int &set_h, int &set_bits, bool &set_windowed)
{
	BgfxRenderer::Get_Device_Resolution(set_w, set_h, set_bits, set_windowed);
}

void DX8Wrapper::Get_Render_Target_Resolution(int &set_w, int &set_h, int &set_bits, bool &set_windowed)
{
	BgfxRenderer::Get_Render_Target_Resolution(set_w, set_h, set_bits, set_windowed);
}

unsigned long DX8Wrapper::Get_FrameCount(void)
{
	return FrameCount;
}

unsigned DX8Wrapper::Get_Last_Frame_Matrix_Changes()
{
	return g_last_frame_matrix_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Material_Changes()
{
	return g_last_frame_material_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Vertex_Buffer_Changes()
{
	return g_last_frame_vertex_buffer_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Index_Buffer_Changes()
{
	return g_last_frame_index_buffer_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Light_Changes()
{
	return g_last_frame_light_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Texture_Changes()
{
	return g_last_frame_texture_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Render_State_Changes()
{
	return g_last_frame_render_state_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_Texture_Stage_State_Changes()
{
	return g_last_frame_texture_stage_state_changes;
}

unsigned DX8Wrapper::Get_Last_Frame_DX8_Calls()
{
	return g_last_frame_dx8_calls;
}

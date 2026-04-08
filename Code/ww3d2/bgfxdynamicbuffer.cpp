#include "dx8wrapper.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>

#include "bgfxrenderer.h"
#include "boxrobj.h"
#include "dx8renderer.h"
#include "dx8texman.h"
#include "formconv.h"
#include "missingtexture.h"
#include "pointgr.h"
#include "rddesc.h"
#include "render2d.h"
#include "shattersystem.h"
#include "surfaceclass.h"
#include "texture.h"
#include "textureloader.h"
#include "vertmaterial.h"
#include "ww3d.h"
#include "light.h"

namespace
{
const unsigned kDefaultDynamicVertexCount = 5000;
static const FVFInfoClass kDynamicFVFInfo(dynamic_fvf_type);
DX8Caps *Ensure_Caps();

struct SubmissionVertex
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	uint32_t diffuse;
	uint32_t specular;
	float u0;
	float v0;
	float u1;
	float v1;
};

SortingVertexBufferClass *g_dynamic_sorting_vertex_buffer = nullptr;
bool g_dynamic_sorting_vertex_buffer_in_use = false;
unsigned short g_dynamic_sorting_vertex_buffer_size = 0;
unsigned short g_dynamic_sorting_vertex_buffer_offset = 0;

unsigned g_vertex_buffer_count = 0;
unsigned g_vertex_buffer_total_vertices = 0;
unsigned g_vertex_buffer_total_size = 0;
D3DMATERIAL8 g_current_dx8_material = {
	{1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f, 1.0f},
	{0.0f, 0.0f, 0.0f, 1.0f},
	1.0f};

RenderDeviceDescClass g_render_device_desc;
DX8Caps *g_stub_caps = nullptr;
int g_swap_interval = 0;
constexpr HRESULT kD3DErrInvalidCall = -11;
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

float Decode_Float_From_Dword(unsigned value)
{
	float decoded = 0.0f;
	std::memcpy(&decoded, &value, sizeof(decoded));
	return decoded;
}

void Copy_Color_Value(float *destination, const D3DCOLORVALUE &source)
{
	destination[0] = source.r;
	destination[1] = source.g;
	destination[2] = source.b;
	destination[3] = source.a;
}

void Copy_Packed_Color(float *destination, unsigned color)
{
	destination[0] = static_cast<float>((color >> 16) & 0xffu) / 255.0f;
	destination[1] = static_cast<float>((color >> 8) & 0xffu) / 255.0f;
	destination[2] = static_cast<float>(color & 0xffu) / 255.0f;
	destination[3] = static_cast<float>((color >> 24) & 0xffu) / 255.0f;
}

unsigned Normalize_Material_Source(unsigned source)
{
	switch (source) {
	case D3DMCS_COLOR1:
	case D3DMCS_COLOR2:
		return source;
	default:
		return D3DMCS_MATERIAL;
	}
}

unsigned Sanitize_Texcoord_Index(unsigned stage)
{
	const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, D3DTSS_TEXCOORDINDEX);
	return value != 0x12345678u ? value : (D3DTSS_TCI_PASSTHRU | stage);
}

unsigned Sanitize_Texture_Transform_Flags(unsigned stage)
{
	const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, D3DTSS_TEXTURETRANSFORMFLAGS);
	return value != 0x12345678u ? value : D3DTTFF_DISABLE;
}

unsigned Get_Texture_Coord_Count(unsigned transform_flags)
{
	const unsigned count = transform_flags & 0xffu;
	if (count >= 1u && count <= 4u) {
		return count;
	}

	return 2u;
}

Vector4 Get_Passthrough_Texcoord(unsigned source_set, float u0, float v0, float u1, float v1)
{
	switch (source_set) {
	case 1u:
		return Vector4(u1, v1, 0.0f, 1.0f);
	default:
		return Vector4(u0, v0, 0.0f, 1.0f);
	}
}

Vector4 Transform_Position_To_Camera(const SubmissionVertex &vertex, const Matrix4 &world, const Matrix4 &view)
{
	const Vector4 position(vertex.x, vertex.y, vertex.z, 1.0f);
	Vector4 world_position;
	Vector4 camera_position;
	Matrix4::Transform_Vector(world, position, &world_position);
	Matrix4::Transform_Vector(view, world_position, &camera_position);
	return camera_position;
}

Vector4 Transform_Normal_To_Camera(const SubmissionVertex &vertex, const Matrix4 &world, const Matrix4 &view)
{
	const Vector4 normal(vertex.nx, vertex.ny, vertex.nz, 0.0f);
	Vector4 world_normal;
	Vector4 camera_normal;
	Matrix4::Transform_Vector(world, normal, &world_normal);
	Matrix4::Transform_Vector(view, world_normal, &camera_normal);

	Vector3 normalized(camera_normal.X, camera_normal.Y, camera_normal.Z);
	if (normalized.Length2() > 1.0e-12f) {
		normalized.Normalize();
	}

	return Vector4(normalized.X, normalized.Y, normalized.Z, 1.0f);
}

Vector4 Resolve_Texture_Coordinate_Source(
	const SubmissionVertex &vertex,
	float u0,
	float v0,
	float u1,
	float v1,
	unsigned texcoord_index,
	const Matrix4 &world,
	const Matrix4 &view)
{
	const unsigned source_set = texcoord_index & 0xffffu;
	switch (texcoord_index & 0xffff0000u) {
	case D3DTSS_TCI_CAMERASPACENORMAL:
		return Transform_Normal_To_Camera(vertex, world, view);

	case D3DTSS_TCI_CAMERASPACEPOSITION:
		return Transform_Position_To_Camera(vertex, world, view);

	case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:
	{
		const Vector4 camera_position = Transform_Position_To_Camera(vertex, world, view);
		const Vector4 camera_normal = Transform_Normal_To_Camera(vertex, world, view);
		Vector3 eye_vector(-camera_position.X, -camera_position.Y, -camera_position.Z);
		Vector3 normal(camera_normal.X, camera_normal.Y, camera_normal.Z);
		if (eye_vector.Length2() > 1.0e-12f) {
			eye_vector.Normalize();
		}
		if (normal.Length2() > 1.0e-12f) {
			normal.Normalize();
		}

		const float ndote = Vector3::Dot_Product(normal, eye_vector);
		const Vector3 reflection = normal * (2.0f * ndote) - eye_vector;
		return Vector4(reflection.X, reflection.Y, reflection.Z, 1.0f);
	}

	default:
		return Get_Passthrough_Texcoord(source_set, u0, v0, u1, v1);
	}
}

Vector2 Resolve_Texture_Coordinates(
	const SubmissionVertex &vertex,
	float u0,
	float v0,
	float u1,
	float v1,
	unsigned texcoord_index,
	unsigned transform_flags,
	const Matrix4 &texture_transform,
	const Matrix4 &world,
	const Matrix4 &view)
{
	Vector4 coordinate = Resolve_Texture_Coordinate_Source(vertex, u0, v0, u1, v1, texcoord_index, world, view);
	if (transform_flags != D3DTTFF_DISABLE) {
		Vector4 transformed;
		Matrix4::Transform_Vector(texture_transform, coordinate, &transformed);
		coordinate = transformed;

		if ((transform_flags & D3DTTFF_PROJECTED) != 0u) {
			const unsigned count = Get_Texture_Coord_Count(transform_flags);
			const unsigned q_index = count > 1u ? count - 1u : 1u;
			const float q = coordinate[static_cast<int>(q_index)];
			if (WWMath::Fabs(q) > 1.0e-6f) {
				coordinate.X /= q;
				coordinate.Y /= q;
			}
		}
	}

	return Vector2(coordinate.X, coordinate.Y);
}

void Populate_Fixed_Function_Lighting_Inputs(
	BgfxRenderer::FixedFunctionShaderInputs &shader_inputs,
	const VertexBufferClass *vertex_buffer,
	unsigned ambient_color,
	unsigned lighting_enabled,
	unsigned diffuse_source,
	unsigned ambient_source,
	unsigned emissive_source,
	const D3DLIGHT8 *lights,
	const bool *light_enable)
{
	Copy_Color_Value(shader_inputs.MaterialAmbient, g_current_dx8_material.Ambient);
	Copy_Color_Value(shader_inputs.MaterialDiffuse, g_current_dx8_material.Diffuse);
	Copy_Color_Value(shader_inputs.MaterialEmissive, g_current_dx8_material.Emissive);
	Copy_Packed_Color(shader_inputs.SceneAmbient, ambient_color);

	const unsigned fvf = vertex_buffer != nullptr ? vertex_buffer->FVF_Info().Get_FVF() : 0u;
	const bool has_normals = (fvf & DX8_FVF_FLAG_NORMAL) != 0u;
	shader_inputs.LightingConfig[0] = lighting_enabled != 0u ? 1.0f : 0.0f;
	shader_inputs.LightingConfig[1] = has_normals ? 1.0f : 0.0f;
	shader_inputs.LightingConfig[2] = static_cast<float>(Normalize_Material_Source(diffuse_source));
	shader_inputs.LightingConfig[3] = static_cast<float>(Normalize_Material_Source(ambient_source));
	shader_inputs.MaterialSourceConfig[0] = static_cast<float>(Normalize_Material_Source(emissive_source));

	for (int light_index = 0; light_index < 4; ++light_index) {
		const size_t offset = static_cast<size_t>(light_index) * 4u;
		shader_inputs.LightDirections[offset + 0] = lights[light_index].Direction.x;
		shader_inputs.LightDirections[offset + 1] = lights[light_index].Direction.y;
		shader_inputs.LightDirections[offset + 2] = lights[light_index].Direction.z;
		shader_inputs.LightDirections[offset + 3] = 0.0f;
		shader_inputs.LightDiffuse[offset + 0] = lights[light_index].Diffuse.r;
		shader_inputs.LightDiffuse[offset + 1] = lights[light_index].Diffuse.g;
		shader_inputs.LightDiffuse[offset + 2] = lights[light_index].Diffuse.b;
		shader_inputs.LightDiffuse[offset + 3] = light_enable[light_index] ? 1.0f : 0.0f;
	}
}

unsigned Sanitize_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state)
{
	const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, state);
	if (value != 0x12345678u) {
		return value;
	}

	switch (state) {
	case D3DTSS_COLOROP:
		return stage == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
	case D3DTSS_COLORARG0:
		return D3DTA_CURRENT;
	case D3DTSS_COLORARG1:
		return D3DTA_TEXTURE;
	case D3DTSS_COLORARG2:
		return D3DTA_CURRENT;
	case D3DTSS_ALPHAOP:
		return stage == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
	case D3DTSS_ALPHAARG0:
		return D3DTA_CURRENT;
	case D3DTSS_ALPHAARG1:
		return D3DTA_TEXTURE;
	case D3DTSS_ALPHAARG2:
		return D3DTA_CURRENT;
	default:
		return 0u;
	}
}

void Populate_Fixed_Function_Stage_Inputs(BgfxRenderer::FixedFunctionShaderInputs &shader_inputs)
{
	const unsigned texture_factor = DX8Wrapper::Get_DX8_Render_State(D3DRS_TEXTUREFACTOR);
	shader_inputs.TextureFactor = texture_factor != 0x12345678u
		? texture_factor
		: 0xffffffffu;

	shader_inputs.Stage0Color[0] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLOROP));
	shader_inputs.Stage0Color[1] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLORARG0));
	shader_inputs.Stage0Color[2] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLORARG1));
	shader_inputs.Stage0Color[3] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLORARG2));
	shader_inputs.Stage0Alpha[0] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAOP));
	shader_inputs.Stage0Alpha[1] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAARG0));
	shader_inputs.Stage0Alpha[2] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAARG1));
	shader_inputs.Stage0Alpha[3] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAARG2));
	shader_inputs.Stage1Color[0] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLOROP));
	shader_inputs.Stage1Color[1] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLORARG0));
	shader_inputs.Stage1Color[2] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLORARG1));
	shader_inputs.Stage1Color[3] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLORARG2));
	shader_inputs.Stage1Alpha[0] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAOP));
	shader_inputs.Stage1Alpha[1] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAARG0));
	shader_inputs.Stage1Alpha[2] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAARG1));
	shader_inputs.Stage1Alpha[3] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAARG2));
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
Matrix4 DX8Wrapper::TextureMatrices[MAX_TEXTURE_STAGES] = {Matrix4(true), Matrix4(true)};
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
	_MainThreadID = ThreadClass::_Get_Current_Thread_ID();
	WWDEBUG_SAY(("DX8Wrapper main thread: 0x%x\n", _MainThreadID));
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
	CurrentCaps = Ensure_Caps();
	Invalidate_Cached_Render_States();
	render_state.view = BgfxRenderer::Get_Current_View_Matrix();
	ProjectionMatrix = BgfxRenderer::Get_Current_Projection_Matrix();
	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, ResolutionWidth, ResolutionHeight));
	MissingTexture::_Init();
	TextureClass::_Init_Filters((TextureClass::TextureFilterMode)WW3D::Get_Texture_Filter());
	TheDX8MeshRenderer.Init();
	BoxRenderObjClass::Init();
	VertexMaterialClass::Init();
	PointGroupClass::_Init();
	ShatterSystem::Init();
	TextureLoader::Init();
	Set_Default_Global_Render_States();
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns(void)
{
	TextureLoader::Deinit();
	ShatterSystem::Shutdown();
	PointGroupClass::_Shutdown();
	VertexMaterialClass::Shutdown();
	BoxRenderObjClass::Shutdown();
	TheDX8MeshRenderer.Shutdown();
	MissingTexture::_Deinit();
	DynamicVBAccessClass::_Deinit();
	DynamicIBAccessClass::_Deinit();
}

void DX8Wrapper::Invalidate_Cached_Render_States(void)
{
	for (unsigned &state : RenderStates) {
		state = 0x12345678;
	}

	for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
		for (unsigned index = 0; index < 32; ++index) {
			TextureStageStates[stage][index] = 0x12345678;
		}
		Textures[stage] = nullptr;
	}

	ShaderClass::Invalidate();
}

namespace
{
inline DWORD F2DW(float value)
{
	return *reinterpret_cast<unsigned *>(&value);
}

bool Is_Material_Render_State(D3DRENDERSTATETYPE state)
{
	switch (state) {
	case D3DRS_LIGHTING:
	case D3DRS_AMBIENTMATERIALSOURCE:
	case D3DRS_DIFFUSEMATERIALSOURCE:
	case D3DRS_EMISSIVEMATERIALSOURCE:
		return true;
	default:
		return false;
	}
}

bool Is_Material_Texture_Stage_State(D3DTEXTURESTAGESTATETYPE state)
{
	switch (state) {
	case D3DTSS_TEXCOORDINDEX:
	case D3DTSS_TEXTURETRANSFORMFLAGS:
	case D3DTSS_BUMPENVMAT00:
	case D3DTSS_BUMPENVMAT01:
	case D3DTSS_BUMPENVMAT10:
	case D3DTSS_BUMPENVMAT11:
	case D3DTSS_BUMPENVLSCALE:
	case D3DTSS_BUMPENVLOFFSET:
		return true;
	default:
		return false;
	}
}
}

void DX8Wrapper::Set_Default_Global_Render_States(void)
{
	const D3DCAPS8 &caps = Get_Current_Caps()->Get_DX8_Caps();

	Set_DX8_Render_State(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	Set_DX8_Render_State(D3DRS_ZWRITEENABLE, TRUE);
	Set_DX8_Render_State(D3DRS_ALPHATESTENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_CULLMODE, D3DCULL_CW);
	Set_DX8_Render_State(D3DRS_SPECULARENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_LIGHTING, FALSE);
	Set_DX8_Render_State(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_RANGEFOGENABLE, (caps.RasterCaps & D3DPRASTERCAPS_FOGRANGE) ? TRUE : FALSE);
	Set_DX8_Render_State(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
	Set_DX8_Render_State(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
	Set_DX8_Render_State(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_COLORVERTEX, TRUE);
	Set_DX8_Render_State(D3DRS_ZBIAS, 0);
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE, F2DW(1.0f));
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00, F2DW(1.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01, F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10, F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11, F2DW(1.0f));

	for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
		TextureMatrices[stage].Make_Identity();
	}
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
		if (transform >= D3DTS_TEXTURE0 && transform < D3DTS_TEXTURE0 + MAX_TEXTURE_STAGES) {
			TextureMatrices[transform - D3DTS_TEXTURE0] = m;
			render_state.material_state_dirty = true;
		}
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
		if (transform >= D3DTS_TEXTURE0 && transform < D3DTS_TEXTURE0 + MAX_TEXTURE_STAGES) {
			m = TextureMatrices[transform - D3DTS_TEXTURE0];
		} else {
			m.Make_Identity();
		}
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
	if (Is_Material_Render_State(state)) {
		render_state.material_state_dirty = true;
	}
	++render_state_changes;
}

void DX8Wrapper::Set_Light_Environment(LightEnvironmentClass *light_env)
{
	if (light_env != nullptr) {
		const int light_count = light_env->Get_Light_Count();
		Set_DX8_Render_State(D3DRS_AMBIENT, Convert_Color(light_env->Get_Equivalent_Ambient(), 0.0f));

		D3DLIGHT8 light = {};
		light.Type = D3DLIGHT_DIRECTIONAL;

		int light_index = 0;
		for (; light_index < light_count && light_index < 4; ++light_index) {
			(Vector3 &)light.Diffuse = light_env->Get_Light_Diffuse(light_index);
			const Vector3 direction = -light_env->Get_Light_Direction(light_index);
			light.Direction = (const D3DVECTOR &)direction;
			Set_Light(light_index, &light);
		}

		for (; light_index < 4; ++light_index) {
			Set_Light(light_index, nullptr);
		}
	}
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
	if (render_state_changed == 0 && !render_state.material_state_dirty) {
		return;
	}

	const unsigned changed = render_state_changed;
	if (changed & SHADER_CHANGED) {
		render_state.shader.Apply();
	}

	unsigned texture_mask = TEXTURE0_CHANGED;
	for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage, texture_mask <<= 1) {
		if ((changed & texture_mask) == 0) {
			continue;
		}

		if (render_state.Textures[stage] != nullptr) {
			render_state.Textures[stage]->Apply(stage);
		} else {
			TextureClass::Apply_Null(stage);
		}
	}

	if ((changed & MATERIAL_CHANGED) != 0 || render_state.material_state_dirty) {
		VertexMaterialClass *material = const_cast<VertexMaterialClass *>(render_state.material);
		if (material != nullptr) {
			material->Apply();
		} else {
			VertexMaterialClass::Apply_Null();
		}
		render_state.material_state_dirty = false;
	}

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

	Apply_Render_State_Changes();

	const uint32_t triangle_index_count = static_cast<uint32_t>(polygon_count) * 3u;
	const bgfx::VertexLayout &layout = BgfxRenderer::Get_Fixed_Function_Layout();
	if (bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count ||
		bgfx::getAvailTransientIndexBuffer(triangle_index_count) < triangle_index_count) {
		return;
	}

	bgfx::TransientVertexBuffer transient_vertex_buffer;
	bgfx::TransientIndexBuffer transient_index_buffer;
	bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, layout);
	bgfx::allocTransientIndexBuffer(&transient_index_buffer, triangle_index_count);

	const unsigned tc0_index = Sanitize_Texcoord_Index(0);
	const unsigned tc0_transform_flags = Sanitize_Texture_Transform_Flags(0);
	const unsigned tc1_index = Sanitize_Texcoord_Index(1);
	const unsigned tc1_transform_flags = Sanitize_Texture_Transform_Flags(1);
	Matrix4 texture0_transform(true);
	Matrix4 texture1_transform(true);
	DX8Wrapper::Get_Transform(D3DTS_TEXTURE0, texture0_transform);
	DX8Wrapper::Get_Transform(D3DTS_TEXTURE1, texture1_transform);

	SubmissionVertex *submission_vertices = reinterpret_cast<SubmissionVertex *>(transient_vertex_buffer.data);
	VertexBufferClass::AppendLockClass vertex_lock(render_state.vertex_buffer, render_state.vba_offset + min_vertex_index, vertex_count);
	const unsigned char *source_vertices = reinterpret_cast<const unsigned char *>(vertex_lock.Get_Vertex_Array());
	for (unsigned short vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
		const unsigned char *vertex = source_vertices + vertex_index * render_state.vertex_buffer->FVF_Info().Get_FVF_Size();
		submission_vertices[vertex_index].x = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[0];
		submission_vertices[vertex_index].y = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[1];
		submission_vertices[vertex_index].z = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[2];
		if ((render_state.vertex_buffer->FVF_Info().Get_FVF() & DX8_FVF_FLAG_NORMAL) != 0u) {
			submission_vertices[vertex_index].nx = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Normal_Offset())[0];
			submission_vertices[vertex_index].ny = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Normal_Offset())[1];
			submission_vertices[vertex_index].nz = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Normal_Offset())[2];
		} else {
			submission_vertices[vertex_index].nx = 0.0f;
			submission_vertices[vertex_index].ny = 0.0f;
			submission_vertices[vertex_index].nz = 1.0f;
		}
		submission_vertices[vertex_index].diffuse = render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset() < render_state.vertex_buffer->FVF_Info().Get_FVF_Size()
			? BgfxRenderer::Convert_Packed_Color(*reinterpret_cast<const unsigned *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset()))
			: 0xffffffffu;
		submission_vertices[vertex_index].specular = render_state.vertex_buffer->FVF_Info().Get_Specular_Offset() < render_state.vertex_buffer->FVF_Info().Get_FVF_Size()
			? BgfxRenderer::Convert_Packed_Color(*reinterpret_cast<const unsigned *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Specular_Offset()))
			: 0u;
		const float raw_u0 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[0];
		const float raw_v0 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[1];
		submission_vertices[vertex_index].u0 = raw_u0;
		submission_vertices[vertex_index].v0 = raw_v0;
		float raw_u1 = 0.0f;
		float raw_v1 = 0.0f;
		if (DX8_FVF_Get_Texcoord_Count(render_state.vertex_buffer->FVF_Info().Get_FVF()) > 1) {
			raw_u1 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(1))[0];
			raw_v1 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(1))[1];
			submission_vertices[vertex_index].u1 = raw_u1;
			submission_vertices[vertex_index].v1 = raw_v1;
		} else {
			submission_vertices[vertex_index].u1 = 0.0f;
			submission_vertices[vertex_index].v1 = 0.0f;
		}

		const Vector2 resolved_tc0 = Resolve_Texture_Coordinates(
			submission_vertices[vertex_index],
			raw_u0,
			raw_v0,
			raw_u1,
			raw_v1,
			tc0_index,
			tc0_transform_flags,
			texture0_transform,
			render_state.world,
			render_state.view);
		submission_vertices[vertex_index].u0 = resolved_tc0.X;
		submission_vertices[vertex_index].v0 = resolved_tc0.Y;

		const Vector2 resolved_tc1 = Resolve_Texture_Coordinates(
			submission_vertices[vertex_index],
			raw_u0,
			raw_v0,
			raw_u1,
			raw_v1,
			tc1_index,
			tc1_transform_flags,
			texture1_transform,
			render_state.world,
			render_state.view);
		submission_vertices[vertex_index].u1 = resolved_tc1.X;
		submission_vertices[vertex_index].v1 = resolved_tc1.Y;
	}

	uint16_t *submission_indices = reinterpret_cast<uint16_t *>(transient_index_buffer.data);
	IndexBufferClass::AppendLockClass index_lock(render_state.index_buffer, render_state.iba_offset + start_index, triangle_index_count);
	const unsigned short *source_indices = index_lock.Get_Index_Array();
	for (uint32_t index = 0; index < triangle_index_count; ++index) {
		const unsigned short resolved_index = static_cast<unsigned short>(source_indices[index] + render_state.index_base_offset);
		submission_indices[index] = static_cast<uint16_t>(resolved_index - min_vertex_index);
	}

	const Matrix4 world_transform = render_state.world.Transpose();
	bgfx::setTransform(&world_transform[0][0]);
	bgfx::setVertexBuffer(0, &transient_vertex_buffer);
	bgfx::setIndexBuffer(&transient_index_buffer);
	BgfxRenderer::FixedFunctionShaderInputs shader_inputs;
	shader_inputs.FogEnabled = DX8Wrapper::Get_Fog_Enable();
	shader_inputs.FogColor = DX8Wrapper::Get_Fog_Color();
	shader_inputs.BumpEnvMatrix[0] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00));
	shader_inputs.BumpEnvMatrix[1] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01));
	shader_inputs.BumpEnvMatrix[2] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10));
	shader_inputs.BumpEnvMatrix[3] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11));
	shader_inputs.BumpEnvLuminanceScale = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE));
	shader_inputs.BumpEnvLuminanceOffset = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET));
	Populate_Fixed_Function_Stage_Inputs(shader_inputs);
	Populate_Fixed_Function_Lighting_Inputs(
		shader_inputs,
		render_state.vertex_buffer,
		RenderStates[D3DRS_AMBIENT],
		RenderStates[D3DRS_LIGHTING],
		RenderStates[D3DRS_DIFFUSEMATERIALSOURCE],
		RenderStates[D3DRS_AMBIENTMATERIALSOURCE],
		RenderStates[D3DRS_EMISSIVEMATERIALSOURCE],
		render_state.Lights,
		render_state.LightEnable);
	bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), Resolve_Texture_Handle(render_state.Textures[0]), Resolve_Sampler_Flags(render_state.Textures[0]));
	bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), Resolve_Texture_Handle(render_state.Textures[1]), Resolve_Sampler_Flags(render_state.Textures[1]));
	BgfxRenderer::Apply_Fixed_Function_Shader_Inputs(render_state.shader, shader_inputs);
	bgfx::setState(BgfxRenderer::Build_Render_State(render_state.shader, RenderStates[D3DRS_CULLMODE]));
	bgfx::submit(BgfxRenderer::Get_Main_View_Id(), BgfxRenderer::Get_Fixed_Function_Program());
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

	Apply_Render_State_Changes();

	const unsigned short polygon_count = static_cast<unsigned short>(index_count - 2);
	const uint32_t triangle_index_count = static_cast<uint32_t>(polygon_count) * 3u;

	const bgfx::VertexLayout &layout = BgfxRenderer::Get_Fixed_Function_Layout();
	if (bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count ||
		bgfx::getAvailTransientIndexBuffer(triangle_index_count) < triangle_index_count) {
		return;
	}

	bgfx::TransientVertexBuffer transient_vertex_buffer;
	bgfx::TransientIndexBuffer transient_index_buffer;
	bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, layout);
	bgfx::allocTransientIndexBuffer(&transient_index_buffer, triangle_index_count);

	const unsigned tc0_index = Sanitize_Texcoord_Index(0);
	const unsigned tc0_transform_flags = Sanitize_Texture_Transform_Flags(0);
	const unsigned tc1_index = Sanitize_Texcoord_Index(1);
	const unsigned tc1_transform_flags = Sanitize_Texture_Transform_Flags(1);
	Matrix4 texture0_transform(true);
	Matrix4 texture1_transform(true);
	DX8Wrapper::Get_Transform(D3DTS_TEXTURE0, texture0_transform);
	DX8Wrapper::Get_Transform(D3DTS_TEXTURE1, texture1_transform);

	SubmissionVertex *submission_vertices = reinterpret_cast<SubmissionVertex *>(transient_vertex_buffer.data);
	VertexBufferClass::AppendLockClass vertex_lock(render_state.vertex_buffer, render_state.vba_offset + min_vertex_index, vertex_count);
	const unsigned char *source_vertices = reinterpret_cast<const unsigned char *>(vertex_lock.Get_Vertex_Array());
	for (unsigned short vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
		const unsigned char *vertex = source_vertices + vertex_index * render_state.vertex_buffer->FVF_Info().Get_FVF_Size();
		submission_vertices[vertex_index].x = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[0];
		submission_vertices[vertex_index].y = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[1];
		submission_vertices[vertex_index].z = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Location_Offset())[2];
		if ((render_state.vertex_buffer->FVF_Info().Get_FVF() & DX8_FVF_FLAG_NORMAL) != 0u) {
			submission_vertices[vertex_index].nx = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Normal_Offset())[0];
			submission_vertices[vertex_index].ny = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Normal_Offset())[1];
			submission_vertices[vertex_index].nz = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Normal_Offset())[2];
		} else {
			submission_vertices[vertex_index].nx = 0.0f;
			submission_vertices[vertex_index].ny = 0.0f;
			submission_vertices[vertex_index].nz = 1.0f;
		}
		submission_vertices[vertex_index].diffuse = render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset() < render_state.vertex_buffer->FVF_Info().Get_FVF_Size()
			? BgfxRenderer::Convert_Packed_Color(*reinterpret_cast<const unsigned *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Diffuse_Offset()))
			: 0xffffffffu;
		submission_vertices[vertex_index].specular = render_state.vertex_buffer->FVF_Info().Get_Specular_Offset() < render_state.vertex_buffer->FVF_Info().Get_FVF_Size()
			? BgfxRenderer::Convert_Packed_Color(*reinterpret_cast<const unsigned *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Specular_Offset()))
			: 0u;
		const float raw_u0 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[0];
		const float raw_v0 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(0))[1];
		submission_vertices[vertex_index].u0 = raw_u0;
		submission_vertices[vertex_index].v0 = raw_v0;
		float raw_u1 = 0.0f;
		float raw_v1 = 0.0f;
		if (DX8_FVF_Get_Texcoord_Count(render_state.vertex_buffer->FVF_Info().Get_FVF()) > 1) {
			raw_u1 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(1))[0];
			raw_v1 = reinterpret_cast<const float *>(vertex + render_state.vertex_buffer->FVF_Info().Get_Tex_Offset(1))[1];
			submission_vertices[vertex_index].u1 = raw_u1;
			submission_vertices[vertex_index].v1 = raw_v1;
		} else {
			submission_vertices[vertex_index].u1 = 0.0f;
			submission_vertices[vertex_index].v1 = 0.0f;
		}

		const Vector2 resolved_tc0 = Resolve_Texture_Coordinates(
			submission_vertices[vertex_index],
			raw_u0,
			raw_v0,
			raw_u1,
			raw_v1,
			tc0_index,
			tc0_transform_flags,
			texture0_transform,
			render_state.world,
			render_state.view);
		submission_vertices[vertex_index].u0 = resolved_tc0.X;
		submission_vertices[vertex_index].v0 = resolved_tc0.Y;

		const Vector2 resolved_tc1 = Resolve_Texture_Coordinates(
			submission_vertices[vertex_index],
			raw_u0,
			raw_v0,
			raw_u1,
			raw_v1,
			tc1_index,
			tc1_transform_flags,
			texture1_transform,
			render_state.world,
			render_state.view);
		submission_vertices[vertex_index].u1 = resolved_tc1.X;
		submission_vertices[vertex_index].v1 = resolved_tc1.Y;
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

	const Matrix4 world_transform = render_state.world.Transpose();
	bgfx::setTransform(&world_transform[0][0]);
	bgfx::setVertexBuffer(0, &transient_vertex_buffer);
	bgfx::setIndexBuffer(&transient_index_buffer);
	BgfxRenderer::FixedFunctionShaderInputs shader_inputs;
	shader_inputs.FogEnabled = DX8Wrapper::Get_Fog_Enable();
	shader_inputs.FogColor = DX8Wrapper::Get_Fog_Color();
	shader_inputs.BumpEnvMatrix[0] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00));
	shader_inputs.BumpEnvMatrix[1] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01));
	shader_inputs.BumpEnvMatrix[2] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10));
	shader_inputs.BumpEnvMatrix[3] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11));
	shader_inputs.BumpEnvLuminanceScale = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE));
	shader_inputs.BumpEnvLuminanceOffset = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET));
	Populate_Fixed_Function_Stage_Inputs(shader_inputs);
	Populate_Fixed_Function_Lighting_Inputs(
		shader_inputs,
		render_state.vertex_buffer,
		RenderStates[D3DRS_AMBIENT],
		RenderStates[D3DRS_LIGHTING],
		RenderStates[D3DRS_DIFFUSEMATERIALSOURCE],
		RenderStates[D3DRS_AMBIENTMATERIALSOURCE],
		RenderStates[D3DRS_EMISSIVEMATERIALSOURCE],
		render_state.Lights,
		render_state.LightEnable);
	bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), Resolve_Texture_Handle(render_state.Textures[0]), Resolve_Sampler_Flags(render_state.Textures[0]));
	bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), Resolve_Texture_Handle(render_state.Textures[1]), Resolve_Sampler_Flags(render_state.Textures[1]));
	BgfxRenderer::Apply_Fixed_Function_Shader_Inputs(render_state.shader, shader_inputs);
	bgfx::setState(BgfxRenderer::Build_Render_State(render_state.shader, RenderStates[D3DRS_CULLMODE]));
	bgfx::submit(BgfxRenderer::Get_Main_View_Id(), BgfxRenderer::Get_Fixed_Function_Program());
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


namespace
{
unsigned Bytes_Per_Pixel(D3DFORMAT format)
{
    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        return 4;
    case D3DFMT_R8G8B8:
        return 3;
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
    case D3DFMT_A8L8:
    case D3DFMT_V8U8:
        return 2;
    case D3DFMT_A8:
    case D3DFMT_L8:
    case D3DFMT_A4L4:
    case D3DFMT_R3G3B2:
        return 1;
    case D3DFMT_A8R3G3B2:
        return 2;
    default:
        return 4;
    }
}

unsigned Block_Size(D3DFORMAT format)
{
    switch (format) {
    case D3DFMT_DXT1:
        return 8;
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        return 16;
    default:
        return 0;
    }
}

unsigned Surface_Pitch(unsigned width, D3DFORMAT format)
{
    const unsigned block_size = Block_Size(format);
    if (block_size != 0) {
        return std::max(1u, (width + 3u) / 4u) * block_size;
    }
    return width * Bytes_Per_Pixel(format);
}

unsigned Surface_Size(unsigned width, unsigned height, D3DFORMAT format)
{
    const unsigned block_size = Block_Size(format);
    if (block_size != 0) {
        return std::max(1u, (width + 3u) / 4u) * std::max(1u, (height + 3u) / 4u) * block_size;
    }
    return Surface_Pitch(width, format) * height;
}

class BgfxSurface8 final : public IDirect3DSurface8
{
public:
    BgfxSurface8(unsigned width, unsigned height, D3DFORMAT format)
        : ref_count_(1), desc_{format, D3DRTYPE_SURFACE, 0, D3DPOOL_SYSTEMMEM, Surface_Size(width, height, format), 0, width, height}, pitch_(Surface_Pitch(width, format)), data_(desc_.Size), locked_(false)
    {
    }

    ULONG AddRef() override { return ++ref_count_; }
    ULONG Release() override
    {
        const ULONG remaining = --ref_count_;
        if (remaining == 0) {
            delete this;
        }
        return remaining;
    }
    HRESULT GetDesc(D3DSURFACE_DESC *desc) override
    {
        if (desc == nullptr) {
            return kD3DErrInvalidCall;
        }
        *desc = desc_;
        return D3D_OK;
    }
    HRESULT LockRect(D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD) override
    {
        if (locked_ || locked_rect == nullptr) {
            return kD3DErrInvalidCall;
        }
        locked_ = true;
        locked_rect->Pitch = static_cast<int>(pitch_);
        unsigned offset = 0;
        if (rect != nullptr) {
            const unsigned block_size = Block_Size(desc_.Format);
            if (block_size != 0) {
                offset = (static_cast<unsigned>(rect->top) / 4u) * pitch_ + (static_cast<unsigned>(rect->left) / 4u) * block_size;
            } else {
                offset = static_cast<unsigned>(rect->top) * pitch_ + static_cast<unsigned>(rect->left) * Bytes_Per_Pixel(desc_.Format);
            }
        }
        locked_rect->pBits = data_.data() + offset;
        return D3D_OK;
    }
    HRESULT UnlockRect() override
    {
        if (!locked_) {
            return kD3DErrInvalidCall;
        }
        locked_ = false;
        return D3D_OK;
    }

    unsigned char *Data() { return data_.data(); }
    const unsigned char *Data() const { return data_.data(); }
    unsigned Pitch() const { return pitch_; }

private:
    ULONG ref_count_;
    D3DSURFACE_DESC desc_;
    unsigned pitch_;
    std::vector<unsigned char> data_;
    bool locked_;
};

class BgfxTexture8 final : public IDirect3DTexture8
{
public:
    BgfxTexture8(unsigned width, unsigned height, D3DFORMAT format, unsigned mip_count, D3DPOOL pool)
        : ref_count_(1), pool_(pool)
    {
        unsigned level_width = std::max(1u, width);
        unsigned level_height = std::max(1u, height);
        const unsigned resolved_mip_count = (mip_count == 0) ? 1u : mip_count;
        surfaces_.reserve(resolved_mip_count);
        for (unsigned i = 0; i < resolved_mip_count; ++i) {
            surfaces_.push_back(new BgfxSurface8(level_width, level_height, format));
            level_width = std::max(1u, level_width / 2u);
            level_height = std::max(1u, level_height / 2u);
        }
    }

    ~BgfxTexture8() override
    {
        for (BgfxSurface8 *surface : surfaces_) {
            surface->Release();
        }
    }

    ULONG AddRef() override { return ++ref_count_; }
    ULONG Release() override
    {
        const ULONG remaining = --ref_count_;
        if (remaining == 0) {
            delete this;
        }
        return remaining;
    }
    UINT GetLevelCount() override { return static_cast<UINT>(surfaces_.size()); }
    HRESULT GetSurfaceLevel(UINT level, IDirect3DSurface8 **surface) override
    {
        if (surface == nullptr || level >= surfaces_.size()) {
            return kD3DErrInvalidCall;
        }
        surfaces_[level]->AddRef();
        *surface = surfaces_[level];
        return D3D_OK;
    }
    HRESULT LockRect(UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags) override
    {
        if (level >= surfaces_.size()) {
            return kD3DErrInvalidCall;
        }
        return surfaces_[level]->LockRect(locked_rect, rect, flags);
    }
    HRESULT UnlockRect(UINT level) override
    {
        if (level >= surfaces_.size()) {
            return kD3DErrInvalidCall;
        }
        return surfaces_[level]->UnlockRect();
    }
    D3DPOOL Pool() const { return pool_; }

private:
    ULONG ref_count_;
    D3DPOOL pool_;
    std::vector<BgfxSurface8 *> surfaces_;
};

DX8Caps *Ensure_Caps()
{
    if (g_stub_caps == nullptr) {
        D3DCAPS8 caps = {};
        caps.AdapterOrdinal = 0;
        caps.DeviceType = D3DDEVTYPE_HAL;
        caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
        caps.RasterCaps = D3DPRASTERCAPS_ZBIAS;
        caps.TextureFilterCaps = D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MAGFANISOTROPIC;
        caps.TextureOpCaps = D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_MODULATE | D3DTEXOPCAPS_ADD | D3DTEXOPCAPS_SUBTRACT | D3DTEXOPCAPS_ADDSMOOTH | D3DTEXOPCAPS_BLENDTEXTUREALPHA;
        caps.MaxTextureWidth = 16384;
        caps.MaxTextureHeight = 16384;
        caps.MaxSimultaneousTextures = 2;

        D3DADAPTER_IDENTIFIER8 adapter = {};
        const char *name = bgfx::getRendererName(bgfx::getRendererType());
        std::snprintf(adapter.Driver, sizeof(adapter.Driver), "bgfx");
        std::snprintf(adapter.Description, sizeof(adapter.Description), "%s", name != nullptr ? name : "bgfx");
        std::snprintf(adapter.DeviceName, sizeof(adapter.DeviceName), "bgfx");
        adapter.DriverVersion.HighPart = 1;
        adapter.DriverVersion.LowPart = 0;
        g_stub_caps = new DX8Caps(nullptr, caps, WW3D_FORMAT_A8R8G8B8, adapter);
    }
    return g_stub_caps;
}

void Copy_Surface_Level(BgfxSurface8 &surface, const unsigned char *source_pixels, int source_width, int source_height, int source_pixel_size)
{
    const D3DSURFACE_DESC &desc = [&]() -> const D3DSURFACE_DESC & { static D3DSURFACE_DESC tmp; surface.GetDesc(&tmp); return tmp; }();
    const unsigned row_size = Block_Size(desc.Format) != 0 ? Surface_Pitch(desc.Width, desc.Format) : static_cast<unsigned>(source_width) * static_cast<unsigned>(source_pixel_size);
    const unsigned row_count = Block_Size(desc.Format) != 0 ? std::max(1u, (desc.Height + 3u) / 4u) : desc.Height;
    for (unsigned row = 0; row < row_count; ++row) {
        std::memcpy(surface.Data() + row * surface.Pitch(), source_pixels + row * row_size, row_size);
    }
}
}

void DX8Wrapper::Set_DX8_Material(const D3DMATERIAL8 *mat)
{
    ++material_changes;
    if (mat != nullptr) {
        g_current_dx8_material = *mat;
    }
}

void DX8Wrapper::Set_DX8_Light(int index, D3DLIGHT8 *light)
{
    if (index < 0 || index >= 4) {
        return;
    }
    ++light_changes;
    if (light != nullptr) {
        render_state.Lights[index] = *light;
        render_state.LightEnable[index] = true;
    } else {
        std::memset(&render_state.Lights[index], 0, sizeof(D3DLIGHT8));
        render_state.LightEnable[index] = false;
    }
    CurrentDX8LightEnables[index] = render_state.LightEnable[index];
}

void DX8Wrapper::Set_DX8_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state, unsigned value)
{
    if (stage < MAX_TEXTURE_STAGES && state < 32) {
        TextureStageStates[stage][state] = value;
        if (Is_Material_Texture_Stage_State(state)) {
            render_state.material_state_dirty = true;
        }
        ++texture_stage_state_changes;
    }
}

void DX8Wrapper::Set_DX8_Texture(unsigned int stage, IDirect3DBaseTexture8 *texture)
{
    if (stage >= MAX_TEXTURE_STAGES) {
        return;
    }
    Textures[stage] = texture;
    ++texture_changes;
}

void DX8Wrapper::Set_Light(unsigned index, const LightClass &light)
{
    if (index >= 4) {
        return;
    }

    D3DLIGHT8 dx_light = {};
    dx_light.Type = (light.Get_Type() == LightClass::DIRECTIONAL) ? D3DLIGHT_DIRECTIONAL : D3DLIGHT_POINT;
    Vector3 ambient;
    Vector3 diffuse;
    Vector3 specular;
    light.Get_Ambient(&ambient);
    light.Get_Diffuse(&diffuse);
    light.Get_Specular(&specular);
    const Vector3 position = light.Get_Position();
    Vector3 direction;
    light.Get_Spot_Direction(direction);
    dx_light.Ambient = {ambient.X, ambient.Y, ambient.Z, 1.0f};
    dx_light.Diffuse = {diffuse.X, diffuse.Y, diffuse.Z, 1.0f};
    dx_light.Specular = {specular.X, specular.Y, specular.Z, 1.0f};
    dx_light.Position = {position.X, position.Y, position.Z};
    dx_light.Direction = {direction.X, direction.Y, direction.Z};
    double far_start = 0.0;
    double far_end = 0.0;
    light.Get_Far_Attenuation_Range(far_start, far_end);
    dx_light.Range = static_cast<float>(far_end);
    dx_light.Attenuation0 = 1.0f;
    dx_light.Attenuation1 = 0.0f;
    dx_light.Attenuation2 = 0.0f;
    Set_DX8_Light(static_cast<int>(index), &dx_light);
}

void DX8Wrapper::Begin_Statistics() {}
void DX8Wrapper::End_Statistics() {}

bool DX8Wrapper::Set_Any_Render_Device(void)
{
    return Set_Render_Device(0, ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed ? 1 : 0, false);
}

bool DX8Wrapper::Set_Render_Device(const char *, int width, int height, int bits, int windowed, bool resize_window)
{
    return Set_Render_Device(0, width, height, bits, windowed, resize_window);
}

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed, bool)
{
    CurRenderDevice = (dev < 0) ? 0 : dev;
    if (width > 0) {
        ResolutionWidth = width;
    }
    if (height > 0) {
        ResolutionHeight = height;
    }
    if (bits > 0) {
        BitDepth = bits;
    }
    if (windowed >= 0) {
        IsWindowed = (windowed != 0);
    }
    Ensure_Caps();
    CurrentCaps = g_stub_caps;
    if (BgfxRenderer::Is_Initted()) {
        BgfxRenderer::Reset();
    }
    return true;
}

bool DX8Wrapper::Set_Next_Render_Device(void)
{
    return true;
}

bool DX8Wrapper::Toggle_Windowed(void)
{
    IsWindowed = !IsWindowed;
    return Set_Render_Device(CurRenderDevice, ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed ? 1 : 0, false);
}

int DX8Wrapper::Get_Render_Device_Count(void)
{
    return 1;
}

int DX8Wrapper::Get_Render_Device(void)
{
    return CurRenderDevice < 0 ? 0 : CurRenderDevice;
}

const RenderDeviceDescClass &DX8Wrapper::Get_Render_Device_Desc(int)
{
    const char *renderer_name = bgfx::getRendererName(bgfx::getRendererType());
    if (renderer_name == nullptr) {
        renderer_name = "bgfx";
    }
    g_render_device_desc.reset_resolution_list();
    g_render_device_desc.set_device_name(renderer_name);
    g_render_device_desc.set_device_vendor("bgfx");
    g_render_device_desc.set_device_platform("Cross-platform");
    g_render_device_desc.set_driver_name("bgfx");
    g_render_device_desc.set_driver_vendor("bgfx");
    g_render_device_desc.set_driver_version("1");
    g_render_device_desc.set_hardware_name(renderer_name);
    g_render_device_desc.set_hardware_vendor("bgfx");
    g_render_device_desc.set_hardware_chipset(renderer_name);
    g_render_device_desc.Caps = Ensure_Caps()->Get_DX8_Caps();
    g_render_device_desc.AdapterIdentifier = D3DADAPTER_IDENTIFIER8{};
    std::snprintf(g_render_device_desc.AdapterIdentifier.Driver, sizeof(g_render_device_desc.AdapterIdentifier.Driver), "bgfx");
    std::snprintf(g_render_device_desc.AdapterIdentifier.Description, sizeof(g_render_device_desc.AdapterIdentifier.Description), "%s", renderer_name);
    std::snprintf(g_render_device_desc.AdapterIdentifier.DeviceName, sizeof(g_render_device_desc.AdapterIdentifier.DeviceName), "bgfx");
    g_render_device_desc.add_resolution(ResolutionWidth, ResolutionHeight, BitDepth);
    return g_render_device_desc;
}

const char *DX8Wrapper::Get_Render_Device_Name(int)
{
    return Get_Render_Device_Desc(0).Get_Device_Name();
}

bool DX8Wrapper::Set_Device_Resolution(int width, int height, int bits, int windowed, bool resize_window)
{
    return Set_Render_Device(CurRenderDevice, width, height, bits, windowed, resize_window);
}

bool DX8Wrapper::Registry_Save_Render_Device(const char *)
{
    return true;
}

bool DX8Wrapper::Registry_Save_Render_Device(const char *, int, int, int, int, bool, int)
{
    return true;
}

bool DX8Wrapper::Registry_Load_Render_Device(const char *, bool resize_window)
{
    return Set_Render_Device(CurRenderDevice, ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed ? 1 : 0, resize_window);
}

bool DX8Wrapper::Registry_Load_Render_Device(const char *, char *device, int device_len, int &width, int &height, int &depth, int &windowed, int &texture_depth)
{
    if (device != nullptr && device_len > 0) {
        std::snprintf(device, static_cast<size_t>(device_len), "%s", Get_Render_Device_Name(0));
    }
    width = ResolutionWidth;
    height = ResolutionHeight;
    depth = BitDepth;
    windowed = IsWindowed ? 1 : 0;
    texture_depth = TextureBitDepth;
    return true;
}

void DX8Wrapper::Set_Swap_Interval(int swap)
{
    g_swap_interval = swap;
}

int DX8Wrapper::Get_Swap_Interval(void)
{
    return g_swap_interval;
}

IDirect3DSwapChain8 *DX8Wrapper::Create_Additional_Swap_Chain(HWND)
{
    return nullptr;
}

IDirect3DTexture8 *DX8Wrapper::_Create_DX8_Texture(unsigned int width, unsigned int height, WW3DFormat format, TextureClass::MipCountType mip_level_count, D3DPOOL pool, bool)
{
    return new BgfxTexture8(width, height, WW3DFormat_To_D3DFormat(format), static_cast<unsigned>(mip_level_count == TextureClass::MIP_LEVELS_ALL ? 1 : mip_level_count), pool);
}

IDirect3DTexture8 *DX8Wrapper::_Create_DX8_Texture(const char *filename, TextureClass::MipCountType mip_level_count)
{
    IDirect3DSurface8 *surface = _Create_DX8_Surface(filename);
    if (surface == nullptr) {
        return nullptr;
    }
    IDirect3DTexture8 *texture = _Create_DX8_Texture(surface, mip_level_count);
    surface->Release();
    return texture;
}

IDirect3DTexture8 *DX8Wrapper::_Create_DX8_Texture(IDirect3DSurface8 *surface, TextureClass::MipCountType mip_level_count)
{
    if (surface == nullptr) {
        return nullptr;
    }
    D3DSURFACE_DESC desc = {};
    if (FAILED(surface->GetDesc(&desc))) {
        return nullptr;
    }

    BgfxTexture8 *texture = new BgfxTexture8(desc.Width, desc.Height, desc.Format, static_cast<unsigned>(mip_level_count == TextureClass::MIP_LEVELS_ALL ? 1 : mip_level_count), D3DPOOL_MANAGED);
    BgfxSurface8 *level0 = nullptr;
    IDirect3DSurface8 *surface_level = nullptr;
    if (SUCCEEDED(texture->GetSurfaceLevel(0, &surface_level))) {
        level0 = static_cast<BgfxSurface8 *>(surface_level);
        D3DLOCKED_RECT locked = {};
        if (SUCCEEDED(surface->LockRect(&locked, nullptr, 0))) {
            Copy_Surface_Level(*level0, static_cast<const unsigned char *>(locked.pBits), static_cast<int>(desc.Width), static_cast<int>(desc.Height), static_cast<int>(Bytes_Per_Pixel(desc.Format)));
            surface->UnlockRect();
        }
        surface_level->Release();
    }
    return texture;
}

IDirect3DSurface8 *DX8Wrapper::_Create_DX8_Surface(unsigned int width, unsigned int height, WW3DFormat format)
{
    return new BgfxSurface8(width, height, WW3DFormat_To_D3DFormat(format));
}

IDirect3DSurface8 *DX8Wrapper::_Create_DX8_Surface(const char *filename)
{
    if (filename == nullptr || filename[0] == '\0') {
        return nullptr;
    }

    StringClass filename_string(filename, true);
    SurfaceClass *loaded_surface = TextureLoader::Load_Surface_Immediate(
        filename_string,
        WW3D_FORMAT_UNKNOWN,
        true);
    if (loaded_surface == nullptr) {
        loaded_surface = MissingTexture::_Create_Missing_Surface_Instance();
    }
    if (loaded_surface == nullptr) {
        return nullptr;
    }

    IDirect3DSurface8 *surface = loaded_surface->Acquire_DX8_Surface();
    loaded_surface->Release_Ref();
    return surface;
}

IDirect3DSurface8 *DX8Wrapper::_Get_DX8_Front_Buffer()
{
    return nullptr;
}

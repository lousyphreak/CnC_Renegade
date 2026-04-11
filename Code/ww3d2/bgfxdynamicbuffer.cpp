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

RenderDeviceDescClass g_render_device_desc;
DX8Caps *g_stub_caps = nullptr;
int g_swap_interval = 0;
constexpr HRESULT kD3DErrInvalidCall = -11;
std::unordered_map<const DX8VertexBufferClass *, std::vector<unsigned char>> g_dx8_vertex_buffers;

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

static DX8Caps *Ensure_Caps()
{
	if (g_stub_caps == nullptr) {
		D3DCAPS8 caps = {};
		caps.AdapterOrdinal = 0;
		caps.DeviceType = D3DDEVTYPE_HAL;
		caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
		caps.RasterCaps = D3DPRASTERCAPS_ZBIAS;
		caps.TextureFilterCaps = D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MAGFANISOTROPIC;
		caps.TextureOpCaps =
			D3DTEXOPCAPS_DISABLE |
			D3DTEXOPCAPS_SELECTARG1 |
			D3DTEXOPCAPS_MODULATE |
			D3DTEXOPCAPS_ADD |
			D3DTEXOPCAPS_ADDSMOOTH |
			D3DTEXOPCAPS_BLENDTEXTUREALPHA |
			D3DTEXOPCAPS_BLENDCURRENTALPHA |
			D3DTEXOPCAPS_BUMPENVMAP |
			D3DTEXOPCAPS_BUMPENVMAPLUMINANCE |
			D3DTEXOPCAPS_DOTPRODUCT3 |
			D3DTEXOPCAPS_SUBTRACT;
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
void DX8Wrapper::Set_DX8_Material(const D3DMATERIAL8 *mat)
{
    ++material_changes;
    (void)mat;
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

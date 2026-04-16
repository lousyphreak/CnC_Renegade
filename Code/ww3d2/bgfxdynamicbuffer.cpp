#include "dx8wrapper.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>

#include "indexbuffer.h"
#include "vertexbuffer.h"
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
static const VertexFormatInfoClass kDynamicFVFInfo(dynamic_vertex_format);

RenderVertexBufferClass *g_dynamic_render_vertex_buffer = nullptr;
bool g_dynamic_render_vertex_buffer_in_use = false;
unsigned short g_dynamic_render_vertex_buffer_size = kDefaultDynamicVertexCount;
unsigned short g_dynamic_render_vertex_buffer_offset = 0;

SortingVertexBufferClass *g_dynamic_sorting_vertex_buffer = nullptr;
bool g_dynamic_sorting_vertex_buffer_in_use = false;
unsigned short g_dynamic_sorting_vertex_buffer_size = 0;
unsigned short g_dynamic_sorting_vertex_buffer_offset = 0;
std::vector<SortingVertexBufferClass *> g_stale_dynamic_sorting_vertex_buffers;

unsigned g_vertex_buffer_count = 0;
unsigned g_vertex_buffer_total_vertices = 0;
unsigned g_vertex_buffer_total_size = 0;

RenderDeviceDescClass g_render_device_desc;
DX8Caps *g_bgfx_caps = nullptr;
int g_swap_interval = 1;
constexpr HRESULT kD3DErrInvalidCall = -11;

void Release_Stale_Dynamic_Sorting_Vertex_Buffers()
{
	auto it = g_stale_dynamic_sorting_vertex_buffers.begin();
	while (it != g_stale_dynamic_sorting_vertex_buffers.end()) {
		SortingVertexBufferClass *buffer = *it;
		if ((buffer == nullptr) || (buffer->Engine_Refs() == 0)) {
			if (buffer != nullptr) {
				buffer->Release_Ref();
			}
			it = g_stale_dynamic_sorting_vertex_buffers.erase(it);
		} else {
			++it;
		}
	}
}

void Swizzle_Vertex_Colors_In_Place(unsigned char *vertex_data, const VertexFormatInfoClass &fvf_info, unsigned short vertex_count)
{
	const unsigned fvf = fvf_info.Get_Vertex_Format();
	const unsigned vertex_size = fvf_info.Get_Vertex_Size();
	const bool has_diffuse = (fvf & VERTEX_FORMAT_FLAG_DIFFUSE) != 0u;
	const bool has_specular = (fvf & VERTEX_FORMAT_FLAG_SPECULAR) != 0u;

	if (has_diffuse) {
		const unsigned diffuse_offset = fvf_info.Get_Diffuse_Offset();
		for (unsigned short v = 0; v < vertex_count; ++v) {
			uint32_t *color_ptr = reinterpret_cast<uint32_t *>(
				vertex_data + static_cast<size_t>(v) * vertex_size + diffuse_offset);
			*color_ptr = BgfxRenderer::Convert_Packed_Color(*color_ptr);
		}
	}

	if (has_specular) {
		const unsigned specular_offset = fvf_info.Get_Specular_Offset();
		for (unsigned short v = 0; v < vertex_count; ++v) {
			uint32_t *color_ptr = reinterpret_cast<uint32_t *>(
				vertex_data + static_cast<size_t>(v) * vertex_size + specular_offset);
			*color_ptr = BgfxRenderer::Convert_Packed_Color(*color_ptr);
		}
	}
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
DX8Caps *DX8Wrapper::CurrentCaps = g_bgfx_caps;
D3DADAPTER_IDENTIFIER8 DX8Wrapper::CurrentAdapterIdentifier = {};
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

RenderStateStruct::RenderStateStruct()
	: material(nullptr),
	  material_state{{1.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		1.0f},
	  material_crc(0),
	  material_state_dirty(false),
	  vertex_buffer(nullptr),
	  index_buffer(nullptr)
{
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		Textures[i] = nullptr;
	}
}

RenderStateStruct::~RenderStateStruct()
{
	REF_PTR_RELEASE(material);
	REF_PTR_RELEASE(vertex_buffer);
	REF_PTR_RELEASE(index_buffer);
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		REF_PTR_RELEASE(Textures[i]);
	}
}

RenderStateStruct &RenderStateStruct::operator=(const RenderStateStruct &src)
{
	REF_PTR_SET(material, src.material);
	material_state = src.material_state;
	material_crc = src.material_crc;
	material_state_dirty = src.material_state_dirty;
	REF_PTR_SET(vertex_buffer, src.vertex_buffer);
	REF_PTR_SET(index_buffer, src.index_buffer);
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		REF_PTR_SET(Textures[i], src.Textures[i]);
	}

	LightEnable[0] = src.LightEnable[0];
	LightEnable[1] = src.LightEnable[1];
	LightEnable[2] = src.LightEnable[2];
	LightEnable[3] = src.LightEnable[3];
	if (LightEnable[0]) {
		Lights[0] = src.Lights[0];
		if (LightEnable[1]) {
			Lights[1] = src.Lights[1];
			if (LightEnable[2]) {
				Lights[2] = src.Lights[2];
				if (LightEnable[3]) {
					Lights[3] = src.Lights[3];
				}
			}
		}
	}

	shader = src.shader;
	world = src.world;
	view = src.view;
	vertex_buffer_type = src.vertex_buffer_type;
	index_buffer_type = src.index_buffer_type;
	vba_offset = src.vba_offset;
	vba_count = src.vba_count;
	iba_offset = src.iba_offset;
	index_base_offset = src.index_base_offset;

	return *this;
}

void DX8Wrapper::Set_Render_State(const RenderStateStruct &state)
{
	if (render_state.index_buffer != nullptr) {
		render_state.index_buffer->Release_Engine_Ref();
	}

	if (render_state.vertex_buffer != nullptr) {
		render_state.vertex_buffer->Release_Engine_Ref();
	}

	render_state = state;
	render_state_changed = 0xffffffff;

	if (render_state.index_buffer != nullptr) {
		render_state.index_buffer->Add_Engine_Ref();
	}

	if (render_state.vertex_buffer != nullptr) {
		render_state.vertex_buffer->Add_Engine_Ref();
	}
}

void DX8Wrapper::Release_Render_State()
{
	if (render_state.index_buffer != nullptr) {
		render_state.index_buffer->Release_Engine_Ref();
	}

	if (render_state.vertex_buffer != nullptr) {
		render_state.vertex_buffer->Release_Engine_Ref();
	}

	REF_PTR_RELEASE(render_state.vertex_buffer);
	REF_PTR_RELEASE(render_state.index_buffer);
	REF_PTR_RELEASE(render_state.material);
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		REF_PTR_RELEASE(render_state.Textures[i]);
	}
}

static DX8Caps *Ensure_Caps()
{
	if (g_bgfx_caps == nullptr) {
		D3DCAPS8 caps = {};
		caps.AdapterOrdinal = 0;
		caps.DeviceType = D3DDEVTYPE_HAL;
		caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
		caps.RasterCaps = D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_FOGRANGE;
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
		g_bgfx_caps = new DX8Caps(caps, WW3D_FORMAT_A8R8G8B8, adapter);
	}

	return g_bgfx_caps;
}

VertexBufferClass::VertexBufferClass(unsigned type_, unsigned FVF, unsigned short vertex_count_)
	: type(type_), VertexCount(vertex_count_), engine_refs(0), fvf_info(new VertexFormatInfoClass(FVF))
{
	++g_vertex_buffer_count;
	g_vertex_buffer_total_vertices += VertexCount;
	g_vertex_buffer_total_size += VertexCount * fvf_info->Get_Vertex_Size();
}

VertexBufferClass::~VertexBufferClass()
{
	--g_vertex_buffer_count;
	g_vertex_buffer_total_vertices -= VertexCount;
	g_vertex_buffer_total_size -= VertexCount * fvf_info->Get_Vertex_Size();
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
	case BUFFER_TYPE_RENDER:
	case BUFFER_TYPE_DYNAMIC_RENDER:
		Vertices = static_cast<RenderVertexBufferClass *>(vertex_buffer)->Get_Source_Vertex_Data();
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
	case BUFFER_TYPE_RENDER:
	case BUFFER_TYPE_DYNAMIC_RENDER:
		Vertices = static_cast<RenderVertexBufferClass *>(vertex_buffer)->Get_Source_Vertex_Data() + start_index * vertex_buffer->Vertex_Format_Info().Get_Vertex_Size();
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
	: VertexBufferClass(BUFFER_TYPE_SORTING, dynamic_vertex_format, vertex_count), VertexBuffer(new VertexFormatXYZNDUV2[vertex_count])
{
}

SortingVertexBufferClass::~SortingVertexBufferClass()
{
	delete[] VertexBuffer;
}

RenderVertexBufferClass::RenderVertexBufferClass(unsigned FVF, unsigned short vertex_count_, UsageType, unsigned type)
#if !RENEGADE_WITH_BGFX_RENDERER
	: VertexBufferClass(type, FVF, vertex_count_), VertexBuffer(nullptr)
#else
	: VertexBufferClass(type, FVF, vertex_count_),
	  BgfxVertexBuffer(BGFX_INVALID_HANDLE),
	  BgfxDynamicVertexBuffer(BGFX_INVALID_HANDLE),
	  BgfxUsesDynamicBuffer(type == BUFFER_TYPE_DYNAMIC_RENDER),
	  BgfxVertexBufferDirty(true),
	  VertexData(static_cast<size_t>(Vertex_Format_Info().Get_Vertex_Size()) * vertex_count_),
	  BgfxLayoutInitialized(false)
#endif
{
	WWASSERT(type == BUFFER_TYPE_RENDER || type == BUFFER_TYPE_DYNAMIC_RENDER);
}

RenderVertexBufferClass::RenderVertexBufferClass(const Vector3 *vertices, const Vector3 *normals, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: RenderVertexBufferClass(VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX1 | VERTEX_FORMAT_FLAG_NORMAL, vertex_count_, usage)
{
	Copy(vertices, normals, tex_coords, 0, vertex_count_);
}

RenderVertexBufferClass::RenderVertexBufferClass(const Vector3 *vertices, const Vector3 *normals, const Vector4 *diffuse, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: RenderVertexBufferClass(VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX1 | VERTEX_FORMAT_FLAG_NORMAL | VERTEX_FORMAT_FLAG_DIFFUSE, vertex_count_, usage)
{
	Copy(vertices, normals, tex_coords, diffuse, 0, vertex_count_);
}

RenderVertexBufferClass::RenderVertexBufferClass(const Vector3 *vertices, const Vector4 *diffuse, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: RenderVertexBufferClass(VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX1 | VERTEX_FORMAT_FLAG_DIFFUSE, vertex_count_, usage)
{
	Copy(vertices, tex_coords, diffuse, 0, vertex_count_);
}

RenderVertexBufferClass::RenderVertexBufferClass(const Vector3 *vertices, const Vector2 *tex_coords, unsigned short vertex_count_, UsageType usage)
	: RenderVertexBufferClass(VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX1, vertex_count_, usage)
{
	Copy(vertices, tex_coords, 0, vertex_count_);
}

RenderVertexBufferClass::~RenderVertexBufferClass()
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (bgfx::isValid(BgfxVertexBuffer)) {
		if (BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxVertexBuffer);
		}
		BgfxVertexBuffer = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(BgfxDynamicVertexBuffer)) {
		if (BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxDynamicVertexBuffer);
		}
		BgfxDynamicVertexBuffer = BGFX_INVALID_HANDLE;
	}
#endif
}

#if RENEGADE_WITH_BGFX_RENDERER
void RenderVertexBufferClass::Init_Bgfx_Layout() const
{
	if (BgfxLayoutInitialized) return;

	const unsigned fvf = Vertex_Format_Info().Get_Vertex_Format();
	const bool has_normal = (fvf & VERTEX_FORMAT_FLAG_NORMAL) != 0u;
	const bool has_diffuse = (fvf & VERTEX_FORMAT_FLAG_DIFFUSE) != 0u;
	const bool has_specular = (fvf & VERTEX_FORMAT_FLAG_SPECULAR) != 0u;
	const unsigned texcoord_count = VERTEX_FORMAT_Get_Texcoord_Count(fvf);

	BgfxLayout.begin();
	BgfxLayout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	if (has_normal) {
		BgfxLayout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	}
	if (has_diffuse) {
		BgfxLayout.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
	}
	if (has_specular) {
		BgfxLayout.add(bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true);
	}
	for (unsigned t = 0; t < texcoord_count; ++t) {
		const bgfx::Attrib::Enum attrib = static_cast<bgfx::Attrib::Enum>(
			static_cast<int>(bgfx::Attrib::TexCoord0) + t);
		BgfxLayout.add(attrib, static_cast<uint8_t>(VERTEX_FORMAT_Get_Texcoord_Size(fvf, t)), bgfx::AttribType::Float);
	}
	BgfxLayout.end();

	BgfxLayoutInitialized = true;
}
#endif

void RenderVertexBufferClass::Create_Vertex_Buffer(UsageType)
{
#if RENEGADE_WITH_BGFX_RENDERER
	Mark_Bgfx_Buffer_Dirty();
#endif
}

void RenderVertexBufferClass::Copy(const Vector3 *loc, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_Source_Vertex_Data() + first_vertex * Vertex_Format_Info().Get_Vertex_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Location_Offset()) = loc[i];
		vertices += Vertex_Format_Info().Get_Vertex_Size();
	}
}

void RenderVertexBufferClass::Copy(const Vector3 *loc, const Vector2 *uv, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_Source_Vertex_Data() + first_vertex * Vertex_Format_Info().Get_Vertex_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector2 *>(vertices + Vertex_Format_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += Vertex_Format_Info().Get_Vertex_Size();
	}
}

void RenderVertexBufferClass::Copy(const Vector3 *loc, const Vector3 *norm, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_Source_Vertex_Data() + first_vertex * Vertex_Format_Info().Get_Vertex_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Normal_Offset()) = norm[i];
		vertices += Vertex_Format_Info().Get_Vertex_Size();
	}
}

void RenderVertexBufferClass::Copy(const Vector3 *loc, const Vector3 *norm, const Vector2 *uv, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_Source_Vertex_Data() + first_vertex * Vertex_Format_Info().Get_Vertex_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Normal_Offset()) = norm[i];
		*reinterpret_cast<Vector2 *>(vertices + Vertex_Format_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += Vertex_Format_Info().Get_Vertex_Size();
	}
}

void RenderVertexBufferClass::Copy(const Vector3 *loc, const Vector3 *norm, const Vector2 *uv, const Vector4 *diffuse, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_Source_Vertex_Data() + first_vertex * Vertex_Format_Info().Get_Vertex_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Normal_Offset()) = norm[i];
		*reinterpret_cast<unsigned *>(vertices + Vertex_Format_Info().Get_Diffuse_Offset()) = DX8Wrapper::Convert_Color(diffuse[i]);
		*reinterpret_cast<Vector2 *>(vertices + Vertex_Format_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += Vertex_Format_Info().Get_Vertex_Size();
	}
}

void RenderVertexBufferClass::Copy(const Vector3 *loc, const Vector2 *uv, const Vector4 *diffuse, unsigned first_vertex, unsigned count)
{
	unsigned char *vertices = Get_Source_Vertex_Data() + first_vertex * Vertex_Format_Info().Get_Vertex_Size();
	for (unsigned i = 0; i < count; ++i) {
		*reinterpret_cast<Vector3 *>(vertices + Vertex_Format_Info().Get_Location_Offset()) = loc[i];
		*reinterpret_cast<unsigned *>(vertices + Vertex_Format_Info().Get_Diffuse_Offset()) = DX8Wrapper::Convert_Color(diffuse[i]);
		*reinterpret_cast<Vector2 *>(vertices + Vertex_Format_Info().Get_Tex_Offset(0)) = uv[i];
		vertices += Vertex_Format_Info().Get_Vertex_Size();
	}
}

#if RENEGADE_WITH_BGFX_RENDERER
unsigned char *RenderVertexBufferClass::Get_Source_Vertex_Data()
{
	Mark_Bgfx_Buffer_Dirty();
	return VertexData.data();
}

const unsigned char *RenderVertexBufferClass::Get_Source_Vertex_Data() const
{
	return VertexData.data();
}

bool RenderVertexBufferClass::Ensure_Bgfx_Buffer() const
{
	return Sync_Bgfx_Buffer();
}

bgfx::VertexBufferHandle RenderVertexBufferClass::Get_Bgfx_Vertex_Buffer() const
{
	return BgfxVertexBuffer;
}

bgfx::DynamicVertexBufferHandle RenderVertexBufferClass::Get_Bgfx_Dynamic_Vertex_Buffer() const
{
	return BgfxDynamicVertexBuffer;
}

const bgfx::VertexLayout &RenderVertexBufferClass::Get_Bgfx_Vertex_Layout() const
{
	Init_Bgfx_Layout();
	return BgfxLayout;
}

bool RenderVertexBufferClass::Uses_Dynamic_Bgfx_Buffer() const
{
	return BgfxUsesDynamicBuffer;
}

bool RenderVertexBufferClass::Update_Bgfx_Dynamic_Buffer(unsigned start_vertex, const bgfx::Memory *memory) const
{
	if (!BgfxUsesDynamicBuffer || !BgfxRenderer::Is_Initted()) {
		return false;
	}

	Init_Bgfx_Layout();
	if (!bgfx::isValid(BgfxDynamicVertexBuffer)) {
		BgfxDynamicVertexBuffer = bgfx::createDynamicVertexBuffer(VertexCount, BgfxLayout);
		if (!bgfx::isValid(BgfxDynamicVertexBuffer)) {
			return false;
		}
	}

	bgfx::update(BgfxDynamicVertexBuffer, static_cast<uint32_t>(start_vertex), memory);
	BgfxVertexBufferDirty = false;
	return true;
}

void RenderVertexBufferClass::Mark_Bgfx_Buffer_Dirty()
{
	if (!BgfxUsesDynamicBuffer) {
		// Destroy the old immutable buffer so the next Sync recreates it
		if (bgfx::isValid(BgfxVertexBuffer)) {
			if (BgfxRenderer::Is_Initted()) {
				bgfx::destroy(BgfxVertexBuffer);
			}
			BgfxVertexBuffer = BGFX_INVALID_HANDLE;
		}
	}
	BgfxVertexBufferDirty = true;
}

bool RenderVertexBufferClass::Sync_Bgfx_Buffer() const
{
	if (!BgfxRenderer::Is_Initted()) {
		return false;
	}

	Init_Bgfx_Layout();

	if (BgfxUsesDynamicBuffer) {
		if (!bgfx::isValid(BgfxDynamicVertexBuffer)) {
			BgfxDynamicVertexBuffer = bgfx::createDynamicVertexBuffer(VertexCount, BgfxLayout);
			if (!bgfx::isValid(BgfxDynamicVertexBuffer)) {
				return false;
			}
		}

		if (!BgfxVertexBufferDirty) {
			return true;
		}

		const unsigned total_bytes = Vertex_Format_Info().Get_Vertex_Size() * VertexCount;
		const bgfx::Memory *mem = bgfx::copy(VertexData.data(), static_cast<uint32_t>(total_bytes));
		Swizzle_Vertex_Colors_In_Place(mem->data, Vertex_Format_Info(), VertexCount);
		bgfx::update(BgfxDynamicVertexBuffer, 0, mem);
		BgfxVertexBufferDirty = false;
		return true;
	}

	if (bgfx::isValid(BgfxVertexBuffer) && !BgfxVertexBufferDirty) {
		return true;
	}

	// Destroy any existing buffer (immutable buffers can't be updated)
	if (bgfx::isValid(BgfxVertexBuffer)) {
		bgfx::destroy(BgfxVertexBuffer);
		BgfxVertexBuffer = BGFX_INVALID_HANDLE;
	}

	const unsigned vertex_size = Vertex_Format_Info().Get_Vertex_Size();
	const uint32_t total_bytes = static_cast<uint32_t>(vertex_size) * VertexCount;

	const bgfx::Memory *mem = bgfx::alloc(total_bytes);
	std::memcpy(mem->data, VertexData.data(), total_bytes);
	Swizzle_Vertex_Colors_In_Place(mem->data, Vertex_Format_Info(), VertexCount);

	BgfxVertexBuffer = bgfx::createVertexBuffer(mem, BgfxLayout);
	BgfxVertexBufferDirty = false;
	return bgfx::isValid(BgfxVertexBuffer);
}
#endif

DynamicVBAccessClass::DynamicVBAccessClass(unsigned type, unsigned fvf, unsigned short vertex_count)
	: FVFInfo(kDynamicFVFInfo), Type(type), VertexCount(vertex_count), VertexBufferOffset(0), VertexBuffer(nullptr)
{
	WWASSERT(fvf == dynamic_vertex_format);
	WWASSERT(Type == BUFFER_TYPE_DYNAMIC_RENDER || Type == BUFFER_TYPE_DYNAMIC_SORTING);
	if (Type == BUFFER_TYPE_DYNAMIC_RENDER) {
		Allocate_Render_Dynamic_Buffer();
	} else {
		Allocate_Sorting_Dynamic_Buffer();
	}
}

DynamicVBAccessClass::~DynamicVBAccessClass()
{
	if (Type == BUFFER_TYPE_DYNAMIC_RENDER) {
		g_dynamic_render_vertex_buffer_in_use = false;
		g_dynamic_render_vertex_buffer_offset += VertexCount;
	} else {
		g_dynamic_sorting_vertex_buffer_in_use = false;
		g_dynamic_sorting_vertex_buffer_offset += VertexCount;
	}

	REF_PTR_RELEASE(VertexBuffer);
}

void DynamicVBAccessClass::_Deinit()
{
	REF_PTR_RELEASE(g_dynamic_render_vertex_buffer);
	g_dynamic_render_vertex_buffer_in_use = false;
	g_dynamic_render_vertex_buffer_size = kDefaultDynamicVertexCount;
	g_dynamic_render_vertex_buffer_offset = 0;

	REF_PTR_RELEASE(g_dynamic_sorting_vertex_buffer);
	for (SortingVertexBufferClass *buffer : g_stale_dynamic_sorting_vertex_buffers) {
		if (buffer != nullptr) {
			buffer->Release_Ref();
		}
	}
	g_stale_dynamic_sorting_vertex_buffers.clear();
	g_dynamic_sorting_vertex_buffer_in_use = false;
	g_dynamic_sorting_vertex_buffer_size = 0;
	g_dynamic_sorting_vertex_buffer_offset = 0;
}

void DynamicVBAccessClass::_Reset(bool frame_changed)
{
	Release_Stale_Dynamic_Sorting_Vertex_Buffers();
	g_dynamic_sorting_vertex_buffer_offset = 0;
	if (frame_changed) {
		g_dynamic_render_vertex_buffer_offset = 0;
	}
}

DynamicVBAccessClass::WriteLockClass::WriteLockClass(DynamicVBAccessClass *vb_access)
	: DynamicVBAccess(vb_access), Vertices(nullptr)
#if RENEGADE_WITH_BGFX_RENDERER
	, BgfxMemory(nullptr)
#endif
{
	WWASSERT(vb_access != nullptr);
	DynamicVBAccess->VertexBuffer->Add_Ref();
	switch (DynamicVBAccess->Get_Type()) {
	case BUFFER_TYPE_DYNAMIC_RENDER:
	{
		const unsigned vertex_bytes =
			static_cast<unsigned>(DynamicVBAccess->Get_Vertex_Count()) *
			DynamicVBAccess->VertexBuffer->Vertex_Format_Info().Get_Vertex_Size();
		BgfxMemory = bgfx::alloc(static_cast<uint32_t>(vertex_bytes));
		Vertices = reinterpret_cast<VertexFormatXYZNDUV2 *>(BgfxMemory->data);
		break;
	}
	case BUFFER_TYPE_DYNAMIC_SORTING:
		Vertices = static_cast<SortingVertexBufferClass *>(DynamicVBAccess->VertexBuffer)->VertexBuffer + DynamicVBAccess->VertexBufferOffset;
		break;
	default:
		WWASSERT(0);
		break;
	}
}

DynamicVBAccessClass::WriteLockClass::~WriteLockClass()
{
	switch (DynamicVBAccess->Get_Type()) {
	case BUFFER_TYPE_DYNAMIC_RENDER:
		WWASSERT(BgfxMemory != nullptr);
		Swizzle_Vertex_Colors_In_Place(BgfxMemory->data, DynamicVBAccess->Vertex_Format_Info(), DynamicVBAccess->Get_Vertex_Count());
		WWASSERT(static_cast<RenderVertexBufferClass *>(DynamicVBAccess->VertexBuffer)->Update_Bgfx_Dynamic_Buffer(
			DynamicVBAccess->VertexBufferOffset,
			BgfxMemory));
		break;
	case BUFFER_TYPE_DYNAMIC_SORTING:
		break;
	default:
		WWASSERT(0);
		break;
	}
	DynamicVBAccess->VertexBuffer->Release_Ref();
}

void DynamicVBAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	WWASSERT(!g_dynamic_sorting_vertex_buffer_in_use);
	g_dynamic_sorting_vertex_buffer_in_use = true;

	const unsigned required_vertex_count = g_dynamic_sorting_vertex_buffer_offset + VertexCount;
	WWASSERT(required_vertex_count < 65536);
	if (required_vertex_count > g_dynamic_sorting_vertex_buffer_size) {
		if (g_dynamic_sorting_vertex_buffer != nullptr) {
			if (g_dynamic_sorting_vertex_buffer->Engine_Refs() > 0) {
				g_stale_dynamic_sorting_vertex_buffers.push_back(g_dynamic_sorting_vertex_buffer);
			} else {
				REF_PTR_RELEASE(g_dynamic_sorting_vertex_buffer);
			}
			g_dynamic_sorting_vertex_buffer = nullptr;
		}
		g_dynamic_sorting_vertex_buffer_size = std::max<unsigned short>(static_cast<unsigned short>(required_vertex_count), static_cast<unsigned short>(kDefaultDynamicVertexCount));
	}

	if (g_dynamic_sorting_vertex_buffer == nullptr) {
		g_dynamic_sorting_vertex_buffer = NEW_REF(SortingVertexBufferClass, (g_dynamic_sorting_vertex_buffer_size));
		g_dynamic_sorting_vertex_buffer_offset = 0;
	}

	REF_PTR_SET(VertexBuffer, g_dynamic_sorting_vertex_buffer);
	VertexBufferOffset = g_dynamic_sorting_vertex_buffer_offset;
}

void DynamicVBAccessClass::Allocate_Render_Dynamic_Buffer()
{
	WWASSERT(!g_dynamic_render_vertex_buffer_in_use);
	g_dynamic_render_vertex_buffer_in_use = true;

	if (VertexCount > g_dynamic_render_vertex_buffer_size) {
		REF_PTR_RELEASE(g_dynamic_render_vertex_buffer);
		g_dynamic_render_vertex_buffer_size = std::max<unsigned short>(
			static_cast<unsigned short>(VertexCount),
			static_cast<unsigned short>(kDefaultDynamicVertexCount));
	}

	if (g_dynamic_render_vertex_buffer == nullptr) {
		g_dynamic_render_vertex_buffer = NEW_REF(RenderVertexBufferClass, (
			dynamic_vertex_format,
			g_dynamic_render_vertex_buffer_size,
			RenderVertexBufferClass::USAGE_DYNAMIC,
			BUFFER_TYPE_DYNAMIC_RENDER));
		g_dynamic_render_vertex_buffer_offset = 0;
	}

	if (static_cast<unsigned>(VertexCount) + g_dynamic_render_vertex_buffer_offset > g_dynamic_render_vertex_buffer_size) {
		g_dynamic_render_vertex_buffer_offset = 0;
	}

	REF_PTR_SET(VertexBuffer, g_dynamic_render_vertex_buffer);
	VertexBufferOffset = g_dynamic_render_vertex_buffer_offset;
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

	Get_Device_Resolution(ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed);
	Do_Onetime_Device_Dependent_Inits();
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
	Set_DX8_Render_State(D3DRS_FILLMODE, D3DFILL_SOLID);
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
	Set_DX8_Render_State(D3DRS_LOCALVIEWER, TRUE);
	Set_DX8_Render_State(D3DRS_NORMALIZENORMALS, FALSE);
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
	Set_DX8_Render_State(D3DRS_ZBIAS, static_cast<unsigned>(zbias));
}

void DX8Wrapper::Set_Pseudo_ZBias(int zbias)
{
	Set_DX8_Render_State(D3DRS_ZBIAS, static_cast<unsigned>(zbias));
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
		ZNear = 0.0f;
		ZFar = 0.0f;
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
		if (ZNear != ZFar) {
			const int zbias = static_cast<int>(Get_DX8_Render_State(D3DRS_ZBIAS));
			if (zbias != 0) {
				float projection_bias = static_cast<float>(zbias);
				projection_bias *= (1.0f / 16.0f);
				projection_bias *= 1.0f / (ZFar - ZNear);
				m[2][2] -= projection_bias * m[3][2];
			}
		}
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
	if (state == D3DRS_ZBIAS) {
		ZBias = static_cast<int>(value);
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
	if (mat != nullptr) {
		render_state.material_state = *mat;
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

void DX8Wrapper::Set_Light(unsigned index, const LightClass &light)
{
	if (index >= 4) {
		return;
	}

	D3DLIGHT8 dx_light = {};
	Vector3 color;
	std::memset(&dx_light, 0, sizeof(dx_light));

	switch (light.Get_Type()) {
	case LightClass::POINT:
		dx_light.Type = D3DLIGHT_POINT;
		break;
	case LightClass::DIRECTIONAL:
		dx_light.Type = D3DLIGHT_DIRECTIONAL;
		break;
	case LightClass::SPOT:
		dx_light.Type = D3DLIGHT_SPOT;
		break;
	}

	light.Get_Diffuse(&color);
	color *= light.Get_Intensity();
	dx_light.Diffuse = {color.X, color.Y, color.Z, 1.0f};

	light.Get_Specular(&color);
	color *= light.Get_Intensity();
	dx_light.Specular = {color.X, color.Y, color.Z, 1.0f};

	light.Get_Ambient(&color);
	color *= light.Get_Intensity();
	dx_light.Ambient = {color.X, color.Y, color.Z, 1.0f};

	const Vector3 position = light.Get_Position();
	dx_light.Position = {position.X, position.Y, position.Z};

	Vector3 direction;
	light.Get_Spot_Direction(direction);
	dx_light.Direction = {direction.X, direction.Y, direction.Z};

	dx_light.Range = light.Get_Attenuation_Range();
	dx_light.Falloff = light.Get_Spot_Exponent();
	dx_light.Theta = light.Get_Spot_Angle();
	dx_light.Phi = light.Get_Spot_Angle();

	double atten_start = 0.0;
	double atten_end = 0.0;
	light.Get_Far_Attenuation_Range(atten_start, atten_end);
	dx_light.Attenuation0 = 1.0f;
	if (std::abs(atten_start - atten_end) < 1.0e-5) {
		dx_light.Attenuation1 = 0.0f;
	} else {
		dx_light.Attenuation1 = static_cast<float>(1.0 / atten_start);
	}
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

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed, bool resize_window)
{
    CurRenderDevice = (dev < 0) ? 0 : dev;
    int requested_width = ResolutionWidth;
    int requested_height = ResolutionHeight;
    int requested_bits = BitDepth;
    bool requested_windowed = IsWindowed;

    if (width > 0) {
        requested_width = width;
    }
    if (height > 0) {
        requested_height = height;
    }
    if (bits > 0) {
        requested_bits = bits;
    }
    if (windowed >= 0) {
        requested_windowed = (windowed != 0);
    }

    ResolutionWidth = requested_width;
    ResolutionHeight = requested_height;
    BitDepth = requested_bits;
    IsWindowed = requested_windowed;

    if (BgfxRenderer::Is_Initted()) {
        if (!BgfxRenderer::Configure_Window(Hwnd, requested_width, requested_height, requested_bits, requested_windowed, resize_window)) {
            return false;
        }

        BgfxRenderer::Get_Device_Resolution(ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed);
    }

    Render2DClass::Set_Screen_Resolution(RectClass(0, 0, ResolutionWidth, ResolutionHeight));
    CurrentCaps = Ensure_Caps();
    if (BgfxRenderer::Is_Initted()) {
        BgfxRenderer::Reset();
        Invalidate_Cached_Render_States();
        render_state.view = BgfxRenderer::Get_Current_View_Matrix();
        ProjectionMatrix = BgfxRenderer::Get_Current_Projection_Matrix();
        Set_Default_Global_Render_States();
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

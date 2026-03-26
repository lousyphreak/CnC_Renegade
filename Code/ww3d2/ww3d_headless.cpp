#include "ww3d.h"

#include "assetmgr.h"
#include "dx8wrapper.h"
#include "rddesc.h"
#include "shader.h"
#include "vertmaterial.h"

#include <algorithm>
#include <cstring>

namespace {

constexpr int kFallbackRenderWidth = 640;
constexpr int kFallbackRenderHeight = 480;

void *g_window = NULL;
int g_collision_box_mask = 0;
int g_texture_reduction = 0;
int g_texture_bitdepth = 32;
long g_swap_interval = 0;
bool g_using_runtime_renderer = false;
RenderDeviceDescClass g_headless_render_device_desc;

bool Has_Runtime_Renderer()
{
	return g_using_runtime_renderer;
}

} // namespace

unsigned int WW3D::SyncTime = 0;
unsigned int WW3D::PreviousSyncTime = 0;
bool WW3D::IsSortingEnabled = true;
float WW3D::PixelCenterX = 0.0f;
float WW3D::PixelCenterY = 0.0f;
bool WW3D::IsInitted = false;
bool WW3D::IsRendering = false;
bool WW3D::IsCapturing = false;
bool WW3D::IsScreenUVBiased = false;
bool WW3D::IsBackfaceDebugEnabled = false;
bool WW3D::AreDecalsEnabled = true;
float WW3D::DecalRejectionDistance = 1000000.0f;
bool WW3D::AreStaticSortListsEnabled = false;
bool WW3D::MungeSortOnLoad = false;
FrameGrabClass * WW3D::Movie = NULL;
bool WW3D::PauseRecord = false;
bool WW3D::RecordNextFrame = false;
int WW3D::FrameCount = 0;
VertexMaterialClass * WW3D::DefaultDebugMaterial = NULL;
VertexMaterialClass * WW3D::BackfaceDebugMaterial = NULL;
ShaderClass WW3D::DefaultDebugShader;
ShaderClass WW3D::LightmapDebugShader;
WW3D::PrelitModeEnum WW3D::PrelitMode = WW3D::PRELIT_MODE_LIGHTMAP_MULTI_PASS;
bool WW3D::ExposePrelit = false;
int WW3D::TextureFilter = 0;
bool WW3D::SnapshotActivated = false;
bool WW3D::ThumbnailEnabled = true;
WW3D::MeshDrawModeEnum WW3D::MeshDrawMode = WW3D::MESH_DRAW_MODE_NONE;
WW3D::NPatchesGapFillingModeEnum WW3D::NPatchesGapFillingMode = WW3D::NPATCHES_GAP_FILLING_DISABLED;
unsigned WW3D::NPatchesLevel = 1;
bool WW3D::IsTexturingEnabled = false;
bool WW3D::Lite = true;
float WW3D::DefaultNativeScreenSize = 1.0f;
RefRenderObjListClass * WW3D::DefaultStaticSortLists = NULL;
RefRenderObjListClass * WW3D::CurrentStaticSortLists = NULL;
unsigned int WW3D::MinStaticSortLevel = 1;
unsigned int WW3D::MaxStaticSortLevel = MAX_SORT_LEVEL;
int WW3D::LastFrameMemoryAllocations = 0;
int WW3D::LastFrameMemoryFrees = 0;
long WW3D::UserStat0 = 0;
long WW3D::UserStat1 = 0;
long WW3D::UserStat2 = 0;

WW3DErrorType WW3D::Init(void * hwnd, char *, bool lite)
{
	g_window = hwnd;
	Lite = lite;
	IsInitted = true;
	IsRendering = false;
	SyncTime = 0;
	PreviousSyncTime = 0;
	FrameCount = 0;
	VertexMaterialClass::Init();
	DefaultDebugMaterial = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	DefaultDebugShader = ShaderClass::_PresetOpaqueShader;
	LightmapDebugShader = ShaderClass::_PresetAdditiveShader;
	if (WW3DAssetManager::Get_Instance() == NULL) {
		new WW3DAssetManager();
	}

	g_headless_render_device_desc = RenderDeviceDescClass();
	g_headless_render_device_desc.reset_resolution_list();
	g_headless_render_device_desc.set_device_name("Headless");
	g_headless_render_device_desc.set_driver_name("SDL3");
	g_headless_render_device_desc.set_driver_version(Lite ? "lite" : "stub");
	g_headless_render_device_desc.add_resolution(kFallbackRenderWidth, kFallbackRenderHeight, g_texture_bitdepth);
	g_using_runtime_renderer = false;

#if RENEGADE_WITH_BGFX_RENDERER
	if (!Lite) {
		g_using_runtime_renderer = true;
		DX8Wrapper::Set_Texture_Bitdepth(g_texture_bitdepth);
		DX8Wrapper::Set_Swap_Interval(static_cast<int>(g_swap_interval));
		if (!DX8Wrapper::Init(hwnd, false)) {
			g_using_runtime_renderer = false;
			IsInitted = false;
			VertexMaterialClass::Shutdown();
			WW3DAssetManager::Delete_This();
			return WW3D_ERROR_DIRECTX8_INITIALIZATION_FAILED;
		}
	}
#endif

	return WW3D_ERROR_OK;
}

WW3DErrorType WW3D::Shutdown(void)
{
	IsRendering = false;
	IsInitted = false;
	g_using_runtime_renderer = false;

#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Shutdown();
	}
#endif

	if (DefaultDebugMaterial != NULL) {
		DefaultDebugMaterial->Release_Ref();
		DefaultDebugMaterial = NULL;
	}
	if (BackfaceDebugMaterial != NULL) {
		BackfaceDebugMaterial->Release_Ref();
		BackfaceDebugMaterial = NULL;
	}
	VertexMaterialClass::Shutdown();
	WW3DAssetManager::Delete_This();
	return WW3D_ERROR_OK;
}

const int WW3D::Get_Render_Device_Count(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Get_Render_Device_Count();
	}
#endif
	return 1;
}

const char * WW3D::Get_Render_Device_Name(int device_index)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Get_Render_Device_Name(device_index);
	}
#else
	(void)device_index;
#endif
	return g_headless_render_device_desc.Get_Device_Name();
}

const RenderDeviceDescClass & WW3D::Get_Render_Device_Desc(int device_index)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Get_Render_Device_Desc(device_index);
	}
#else
	(void)device_index;
#endif
	return g_headless_render_device_desc;
}

int WW3D::Get_Render_Device(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Get_Render_Device();
	}
#endif
	return 0;
}

WW3DErrorType WW3D::Set_Render_Device(int device, int width, int height, int bits, int windowed, bool resize_window)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Set_Render_Device(device, width, height, bits, windowed, resize_window) ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
	}
#else
	(void)device;
	(void)width;
	(void)height;
	(void)bits;
	(void)windowed;
	(void)resize_window;
#endif
	return Set_Device_Resolution(width, height, bits, windowed, resize_window);
}

WW3DErrorType WW3D::Set_Render_Device(const char * device_name, int width, int height, int bits, int windowed, bool resize_window)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Set_Render_Device(device_name, width, height, bits, windowed, resize_window) ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
	}
#else
	(void)device_name;
#endif
	return Set_Device_Resolution(width, height, bits, windowed, resize_window);
}

WW3DErrorType WW3D::Set_Next_Render_Device(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Set_Next_Render_Device() ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
	}
#endif
	return WW3D_ERROR_OK;
}

WW3DErrorType WW3D::Set_Any_Render_Device(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Set_Any_Render_Device() ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
	}
#endif
	return WW3D_ERROR_OK;
}

void WW3D::Get_Pixel_Center(float & x, float & y) { x = PixelCenterX; y = PixelCenterY; }

void WW3D::Get_Render_Target_Resolution(int & w, int & h, int & bits, bool & windowed)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Get_Render_Target_Resolution(w, h, bits, windowed);
		return;
	}
#endif
	w = kFallbackRenderWidth;
	h = kFallbackRenderHeight;
	windowed = true;
	bits = g_texture_bitdepth;
}

void WW3D::Get_Device_Resolution(int & w, int & h, int & bits, bool & windowed)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Get_Device_Resolution(w, h, bits, windowed);
		return;
	}
#endif
	Get_Render_Target_Resolution(w, h, bits, windowed);
}

WW3DErrorType WW3D::Set_Device_Resolution(int width, int height, int bits, int windowed, bool resize_window)
{
	if (bits > 0) {
		g_texture_bitdepth = bits;
#if RENEGADE_WITH_BGFX_RENDERER
		if (Has_Runtime_Renderer()) {
			DX8Wrapper::Set_Texture_Bitdepth(bits);
		}
#endif
		g_headless_render_device_desc = RenderDeviceDescClass();
		g_headless_render_device_desc.reset_resolution_list();
		g_headless_render_device_desc.set_device_name("Headless");
		g_headless_render_device_desc.set_driver_name("SDL3");
		g_headless_render_device_desc.set_driver_version(Lite ? "lite" : "stub");
		g_headless_render_device_desc.add_resolution(kFallbackRenderWidth, kFallbackRenderHeight, g_texture_bitdepth);
	}

#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Set_Device_Resolution(width, height, bits, windowed, resize_window) ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
	}
#else
	(void)width;
	(void)height;
	(void)windowed;
	(void)resize_window;
#endif

	return WW3D_ERROR_OK;
}

bool WW3D::Is_Windowed(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Is_Windowed();
	}
#endif
	return true;
}

WW3DErrorType WW3D::Toggle_Windowed(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Toggle_Windowed() ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
	}
#endif
	return WW3D_ERROR_OK;
}

void WW3D::Set_Window(void * hwnd)
{
	g_window = hwnd;
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Update_Window(hwnd);
	}
#endif
}

void * WW3D::Get_Window(void) { return g_window; }
WW3DErrorType WW3D::On_Activate_App(void) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::On_Deactivate_App(void) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::Registry_Save_Render_Device(const char *) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::Registry_Save_Render_Device(const char *, int, int, int, int, bool, int) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::Registry_Load_Render_Device(const char *, bool) { return Set_Any_Render_Device(); }

bool WW3D::Registry_Load_Render_Device(const char *, char * device, int device_len, int & width, int & height, int & depth, int & windowed, int & texture_depth)
{
	if (device != NULL && device_len > 0) {
		std::strncpy(device, Get_Render_Device_Name(0), static_cast<size_t>(device_len) - 1);
		device[device_len - 1] = '\0';
	}

	bool is_windowed = true;
	Get_Device_Resolution(width, height, depth, is_windowed);
	windowed = is_windowed ? 1 : 0;
	texture_depth = g_texture_bitdepth;
	return true;
}

void WW3D::Set_Texture_Filter(int filter) { TextureFilter = filter; }

WW3DErrorType WW3D::Begin_Render(bool clear, bool clearz, const Vector3 & color, void(*)(void))
{
	IsRendering = true;
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		if (clear || clearz) {
			int width = 0;
			int height = 0;
			int bits = 0;
			bool windowed = true;
			Get_Render_Target_Resolution(width, height, bits, windowed);
			RenderViewportClass viewport(
				0,
				0,
				static_cast<unsigned>(std::max(width, 1)),
				static_cast<unsigned>(std::max(height, 1)),
				0.0f,
				1.0f);
			DX8Wrapper::Set_Viewport(viewport);
		}
		DX8Wrapper::Clear(clear, clearz, color);
		DX8Wrapper::Begin_Scene();
	}
#endif
	return WW3D_ERROR_OK;
}

WW3DErrorType WW3D::Render(const LayerListClass &) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::Render(const LayerClass &) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::Render(SceneClass *, CameraClass *, bool, bool, const Vector3 &) { return WW3D_ERROR_OK; }
WW3DErrorType WW3D::Render(RenderObjClass &, RenderInfoClass &) { return WW3D_ERROR_OK; }
void WW3D::Flush(RenderInfoClass &) {}

WW3DErrorType WW3D::End_Render(bool flip_frame)
{
	IsRendering = false;
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::End_Scene(flip_frame);
	}
#endif
	return WW3D_ERROR_OK;
}

void WW3D::Flip_To_Primary(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Flip_To_Primary();
	}
#endif
}

void WW3D::Sync(unsigned int sync_time) { PreviousSyncTime = SyncTime; SyncTime = sync_time; ++FrameCount; }
unsigned int WW3D::Get_Last_Frame_Poly_Count(void) { return 0; }
unsigned int WW3D::Get_Last_Frame_Vertex_Count(void) { return 0; }
void WW3D::Make_Screen_Shot(const char *) {}
void WW3D::Start_Movie_Capture(const char *, float) { IsCapturing = true; }
void WW3D::Stop_Movie_Capture(void) { IsCapturing = false; }
void WW3D::Toggle_Movie_Capture(const char * filename_base, float frame_rate) { if (IsCapturing) Stop_Movie_Capture(); else Start_Movie_Capture(filename_base, frame_rate); }
void WW3D::Start_Single_Frame_Movie_Capture(const char *) { IsCapturing = true; RecordNextFrame = true; }
void WW3D::Capture_Next_Movie_Frame() { RecordNextFrame = true; }
void WW3D::Update_Movie_Capture(void) { RecordNextFrame = false; }
float WW3D::Get_Movie_Capture_Frame_Rate(void) { return 0.0f; }
void WW3D::Pause_Movie(bool mode) { PauseRecord = mode; }
bool WW3D::Is_Movie_Paused() { return PauseRecord; }
bool WW3D::Is_Recording_Next_Frame() { return RecordNextFrame; }
bool WW3D::Is_Movie_Ready() { return false; }

void WW3D::Set_Ext_Swap_Interval(long swap)
{
	g_swap_interval = swap;
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Set_Swap_Interval(static_cast<int>(swap));
	}
#endif
}

long WW3D::Get_Ext_Swap_Interval(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Get_Swap_Interval();
	}
#endif
	return g_swap_interval;
}

void WW3D::Set_Texture_Reduction(int value) { g_texture_reduction = value; }
int WW3D::Get_Texture_Reduction(void) { return g_texture_reduction; }
void WW3D::_Invalidate_Mesh_Cache() {}
void WW3D::_Invalidate_Textures() {}
void WW3D::Enable_Sorting(bool onoff) { IsSortingEnabled = onoff; }
void WW3D::Set_Collision_Box_Display_Mask(int mask) { g_collision_box_mask = mask; }
int WW3D::Get_Collision_Box_Display_Mask(void) { return g_collision_box_mask; }

void WW3D::Normalize_Coordinates(int x, int y, float & fx, float & fy)
{
	int width = kFallbackRenderWidth;
	int height = kFallbackRenderHeight;
	int bits = g_texture_bitdepth;
	bool windowed = true;
	Get_Device_Resolution(width, height, bits, windowed);
	fx = static_cast<float>(x) / static_cast<float>(std::max(width, 1));
	fy = static_cast<float>(y) / static_cast<float>(std::max(height, 1));
}

VertexMaterialClass * WW3D::Peek_Default_Debug_Material(void) { if (DefaultDebugMaterial != NULL) DefaultDebugMaterial->Add_Ref(); return DefaultDebugMaterial; }
ShaderClass WW3D::Peek_Default_Debug_Shader(void) { return DefaultDebugShader; }
ShaderClass WW3D::Peek_Backface_Debug_Shader(void) { return DefaultDebugShader; }
ShaderClass WW3D::Peek_Lightmap_Debug_Shader(void) { return LightmapDebugShader; }
void WW3D::Set_Texture_Bitdepth(int bitdepth)
{
	g_texture_bitdepth = bitdepth;
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		DX8Wrapper::Set_Texture_Bitdepth(bitdepth);
	}
#endif
	g_headless_render_device_desc = RenderDeviceDescClass();
	g_headless_render_device_desc.reset_resolution_list();
	g_headless_render_device_desc.set_device_name("Headless");
	g_headless_render_device_desc.set_driver_name("SDL3");
	g_headless_render_device_desc.set_driver_version(Lite ? "lite" : "stub");
	g_headless_render_device_desc.add_resolution(kFallbackRenderWidth, kFallbackRenderHeight, g_texture_bitdepth);
}

int WW3D::Get_Texture_Bitdepth()
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (Has_Runtime_Renderer()) {
		return DX8Wrapper::Get_Texture_Bitdepth();
	}
#endif
	return g_texture_bitdepth;
}

void WW3D::Set_NPatches_Gap_Filling_Mode(NPatchesGapFillingModeEnum mode) { NPatchesGapFillingMode = mode; }
void WW3D::Set_NPatches_Level(unsigned level) { NPatchesLevel = level; }
void WW3D::Enable_Texturing(bool b) { IsTexturingEnabled = b; }
void WW3D::Add_To_Static_Sort_List(RenderObjClass *, unsigned int) {}
void WW3D::Render_And_Clear_Static_Sort_Lists(RenderInfoClass &) {}
void WW3D::Override_Current_Static_Sort_Lists(RefRenderObjListClass * sort_list, unsigned int min_sort, unsigned int max_sort) { CurrentStaticSortLists = sort_list; MinStaticSortLevel = min_sort; MaxStaticSortLevel = max_sort; }
void WW3D::Reset_Current_Static_Sort_Lists_To_Default(void) { CurrentStaticSortLists = DefaultStaticSortLists; MinStaticSortLevel = 1; MaxStaticSortLevel = MAX_SORT_LEVEL; }

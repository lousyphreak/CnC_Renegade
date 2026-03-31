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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/dx8wrapper.cpp                         $*
 *                                                                                             *
 *              Original Author:: Jani Penttinen                                               *
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 3/12/02 4:27p                                               $*
 *                                                                                             *
 *                    $Revision:: 170                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   DX8Wrapper::_Update_Texture -- Copies a texture from system memory to video memory        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "dx8wrapper.h"

#include "bgfx_compat_resources.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "rddesc.h"
#include "render2d.h"
#include "registry.h"
#include "shader.h"
#include "texture.h"
#include "vertmaterial.h"
#include "ww3d.h"
#include "wwdebug.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "glsl/fs_bootstrap.sc.bin.h"
#include "glsl/vs_bootstrap.sc.bin.h"
#include "spirv/fs_bootstrap.sc.bin.h"
#include "spirv/vs_bootstrap.sc.bin.h"
#if defined(_WIN32)
#include "dx11/fs_bootstrap.sc.bin.h"
#include "dx11/vs_bootstrap.sc.bin.h"
#endif

namespace {

constexpr int kFallbackRenderWidth = 640;
constexpr int kFallbackRenderHeight = 480;
constexpr bgfx::ViewId kBootstrapViewId = 0;

const float kIdentityMatrix[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

struct BgfxGuiVertex {
	float x;
	float y;
	float z;
	uint32_t abgr;
	float u;
	float v;
};

struct BgfxDx8WrapperState {
	SDL_Window *window = nullptr;
	SDL_GLContext gl_context = nullptr;
	RenderDeviceDescClass render_device_desc;
	RenderViewportClass viewport = RenderViewportClass(0u, 0u, static_cast<unsigned>(kFallbackRenderWidth), static_cast<unsigned>(kFallbackRenderHeight));
	uint32_t reset_flags = BGFX_RESET_NONE;
	uint32_t clear_color = 0x101820FFu;
	float clear_depth = 1.0f;
	uint8_t clear_stencil = 0;
	uint16_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
	int width = kFallbackRenderWidth;
	int height = kFallbackRenderHeight;
	int bit_depth = 32;
	int swap_interval = 0;
	bool initialized = false;
	bool windowed = true;
	bgfx::VertexLayout gui_layout;
	bgfx::ProgramHandle gui_program = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle texture_uniform = BGFX_INVALID_HANDLE;
	bool layout_ready = false;
	Matrix4 world;
	Matrix4 view;
	Matrix4 projection;
	const unsigned char *vertex_data = nullptr;
	const FVFInfoClass *vertex_fvf = nullptr;
	unsigned short vertex_count = 0;
	const unsigned short *index_data = nullptr;
	unsigned short index_count = 0;
	unsigned short index_base_offset = 0;
	TextureClass *textures[MAX_TEXTURE_STAGES] = { nullptr, nullptr };
	ShaderClass shader;
	const VertexMaterialClass *material = nullptr;
	unsigned render_states[256] = { 0 };
	unsigned texture_stage_states[MAX_TEXTURE_STAGES][32] = { { 0 } };
	unsigned draw_calls = 0;

	BgfxDx8WrapperState()
	{
		world.Make_Identity();
		view.Make_Identity();
		projection.Make_Identity();
	}
};

BgfxDx8WrapperState g_bgfx;

uint32_t Compose_Reset_Flags()
{
	return g_bgfx.swap_interval > 0 ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
}

uint32_t DX8_To_BGFX_Color(uint32_t color)
{
	return (color & 0xFF00FF00u) | ((color & 0x00FF0000u) >> 16) | ((color & 0x000000FFu) << 16);
}

void Reset_Draw_State()
{
	g_bgfx.vertex_data = nullptr;
	g_bgfx.vertex_fvf = nullptr;
	g_bgfx.vertex_count = 0;
	g_bgfx.index_data = nullptr;
	g_bgfx.index_count = 0;
	g_bgfx.index_base_offset = 0;
	g_bgfx.textures[0] = nullptr;
	g_bgfx.textures[1] = nullptr;
	g_bgfx.material = nullptr;
	g_bgfx.shader = ShaderClass();
	for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
		for (unsigned state = 0; state < 32; ++state) {
			g_bgfx.texture_stage_states[stage][state] = 0;
		}
	}
	for (unsigned state = 0; state < 256; ++state) {
		g_bgfx.render_states[state] = 0;
	}
}

bool Query_Window_Size(SDL_Window *window, int &width, int &height)
{
	if (window == nullptr) {
		width = kFallbackRenderWidth;
		height = kFallbackRenderHeight;
		return false;
	}

	if (!SDL_GetWindowSizeInPixels(window, &width, &height)) {
		if (!SDL_GetWindowSize(window, &width, &height)) {
			width = kFallbackRenderWidth;
			height = kFallbackRenderHeight;
			return false;
		}
	}

	width = std::max(width, 1);
	height = std::max(height, 1);
	return true;
}
void Ensure_Window_Ready_For_BGFX(SDL_Window *window)
{
	if (window == nullptr) {
		return;
	}

	SDL_ShowWindow(window);
	SDL_RaiseWindow(window);
	SDL_SyncWindow(window);
	SDL_PumpEvents();
}

bool Populate_Platform_Data(SDL_Window *window, bgfx::PlatformData &platform_data)
{
	if (window == nullptr) {
		return false;
	}

	const SDL_PropertiesID properties = SDL_GetWindowProperties(window);
	if (properties == 0) {
		return false;
	}

#if defined(_WIN32)
	platform_data.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	platform_data.type = bgfx::NativeWindowHandleType::Default;
	return platform_data.nwh != nullptr;
#elif defined(__APPLE__)
	platform_data.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
	platform_data.type = bgfx::NativeWindowHandleType::Default;
	return platform_data.nwh != nullptr;
#else
	const char *video_driver = SDL_GetCurrentVideoDriver();
	if (video_driver != nullptr && std::strcmp(video_driver, "wayland") == 0) {
		platform_data.ndt = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
		platform_data.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
		platform_data.type = bgfx::NativeWindowHandleType::Wayland;
		return platform_data.ndt != nullptr && platform_data.nwh != nullptr;
	}

	if (video_driver != nullptr && std::strcmp(video_driver, "x11") == 0) {
		platform_data.ndt = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
		const Sint64 x11_window = SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
		platform_data.nwh = reinterpret_cast<void *>(static_cast<uintptr_t>(x11_window));
		platform_data.type = bgfx::NativeWindowHandleType::Default;
		return platform_data.ndt != nullptr && x11_window != 0;
	}
	return false;
#endif
}

void Update_Windowed_State()
{
	if (g_bgfx.window == nullptr) {
		g_bgfx.windowed = true;
		return;
	}

	const SDL_WindowFlags flags = SDL_GetWindowFlags(g_bgfx.window);
	g_bgfx.windowed = (flags & SDL_WINDOW_FULLSCREEN) == 0;
}

void Apply_View_Rect()
{
	const uint16_t x = static_cast<uint16_t>(std::min(g_bgfx.viewport.X, 0xFFFFu));
	const uint16_t y = static_cast<uint16_t>(std::min(g_bgfx.viewport.Y, 0xFFFFu));
	const uint16_t width = static_cast<uint16_t>(std::min(std::max(g_bgfx.viewport.Width, 1u), 0xFFFFu));
	const uint16_t height = static_cast<uint16_t>(std::min(std::max(g_bgfx.viewport.Height, 1u), 0xFFFFu));
	bgfx::setViewRect(kBootstrapViewId, x, y, width, height);
}

bool Sync_Backbuffer(bool force_reset)
{
	if (!g_bgfx.initialized) {
		return false;
	}

	int width = g_bgfx.width;
	int height = g_bgfx.height;
	Query_Window_Size(g_bgfx.window, width, height);

	const uint32_t reset_flags = Compose_Reset_Flags();
	const bool changed = force_reset || width != g_bgfx.width || height != g_bgfx.height || reset_flags != g_bgfx.reset_flags;
	if (changed) {
		g_bgfx.width = width;
		g_bgfx.height = height;
		g_bgfx.reset_flags = reset_flags;
		bgfx::reset(static_cast<uint32_t>(g_bgfx.width), static_cast<uint32_t>(g_bgfx.height), g_bgfx.reset_flags);
		DX8Wrapper::Refresh_Render_Device_Desc();
		Render2DClass::Set_Screen_Resolution(RectClass(0, 0, g_bgfx.width, g_bgfx.height));
	}

	Update_Windowed_State();
	Apply_View_Rect();
	return true;
}

bool Ensure_Gui_Resources()
{
	if (!g_bgfx.initialized) {
		return false;
	}

	if (!g_bgfx.layout_ready) {
		g_bgfx.gui_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
		g_bgfx.layout_ready = true;
	}

	if (!bgfx::isValid(g_bgfx.texture_uniform)) {
		g_bgfx.texture_uniform = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	}

	if (!bgfx::isValid(g_bgfx.gui_program)) {
		const uint8_t *vs_data = nullptr;
		uint32_t vs_size = 0;
		const uint8_t *fs_data = nullptr;
		uint32_t fs_size = 0;
		switch (bgfx::getRendererType()) {
		case bgfx::RendererType::Vulkan:
			vs_data = vs_bootstrap_spv; vs_size = sizeof(vs_bootstrap_spv);
			fs_data = fs_bootstrap_spv; fs_size = sizeof(fs_bootstrap_spv);
			break;
#if defined(_WIN32)
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12:
			vs_data = vs_bootstrap_dx11; vs_size = sizeof(vs_bootstrap_dx11);
			fs_data = fs_bootstrap_dx11; fs_size = sizeof(fs_bootstrap_dx11);
			break;
#endif
		default:
			vs_data = vs_bootstrap_glsl; vs_size = sizeof(vs_bootstrap_glsl);
			fs_data = fs_bootstrap_glsl; fs_size = sizeof(fs_bootstrap_glsl);
			break;
		}
		const bgfx::ShaderHandle vertex_shader = bgfx::createShader(bgfx::copy(vs_data, vs_size));
		const bgfx::ShaderHandle fragment_shader = bgfx::createShader(bgfx::copy(fs_data, fs_size));
		g_bgfx.gui_program = bgfx::createProgram(vertex_shader, fragment_shader, true);
		WWRELEASE_SAY(("BGFX2D: GUI shader program initialized\n"));
	}

	return bgfx::isValid(g_bgfx.gui_program) && bgfx::isValid(g_bgfx.texture_uniform);
}

uint64_t Build_BGFX_State()
{
	uint64_t state = 0;
	if (g_bgfx.shader.Get_Color_Mask() != ShaderClass::COLOR_WRITE_DISABLE) {
		state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
	}
	if (g_bgfx.shader.Get_Depth_Mask() == ShaderClass::DEPTH_WRITE_ENABLE) {
		state |= BGFX_STATE_WRITE_Z;
	}

	switch (g_bgfx.shader.Get_Depth_Compare()) {
		case ShaderClass::PASS_LEQUAL: state |= BGFX_STATE_DEPTH_TEST_LEQUAL; break;
		case ShaderClass::PASS_LESS: state |= BGFX_STATE_DEPTH_TEST_LESS; break;
		case ShaderClass::PASS_EQUAL: state |= BGFX_STATE_DEPTH_TEST_EQUAL; break;
		case ShaderClass::PASS_GEQUAL: state |= BGFX_STATE_DEPTH_TEST_GEQUAL; break;
		case ShaderClass::PASS_GREATER: state |= BGFX_STATE_DEPTH_TEST_GREATER; break;
		case ShaderClass::PASS_NOTEQUAL: state |= BGFX_STATE_DEPTH_TEST_NOTEQUAL; break;
		case ShaderClass::PASS_NEVER: state |= BGFX_STATE_DEPTH_TEST_NEVER; break;
		case ShaderClass::PASS_ALWAYS:
		default: state |= BGFX_STATE_DEPTH_TEST_ALWAYS; break;
	}

	const ShaderClass::SrcBlendFuncType src = g_bgfx.shader.Get_Src_Blend_Func();
	const ShaderClass::DstBlendFuncType dst = g_bgfx.shader.Get_Dst_Blend_Func();
	if (!(src == ShaderClass::SRCBLEND_ONE && dst == ShaderClass::DSTBLEND_ZERO)) {
		uint64_t src_factor = BGFX_STATE_BLEND_ONE;
		uint64_t dst_factor = BGFX_STATE_BLEND_ZERO;
		switch (src) {
			case ShaderClass::SRCBLEND_ZERO: src_factor = BGFX_STATE_BLEND_ZERO; break;
			case ShaderClass::SRCBLEND_SRC_ALPHA: src_factor = BGFX_STATE_BLEND_SRC_ALPHA; break;
			case ShaderClass::SRCBLEND_ONE_MINUS_SRC_ALPHA: src_factor = BGFX_STATE_BLEND_INV_SRC_ALPHA; break;
			case ShaderClass::SRCBLEND_ONE:
			default: src_factor = BGFX_STATE_BLEND_ONE; break;
		}
		switch (dst) {
			case ShaderClass::DSTBLEND_ONE: dst_factor = BGFX_STATE_BLEND_ONE; break;
			case ShaderClass::DSTBLEND_SRC_COLOR: dst_factor = BGFX_STATE_BLEND_SRC_COLOR; break;
			case ShaderClass::DSTBLEND_ONE_MINUS_SRC_COLOR: dst_factor = BGFX_STATE_BLEND_INV_SRC_COLOR; break;
			case ShaderClass::DSTBLEND_SRC_ALPHA: dst_factor = BGFX_STATE_BLEND_SRC_ALPHA; break;
			case ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA: dst_factor = BGFX_STATE_BLEND_INV_SRC_ALPHA; break;
			case ShaderClass::DSTBLEND_ZERO:
			default: dst_factor = BGFX_STATE_BLEND_ZERO; break;
		}
		state |= BGFX_STATE_BLEND_FUNC(src_factor, dst_factor);
	}

	state |= BGFX_STATE_MSAA;
	return state;
}

bool Submit_Triangles(unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	if (!DX8Wrapper::_Is_Triangle_Draw_Enabled() || !g_bgfx.initialized || polygon_count == 0 || !Ensure_Gui_Resources()) {
		return false;
	}
	if (g_bgfx.vertex_data == nullptr || g_bgfx.index_data == nullptr || g_bgfx.vertex_fvf == nullptr) {
		return false;
	}

	const uint32_t index_total = static_cast<uint32_t>(polygon_count) * 3U;
	if (vertex_count == 0 || index_total == 0) {
		return false;
	}

	if (bgfx::getAvailTransientVertexBuffer(vertex_count, g_bgfx.gui_layout) < vertex_count || bgfx::getAvailTransientIndexBuffer(index_total) < index_total) {
		WWRELEASE_SAY(("BGFX2D: transient buffer allocation failed (%u vertices, %u indices)\n", vertex_count, index_total));
		return false;
	}

	bgfx::TransientVertexBuffer tvb;
	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientVertexBuffer(&tvb, vertex_count, g_bgfx.gui_layout);
	bgfx::allocTransientIndexBuffer(&tib, index_total);

	const unsigned vertex_stride = g_bgfx.vertex_fvf->Get_FVF_Size();
	const unsigned location_offset = g_bgfx.vertex_fvf->Get_Location_Offset();
	const unsigned diffuse_offset = g_bgfx.vertex_fvf->Get_Diffuse_Offset();
	const unsigned tex_offset = g_bgfx.vertex_fvf->Get_Tex_Offset(0);
	BgfxGuiVertex *dst_vertices = reinterpret_cast<BgfxGuiVertex *>(tvb.data);
	for (unsigned short i = 0; i < vertex_count; ++i) {
		const unsigned char *src = g_bgfx.vertex_data + static_cast<size_t>(min_vertex_index + i) * vertex_stride;
		const float *position = reinterpret_cast<const float *>(src + location_offset);
		const uint32_t diffuse = *reinterpret_cast<const uint32_t *>(src + diffuse_offset);
		const float *uv = reinterpret_cast<const float *>(src + tex_offset);
		dst_vertices[i].x = position[0];
		dst_vertices[i].y = position[1];
		dst_vertices[i].z = position[2];
		dst_vertices[i].abgr = DX8_To_BGFX_Color(diffuse);
		dst_vertices[i].u = uv[0];
		dst_vertices[i].v = uv[1];
	}

	uint16_t *dst_indices = reinterpret_cast<uint16_t *>(tib.data);
	for (uint32_t i = 0; i < index_total; ++i) {
		const uint16_t source_index = g_bgfx.index_data[start_index + i] + g_bgfx.index_base_offset;
		dst_indices[i] = static_cast<uint16_t>(source_index - min_vertex_index);
	}

	const bgfx::TextureHandle texture_handle = BgfxCompat_Get_Texture_Handle(g_bgfx.textures[0]);
	const uint64_t sampler_flags = BgfxCompat_Get_Sampler_Flags(g_bgfx.textures[0]);
	bgfx::setViewTransform(kBootstrapViewId, kIdentityMatrix, kIdentityMatrix);
	bgfx::setTransform(kIdentityMatrix);
	bgfx::setTexture(0, g_bgfx.texture_uniform, bgfx::isValid(texture_handle) ? texture_handle : BgfxCompat_Get_White_Texture(), sampler_flags);
	bgfx::setVertexBuffer(0, &tvb);
	bgfx::setIndexBuffer(&tib);
	bgfx::setState(Build_BGFX_State());
	bgfx::submit(kBootstrapViewId, g_bgfx.gui_program);

	++g_bgfx.draw_calls;
	return true;
}

void Copy_Surface_Rectangles(
	IDirect3DSurface8 *source_surface,
	const RECT *source_rects,
	UINT rect_count,
	IDirect3DSurface8 *destination_surface,
	const POINT *dest_points)
{
	BgfxCompatSurface *dst = BgfxCompat_To_Surface(destination_surface);
	const BgfxCompatSurface *src = BgfxCompat_To_Surface(source_surface);
	if (dst == nullptr || src == nullptr || source_rects == nullptr || dest_points == nullptr || rect_count == 0 || dst->format != src->format) {
		return;
	}

	const unsigned pixel_size = BgfxCompat_Get_Pixel_Size(dst->format);
	for (UINT rect_index = 0; rect_index < rect_count; ++rect_index) {
		const RECT &src_rect = source_rects[rect_index];
		const POINT &dst_point = dest_points[rect_index];
		const unsigned width = static_cast<unsigned>(std::max(src_rect.right - src_rect.left, 0L));
		const unsigned height = static_cast<unsigned>(std::max(src_rect.bottom - src_rect.top, 0L));
		for (unsigned row = 0; row < height; ++row) {
			const size_t src_offset = (static_cast<size_t>(src_rect.top + static_cast<LONG>(row)) * static_cast<size_t>(src->width) + static_cast<size_t>(src_rect.left)) * pixel_size;
			const size_t dst_offset = (static_cast<size_t>(dst_point.y + static_cast<LONG>(row)) * static_cast<size_t>(dst->width) + static_cast<size_t>(dst_point.x)) * pixel_size;
			std::memcpy(dst->bytes.data() + dst_offset, src->bytes.data() + src_offset, static_cast<size_t>(width) * pixel_size);
		}
	}
}

bool Initialize_Bgfx(SDL_Window *window)
{
	int width = kFallbackRenderWidth;
	int height = kFallbackRenderHeight;
	Query_Window_Size(window, width, height);

	if (window != nullptr) {
		Ensure_Window_Ready_For_BGFX(window);
		SDL_PumpEvents();
		SDL_SyncWindow(window);
	}

	bgfx::renderFrame();

	bgfx::Init init;
	init.type = bgfx::RendererType::Count;
	init.vendorId = BGFX_PCI_ID_NONE;
	init.resolution.width = static_cast<uint32_t>(width);
	init.resolution.height = static_cast<uint32_t>(height);
	init.resolution.reset = Compose_Reset_Flags();

	if (!Populate_Platform_Data(window, init.platformData)) {
		SDL_SetError("Unable to extract native window/display handles for bgfx initialization.");
		return false;
	}

	if (!bgfx::init(init)) {
		SDL_SetError("bgfx::init failed.");
		return false;
	}

	g_bgfx.window = window;
	g_bgfx.width = width;
	g_bgfx.height = height;
	g_bgfx.reset_flags = init.resolution.reset;
	g_bgfx.initialized = true;
	Reset_Draw_State();
	g_bgfx.viewport = RenderViewportClass(0u, 0u, static_cast<unsigned>(width), static_cast<unsigned>(height));
	Update_Windowed_State();
	DX8Wrapper::Refresh_Render_Device_Desc();
	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, width, height));

	bgfx::setViewName(kBootstrapViewId, "Bootstrap");
	Apply_View_Rect();
	bgfx::setViewClear(kBootstrapViewId, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
	return true;
}

void Shutdown_Bgfx()
{
	if (!g_bgfx.initialized) {
		return;
	}

	if (bgfx::isValid(g_bgfx.gui_program)) {
		bgfx::destroy(g_bgfx.gui_program);
		g_bgfx.gui_program = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.texture_uniform)) {
		bgfx::destroy(g_bgfx.texture_uniform);
		g_bgfx.texture_uniform = BGFX_INVALID_HANDLE;
	}
	BgfxCompat_Shutdown_Texture_System();

	bgfx::frame();
	bgfx::shutdown();

#if defined(__linux__)
	if (g_bgfx.gl_context != nullptr) {
		SDL_GL_DestroyContext(g_bgfx.gl_context);
		g_bgfx.gl_context = nullptr;
	}
#endif

	g_bgfx = BgfxDx8WrapperState();
}

} // namespace

void DX8Wrapper::Refresh_Render_Device_Desc(void)
{
	g_bgfx.render_device_desc = RenderDeviceDescClass();
	g_bgfx.render_device_desc.reset_resolution_list();
	g_bgfx.render_device_desc.set_device_name("bgfx");
	const char *video_driver = SDL_GetCurrentVideoDriver();
	g_bgfx.render_device_desc.set_driver_name(video_driver != nullptr ? video_driver : "SDL3");
	g_bgfx.render_device_desc.set_driver_version("bootstrap");
	g_bgfx.render_device_desc.add_resolution(g_bgfx.width, g_bgfx.height, g_bgfx.bit_depth);
}

bool DX8Wrapper::Init(void *hwnd, bool lite)
{
	if (lite) {
		return true;
	}

	g_bgfx.window = reinterpret_cast<SDL_Window *>(hwnd);
	g_bgfx.reset_flags = Compose_Reset_Flags();
	return Initialize_Bgfx(g_bgfx.window);
}

void DX8Wrapper::Shutdown(void)
{
	Shutdown_Bgfx();
}

void DX8Wrapper::Begin_Scene(void)
{
	if (!g_bgfx.initialized) {
		return;
	}

	Sync_Backbuffer(false);
	bgfx::setViewClear(kBootstrapViewId, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
	bgfx::setViewTransform(kBootstrapViewId, kIdentityMatrix, kIdentityMatrix);
	bgfx::touch(kBootstrapViewId);
	g_bgfx.draw_calls = 0;
}

void DX8Wrapper::End_Scene(bool flip_frame)
{
	if (!g_bgfx.initialized) {
		return;
	}

	bgfx::frame(flip_frame ? BGFX_FRAME_NONE : BGFX_FRAME_FLUSH);
}

void DX8Wrapper::Flip_To_Primary(void)
{
}

void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float z, unsigned int stencil)
{
	g_bgfx.clear_flags = BGFX_CLEAR_NONE;
	if (clear_color) {
		g_bgfx.clear_flags |= BGFX_CLEAR_COLOR;
	}
	if (clear_z_stencil) {
		g_bgfx.clear_flags |= BGFX_CLEAR_DEPTH;
	}
	g_bgfx.clear_color = Convert_Color(color, 1.0f);
	g_bgfx.clear_depth = z;
	g_bgfx.clear_stencil = static_cast<uint8_t>(stencil & 0xFFu);

	if (g_bgfx.initialized) {
		bgfx::setViewClear(kBootstrapViewId, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
	}
}

void DX8Wrapper::Set_Viewport(const RenderViewportClass &viewport)
{
	g_bgfx.viewport = viewport;
	if (g_bgfx.initialized) {
		Apply_View_Rect();
	}
}

bool DX8Wrapper::Set_Any_Render_Device(void)
{
	return g_bgfx.initialized;
}

bool DX8Wrapper::Set_Render_Device(const char *dev_name, int width, int height, int bits, int windowed, bool resize_window)
{
	if (dev_name != nullptr && std::strcmp(dev_name, g_bgfx.render_device_desc.Get_Device_Name()) != 0) {
		return false;
	}

	return Set_Device_Resolution(width, height, bits, windowed, resize_window);
}

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed, bool resize_window)
{
	if (dev > 0) {
		return false;
	}

	return Set_Device_Resolution(width, height, bits, windowed, resize_window);
}

bool DX8Wrapper::Set_Next_Render_Device(void)
{
	return g_bgfx.initialized;
}

int DX8Wrapper::Get_Render_Device_Count(void)
{
	return 1;
}

int DX8Wrapper::Get_Render_Device(void)
{
	return 0;
}

const char *DX8Wrapper::Get_Render_Device_Name(int)
{
	return g_bgfx.render_device_desc.Get_Device_Name();
}

const RenderDeviceDescClass &DX8Wrapper::Get_Render_Device_Desc(int)
{
	return g_bgfx.render_device_desc;
}

bool DX8Wrapper::Set_Device_Resolution(int width, int height, int bits, int windowed, bool)
{
	if (bits > 0) {
		g_bgfx.bit_depth = bits;
		Refresh_Render_Device_Desc();
	}

	if (g_bgfx.window == nullptr) {
		return false;
	}

	if (windowed != -1 && !SDL_SetWindowFullscreen(g_bgfx.window, windowed == 0)) {
		return false;
	}

	if (width > 0 && height > 0 && !SDL_SetWindowSize(g_bgfx.window, width, height)) {
		return false;
	}

	if (g_bgfx.initialized) {
		return Sync_Backbuffer(true);
	}

	Query_Window_Size(g_bgfx.window, g_bgfx.width, g_bgfx.height);
	Update_Windowed_State();
	Refresh_Render_Device_Desc();
	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, g_bgfx.width, g_bgfx.height));
	return true;
}

void DX8Wrapper::Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed)
{
	width = g_bgfx.width;
	height = g_bgfx.height;
	bits = g_bgfx.bit_depth;
	windowed = g_bgfx.windowed;
}

void DX8Wrapper::Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed)
{
	Get_Device_Resolution(width, height, bits, windowed);
}

int DX8Wrapper::Get_Device_Resolution_Width(void)
{
	return g_bgfx.width;
}

int DX8Wrapper::Get_Device_Resolution_Height(void)
{
	return g_bgfx.height;
}

bool DX8Wrapper::Is_Windowed(void)
{
	return g_bgfx.windowed;
}

bool DX8Wrapper::Toggle_Windowed(void)
{
	if (g_bgfx.window == nullptr) {
		return false;
	}

	if (!SDL_SetWindowFullscreen(g_bgfx.window, g_bgfx.windowed)) {
		return false;
	}

	return !g_bgfx.initialized || Sync_Backbuffer(true);
}

void DX8Wrapper::Set_Swap_Interval(int swap)
{
	g_bgfx.swap_interval = std::max(swap, 0);
	if (g_bgfx.initialized) {
		Sync_Backbuffer(true);
	}
}

int DX8Wrapper::Get_Swap_Interval(void)
{
	return g_bgfx.swap_interval;
}

void DX8Wrapper::Set_Texture_Bitdepth(int depth)
{
	g_bgfx.bit_depth = depth;
	Refresh_Render_Device_Desc();
}

int DX8Wrapper::Get_Texture_Bitdepth(void)
{
	return g_bgfx.bit_depth;
}

void DX8Wrapper::Update_Window(void *hwnd)
{
	g_bgfx.window = reinterpret_cast<SDL_Window *>(hwnd);
	if (g_bgfx.initialized) {
		Sync_Backbuffer(true);
	}
}

bool DX8Wrapper::Is_Initted()
{
	return g_bgfx.initialized;
}

bool DX8Wrapper::Registry_Save_Render_Device(const char *sub_key)
{
	return Registry_Save_Render_Device(sub_key, 0, g_bgfx.width, g_bgfx.height, g_bgfx.bit_depth, g_bgfx.windowed, g_bgfx.bit_depth);
}

bool DX8Wrapper::Registry_Save_Render_Device(const char *sub_key, int, int width, int height, int depth, bool windowed, int texture_depth)
{
	if (sub_key == nullptr) {
		return false;
	}

	RegistryClass registry(sub_key);
	if (!registry.Is_Valid()) {
		return false;
	}

	registry.Set_String("RenderDeviceName", g_bgfx.render_device_desc.Get_Device_Name());
	registry.Set_Int("RenderDeviceWidth", width > 0 ? width : g_bgfx.width);
	registry.Set_Int("RenderDeviceHeight", height > 0 ? height : g_bgfx.height);
	registry.Set_Int("RenderDeviceDepth", depth > 0 ? depth : g_bgfx.bit_depth);
	registry.Set_Int("RenderDeviceWindowed", windowed ? 1 : 0);
	registry.Set_Int("RenderDeviceTextureDepth", texture_depth > 0 ? texture_depth : g_bgfx.bit_depth);
	return true;
}

bool DX8Wrapper::Registry_Load_Render_Device(const char *sub_key, bool resize_window)
{
	char device[256] = {};
	int width = g_bgfx.width;
	int height = g_bgfx.height;
	int depth = g_bgfx.bit_depth;
	int windowed = g_bgfx.windowed ? 1 : 0;
	int texture_depth = g_bgfx.bit_depth;

	if (!Registry_Load_Render_Device(sub_key, device, sizeof(device), width, height, depth, windowed, texture_depth)) {
		return Set_Any_Render_Device();
	}

	Set_Texture_Bitdepth(texture_depth);
	return Set_Render_Device(device[0] != '\0' ? device : g_bgfx.render_device_desc.Get_Device_Name(), width, height, depth, windowed, resize_window);
}

bool DX8Wrapper::Registry_Load_Render_Device(const char *sub_key, char *device, int device_len, int &width, int &height, int &depth, int &windowed, int &texture_depth)
{
	if (sub_key == nullptr) {
		return false;
	}

	RegistryClass registry(sub_key);
	if (!registry.Is_Valid()) {
		return false;
	}

	char device_name[256] = {};
	registry.Get_String("RenderDeviceName", device_name, sizeof(device_name), g_bgfx.render_device_desc.Get_Device_Name());
	width = registry.Get_Int("RenderDeviceWidth", g_bgfx.width);
	height = registry.Get_Int("RenderDeviceHeight", g_bgfx.height);
	depth = registry.Get_Int("RenderDeviceDepth", g_bgfx.bit_depth);
	windowed = registry.Get_Int("RenderDeviceWindowed", g_bgfx.windowed ? 1 : 0);
	texture_depth = registry.Get_Int("RenderDeviceTextureDepth", g_bgfx.bit_depth);

	if (device != nullptr && device_len > 0) {
		std::snprintf(device, static_cast<size_t>(device_len), "%s", device_name);
	}

	return true;
}

void DX8Wrapper::Set_Transform(TransformSlot transform, const Matrix4 &m)
{
	switch (transform) {
		case TRANSFORM_WORLD: g_bgfx.world = m; break;
		case TRANSFORM_VIEW: g_bgfx.view = m; break;
		case TRANSFORM_PROJECTION: g_bgfx.projection = m; break;
		default: break;
	}
}

void DX8Wrapper::Set_Transform(TransformSlot transform, const Matrix3D &m)
{
	Set_Transform(transform, Matrix4(m));
}

void DX8Wrapper::Get_Transform(TransformSlot transform, Matrix4 &m)
{
	switch (transform) {
		case TRANSFORM_WORLD: m = g_bgfx.world; break;
		case TRANSFORM_VIEW: m = g_bgfx.view; break;
		case TRANSFORM_PROJECTION: m = g_bgfx.projection; break;
		default: m.Make_Identity(); break;
	}
}

void DX8Wrapper::Set_Projection_Transform_With_Z_Bias(const Matrix4 &matrix, float, float)
{
	g_bgfx.projection = matrix;
}

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass *vb)
{
	g_bgfx.vertex_data = vb != nullptr ? vb->Get_Vertex_Data() : nullptr;
	g_bgfx.vertex_fvf = vb != nullptr ? &vb->FVF_Info() : nullptr;
	g_bgfx.vertex_count = vb != nullptr ? vb->Get_Vertex_Count() : 0;
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass &vba)
{
	g_bgfx.vertex_data = vba.Get_Vertex_Data();
	g_bgfx.vertex_fvf = &vba.FVF_Info();
	g_bgfx.vertex_count = vba.Get_Vertex_Count();
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass *ib, unsigned short index_base_offset)
{
	g_bgfx.index_data = ib != nullptr ? ib->Get_Index_Data() : nullptr;
	g_bgfx.index_count = ib != nullptr ? ib->Get_Index_Count() : 0;
	g_bgfx.index_base_offset = index_base_offset;
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass &iba, unsigned short index_base_offset)
{
	g_bgfx.index_data = iba.Get_Index_Data();
	g_bgfx.index_count = iba.Get_Index_Count();
	g_bgfx.index_base_offset = index_base_offset;
}

void DX8Wrapper::Set_Index_Buffer_Index_Offset(unsigned offset)
{
	g_bgfx.index_base_offset = static_cast<unsigned short>(offset);
}

void DX8Wrapper::Draw_Triangles(unsigned, unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	Submit_Triangles(start_index, polygon_count, min_vertex_index, vertex_count);
}

void DX8Wrapper::Draw_Triangles(unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	Submit_Triangles(start_index, polygon_count, min_vertex_index, vertex_count);
}

void DX8Wrapper::Set_Texture(unsigned stage, TextureClass *texture)
{
	if (stage < MAX_TEXTURE_STAGES) {
		g_bgfx.textures[stage] = texture;
	}
}

void DX8Wrapper::Set_Material(const VertexMaterialClass *material)
{
	g_bgfx.material = material;
}

void DX8Wrapper::Set_Shader(const ShaderClass &shader)
{
	g_bgfx.shader = shader;
}

void DX8Wrapper::Set_DX8_Texture_Stage_State(unsigned stage, unsigned state, unsigned value)
{
	if (stage < MAX_TEXTURE_STAGES && state < 32) {
		g_bgfx.texture_stage_states[stage][state] = value;
	}
}

void DX8Wrapper::Set_DX8_Render_State(unsigned state, unsigned value)
{
	if (state < 256) {
		g_bgfx.render_states[state] = value;
	}
}

void DX8Wrapper::Set_World_Identity()
{
	g_bgfx.world.Make_Identity();
}

void DX8Wrapper::Set_View_Identity()
{
	g_bgfx.view.Make_Identity();
}

void DX8Wrapper::_Copy_DX8_Rects(IDirect3DSurface8 *pSourceSurface, const RECT *pSourceRectsArray, UINT cRects, IDirect3DSurface8 *pDestinationSurface, const POINT *pDestPointsArray)
{
	Copy_Surface_Rectangles(pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);
}


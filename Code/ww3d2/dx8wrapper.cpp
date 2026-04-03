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
#include <cmath>
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
	uint32_t clear_color = 0x000000FFu;
	float clear_depth = 1.0f;
	uint8_t clear_stencil = 0;
	uint16_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
	int width = kFallbackRenderWidth;
	int height = kFallbackRenderHeight;
	int window_width = kFallbackRenderWidth;
	int window_height = kFallbackRenderHeight;
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
	Matrix4 texture_transforms[MAX_TEXTURE_STAGES];
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
	const VertexBufferClass *current_vb = nullptr;
	const IndexBufferClass *current_ib = nullptr;
	unsigned current_vb_type = BUFFER_TYPE_INVALID;
	unsigned current_ib_type = BUFFER_TYPE_INVALID;
	unsigned current_vba_offset = 0;
	unsigned current_iba_offset = 0;

	BgfxDx8WrapperState()
	{
		world.Make_Identity();
		view.Make_Identity();
		projection.Make_Identity();
		for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
			texture_transforms[stage].Make_Identity();
		}
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

// bgfx::setViewClear() expects RGBA, but DX8Wrapper::Convert_Color() produces ARGB.
uint32_t ARGB_To_RGBA(uint32_t argb)
{
	return (argb << 8) | ((argb >> 24) & 0xFFu);
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
	g_bgfx.current_vb = nullptr;
	g_bgfx.current_ib = nullptr;
	g_bgfx.current_vb_type = BUFFER_TYPE_INVALID;
	g_bgfx.current_ib_type = BUFFER_TYPE_INVALID;
	g_bgfx.current_vba_offset = 0;
	g_bgfx.current_iba_offset = 0;
	for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
		g_bgfx.texture_transforms[stage].Make_Identity();
		for (unsigned state = 0; state < 32; ++state) {
			g_bgfx.texture_stage_states[stage][state] = 0;
		}
	}
	for (unsigned state = 0; state < 256; ++state) {
		g_bgfx.render_states[state] = 0;
	}
}

void Apply_Material_Texture_State(const VertexMaterialClass *material)
{
	if (material == nullptr) {
		DX8Wrapper::Set_DX8_Render_State(D3DRS_LIGHTING, false);
		DX8Wrapper::Set_DX8_Render_State(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
		DX8Wrapper::Set_DX8_Render_State(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
		DX8Wrapper::Set_DX8_Render_State(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);

		for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | stage);
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		}
		return;
	}

	// Legacy accessors are not const-correct even though material apply is logically read-only.
	VertexMaterialClass *mutable_material = const_cast<VertexMaterialClass *>(material);
	DX8Wrapper::Set_DX8_Render_State(D3DRS_LIGHTING, mutable_material->Get_Lighting());
	DX8Wrapper::Set_DX8_Render_State(D3DRS_AMBIENTMATERIALSOURCE, mutable_material->Get_Ambient_Color_Source());
	DX8Wrapper::Set_DX8_Render_State(D3DRS_DIFFUSEMATERIALSOURCE, mutable_material->Get_Diffuse_Color_Source());
	DX8Wrapper::Set_DX8_Render_State(D3DRS_EMISSIVEMATERIALSOURCE, mutable_material->Get_Emissive_Color_Source());

	for (unsigned stage = 0; stage < MAX_TEXTURE_STAGES; ++stage) {
		TextureMapperClass *mapper = mutable_material->Get_Mapper(stage);
		if (mapper != nullptr) {
			mapper->Apply(mutable_material->Get_UV_Source(stage));
			mapper->Release_Ref();
		} else {
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | mutable_material->Get_UV_Source(stage));
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		}
	}
}

Vector3 Transform_Normal_To_Camera_Space(const Matrix4 &world_view, const Vector3 &normal)
{
	return Vector3(
		world_view[0][0] * normal.X + world_view[0][1] * normal.Y + world_view[0][2] * normal.Z,
		world_view[1][0] * normal.X + world_view[1][1] * normal.Y + world_view[1][2] * normal.Z,
		world_view[2][0] * normal.X + world_view[2][1] * normal.Y + world_view[2][2] * normal.Z);
}

Vector3 Normalize_Vector(const Vector3 &value)
{
	const float length_squared = value.X * value.X + value.Y * value.Y + value.Z * value.Z;
	if (length_squared <= 1.0e-12f) {
		return Vector3(0.0f, 0.0f, 1.0f);
	}

	const float inverse_length = 1.0f / std::sqrt(length_squared);
	return Vector3(value.X * inverse_length, value.Y * inverse_length, value.Z * inverse_length);
}

Vector4 Generate_Stage0_Texture_Input(
	const Matrix4 &world_view,
	unsigned texcoord_generation,
	unsigned texcoord_set,
	unsigned texcoord_count,
	unsigned tex_offset,
	bool has_normal,
	unsigned normal_offset,
	const unsigned char *src,
	const float *position,
	bool *generated)
{
	*generated = false;

	if (texcoord_generation == D3DTSS_TCI_PASSTHRU) {
		if (texcoord_set < texcoord_count) {
			const float *uv = reinterpret_cast<const float *>(src + tex_offset);
			*generated = true;
			return Vector4(uv[0], uv[1], 0.0f, 1.0f);
		}
		return Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	const Vector3 object_position(position[0], position[1], position[2]);
	Vector4 camera_position;
	Matrix4::Transform_Vector(world_view, object_position, &camera_position);

	if (texcoord_generation == D3DTSS_TCI_CAMERASPACEPOSITION) {
		*generated = true;
		return camera_position;
	}

	if (!has_normal) {
		return Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	const float *normal_data = reinterpret_cast<const float *>(src + normal_offset);
	const Vector3 object_normal(normal_data[0], normal_data[1], normal_data[2]);
	const Vector3 camera_normal = Normalize_Vector(Transform_Normal_To_Camera_Space(world_view, object_normal));

	if (texcoord_generation == D3DTSS_TCI_CAMERASPACENORMAL) {
		*generated = true;
		return Vector4(camera_normal.X, camera_normal.Y, camera_normal.Z, 1.0f);
	}

	if (texcoord_generation == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR) {
		const Vector3 to_eye = Normalize_Vector(Vector3(-camera_position.X, -camera_position.Y, -camera_position.Z));
		const float dot = camera_normal.X * to_eye.X + camera_normal.Y * to_eye.Y + camera_normal.Z * to_eye.Z;
		const Vector3 reflection(
			2.0f * dot * camera_normal.X - to_eye.X,
			2.0f * dot * camera_normal.Y - to_eye.Y,
			2.0f * dot * camera_normal.Z - to_eye.Z);
		*generated = true;
		return Vector4(reflection.X, reflection.Y, reflection.Z, 1.0f);
	}

	return Vector4(0.0f, 0.0f, 0.0f, 1.0f);
}

uint32_t Resolve_Diffuse_Color(uint32_t vertex_diffuse, bool has_diffuse)
{
	if (g_bgfx.material == nullptr) {
		return has_diffuse ? vertex_diffuse : 0xFFFFFFFFU;
	}

	// Mesh/material preprocessing already bakes material color and opacity into COLOR1 vertex diffuse when requested.
	const unsigned diffuse_source = g_bgfx.render_states[D3DRS_DIFFUSEMATERIALSOURCE];
	if (diffuse_source == D3DMCS_COLOR1 && has_diffuse) {
		return vertex_diffuse;
	}

	Vector3 diffuse_color(1.0f, 1.0f, 1.0f);
	Vector3 emissive_color(0.0f, 0.0f, 0.0f);
	g_bgfx.material->Get_Diffuse(&diffuse_color);
	g_bgfx.material->Get_Emissive(&emissive_color);

	Vector3 resolved = diffuse_color;
	if (g_bgfx.render_states[D3DRS_LIGHTING] != 0) {
		resolved.X = std::min(resolved.X + emissive_color.X, 1.0f);
		resolved.Y = std::min(resolved.Y + emissive_color.Y, 1.0f);
		resolved.Z = std::min(resolved.Z + emissive_color.Z, 1.0f);
	}

	return DX8Wrapper::Convert_Color(Vector4(resolved.X, resolved.Y, resolved.Z, g_bgfx.material->Get_Opacity()));
}

void Clamp_Window_Size(int &width, int &height)
{
	width = std::max(width, 1);
	height = std::max(height, 1);
}

bool Query_Window_Size(SDL_Window *window, int &width, int &height)
{
	if (window == nullptr) {
		width = kFallbackRenderWidth;
		height = kFallbackRenderHeight;
		return false;
	}

	if (!SDL_GetWindowSize(window, &width, &height)) {
		width = kFallbackRenderWidth;
		height = kFallbackRenderHeight;
		return false;
	}

	Clamp_Window_Size(width, height);
	return true;
}

bool Query_Window_Pixel_Size(SDL_Window *window, int &width, int &height)
{
	if (window == nullptr) {
		width = kFallbackRenderWidth;
		height = kFallbackRenderHeight;
		return false;
	}

	if (!SDL_GetWindowSizeInPixels(window, &width, &height)) {
		return Query_Window_Size(window, width, height);
	}

	Clamp_Window_Size(width, height);
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

	int window_width = g_bgfx.window_width;
	int window_height = g_bgfx.window_height;
	Query_Window_Size(g_bgfx.window, window_width, window_height);

	int width = g_bgfx.width;
	int height = g_bgfx.height;
	Query_Window_Pixel_Size(g_bgfx.window, width, height);

	const uint32_t reset_flags = Compose_Reset_Flags();
	const bool render_changed = force_reset || width != g_bgfx.width || height != g_bgfx.height || reset_flags != g_bgfx.reset_flags;
	const bool window_changed = window_width != g_bgfx.window_width || window_height != g_bgfx.window_height;
	if (render_changed) {
		g_bgfx.width = width;
		g_bgfx.height = height;
		g_bgfx.reset_flags = reset_flags;
		bgfx::reset(static_cast<uint32_t>(g_bgfx.width), static_cast<uint32_t>(g_bgfx.height), g_bgfx.reset_flags);
	}

	if (render_changed || window_changed) {
		g_bgfx.window_width = window_width;
		g_bgfx.window_height = window_height;
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

Matrix4 Adjust_Projection_For_BGFX(const Matrix4 &projection)
{
	const bgfx::Caps *caps = bgfx::getCaps();

	// CameraClass::Get_D3D_Projection_Matrix already maps clip-space depth to [0, 1].
	// Backends that use [0, 1] (Vulkan, D3D11/12, Metal) need no adjustment.
	if (caps == nullptr || !caps->homogeneousDepth) {
		return projection;
	}

	// OpenGL uses homogeneous depth [-1, 1]. Convert the D3D-style [0, 1] projection
	// back to [-1, 1]: z_ndc' = 2 * z_ndc - 1.
	Matrix4 depth_remap(true);
	depth_remap[2][2] = 2.0f;
	depth_remap[2][3] = -1.0f;
	return depth_remap * projection;
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

	if (g_bgfx.shader.Get_Alpha_Test() == ShaderClass::ALPHATEST_ENABLE) {
		uint8_t alpha_ref = 0x60;
		if (g_bgfx.shader.Get_Src_Blend_Func() == ShaderClass::SRCBLEND_ONE_MINUS_SRC_ALPHA) {
			alpha_ref = static_cast<uint8_t>(0xFF - alpha_ref);
		}
		state |= BGFX_STATE_ALPHA_REF(alpha_ref);
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

	if (g_bgfx.shader.Get_Cull_Mode() == ShaderClass::CULL_MODE_ENABLE) {
		state |= ShaderClass::Is_Backface_Culling_Inverted() ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW;
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
	const unsigned vertex_format = g_bgfx.vertex_fvf->Get_FVF();
	const unsigned texcoord_count = (vertex_format & D3DFVF_TEXCOUNT_MASK) >> 8;
	const unsigned location_offset = g_bgfx.vertex_fvf->Get_Location_Offset();
	const unsigned normal_offset = g_bgfx.vertex_fvf->Get_Normal_Offset();
	const unsigned diffuse_offset = g_bgfx.vertex_fvf->Get_Diffuse_Offset();
	const bool has_diffuse = (vertex_format & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE;
	const bool has_normal = (vertex_format & D3DFVF_NORMAL) == D3DFVF_NORMAL;
	const unsigned texcoord_index = g_bgfx.texture_stage_states[0][D3DTSS_TEXCOORDINDEX];
	const unsigned texcoord_generation = texcoord_index & 0xFFFF0000u;
	const unsigned selected_texcoord = texcoord_index & 0x0000FFFFu;
	const unsigned texcoord_transform = g_bgfx.texture_stage_states[0][D3DTSS_TEXTURETRANSFORMFLAGS];
	const bool has_selected_texcoord = texcoord_count > selected_texcoord;
	const unsigned tex_offset = has_selected_texcoord ? g_bgfx.vertex_fvf->Get_Tex_Offset(selected_texcoord) : 0;
	const bool apply_texture_transform = texcoord_transform != D3DTTFF_DISABLE;
	const bool projected_texture = (texcoord_transform & D3DTTFF_PROJECTED) == D3DTTFF_PROJECTED;
	const Matrix4 &texture_transform = g_bgfx.texture_transforms[0];
	const Matrix4 world_view = g_bgfx.view * g_bgfx.world;
	BgfxGuiVertex *dst_vertices = reinterpret_cast<BgfxGuiVertex *>(tvb.data);
	for (unsigned short i = 0; i < vertex_count; ++i) {
		const unsigned char *src = g_bgfx.vertex_data + static_cast<size_t>(min_vertex_index + i) * vertex_stride;
		const float *position = reinterpret_cast<const float *>(src + location_offset);
		const uint32_t diffuse = has_diffuse ? *reinterpret_cast<const uint32_t *>(src + diffuse_offset) : 0xFFFFFFFFU;
		const uint32_t resolved_diffuse = Resolve_Diffuse_Color(diffuse, has_diffuse);
		dst_vertices[i].x = position[0];
		dst_vertices[i].y = position[1];
		dst_vertices[i].z = position[2];
		dst_vertices[i].abgr = DX8_To_BGFX_Color(resolved_diffuse);

		bool generated_texcoord = false;
		const Vector4 stage0_input = Generate_Stage0_Texture_Input(
			world_view,
			texcoord_generation,
			selected_texcoord,
			texcoord_count,
			tex_offset,
			has_normal,
			normal_offset,
			src,
			position,
			&generated_texcoord);
		if (generated_texcoord) {
			Vector4 transformed = stage0_input;
			if (apply_texture_transform) {
				Matrix4::Transform_Vector(texture_transform, stage0_input, &transformed);
			}

			if (projected_texture && std::fabs(transformed.W) > 1.0e-12f) {
				dst_vertices[i].u = transformed.X / transformed.W;
				dst_vertices[i].v = transformed.Y / transformed.W;
			} else {
				dst_vertices[i].u = transformed.X;
				dst_vertices[i].v = transformed.Y;
			}
		} else {
			dst_vertices[i].u = 0.0f;
			dst_vertices[i].v = 0.0f;
		}
	}

	uint16_t *dst_indices = reinterpret_cast<uint16_t *>(tib.data);
	for (uint32_t i = 0; i < index_total; ++i) {
		const uint16_t source_index = g_bgfx.index_data[start_index + i] + g_bgfx.index_base_offset;
		dst_indices[i] = static_cast<uint16_t>(source_index - min_vertex_index);
	}

	// bgfx::setViewTransform is view-level (shared by ALL draw calls in a view), not
	// per-draw-call.  Since the engine changes view/projection per draw call (e.g. 3D
	// backdrop vs 2D UI) but all draw calls share kBootstrapViewId, the last
	// setViewTransform wins and earlier ones are lost.
	//
	// Fix: compute the full MVP on the CPU and pass it via bgfx::setTransform (which IS
	// per-draw-call).  Begin_Scene already sets the view-level view/proj to identity,
	// so u_modelViewProj = model * I * I = model = our pre-computed MVP.
	//
	// The engine's Matrix4 uses row-major storage with column-vector convention (M * v).
	// bgfx expects the model matrix in row-major with row-vector convention (v * M).
	// Transposing converts between the two conventions.
	const Matrix4 adjusted_projection = Adjust_Projection_For_BGFX(g_bgfx.projection);
	const Matrix4 mvp_engine = adjusted_projection * g_bgfx.view * g_bgfx.world;
	const Matrix4 mvp_bgfx = mvp_engine.Transpose();

	const bgfx::TextureHandle texture_handle = BgfxCompat_Get_Texture_Handle(g_bgfx.textures[0]);
	const uint64_t sampler_flags = BgfxCompat_Get_Sampler_Flags(g_bgfx.textures[0]);
	bgfx::setTransform(&mvp_bgfx[0][0]);
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
	int window_width = kFallbackRenderWidth;
	int window_height = kFallbackRenderHeight;
	Query_Window_Size(window, window_width, window_height);

	int width = kFallbackRenderWidth;
	int height = kFallbackRenderHeight;
	Query_Window_Pixel_Size(window, width, height);

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
	g_bgfx.window_width = window_width;
	g_bgfx.window_height = window_height;
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
	g_bgfx.render_device_desc.add_resolution(g_bgfx.window_width, g_bgfx.window_height, g_bgfx.bit_depth);
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
	g_bgfx.clear_color = ARGB_To_RGBA(Convert_Color(color, 1.0f));
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

	Query_Window_Size(g_bgfx.window, g_bgfx.window_width, g_bgfx.window_height);
	Query_Window_Pixel_Size(g_bgfx.window, g_bgfx.width, g_bgfx.height);
	Update_Windowed_State();
	Refresh_Render_Device_Desc();
	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, g_bgfx.width, g_bgfx.height));
	return true;
}

void DX8Wrapper::Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed)
{
	width = g_bgfx.window_width;
	height = g_bgfx.window_height;
	bits = g_bgfx.bit_depth;
	windowed = g_bgfx.windowed;
}

void DX8Wrapper::Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed)
{
	width = g_bgfx.width;
	height = g_bgfx.height;
	bits = g_bgfx.bit_depth;
	windowed = g_bgfx.windowed;
}

int DX8Wrapper::Get_Device_Resolution_Width(void)
{
	return g_bgfx.window_width;
}

int DX8Wrapper::Get_Device_Resolution_Height(void)
{
	return g_bgfx.window_height;
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
	return Registry_Save_Render_Device(sub_key, 0, g_bgfx.window_width, g_bgfx.window_height, g_bgfx.bit_depth, g_bgfx.windowed, g_bgfx.bit_depth);
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
	registry.Set_Int("RenderDeviceWidth", width > 0 ? width : g_bgfx.window_width);
	registry.Set_Int("RenderDeviceHeight", height > 0 ? height : g_bgfx.window_height);
	registry.Set_Int("RenderDeviceDepth", depth > 0 ? depth : g_bgfx.bit_depth);
	registry.Set_Int("RenderDeviceWindowed", windowed ? 1 : 0);
	registry.Set_Int("RenderDeviceTextureDepth", texture_depth > 0 ? texture_depth : g_bgfx.bit_depth);
	return true;
}

bool DX8Wrapper::Registry_Load_Render_Device(const char *sub_key, bool resize_window)
{
	char device[256] = {};
	int width = g_bgfx.window_width;
	int height = g_bgfx.window_height;
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
	width = registry.Get_Int("RenderDeviceWidth", g_bgfx.window_width);
	height = registry.Get_Int("RenderDeviceHeight", g_bgfx.window_height);
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
		case TRANSFORM_TEXTURE0:
		case static_cast<TransformSlot>(TRANSFORM_TEXTURE0 + 1):
		{
			const unsigned stage = static_cast<unsigned>(transform - TRANSFORM_TEXTURE0);
			if (stage < MAX_TEXTURE_STAGES) {
				g_bgfx.texture_transforms[stage] = m;
			}
			break;
		}
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
		case TRANSFORM_TEXTURE0:
		case static_cast<TransformSlot>(TRANSFORM_TEXTURE0 + 1):
		{
			const unsigned stage = static_cast<unsigned>(transform - TRANSFORM_TEXTURE0);
			if (stage < MAX_TEXTURE_STAGES) {
				m = g_bgfx.texture_transforms[stage];
			} else {
				m.Make_Identity();
			}
			break;
		}
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
	g_bgfx.current_vb = vb;
	g_bgfx.current_vb_type = vb != nullptr ? vb->Type() : BUFFER_TYPE_INVALID;
	g_bgfx.current_vba_offset = 0;
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass &vba)
{
	g_bgfx.vertex_data = vba.Get_Vertex_Data();
	g_bgfx.vertex_fvf = &vba.FVF_Info();
	g_bgfx.vertex_count = vba.Get_Vertex_Count();
	g_bgfx.current_vb = nullptr;
	g_bgfx.current_vb_type = vba.Get_Type();
	g_bgfx.current_vba_offset = 0;
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass *ib, unsigned short index_base_offset)
{
	g_bgfx.index_data = ib != nullptr ? ib->Get_Index_Data() : nullptr;
	g_bgfx.index_count = ib != nullptr ? ib->Get_Index_Count() : 0;
	g_bgfx.index_base_offset = index_base_offset;
	g_bgfx.current_ib = ib;
	g_bgfx.current_ib_type = ib != nullptr ? ib->Type() : BUFFER_TYPE_INVALID;
	g_bgfx.current_iba_offset = 0;
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass &iba, unsigned short index_base_offset)
{
	g_bgfx.index_data = iba.Get_Index_Data();
	g_bgfx.index_count = iba.Get_Index_Count();
	g_bgfx.index_base_offset = index_base_offset;
	g_bgfx.current_ib = nullptr;
	g_bgfx.current_ib_type = iba.Get_Type();
	g_bgfx.current_iba_offset = 0;
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
	Apply_Material_Texture_State(material);
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

void DX8Wrapper::Get_Render_State(RenderStateStruct &state)
{
	state.shader = g_bgfx.shader;
	state.material = const_cast<VertexMaterialClass*>(g_bgfx.material);
	if (state.material) state.material->Add_Ref();
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		state.Textures[i] = g_bgfx.textures[i];
		if (state.Textures[i]) state.Textures[i]->Add_Ref();
	}
	state.world = g_bgfx.world;
	state.view = g_bgfx.view;
	state.projection = g_bgfx.projection;
	state.vertex_buffer = const_cast<VertexBufferClass*>(g_bgfx.current_vb);
	if (state.vertex_buffer) state.vertex_buffer->Add_Ref();
	state.index_buffer = const_cast<IndexBufferClass*>(g_bgfx.current_ib);
	if (state.index_buffer) state.index_buffer->Add_Ref();
	state.vertex_buffer_type = g_bgfx.current_vb_type;
	state.index_buffer_type = g_bgfx.current_ib_type;
	state.vba_offset = g_bgfx.current_vba_offset;
	state.iba_offset = g_bgfx.current_iba_offset;
	state.index_base_offset = g_bgfx.index_base_offset;
}

void DX8Wrapper::Set_Render_State(const RenderStateStruct &state)
{
	Set_Shader(state.shader);
	Set_Material(state.material);
	for (unsigned i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		Set_Texture(i, state.Textures[i]);
	}
	Set_Transform(TRANSFORM_WORLD, state.world);
	Set_Transform(TRANSFORM_VIEW, state.view);
	Set_Transform(TRANSFORM_PROJECTION, state.projection);
	if (state.vertex_buffer) {
		Set_Vertex_Buffer(state.vertex_buffer);
	}
	if (state.index_buffer) {
		Set_Index_Buffer(state.index_buffer, static_cast<unsigned short>(state.index_base_offset));
	}
}

void DX8Wrapper::Release_Render_State()
{
}

void DX8Wrapper::Apply_Render_State_Changes()
{
}

void DX8Wrapper::_Copy_DX8_Rects(IDirect3DSurface8 *pSourceSurface, const RECT *pSourceRectsArray, UINT cRects, IDirect3DSurface8 *pDestinationSurface, const POINT *pDestPointsArray)
{
	Copy_Surface_Rectangles(pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);
}

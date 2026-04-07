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
#include "light.h"
#include "lightenvironment.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "rddesc.h"
#include "render2d.h"
#include "refcount.h"
#include "registry.h"
#include "shader.h"
#include "texture.h"
#include "vertmaterial.h"
#include "ww3d.h"
#include "wwdebug.h"
#include "wwperfmon.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "glsl/fs_basic.sc.bin.h"
#include "glsl/fs_scene.sc.bin.h"
#include "glsl/vs_basic.sc.bin.h"
#include "glsl/vs_lit_dynamic.sc.bin.h"
#include "glsl/vs_lit_environment.sc.bin.h"
#include "glsl/vs_unlit.sc.bin.h"
#include "spirv/fs_basic.sc.bin.h"
#include "spirv/fs_scene.sc.bin.h"
#include "spirv/vs_basic.sc.bin.h"
#include "spirv/vs_lit_dynamic.sc.bin.h"
#include "spirv/vs_lit_environment.sc.bin.h"
#include "spirv/vs_unlit.sc.bin.h"
#if defined(_WIN32)
#include "dx11/fs_basic.sc.bin.h"
#include "dx11/fs_scene.sc.bin.h"
#include "dx11/vs_basic.sc.bin.h"
#include "dx11/vs_lit_dynamic.sc.bin.h"
#include "dx11/vs_lit_environment.sc.bin.h"
#include "dx11/vs_unlit.sc.bin.h"
#endif

namespace {

constexpr int kFallbackRenderWidth = 640;
constexpr int kFallbackRenderHeight = 480;
constexpr bgfx::ViewId kFirstSceneViewId = 0;
constexpr bgfx::ViewId kMaxSceneViewId = 249; // View 250 is reserved for render-target readback.
constexpr uint32_t kCompatibilityScratchVertexBytes = 24u << 20;
constexpr uint32_t kCompatibilityScratchIndexBytes = 8u << 20;

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
	float nx;
	float ny;
	float nz;
	uint32_t abgr;
	float u;
	float v;
};

constexpr uint32_t kCompatibilityScratchVertexCapacity = kCompatibilityScratchVertexBytes / sizeof(BgfxGuiVertex);
constexpr uint32_t kCompatibilityScratchIndexCapacity = kCompatibilityScratchIndexBytes / sizeof(uint16_t);

enum BgfxProgramType {
	BGFX_PROGRAM_BASIC = 0,
	BGFX_PROGRAM_UNLIT,
	BGFX_PROGRAM_LIGHT_ENVIRONMENT,
	BGFX_PROGRAM_LIGHT_DYNAMIC,
	BGFX_PROGRAM_COUNT
};

struct EmbeddedShaderBinary {
	const uint8_t *glsl_data = nullptr;
	uint32_t glsl_size = 0;
	const uint8_t *spirv_data = nullptr;
	uint32_t spirv_size = 0;
#if defined(_WIN32)
	const uint8_t *dx11_data = nullptr;
	uint32_t dx11_size = 0;
#endif
};

struct BgfxProgramSelection {
	BgfxProgramType program = BGFX_PROGRAM_BASIC;
	bool fog_enabled = false;
	bool use_gpu_texgen = false;
};

struct BgfxLightState {
	bool enabled = false;
	LightClass::LightType type = LightClass::POINT;
	Vector3 ambient = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 diffuse = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 direction = Vector3(0.0f, 0.0f, -1.0f);
	float far_atten_start = 0.0f;
	float far_atten_end = 1.0f;
	float spot_angle_cos = -1.0f;
};

struct BgfxLightEnvironmentState {
	bool enabled = false;
	Vector3 ambient = Vector3(0.0f, 0.0f, 0.0f);
	unsigned count = 0;
	Vector3 directions[4];
	Vector3 diffuse[4];
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
	bgfx::DynamicVertexBufferHandle compatibility_vertex_buffer = BGFX_INVALID_HANDLE;
	bgfx::DynamicIndexBufferHandle compatibility_index_buffer = BGFX_INVALID_HANDLE;
	uint32_t compatibility_vertex_capacity = 0;
	uint32_t compatibility_index_capacity = 0;
	uint32_t compatibility_vertex_offset = 0;
	uint32_t compatibility_index_offset = 0;
	bgfx::ProgramHandle programs[BGFX_PROGRAM_COUNT] = {
		BGFX_INVALID_HANDLE,
		BGFX_INVALID_HANDLE,
		BGFX_INVALID_HANDLE,
		BGFX_INVALID_HANDLE
	};
	bgfx::UniformHandle texture_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle fog_state_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle fog_color_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle color_adjust_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle material_source_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle material_ambient_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle material_diffuse_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle material_emissive_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle material_state_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle render_ambient_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle light_environment_state_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle light_environment_ambient_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle light_environment_direction_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle light_environment_diffuse_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle world_view_row_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle texgen_state_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle texture_transform_row_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle direct_light_state_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle direct_light_position_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle direct_light_direction_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle direct_light_ambient_uniform = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle direct_light_diffuse_uniform = BGFX_INVALID_HANDLE;
	bool layout_ready = false;
	Matrix4 world;
	Matrix4 view;
	Matrix4 projection;
	Matrix4 texture_transforms[MAX_TEXTURE_STAGES];
	const uint8_t *vertex_data = nullptr;
	const FVFInfoClass *vertex_fvf = nullptr;
	uint16_t vertex_count = 0;
	const uint16_t *index_data = nullptr;
	uint16_t index_count = 0;
	uint16_t index_base_offset = 0;
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
	TextureClass *render_target = nullptr;
	bgfx::ViewId current_view_id = kFirstSceneViewId;
	bgfx::ViewId next_view_id = kFirstSceneViewId;
	bool scene_active = false;
	BgfxLightState lights[4];
	BgfxLightEnvironmentState light_environment;
	float gamma = 1.0f;
	float brightness = 0.0f;
	float contrast = 1.0f;

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

const EmbeddedShaderBinary kVertexShaders[BGFX_PROGRAM_COUNT] = {
	{
		vs_basic_glsl,
		static_cast<uint32_t>(sizeof(vs_basic_glsl)),
		vs_basic_spv,
		static_cast<uint32_t>(sizeof(vs_basic_spv)),
#if defined(_WIN32)
		vs_basic_dx11,
		static_cast<uint32_t>(sizeof(vs_basic_dx11)),
#endif
	},
	{
		vs_unlit_glsl,
		static_cast<uint32_t>(sizeof(vs_unlit_glsl)),
		vs_unlit_spv,
		static_cast<uint32_t>(sizeof(vs_unlit_spv)),
#if defined(_WIN32)
		vs_unlit_dx11,
		static_cast<uint32_t>(sizeof(vs_unlit_dx11)),
#endif
	},
	{
		vs_lit_environment_glsl,
		static_cast<uint32_t>(sizeof(vs_lit_environment_glsl)),
		vs_lit_environment_spv,
		static_cast<uint32_t>(sizeof(vs_lit_environment_spv)),
#if defined(_WIN32)
		vs_lit_environment_dx11,
		static_cast<uint32_t>(sizeof(vs_lit_environment_dx11)),
#endif
	},
	{
		vs_lit_dynamic_glsl,
		static_cast<uint32_t>(sizeof(vs_lit_dynamic_glsl)),
		vs_lit_dynamic_spv,
		static_cast<uint32_t>(sizeof(vs_lit_dynamic_spv)),
#if defined(_WIN32)
		vs_lit_dynamic_dx11,
		static_cast<uint32_t>(sizeof(vs_lit_dynamic_dx11)),
#endif
	},
};

const EmbeddedShaderBinary kFragmentShaders[BGFX_PROGRAM_COUNT] = {
	{
		fs_basic_glsl,
		static_cast<uint32_t>(sizeof(fs_basic_glsl)),
		fs_basic_spv,
		static_cast<uint32_t>(sizeof(fs_basic_spv)),
#if defined(_WIN32)
		fs_basic_dx11,
		static_cast<uint32_t>(sizeof(fs_basic_dx11)),
#endif
	},
	{
		fs_scene_glsl,
		static_cast<uint32_t>(sizeof(fs_scene_glsl)),
		fs_scene_spv,
		static_cast<uint32_t>(sizeof(fs_scene_spv)),
#if defined(_WIN32)
		fs_scene_dx11,
		static_cast<uint32_t>(sizeof(fs_scene_dx11)),
#endif
	},
	{
		fs_scene_glsl,
		static_cast<uint32_t>(sizeof(fs_scene_glsl)),
		fs_scene_spv,
		static_cast<uint32_t>(sizeof(fs_scene_spv)),
#if defined(_WIN32)
		fs_scene_dx11,
		static_cast<uint32_t>(sizeof(fs_scene_dx11)),
#endif
	},
	{
		fs_scene_glsl,
		static_cast<uint32_t>(sizeof(fs_scene_glsl)),
		fs_scene_spv,
		static_cast<uint32_t>(sizeof(fs_scene_spv)),
#if defined(_WIN32)
		fs_scene_dx11,
		static_cast<uint32_t>(sizeof(fs_scene_dx11)),
#endif
	},
};

bgfx::ShaderHandle Create_Embedded_Shader(const EmbeddedShaderBinary &shader)
{
	const uint8_t *data = shader.glsl_data;
	uint32_t size = shader.glsl_size;

	switch (bgfx::getRendererType()) {
	case bgfx::RendererType::Vulkan:
		data = shader.spirv_data;
		size = shader.spirv_size;
		break;
#if defined(_WIN32)
	case bgfx::RendererType::Direct3D11:
	case bgfx::RendererType::Direct3D12:
		data = shader.dx11_data;
		size = shader.dx11_size;
		break;
#endif
	default:
		break;
	}

	return bgfx::createShader(bgfx::copy(data, size));
}

bgfx::ProgramHandle Create_Embedded_Program(BgfxProgramType program)
{
	const bgfx::ShaderHandle vertex_shader = Create_Embedded_Shader(kVertexShaders[program]);
	const bgfx::ShaderHandle fragment_shader = Create_Embedded_Shader(kFragmentShaders[program]);
	return bgfx::createProgram(vertex_shader, fragment_shader, true);
}

void Reset_View_Sequence()
{
	g_bgfx.current_view_id = kFirstSceneViewId;
	g_bgfx.next_view_id = kFirstSceneViewId;
	g_bgfx.scene_active = false;
}

bgfx::ViewId Allocate_Scene_View()
{
	if (g_bgfx.next_view_id > kMaxSceneViewId) {
		WWPerfMonClass::Record_Scene_View_Flush();
		bgfx::frame(BGFX_FRAME_FLUSH);
		Reset_View_Sequence();
	}

	// bgfx framebuffer/viewport state is view-level, so each legacy Begin/End_Scene
	// pair needs its own ordered view to keep offscreen render-target passes in the
	// same bgfx frame without forcing a flush between them.
	const bgfx::ViewId view_id = g_bgfx.next_view_id++;
	g_bgfx.current_view_id = view_id;
	g_bgfx.scene_active = true;
	WWPerfMonClass::Record_Scene_View_Allocation();
	return view_id;
}

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

Vector3 Transform_Normal_To_Camera_Space(const Matrix4 &world_view, const Vector3 &normal);
Vector3 Normalize_Vector(const Vector3 &value);

Vector3 Clamp_Vector(const Vector3 &value)
{
	return Vector3(
		std::min(std::max(value.X, 0.0f), 1.0f),
		std::min(std::max(value.Y, 0.0f), 1.0f),
		std::min(std::max(value.Z, 0.0f), 1.0f));
}

Vector3 Multiply_Vector(const Vector3 &a, const Vector3 &b)
{
	return Vector3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
}

Vector3 Scale_Vector(const Vector3 &value, float scale)
{
	return Vector3(value.X * scale, value.Y * scale, value.Z * scale);
}

void Add_Clamped(Vector3 &accumulator, const Vector3 &value)
{
	accumulator = Clamp_Vector(accumulator + value);
}

Vector3 Color_To_Vector3(uint32_t color)
{
	const Vector4 converted = DX8Wrapper::Convert_Color(color);
	return Vector3(converted.X, converted.Y, converted.Z);
}

Vector3 Resolve_Color_Source(unsigned source, const Vector3 &material_color, uint32_t vertex_diffuse, bool has_diffuse)
{
	if (source == D3DMCS_COLOR1 && has_diffuse) {
		return Color_To_Vector3(vertex_diffuse);
	}
	return material_color;
}

Vector3 View_Rotate_Vector(const Vector3 &value)
{
	return Normalize_Vector(Transform_Normal_To_Camera_Space(g_bgfx.view, value));
}

float Compute_Attenuation(const BgfxLightState &light, const Vector3 &world_position)
{
	if (light.type == LightClass::DIRECTIONAL) {
		return 1.0f;
	}

	Vector3 to_light = light.position - world_position;
	const float distance = to_light.Length();
	const float range = light.far_atten_end - light.far_atten_start;
	float attenuation = 1.0f;
	if (range > 1.0e-6f) {
		attenuation = 1.0f - (distance - light.far_atten_start) / range;
		attenuation = std::min(std::max(attenuation, 0.0f), 1.0f);
	}

	if (light.type == LightClass::SPOT && attenuation > 0.0f) {
		const Vector3 spot_dir = Normalize_Vector(light.direction);
		const Vector3 to_object = Normalize_Vector(world_position - light.position);
		const float cone = Vector3::Dot_Product(spot_dir, to_object);
		if (cone <= light.spot_angle_cos) {
			return 0.0f;
		}
		const float denom = std::max(1.0f - light.spot_angle_cos, 1.0e-6f);
		attenuation *= std::min(std::max((cone - light.spot_angle_cos) / denom, 0.0f), 1.0f);
	}

	return attenuation;
}

struct BgfxDrawLightingState
{
	bool material_present = false;
	bool lighting_enabled = false;
	bool use_light_environment = false;
	bool ambient_uses_vertex_diffuse = false;
	bool diffuse_uses_vertex_diffuse = false;
	bool emissive_uses_vertex_diffuse = false;
	float opacity = 1.0f;
	Vector3 material_ambient = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 material_diffuse = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 material_emissive = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 render_ambient = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 light_environment_ambient = Vector3(0.0f, 0.0f, 0.0f);
	unsigned light_environment_count = 0;
	Vector3 light_environment_camera_directions[4];
	Vector3 light_environment_diffuse[4];
};

BgfxDrawLightingState Build_Draw_Lighting_State(bool has_normal)
{
	BgfxDrawLightingState state;
	state.material_present = g_bgfx.material != nullptr;
	state.lighting_enabled = state.material_present && g_bgfx.render_states[D3DRS_LIGHTING] != 0 && has_normal;
	state.use_light_environment = state.lighting_enabled && g_bgfx.light_environment.enabled;

	if (!state.material_present) {
		return state;
	}

	g_bgfx.material->Get_Ambient(&state.material_ambient);
	g_bgfx.material->Get_Diffuse(&state.material_diffuse);
	g_bgfx.material->Get_Emissive(&state.material_emissive);
	state.opacity = g_bgfx.material->Get_Opacity();
	state.ambient_uses_vertex_diffuse = g_bgfx.render_states[D3DRS_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1;
	state.diffuse_uses_vertex_diffuse = g_bgfx.render_states[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1;
	state.emissive_uses_vertex_diffuse = g_bgfx.render_states[D3DRS_EMISSIVEMATERIALSOURCE] == D3DMCS_COLOR1;
	state.render_ambient = Color_To_Vector3(g_bgfx.render_states[D3DRS_AMBIENT]);

	if (state.use_light_environment) {
		state.light_environment_ambient = g_bgfx.light_environment.ambient;
		state.light_environment_count = g_bgfx.light_environment.count;
		for (unsigned light_index = 0; light_index < state.light_environment_count; ++light_index) {
			state.light_environment_camera_directions[light_index] = View_Rotate_Vector(g_bgfx.light_environment.directions[light_index]);
			state.light_environment_diffuse[light_index] = g_bgfx.light_environment.diffuse[light_index];
		}
	}

	return state;
}

Vector3 Compute_Lit_Color(
	uint32_t vertex_diffuse,
	bool has_diffuse,
	bool has_normal,
	unsigned normal_offset,
	const uint8_t *src,
	const float *position,
	const Matrix4 &world_view)
{
	if (g_bgfx.material == nullptr) {
		return Color_To_Vector3(has_diffuse ? vertex_diffuse : 0xFFFFFFFFU);
	}

	Vector3 ambient_color(0.0f, 0.0f, 0.0f);
	Vector3 diffuse_color(1.0f, 1.0f, 1.0f);
	Vector3 emissive_color(0.0f, 0.0f, 0.0f);
	g_bgfx.material->Get_Ambient(&ambient_color);
	g_bgfx.material->Get_Diffuse(&diffuse_color);
	g_bgfx.material->Get_Emissive(&emissive_color);

	const Vector3 resolved_ambient = Resolve_Color_Source(g_bgfx.render_states[D3DRS_AMBIENTMATERIALSOURCE], ambient_color, vertex_diffuse, has_diffuse);
	const Vector3 resolved_diffuse = Resolve_Color_Source(g_bgfx.render_states[D3DRS_DIFFUSEMATERIALSOURCE], diffuse_color, vertex_diffuse, has_diffuse);
	const Vector3 resolved_emissive = Resolve_Color_Source(g_bgfx.render_states[D3DRS_EMISSIVEMATERIALSOURCE], emissive_color, vertex_diffuse, has_diffuse);
	if (g_bgfx.render_states[D3DRS_LIGHTING] == 0 || !has_normal) {
		return Clamp_Vector(resolved_diffuse + resolved_emissive);
	}

	const float *normal_data = reinterpret_cast<const float *>(src + normal_offset);
	const Vector3 normal = Normalize_Vector(Transform_Normal_To_Camera_Space(world_view, Vector3(normal_data[0], normal_data[1], normal_data[2])));
	Vector4 world_position4;
	Matrix4::Transform_Vector(g_bgfx.world, Vector3(position[0], position[1], position[2]), &world_position4);
	const Vector3 world_position(world_position4.X, world_position4.Y, world_position4.Z);

	Vector3 lit = resolved_emissive;
	Add_Clamped(lit, Multiply_Vector(resolved_ambient, Color_To_Vector3(g_bgfx.render_states[D3DRS_AMBIENT])));

	if (g_bgfx.light_environment.enabled) {
		Add_Clamped(lit, Multiply_Vector(resolved_ambient, g_bgfx.light_environment.ambient));
		for (unsigned light_index = 0; light_index < g_bgfx.light_environment.count; ++light_index) {
			const Vector3 light_dir = View_Rotate_Vector(g_bgfx.light_environment.directions[light_index]);
			const float ndotl = std::max(Vector3::Dot_Product(normal, light_dir), 0.0f);
			Add_Clamped(lit, Multiply_Vector(resolved_diffuse, Scale_Vector(g_bgfx.light_environment.diffuse[light_index], ndotl)));
		}
		return lit;
	}

	for (const BgfxLightState &light : g_bgfx.lights) {
		if (!light.enabled) {
			continue;
		}

		const float attenuation = Compute_Attenuation(light, world_position);
		if (attenuation <= 0.0f) {
			continue;
		}

		Vector3 light_direction = light.direction;
		if (light.type != LightClass::DIRECTIONAL) {
			light_direction = light.position - world_position;
		}
		light_direction = View_Rotate_Vector(light_direction);
		const float ndotl = std::max(Vector3::Dot_Product(normal, light_direction), 0.0f);
		Add_Clamped(lit, Multiply_Vector(resolved_ambient, Scale_Vector(light.ambient, attenuation)));
		Add_Clamped(lit, Multiply_Vector(resolved_diffuse, Scale_Vector(light.diffuse, attenuation * ndotl)));
	}

	return lit;
}

void Reset_Draw_State()
{
	g_bgfx.vertex_data = nullptr;
	g_bgfx.vertex_fvf = nullptr;
	g_bgfx.vertex_count = 0;
	g_bgfx.index_data = nullptr;
	g_bgfx.index_count = 0;
	g_bgfx.index_base_offset = 0;
	REF_PTR_RELEASE(g_bgfx.textures[0]);
	REF_PTR_RELEASE(g_bgfx.textures[1]);
	g_bgfx.material = nullptr;
	g_bgfx.shader = ShaderClass();
	g_bgfx.current_vb = nullptr;
	g_bgfx.current_ib = nullptr;
	g_bgfx.current_vb_type = BUFFER_TYPE_INVALID;
	g_bgfx.current_ib_type = BUFFER_TYPE_INVALID;
	g_bgfx.current_vba_offset = 0;
	g_bgfx.current_iba_offset = 0;
	g_bgfx.compatibility_vertex_offset = 0;
	g_bgfx.compatibility_index_offset = 0;
	REF_PTR_RELEASE(g_bgfx.render_target);
	g_bgfx.light_environment = BgfxLightEnvironmentState();
	for (BgfxLightState &light : g_bgfx.lights) {
		light = BgfxLightState();
	}
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

void Reset_Compatibility_Submit_Scratch()
{
	g_bgfx.compatibility_vertex_offset = 0;
	g_bgfx.compatibility_index_offset = 0;
}

bool Ensure_Compatibility_Submit_Scratch(uint32_t required_vertices, uint32_t required_indices)
{
	uint32_t desired_vertex_capacity = kCompatibilityScratchVertexCapacity;
	if (desired_vertex_capacity < required_vertices) {
		desired_vertex_capacity = required_vertices;
	}

	uint32_t desired_index_capacity = kCompatibilityScratchIndexCapacity;
	if (desired_index_capacity < required_indices) {
		desired_index_capacity = required_indices;
	}

	if (!bgfx::isValid(g_bgfx.compatibility_vertex_buffer) || g_bgfx.compatibility_vertex_capacity < desired_vertex_capacity) {
		if (bgfx::isValid(g_bgfx.compatibility_vertex_buffer) && g_bgfx.compatibility_vertex_offset != 0) {
			return false;
		}
		if (bgfx::isValid(g_bgfx.compatibility_vertex_buffer)) {
			bgfx::destroy(g_bgfx.compatibility_vertex_buffer);
		}
		g_bgfx.compatibility_vertex_buffer = bgfx::createDynamicVertexBuffer(desired_vertex_capacity, g_bgfx.gui_layout);
		g_bgfx.compatibility_vertex_capacity = bgfx::isValid(g_bgfx.compatibility_vertex_buffer) ? desired_vertex_capacity : 0;
		g_bgfx.compatibility_vertex_offset = 0;
	}

	if (!bgfx::isValid(g_bgfx.compatibility_index_buffer) || g_bgfx.compatibility_index_capacity < desired_index_capacity) {
		if (bgfx::isValid(g_bgfx.compatibility_index_buffer) && g_bgfx.compatibility_index_offset != 0) {
			return false;
		}
		if (bgfx::isValid(g_bgfx.compatibility_index_buffer)) {
			bgfx::destroy(g_bgfx.compatibility_index_buffer);
		}
		g_bgfx.compatibility_index_buffer = bgfx::createDynamicIndexBuffer(desired_index_capacity);
		g_bgfx.compatibility_index_capacity = bgfx::isValid(g_bgfx.compatibility_index_buffer) ? desired_index_capacity : 0;
		g_bgfx.compatibility_index_offset = 0;
	}

	return bgfx::isValid(g_bgfx.compatibility_vertex_buffer) && bgfx::isValid(g_bgfx.compatibility_index_buffer);
}

bool Reserve_Compatibility_Submit_Scratch(uint32_t vertex_count, uint32_t index_count, uint32_t &vertex_offset, uint32_t &index_offset)
{
	if (!Ensure_Compatibility_Submit_Scratch(vertex_count, index_count)) {
		return false;
	}

	if (g_bgfx.compatibility_vertex_offset + vertex_count > g_bgfx.compatibility_vertex_capacity ||
		g_bgfx.compatibility_index_offset + index_count > g_bgfx.compatibility_index_capacity) {
		WWRELEASE_SAY((
			"BGFX: compatibility scratch buffer exhausted (verts %u+%u/%u, indices %u+%u/%u)\n",
			g_bgfx.compatibility_vertex_offset,
			vertex_count,
			g_bgfx.compatibility_vertex_capacity,
			g_bgfx.compatibility_index_offset,
			index_count,
			g_bgfx.compatibility_index_capacity));
		return false;
	}

	vertex_offset = g_bgfx.compatibility_vertex_offset;
	index_offset = g_bgfx.compatibility_index_offset;
	g_bgfx.compatibility_vertex_offset += vertex_count;
	g_bgfx.compatibility_index_offset += index_count;
	return true;
}

void Bind_Current_Vertex_Buffer_Slice(unsigned buffer_offset)
{
	g_bgfx.current_vba_offset = buffer_offset;
	if (g_bgfx.current_vb == nullptr) {
		g_bgfx.vertex_data = nullptr;
		g_bgfx.vertex_fvf = nullptr;
		g_bgfx.vertex_count = 0;
		return;
	}

	g_bgfx.vertex_fvf = &g_bgfx.current_vb->FVF_Info();
	const uint16_t total_vertex_count = g_bgfx.current_vb->Get_Vertex_Count();
	if (buffer_offset > total_vertex_count) {
		WWRELEASE_SAY(("BGFX: vertex buffer offset out of bounds (%u > %u)\n", buffer_offset, total_vertex_count));
		g_bgfx.vertex_data = nullptr;
		g_bgfx.vertex_count = 0;
		return;
	}

	g_bgfx.vertex_data = g_bgfx.current_vb->Get_Vertex_Data() + static_cast<size_t>(buffer_offset) * static_cast<size_t>(g_bgfx.vertex_fvf->Get_FVF_Size());
	g_bgfx.vertex_count = static_cast<uint16_t>(total_vertex_count - buffer_offset);
}

void Bind_Current_Index_Buffer_Slice(unsigned buffer_offset)
{
	g_bgfx.current_iba_offset = buffer_offset;
	if (g_bgfx.current_ib == nullptr) {
		g_bgfx.index_data = nullptr;
		g_bgfx.index_count = 0;
		return;
	}

	const uint16_t total_index_count = g_bgfx.current_ib->Get_Index_Count();
	if (buffer_offset > total_index_count) {
		WWRELEASE_SAY(("BGFX: index buffer offset out of bounds (%u > %u)\n", buffer_offset, total_index_count));
		g_bgfx.index_data = nullptr;
		g_bgfx.index_count = 0;
		return;
	}

	g_bgfx.index_data = g_bgfx.current_ib->Get_Index_Data() + buffer_offset;
	g_bgfx.index_count = static_cast<uint16_t>(total_index_count - buffer_offset);
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
	const uint8_t *src,
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

uint32_t Resolve_Diffuse_Color(
	uint32_t vertex_diffuse,
	bool has_diffuse,
	bool has_normal,
	unsigned normal_offset,
	const uint8_t *src,
	const float *position,
	const Matrix4 &world_view)
{
	if (g_bgfx.material == nullptr) {
		return has_diffuse ? vertex_diffuse : 0xFFFFFFFFU;
	}

	float alpha = g_bgfx.material->Get_Opacity();
	if (g_bgfx.render_states[D3DRS_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1 && has_diffuse) {
		alpha = DX8Wrapper::Convert_Color(vertex_diffuse).W;
	}

	const Vector3 resolved = Compute_Lit_Color(vertex_diffuse, has_diffuse, has_normal, normal_offset, src, position, world_view);
	return DX8Wrapper::Convert_Color(Vector4(resolved.X, resolved.Y, resolved.Z, alpha));
}

uint32_t Resolve_Unlit_Diffuse_Color(
	const BgfxDrawLightingState &state,
	uint32_t vertex_diffuse,
	bool has_diffuse)
{
	if (!state.material_present) {
		return has_diffuse ? vertex_diffuse : 0xFFFFFFFFU;
	}

	float alpha = state.opacity;
	if (state.diffuse_uses_vertex_diffuse && has_diffuse) {
		alpha = DX8Wrapper::Convert_Color(vertex_diffuse).W;
	}

	const Vector3 resolved_diffuse = Resolve_Color_Source(
		state.diffuse_uses_vertex_diffuse ? D3DMCS_COLOR1 : D3DMCS_MATERIAL,
		state.material_diffuse,
		vertex_diffuse,
		has_diffuse);
	const Vector3 resolved_emissive = Resolve_Color_Source(
		state.emissive_uses_vertex_diffuse ? D3DMCS_COLOR1 : D3DMCS_MATERIAL,
		state.material_emissive,
		vertex_diffuse,
		has_diffuse);
	const Vector3 resolved = Clamp_Vector(resolved_diffuse + resolved_emissive);
	return DX8Wrapper::Convert_Color(Vector4(resolved.X, resolved.Y, resolved.Z, alpha));
}

float Get_Draw_Fog_Mode()
{
	return (DX8Wrapper::Get_Current_Caps()->Is_Fog_Allowed() && DX8Wrapper::Get_Fog_Enable())
		? static_cast<float>(g_bgfx.shader.Get_Fog_Func())
		: static_cast<float>(ShaderClass::FOG_DISABLE);
}

float Encode_Light_Type(LightClass::LightType type)
{
	switch (type) {
	case LightClass::DIRECTIONAL:
		return 0.0f;
	case LightClass::SPOT:
		return 2.0f;
	case LightClass::POINT:
	default:
		return 1.0f;
	}
}

Vector3 Transform_Position_To_Camera_Space(const Matrix4 &view, const Vector3 &position)
{
	Vector4 camera_position;
	Matrix4::Transform_Vector(view, position, &camera_position);
	return Vector3(camera_position.X, camera_position.Y, camera_position.Z);
}

BgfxProgramSelection Select_BGFX_Program(const BgfxDrawLightingState &draw_lighting_state, bool fog_enabled, bool use_gpu_texgen)
{
	BgfxProgramSelection selection;
	selection.fog_enabled = fog_enabled;
	selection.use_gpu_texgen = use_gpu_texgen;

	if (draw_lighting_state.use_light_environment) {
		selection.program = BGFX_PROGRAM_LIGHT_ENVIRONMENT;
	} else if (draw_lighting_state.lighting_enabled) {
		selection.program = BGFX_PROGRAM_LIGHT_DYNAMIC;
	} else if (draw_lighting_state.material_present || fog_enabled || use_gpu_texgen) {
		selection.program = BGFX_PROGRAM_UNLIT;
	} else {
		selection.program = BGFX_PROGRAM_BASIC;
	}

	return selection;
}

void Set_Color_Adjust_Uniform()
{
	const float gamma_power = 1.0f / std::max(g_bgfx.gamma, 1.0e-4f);
	const float color_adjust[4] = { gamma_power, g_bgfx.brightness, g_bgfx.contrast, 0.0f };
	bgfx::setUniform(g_bgfx.color_adjust_uniform, color_adjust);
}

void Set_Fog_Uniforms(float fog_mode)
{
	const float fog_start = DX8Wrapper::Get_Fog_Start();
	const float fog_end = DX8Wrapper::Get_Fog_End();
	const float fog_inverse_range = fog_end > fog_start ? 1.0f / (fog_end - fog_start) : 0.0f;
	const float fog_state[4] = { fog_mode, fog_start, fog_inverse_range, fog_end };
	const Vector3 fog_color_value = DX8Wrapper::Get_Fog_Color_Vector();
	const float fog_color[4] = { fog_color_value.X, fog_color_value.Y, fog_color_value.Z, 1.0f };
	bgfx::setUniform(g_bgfx.fog_state_uniform, fog_state);
	if (fog_mode != static_cast<float>(ShaderClass::FOG_DISABLE)) {
		bgfx::setUniform(g_bgfx.fog_color_uniform, fog_color);
	}
}

void Set_Material_Uniforms(const BgfxDrawLightingState &draw_lighting_state, bool has_diffuse)
{
	Vector3 material_ambient(1.0f, 1.0f, 1.0f);
	Vector3 material_diffuse(1.0f, 1.0f, 1.0f);
	Vector3 material_emissive(0.0f, 0.0f, 0.0f);
	float opacity = 1.0f;
	float material_source[4] = { 0.0f, has_diffuse ? 1.0f : 0.0f, 0.0f, 0.0f };

	if (draw_lighting_state.material_present) {
		material_ambient = draw_lighting_state.material_ambient;
		material_diffuse = draw_lighting_state.material_diffuse;
		material_emissive = draw_lighting_state.material_emissive;
		opacity = draw_lighting_state.opacity;
		material_source[0] = draw_lighting_state.ambient_uses_vertex_diffuse && has_diffuse ? 1.0f : 0.0f;
		material_source[1] = draw_lighting_state.diffuse_uses_vertex_diffuse && has_diffuse ? 1.0f : 0.0f;
		material_source[2] = draw_lighting_state.emissive_uses_vertex_diffuse && has_diffuse ? 1.0f : 0.0f;
	}

	const float material_state[4] = { opacity, 0.0f, 0.0f, 0.0f };
	const float material_ambient_uniform[4] = { material_ambient.X, material_ambient.Y, material_ambient.Z, 1.0f };
	const float material_diffuse_uniform[4] = { material_diffuse.X, material_diffuse.Y, material_diffuse.Z, 1.0f };
	const float material_emissive_uniform[4] = { material_emissive.X, material_emissive.Y, material_emissive.Z, 1.0f };

	bgfx::setUniform(g_bgfx.material_source_uniform, material_source);
	bgfx::setUniform(g_bgfx.material_ambient_uniform, material_ambient_uniform);
	bgfx::setUniform(g_bgfx.material_diffuse_uniform, material_diffuse_uniform);
	bgfx::setUniform(g_bgfx.material_emissive_uniform, material_emissive_uniform);
	bgfx::setUniform(g_bgfx.material_state_uniform, material_state);
}

void Set_Render_Ambient_Uniform(const Vector3 &render_ambient)
{
	const float render_ambient_uniform[4] = { render_ambient.X, render_ambient.Y, render_ambient.Z, 1.0f };
	bgfx::setUniform(g_bgfx.render_ambient_uniform, render_ambient_uniform);
}

void Set_World_View_Row_Uniform(const Matrix4 &world_view)
{
	const float world_view_rows[3][4] = {
		{ world_view[0][0], world_view[0][1], world_view[0][2], world_view[0][3] },
		{ world_view[1][0], world_view[1][1], world_view[1][2], world_view[1][3] },
		{ world_view[2][0], world_view[2][1], world_view[2][2], world_view[2][3] }
	};
	bgfx::setUniform(g_bgfx.world_view_row_uniform, world_view_rows, 3);
}

void Set_Texgen_Uniforms(
	bool has_normal,
	unsigned texcoord_generation,
	bool apply_texture_transform,
	bool projected_texture,
	const Matrix4 &texture_transform)
{
	float texgen_mode = 0.0f;
	if (texcoord_generation == D3DTSS_TCI_CAMERASPACEPOSITION) {
		texgen_mode = 1.0f;
	} else if (texcoord_generation == D3DTSS_TCI_CAMERASPACENORMAL) {
		texgen_mode = 2.0f;
	} else if (texcoord_generation == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR) {
		texgen_mode = 3.0f;
	}

	const float texgen_state[4] = {
		texgen_mode,
		apply_texture_transform ? 1.0f : 0.0f,
		projected_texture ? 1.0f : 0.0f,
		has_normal ? 1.0f : 0.0f
	};
	bgfx::setUniform(g_bgfx.texgen_state_uniform, texgen_state);

	if (apply_texture_transform) {
		const float texture_transform_rows[4][4] = {
			{ texture_transform[0][0], texture_transform[0][1], texture_transform[0][2], texture_transform[0][3] },
			{ texture_transform[1][0], texture_transform[1][1], texture_transform[1][2], texture_transform[1][3] },
			{ texture_transform[2][0], texture_transform[2][1], texture_transform[2][2], texture_transform[2][3] },
			{ texture_transform[3][0], texture_transform[3][1], texture_transform[3][2], texture_transform[3][3] }
		};
		bgfx::setUniform(g_bgfx.texture_transform_row_uniform, texture_transform_rows, 4);
	}
}

void Set_Light_Environment_Uniforms(const BgfxDrawLightingState &draw_lighting_state)
{
	const float light_environment_state[4] = {
		static_cast<float>(draw_lighting_state.light_environment_count),
		0.0f,
		0.0f,
		0.0f
	};
	const float light_environment_ambient[4] = {
		draw_lighting_state.light_environment_ambient.X,
		draw_lighting_state.light_environment_ambient.Y,
		draw_lighting_state.light_environment_ambient.Z,
		1.0f
	};
	float light_environment_directions[4][4] = { { 0.0f } };
	float light_environment_diffuse[4][4] = { { 0.0f } };

	for (unsigned light_index = 0; light_index < draw_lighting_state.light_environment_count; ++light_index) {
		light_environment_directions[light_index][0] = draw_lighting_state.light_environment_camera_directions[light_index].X;
		light_environment_directions[light_index][1] = draw_lighting_state.light_environment_camera_directions[light_index].Y;
		light_environment_directions[light_index][2] = draw_lighting_state.light_environment_camera_directions[light_index].Z;
		light_environment_diffuse[light_index][0] = draw_lighting_state.light_environment_diffuse[light_index].X;
		light_environment_diffuse[light_index][1] = draw_lighting_state.light_environment_diffuse[light_index].Y;
		light_environment_diffuse[light_index][2] = draw_lighting_state.light_environment_diffuse[light_index].Z;
	}

	bgfx::setUniform(g_bgfx.light_environment_state_uniform, light_environment_state);
	bgfx::setUniform(g_bgfx.light_environment_ambient_uniform, light_environment_ambient);
	bgfx::setUniform(g_bgfx.light_environment_direction_uniform, light_environment_directions, 4);
	bgfx::setUniform(g_bgfx.light_environment_diffuse_uniform, light_environment_diffuse, 4);
}

void Set_Direct_Light_Uniforms()
{
	float direct_light_state[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float direct_light_position[4][4] = { { 0.0f } };
	float direct_light_direction[4][4] = { { 0.0f } };
	float direct_light_ambient[4][4] = { { 0.0f } };
	float direct_light_diffuse[4][4] = { { 0.0f } };

	unsigned light_count = 0;
	for (const BgfxLightState &light : g_bgfx.lights) {
		if (!light.enabled || light_count >= 4) {
			continue;
		}

		const Vector3 camera_position = Transform_Position_To_Camera_Space(g_bgfx.view, light.position);
		const Vector3 camera_direction = View_Rotate_Vector(light.direction);

		direct_light_position[light_count][0] = camera_position.X;
		direct_light_position[light_count][1] = camera_position.Y;
		direct_light_position[light_count][2] = camera_position.Z;
		direct_light_position[light_count][3] = light.far_atten_start;

		direct_light_direction[light_count][0] = camera_direction.X;
		direct_light_direction[light_count][1] = camera_direction.Y;
		direct_light_direction[light_count][2] = camera_direction.Z;
		direct_light_direction[light_count][3] = light.far_atten_end;

		direct_light_ambient[light_count][0] = light.ambient.X;
		direct_light_ambient[light_count][1] = light.ambient.Y;
		direct_light_ambient[light_count][2] = light.ambient.Z;
		direct_light_ambient[light_count][3] = Encode_Light_Type(light.type);

		direct_light_diffuse[light_count][0] = light.diffuse.X;
		direct_light_diffuse[light_count][1] = light.diffuse.Y;
		direct_light_diffuse[light_count][2] = light.diffuse.Z;
		direct_light_diffuse[light_count][3] = light.spot_angle_cos;
		++light_count;
	}

	direct_light_state[0] = static_cast<float>(light_count);
	bgfx::setUniform(g_bgfx.direct_light_state_uniform, direct_light_state);
	bgfx::setUniform(g_bgfx.direct_light_position_uniform, direct_light_position, 4);
	bgfx::setUniform(g_bgfx.direct_light_direction_uniform, direct_light_direction, 4);
	bgfx::setUniform(g_bgfx.direct_light_ambient_uniform, direct_light_ambient, 4);
	bgfx::setUniform(g_bgfx.direct_light_diffuse_uniform, direct_light_diffuse, 4);
}

void Set_Gui_Draw_Uniforms(
	const BgfxProgramSelection &selection,
	const BgfxDrawLightingState &draw_lighting_state,
	bool has_diffuse,
	bool has_normal,
	unsigned texcoord_generation,
	bool apply_texture_transform,
	bool projected_texture,
	const Matrix4 &world_view,
	const Matrix4 &texture_transform)
{
	Set_Color_Adjust_Uniform();

	if (selection.program == BGFX_PROGRAM_BASIC) {
		return;
	}

	const float fog_mode = selection.fog_enabled
		? Get_Draw_Fog_Mode()
		: static_cast<float>(ShaderClass::FOG_DISABLE);
	Set_Fog_Uniforms(fog_mode);
	Set_Material_Uniforms(draw_lighting_state, has_diffuse);

	const bool needs_world_view_rows =
		selection.program == BGFX_PROGRAM_LIGHT_ENVIRONMENT ||
		selection.program == BGFX_PROGRAM_LIGHT_DYNAMIC ||
		selection.fog_enabled ||
		(selection.use_gpu_texgen && texcoord_generation != D3DTSS_TCI_PASSTHRU);
	if (needs_world_view_rows) {
		Set_World_View_Row_Uniform(world_view);
	}

	Set_Texgen_Uniforms(
		has_normal,
		selection.use_gpu_texgen ? texcoord_generation : D3DTSS_TCI_PASSTHRU,
		selection.use_gpu_texgen ? apply_texture_transform : false,
		selection.use_gpu_texgen ? projected_texture : false,
		texture_transform);

	if (selection.program == BGFX_PROGRAM_LIGHT_ENVIRONMENT) {
		Set_Render_Ambient_Uniform(draw_lighting_state.render_ambient);
		Set_Light_Environment_Uniforms(draw_lighting_state);
	} else if (selection.program == BGFX_PROGRAM_LIGHT_DYNAMIC) {
		Set_Render_Ambient_Uniform(draw_lighting_state.render_ambient);
		Set_Direct_Light_Uniforms();
	}
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
	unsigned target_width = g_bgfx.viewport.Width;
	unsigned target_height = g_bgfx.viewport.Height;
	if (g_bgfx.render_target != nullptr) {
		target_width = static_cast<unsigned>(std::max(g_bgfx.render_target->Get_Width(), 1));
		target_height = static_cast<unsigned>(std::max(g_bgfx.render_target->Get_Height(), 1));
	}
	const uint16_t x = static_cast<uint16_t>(std::min(g_bgfx.viewport.X, 0xFFFFu));
	const uint16_t y = static_cast<uint16_t>(std::min(g_bgfx.viewport.Y, 0xFFFFu));
	const uint16_t width = static_cast<uint16_t>(std::min(std::max(target_width, 1u), 0xFFFFu));
	const uint16_t height = static_cast<uint16_t>(std::min(std::max(target_height, 1u), 0xFFFFu));
	bgfx::setViewRect(g_bgfx.current_view_id, x, y, width, height);
}

void Apply_View_Target()
{
	bgfx::setViewFrameBuffer(g_bgfx.current_view_id, BgfxCompat_Get_Frame_Buffer(g_bgfx.render_target));
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
	bgfx::setViewClear(g_bgfx.current_view_id, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
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
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
		g_bgfx.layout_ready = true;
	}

	if (!bgfx::isValid(g_bgfx.texture_uniform)) {
		g_bgfx.texture_uniform = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	}
	if (!bgfx::isValid(g_bgfx.fog_state_uniform)) {
		g_bgfx.fog_state_uniform = bgfx::createUniform("u_fogState", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.fog_color_uniform)) {
		g_bgfx.fog_color_uniform = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.color_adjust_uniform)) {
		g_bgfx.color_adjust_uniform = bgfx::createUniform("u_colorAdjust", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.material_source_uniform)) {
		g_bgfx.material_source_uniform = bgfx::createUniform("u_materialSource", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.material_ambient_uniform)) {
		g_bgfx.material_ambient_uniform = bgfx::createUniform("u_materialAmbient", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.material_diffuse_uniform)) {
		g_bgfx.material_diffuse_uniform = bgfx::createUniform("u_materialDiffuse", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.material_emissive_uniform)) {
		g_bgfx.material_emissive_uniform = bgfx::createUniform("u_materialEmissive", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.material_state_uniform)) {
		g_bgfx.material_state_uniform = bgfx::createUniform("u_materialState", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.render_ambient_uniform)) {
		g_bgfx.render_ambient_uniform = bgfx::createUniform("u_renderAmbient", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.light_environment_state_uniform)) {
		g_bgfx.light_environment_state_uniform = bgfx::createUniform("u_lightEnvState", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.light_environment_ambient_uniform)) {
		g_bgfx.light_environment_ambient_uniform = bgfx::createUniform("u_lightEnvAmbient", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.light_environment_direction_uniform)) {
		g_bgfx.light_environment_direction_uniform = bgfx::createUniform("u_lightEnvDir", bgfx::UniformType::Vec4, 4);
	}
	if (!bgfx::isValid(g_bgfx.light_environment_diffuse_uniform)) {
		g_bgfx.light_environment_diffuse_uniform = bgfx::createUniform("u_lightEnvDiffuse", bgfx::UniformType::Vec4, 4);
	}
	if (!bgfx::isValid(g_bgfx.world_view_row_uniform)) {
		g_bgfx.world_view_row_uniform = bgfx::createUniform("u_worldViewRow", bgfx::UniformType::Vec4, 3);
	}
	if (!bgfx::isValid(g_bgfx.texgen_state_uniform)) {
		g_bgfx.texgen_state_uniform = bgfx::createUniform("u_texGenState", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.texture_transform_row_uniform)) {
		g_bgfx.texture_transform_row_uniform = bgfx::createUniform("u_textureTransformRow", bgfx::UniformType::Vec4, 4);
	}
	if (!bgfx::isValid(g_bgfx.direct_light_state_uniform)) {
		g_bgfx.direct_light_state_uniform = bgfx::createUniform("u_directLightState", bgfx::UniformType::Vec4);
	}
	if (!bgfx::isValid(g_bgfx.direct_light_position_uniform)) {
		g_bgfx.direct_light_position_uniform = bgfx::createUniform("u_directLightPosition", bgfx::UniformType::Vec4, 4);
	}
	if (!bgfx::isValid(g_bgfx.direct_light_direction_uniform)) {
		g_bgfx.direct_light_direction_uniform = bgfx::createUniform("u_directLightDirection", bgfx::UniformType::Vec4, 4);
	}
	if (!bgfx::isValid(g_bgfx.direct_light_ambient_uniform)) {
		g_bgfx.direct_light_ambient_uniform = bgfx::createUniform("u_directLightAmbient", bgfx::UniformType::Vec4, 4);
	}
	if (!bgfx::isValid(g_bgfx.direct_light_diffuse_uniform)) {
		g_bgfx.direct_light_diffuse_uniform = bgfx::createUniform("u_directLightDiffuse", bgfx::UniformType::Vec4, 4);
	}

	for (int program_index = 0; program_index < BGFX_PROGRAM_COUNT; ++program_index) {
		if (!bgfx::isValid(g_bgfx.programs[program_index])) {
			g_bgfx.programs[program_index] = Create_Embedded_Program(static_cast<BgfxProgramType>(program_index));
		}
	}

	return bgfx::isValid(g_bgfx.programs[BGFX_PROGRAM_BASIC]) &&
		bgfx::isValid(g_bgfx.programs[BGFX_PROGRAM_UNLIT]) &&
		bgfx::isValid(g_bgfx.programs[BGFX_PROGRAM_LIGHT_ENVIRONMENT]) &&
		bgfx::isValid(g_bgfx.programs[BGFX_PROGRAM_LIGHT_DYNAMIC]) &&
		bgfx::isValid(g_bgfx.texture_uniform) &&
		bgfx::isValid(g_bgfx.fog_state_uniform) &&
		bgfx::isValid(g_bgfx.fog_color_uniform) &&
		bgfx::isValid(g_bgfx.color_adjust_uniform) &&
		bgfx::isValid(g_bgfx.material_source_uniform) &&
		bgfx::isValid(g_bgfx.material_ambient_uniform) &&
		bgfx::isValid(g_bgfx.material_diffuse_uniform) &&
		bgfx::isValid(g_bgfx.material_emissive_uniform) &&
		bgfx::isValid(g_bgfx.material_state_uniform) &&
		bgfx::isValid(g_bgfx.render_ambient_uniform) &&
		bgfx::isValid(g_bgfx.light_environment_state_uniform) &&
		bgfx::isValid(g_bgfx.light_environment_ambient_uniform) &&
		bgfx::isValid(g_bgfx.light_environment_direction_uniform) &&
		bgfx::isValid(g_bgfx.light_environment_diffuse_uniform) &&
		bgfx::isValid(g_bgfx.world_view_row_uniform) &&
		bgfx::isValid(g_bgfx.texgen_state_uniform) &&
		bgfx::isValid(g_bgfx.texture_transform_row_uniform) &&
		bgfx::isValid(g_bgfx.direct_light_state_uniform) &&
		bgfx::isValid(g_bgfx.direct_light_position_uniform) &&
		bgfx::isValid(g_bgfx.direct_light_direction_uniform) &&
		bgfx::isValid(g_bgfx.direct_light_ambient_uniform) &&
		bgfx::isValid(g_bgfx.direct_light_diffuse_uniform);
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

uint64_t Build_BGFX_State(bool triangle_strip)
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

	if (triangle_strip) {
		state |= BGFX_STATE_PT_TRISTRIP;
	}

	state |= BGFX_STATE_MSAA;
	return state;
}

bool Submit_Primitives(uint16_t start_index, uint16_t primitive_count, uint16_t min_vertex_index, uint16_t vertex_count, bool triangle_strip)
{
	const Uint64 submit_start_ticks = WWPerfMonClass::Begin_Scope();
	if (!DX8Wrapper::_Is_Triangle_Draw_Enabled() || !g_bgfx.initialized || primitive_count == 0 || !Ensure_Gui_Resources()) {
		return false;
	}
	if (g_bgfx.vertex_data == nullptr || g_bgfx.index_data == nullptr || g_bgfx.vertex_fvf == nullptr) {
		return false;
	}

	const uint32_t index_total = triangle_strip
		? static_cast<uint32_t>(primitive_count) + 2U
		: static_cast<uint32_t>(primitive_count) * 3U;
	if (vertex_count == 0 || index_total == 0) {
		return false;
	}

	if (static_cast<uint32_t>(start_index) + index_total > g_bgfx.index_count) {
		WWRELEASE_SAY(("BGFX2D: index buffer range out of bounds (start=%u count=%u available=%u)\n", start_index, index_total, g_bgfx.index_count));
		return false;
	}

	const uint32_t base_vertex_index = static_cast<uint32_t>(g_bgfx.index_base_offset);
	const uint32_t requested_min_source = base_vertex_index + static_cast<uint32_t>(min_vertex_index);
	const uint32_t requested_max_source = requested_min_source + static_cast<uint32_t>(vertex_count) - 1U;
	if (requested_max_source >= g_bgfx.vertex_count) {
		WWRELEASE_SAY(("BGFX2D: vertex buffer range out of bounds (min=%u count=%u base=%u available=%u)\n", min_vertex_index, vertex_count, g_bgfx.index_base_offset, g_bgfx.vertex_count));
		return false;
	}

	// The mesh renderer already precomputes the exact min/range for each polygon renderer.
	// Trusting that range avoids re-scanning every draw's indices on the submit hot path.
	const uint32_t draw_min_source = requested_min_source;
	const uint32_t draw_max_source = requested_max_source;
	const uint32_t source_vertex_count = draw_max_source - draw_min_source + 1U;
	uint32_t scratch_vertex_offset = 0;
	uint32_t scratch_index_offset = 0;
	if (!Reserve_Compatibility_Submit_Scratch(source_vertex_count, index_total, scratch_vertex_offset, scratch_index_offset)) {
		return false;
	}

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
	const BgfxDrawLightingState draw_lighting_state = Build_Draw_Lighting_State(has_normal);
	const bool gpu_texcoord_generation_supported =
		texcoord_generation == D3DTSS_TCI_PASSTHRU ||
		texcoord_generation == D3DTSS_TCI_CAMERASPACEPOSITION ||
		texcoord_generation == D3DTSS_TCI_CAMERASPACENORMAL ||
		texcoord_generation == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR;
	const bool needs_cpu_texcoords = !gpu_texcoord_generation_supported;
	const float fog_mode = Get_Draw_Fog_Mode();
	const bool fog_enabled = fog_mode != static_cast<float>(ShaderClass::FOG_DISABLE);
	const bool use_gpu_texgen = !needs_cpu_texcoords && (texcoord_generation != D3DTSS_TCI_PASSTHRU || apply_texture_transform);
	const BgfxProgramSelection program_selection = Select_BGFX_Program(draw_lighting_state, fog_enabled, use_gpu_texgen);
	if (!needs_cpu_texcoords) {
		WWPerfMonClass::Record_Fast_Submit();
	} else {
		WWPerfMonClass::Record_Slow_Submit();
		WWPerfMonClass::Record_Slow_Submit_Reasons(false, false, needs_cpu_texcoords);
	}

	static std::vector<BgfxGuiVertex> converted_vertices;
	if (converted_vertices.size() < source_vertex_count) {
		converted_vertices.resize(source_vertex_count);
	}
	BgfxGuiVertex *dst_vertices = converted_vertices.data();
	for (uint32_t i = 0; i < source_vertex_count; ++i) {
		const uint8_t *src = g_bgfx.vertex_data + static_cast<size_t>(draw_min_source + i) * vertex_stride;
		const float *position = reinterpret_cast<const float *>(src + location_offset);
		const uint32_t diffuse = has_diffuse ? *reinterpret_cast<const uint32_t *>(src + diffuse_offset) : 0xFFFFFFFFU;
		dst_vertices[i].x = position[0];
		dst_vertices[i].y = position[1];
		dst_vertices[i].z = position[2];
		if (has_normal) {
			const float *normal = reinterpret_cast<const float *>(src + normal_offset);
			dst_vertices[i].nx = normal[0];
			dst_vertices[i].ny = normal[1];
			dst_vertices[i].nz = normal[2];
		} else {
			dst_vertices[i].nx = 0.0f;
			dst_vertices[i].ny = 0.0f;
			dst_vertices[i].nz = 1.0f;
		}
		dst_vertices[i].abgr = DX8_To_BGFX_Color(diffuse);
		if (has_selected_texcoord) {
			const float *uv = reinterpret_cast<const float *>(src + tex_offset);
			dst_vertices[i].u = uv[0];
			dst_vertices[i].v = uv[1];
		} else {
			dst_vertices[i].u = 0.0f;
			dst_vertices[i].v = 0.0f;
		}

		if (needs_cpu_texcoords) {
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
	}

	static std::vector<uint16_t> converted_indices;
	if (converted_indices.size() < index_total) {
		converted_indices.resize(index_total);
	}
	uint16_t *dst_indices = converted_indices.data();
	for (uint32_t i = 0; i < index_total; ++i) {
		const uint32_t source_index = static_cast<uint32_t>(g_bgfx.index_data[start_index + i]) + base_vertex_index;
		if (source_index < draw_min_source || source_index > draw_max_source) {
			WWRELEASE_SAY(("BGFX2D: source index outside declared range (index=%u range=%u..%u)\n", source_index, draw_min_source, draw_max_source));
			return false;
		}
		dst_indices[i] = static_cast<uint16_t>(source_index - draw_min_source);
	}

	bgfx::update(
		g_bgfx.compatibility_vertex_buffer,
		scratch_vertex_offset,
		bgfx::copy(dst_vertices, source_vertex_count * sizeof(BgfxGuiVertex)));
	bgfx::update(
		g_bgfx.compatibility_index_buffer,
		scratch_index_offset,
		bgfx::copy(dst_indices, index_total * sizeof(uint16_t)));

	// bgfx::setViewTransform is view-level (shared by ALL draw calls in a view), not
	// per-draw-call.  Since the engine changes view/projection per draw call (e.g. 3D
	// backdrop vs 2D UI) but all draw calls in the current scene view share that state, the last
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
	Set_Gui_Draw_Uniforms(
		program_selection,
		draw_lighting_state,
		has_diffuse,
		has_normal,
		texcoord_generation,
		apply_texture_transform,
		projected_texture,
		world_view,
		texture_transform);
	bgfx::setVertexBuffer(0, g_bgfx.compatibility_vertex_buffer, scratch_vertex_offset, source_vertex_count);
	bgfx::setIndexBuffer(g_bgfx.compatibility_index_buffer, scratch_index_offset, index_total);
	bgfx::setState(Build_BGFX_State(triangle_strip));
	bgfx::submit(g_bgfx.current_view_id, g_bgfx.programs[program_selection.program]);

	++g_bgfx.draw_calls;
	WWPerfMonClass::Record_Draw_Call();
	WWPerfMonClass::Record_Submitted_Vertex_Count(source_vertex_count);
	WWPerfMonClass::Record_Submitted_Index_Count(index_total);
	WWPerfMonClass::End_Scope(WWPERF_SECTION_SUBMIT_TRIANGLES, submit_start_ticks);
	return true;
}

void Copy_Surface_Rectangles(
	IDirect3DSurface8 *source_surface,
	const RECT *source_rects,
	uint32_t rect_count,
	IDirect3DSurface8 *destination_surface,
	const POINT *dest_points)
{
	BgfxCompatSurface *dst = BgfxCompat_To_Surface(destination_surface);
	const BgfxCompatSurface *src = BgfxCompat_To_Surface(source_surface);
	if (dst == nullptr || src == nullptr || dst->format != src->format) {
		return;
	}

	RECT full_source_rect = {
		0,
		0,
		static_cast<int32_t>(std::min(src->width, dst->width)),
		static_cast<int32_t>(std::min(src->height, dst->height))
	};
	POINT origin = { 0, 0 };
	if (source_rects == nullptr || dest_points == nullptr || rect_count == 0) {
		source_rects = &full_source_rect;
		dest_points = &origin;
		rect_count = 1;
	}

	const unsigned pixel_size = BgfxCompat_Get_Pixel_Size(dst->format);
	for (uint32_t rect_index = 0; rect_index < rect_count; ++rect_index) {
		const RECT &src_rect = source_rects[rect_index];
		const POINT &dst_point = dest_points[rect_index];
		const unsigned width = static_cast<unsigned>(std::max(src_rect.right - src_rect.left, 0));
		const unsigned height = static_cast<unsigned>(std::max(src_rect.bottom - src_rect.top, 0));
		for (unsigned row = 0; row < height; ++row) {
			const size_t src_offset = (static_cast<size_t>(src_rect.top + static_cast<int32_t>(row)) * static_cast<size_t>(src->width) + static_cast<size_t>(src_rect.left)) * pixel_size;
			const size_t dst_offset = (static_cast<size_t>(dst_point.y + static_cast<int32_t>(row)) * static_cast<size_t>(dst->width) + static_cast<size_t>(dst_point.x)) * pixel_size;
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

	bgfx::Init init;
	init.type = bgfx::RendererType::Count;
	init.vendorId = BGFX_PCI_ID_NONE;
	init.debug = false;
	init.profile = false;
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
	Reset_View_Sequence();
	g_bgfx.viewport = RenderViewportClass(0u, 0u, static_cast<unsigned>(width), static_cast<unsigned>(height));
	Update_Windowed_State();
	DX8Wrapper::Refresh_Render_Device_Desc();
	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, width, height));

	bgfx::setViewName(g_bgfx.current_view_id, "Bootstrap");
	bgfx::setViewMode(g_bgfx.current_view_id, bgfx::ViewMode::Sequential);
	Apply_View_Target();
	Apply_View_Rect();
	bgfx::setViewClear(g_bgfx.current_view_id, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
	return true;
}

void Shutdown_Bgfx()
{
	if (!g_bgfx.initialized) {
		return;
	}

	Reset_Draw_State();

	for (bgfx::ProgramHandle &program : g_bgfx.programs) {
		if (bgfx::isValid(program)) {
			bgfx::destroy(program);
			program = BGFX_INVALID_HANDLE;
		}
	}
	if (bgfx::isValid(g_bgfx.texture_uniform)) {
		bgfx::destroy(g_bgfx.texture_uniform);
		g_bgfx.texture_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.fog_state_uniform)) {
		bgfx::destroy(g_bgfx.fog_state_uniform);
		g_bgfx.fog_state_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.fog_color_uniform)) {
		bgfx::destroy(g_bgfx.fog_color_uniform);
		g_bgfx.fog_color_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.color_adjust_uniform)) {
		bgfx::destroy(g_bgfx.color_adjust_uniform);
		g_bgfx.color_adjust_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.material_source_uniform)) {
		bgfx::destroy(g_bgfx.material_source_uniform);
		g_bgfx.material_source_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.material_ambient_uniform)) {
		bgfx::destroy(g_bgfx.material_ambient_uniform);
		g_bgfx.material_ambient_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.material_diffuse_uniform)) {
		bgfx::destroy(g_bgfx.material_diffuse_uniform);
		g_bgfx.material_diffuse_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.material_emissive_uniform)) {
		bgfx::destroy(g_bgfx.material_emissive_uniform);
		g_bgfx.material_emissive_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.material_state_uniform)) {
		bgfx::destroy(g_bgfx.material_state_uniform);
		g_bgfx.material_state_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.render_ambient_uniform)) {
		bgfx::destroy(g_bgfx.render_ambient_uniform);
		g_bgfx.render_ambient_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.light_environment_state_uniform)) {
		bgfx::destroy(g_bgfx.light_environment_state_uniform);
		g_bgfx.light_environment_state_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.light_environment_ambient_uniform)) {
		bgfx::destroy(g_bgfx.light_environment_ambient_uniform);
		g_bgfx.light_environment_ambient_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.light_environment_direction_uniform)) {
		bgfx::destroy(g_bgfx.light_environment_direction_uniform);
		g_bgfx.light_environment_direction_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.light_environment_diffuse_uniform)) {
		bgfx::destroy(g_bgfx.light_environment_diffuse_uniform);
		g_bgfx.light_environment_diffuse_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.world_view_row_uniform)) {
		bgfx::destroy(g_bgfx.world_view_row_uniform);
		g_bgfx.world_view_row_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.texgen_state_uniform)) {
		bgfx::destroy(g_bgfx.texgen_state_uniform);
		g_bgfx.texgen_state_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.texture_transform_row_uniform)) {
		bgfx::destroy(g_bgfx.texture_transform_row_uniform);
		g_bgfx.texture_transform_row_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.direct_light_state_uniform)) {
		bgfx::destroy(g_bgfx.direct_light_state_uniform);
		g_bgfx.direct_light_state_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.direct_light_position_uniform)) {
		bgfx::destroy(g_bgfx.direct_light_position_uniform);
		g_bgfx.direct_light_position_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.direct_light_direction_uniform)) {
		bgfx::destroy(g_bgfx.direct_light_direction_uniform);
		g_bgfx.direct_light_direction_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.direct_light_ambient_uniform)) {
		bgfx::destroy(g_bgfx.direct_light_ambient_uniform);
		g_bgfx.direct_light_ambient_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.direct_light_diffuse_uniform)) {
		bgfx::destroy(g_bgfx.direct_light_diffuse_uniform);
		g_bgfx.direct_light_diffuse_uniform = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.compatibility_vertex_buffer)) {
		bgfx::destroy(g_bgfx.compatibility_vertex_buffer);
		g_bgfx.compatibility_vertex_buffer = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(g_bgfx.compatibility_index_buffer)) {
		bgfx::destroy(g_bgfx.compatibility_index_buffer);
		g_bgfx.compatibility_index_buffer = BGFX_INVALID_HANDLE;
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

	const bgfx::ViewId view_id = Allocate_Scene_View();
	Sync_Backbuffer(false);
	Ensure_Gui_Resources();
	bgfx::setViewName(view_id, g_bgfx.render_target != nullptr ? "BootstrapRT" : "Bootstrap");
	bgfx::setViewClear(view_id, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
	bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
	bgfx::setViewTransform(view_id, kIdentityMatrix, kIdentityMatrix);
	Apply_View_Target();
	bgfx::touch(view_id);
	g_bgfx.draw_calls = 0;
}

void DX8Wrapper::Set_Fog(bool enable, const Vector3 &color, float start, float end)
{
	_Fog_Enable_State() = enable;
	_Fog_Color_State() = color;
	_Fog_Start_State() = std::max(start, 0.0f);
	_Fog_End_State() = std::max(end, _Fog_Start_State());
}

void DX8Wrapper::End_Scene(bool flip_frame)
{
	if (!g_bgfx.initialized) {
		return;
	}

	g_bgfx.scene_active = false;
	if (flip_frame) {
		const Uint64 end_scene_ticks = WWPerfMonClass::Begin_Scope();
		bgfx::frame(BGFX_FRAME_NONE);
		Reset_Compatibility_Submit_Scratch();
		if (WWPerfMonClass::Is_Enabled()) {
			const bgfx::Stats *stats = bgfx::getStats();
			if (stats != nullptr) {
				const double cpu_frame_ms = stats->cpuTimerFreq > 0
					? (1000.0 * static_cast<double>(stats->cpuTimeFrame) / static_cast<double>(stats->cpuTimerFreq))
					: 0.0;
				const double gpu_frame_ms = stats->gpuTimerFreq > 0
					? (1000.0 * static_cast<double>(stats->gpuTimeEnd - stats->gpuTimeBegin) / static_cast<double>(stats->gpuTimerFreq))
					: 0.0;
				const double wait_render_ms = stats->cpuTimerFreq > 0
					? (1000.0 * static_cast<double>(stats->waitRender) / static_cast<double>(stats->cpuTimerFreq))
					: 0.0;
				const double wait_submit_ms = stats->cpuTimerFreq > 0
					? (1000.0 * static_cast<double>(stats->waitSubmit) / static_cast<double>(stats->cpuTimerFreq))
					: 0.0;
				WWPerfMonClass::Record_Bgfx_Frame_Timing(
					cpu_frame_ms,
					gpu_frame_ms,
					wait_render_ms,
					wait_submit_ms,
					stats->numDraw);
			}
		}
		WWPerfMonClass::End_Scope(WWPERF_SECTION_END_SCENE, end_scene_ticks);
		Reset_View_Sequence();
	}
}

void DX8Wrapper::Flip_To_Primary(void)
{
}

void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float z, uint32_t stencil)
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

	if (g_bgfx.initialized && g_bgfx.scene_active) {
		bgfx::setViewClear(g_bgfx.current_view_id, g_bgfx.clear_flags, g_bgfx.clear_color, g_bgfx.clear_depth, g_bgfx.clear_stencil);
	}
}

void DX8Wrapper::Set_Viewport(const RenderViewportClass &viewport)
{
	g_bgfx.viewport = viewport;
	if (g_bgfx.initialized && g_bgfx.scene_active) {
		Apply_View_Target();
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
	if (g_bgfx.render_target != nullptr) {
		width = std::max(g_bgfx.render_target->Get_Width(), 1);
		height = std::max(g_bgfx.render_target->Get_Height(), 1);
	} else {
		width = g_bgfx.width;
		height = g_bgfx.height;
	}
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
	g_bgfx.current_vb = vb;
	g_bgfx.current_vb_type = vb != nullptr ? vb->Type() : BUFFER_TYPE_INVALID;
	Bind_Current_Vertex_Buffer_Slice(0);
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass &vba)
{
	g_bgfx.current_vb = vba.Get_Vertex_Buffer();
	g_bgfx.current_vb_type = vba.Get_Type();
	Bind_Current_Vertex_Buffer_Slice(vba.Get_Vertex_Buffer_Offset());
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass *ib, uint16_t index_base_offset)
{
	g_bgfx.index_base_offset = index_base_offset;
	g_bgfx.current_ib = ib;
	g_bgfx.current_ib_type = ib != nullptr ? ib->Type() : BUFFER_TYPE_INVALID;
	Bind_Current_Index_Buffer_Slice(0);
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass &iba, uint16_t index_base_offset)
{
	g_bgfx.index_base_offset = index_base_offset;
	g_bgfx.current_ib = iba.Get_Index_Buffer();
	g_bgfx.current_ib_type = iba.Get_Type();
	Bind_Current_Index_Buffer_Slice(iba.Get_Index_Buffer_Offset());
}

void DX8Wrapper::Set_Index_Buffer_Index_Offset(unsigned offset)
{
	g_bgfx.index_base_offset = static_cast<uint16_t>(offset);
}

void DX8Wrapper::Draw_Triangles(unsigned, uint16_t start_index, uint16_t polygon_count, uint16_t min_vertex_index, uint16_t vertex_count)
{
	Submit_Primitives(start_index, polygon_count, min_vertex_index, vertex_count, false);
}

void DX8Wrapper::Draw_Triangles(uint16_t start_index, uint16_t polygon_count, uint16_t min_vertex_index, uint16_t vertex_count)
{
	Submit_Primitives(start_index, polygon_count, min_vertex_index, vertex_count, false);
}

void DX8Wrapper::Draw_Strip(uint16_t start_index, uint16_t polygon_count, uint16_t min_vertex_index, uint16_t vertex_count)
{
	if (g_bgfx.index_data == nullptr || polygon_count == 0) {
		return;
	}

	Submit_Primitives(start_index, polygon_count, min_vertex_index, vertex_count, true);
}

void DX8Wrapper::Set_Texture(unsigned stage, TextureClass *texture)
{
	if (stage < MAX_TEXTURE_STAGES && g_bgfx.textures[stage] != texture) {
		REF_PTR_SET(g_bgfx.textures[stage], texture);
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

unsigned DX8Wrapper::Get_DX8_Render_State(unsigned state)
{
	return state < 256 ? g_bgfx.render_states[state] : 0;
}

void DX8Wrapper::Set_Light_Environment(const LightEnvironmentClass *light_environment)
{
	g_bgfx.light_environment = BgfxLightEnvironmentState();
	if (light_environment == nullptr) {
		return;
	}

	g_bgfx.light_environment.enabled = true;
	g_bgfx.light_environment.ambient = light_environment->Get_Equivalent_Ambient();
	g_bgfx.light_environment.count = static_cast<unsigned>(std::min(light_environment->Get_Light_Count(), 4));
	for (unsigned light_index = 0; light_index < g_bgfx.light_environment.count; ++light_index) {
		g_bgfx.light_environment.directions[light_index] = light_environment->Get_Light_Direction(static_cast<int>(light_index));
		g_bgfx.light_environment.diffuse[light_index] = light_environment->Get_Light_Diffuse(static_cast<int>(light_index));
	}
}

void DX8Wrapper::Set_Light(unsigned index, const LightClass *light)
{
	if (index >= 4) {
		return;
	}

	g_bgfx.light_environment.enabled = false;
	BgfxLightState &state = g_bgfx.lights[index];
	state = BgfxLightState();
	if (light == nullptr) {
		return;
	}

	state.enabled = true;
	state.type = light->Get_Type();
	light->Get_Ambient(&state.ambient);
	light->Get_Diffuse(&state.diffuse);
	state.position = light->Get_Position();
	double far_start = 0.0;
	double far_end = 1.0;
	light->Get_Far_Attenuation_Range(far_start, far_end);
	state.far_atten_start = static_cast<float>(far_start);
	state.far_atten_end = static_cast<float>(far_end);
	if (state.type == LightClass::DIRECTIONAL) {
		state.direction = -light->Get_Transform().Get_Z_Vector();
	} else {
		state.direction = light->Get_Transform().Get_Z_Vector();
	}
	if (state.type == LightClass::SPOT) {
		Vector3 spot_direction;
		light->Get_Spot_Direction(spot_direction);
		Matrix3D::Rotate_Vector(light->Get_Transform(), spot_direction, &state.direction);
		state.spot_angle_cos = light->Get_Spot_Angle_Cos();
	}
}

void DX8Wrapper::Set_Light(unsigned index, const LightClass &light)
{
	Set_Light(index, &light);
}

void DX8Wrapper::Set_Render_Target(TextureClass *texture)
{
	REF_PTR_SET(g_bgfx.render_target, texture);
	if (g_bgfx.initialized && g_bgfx.scene_active) {
		Apply_View_Target();
		Apply_View_Rect();
	}
}

void DX8Wrapper::Set_Render_Target(IDirect3DSurface8 *surface)
{
	if (surface == nullptr) {
		Set_Render_Target(static_cast<TextureClass *>(nullptr));
	}
}

void DX8Wrapper::Set_Gamma(float gamma, float brightness, float contrast, bool, bool)
{
	g_bgfx.gamma = std::max(gamma, 1.0e-4f);
	g_bgfx.brightness = brightness;
	g_bgfx.contrast = std::max(contrast, 0.0f);
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
	state.zbias = g_bgfx.render_states[D3DRS_ZBIAS];
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
	Set_DX8_Render_State(D3DRS_ZBIAS, state.zbias);
	Set_Vertex_Buffer(state.vertex_buffer);
	Bind_Current_Vertex_Buffer_Slice(state.vba_offset);
	Set_Index_Buffer(state.index_buffer, static_cast<uint16_t>(state.index_base_offset));
	Bind_Current_Index_Buffer_Slice(state.iba_offset);
}

void DX8Wrapper::Release_Render_State()
{
}

void DX8Wrapper::Apply_Render_State_Changes()
{
}

TextureClass *DX8Wrapper::Create_Render_Target(unsigned width, unsigned height, int format)
{
	const WW3DFormat resolved_format = format == WW3D_FORMAT_UNKNOWN ? WW3D_FORMAT_A8R8G8B8 : static_cast<WW3DFormat>(format);
	return NEW_REF(TextureClass, (width, height, resolved_format, TextureClass::MIP_LEVELS_1, TextureClass::POOL_DEFAULT, true));
}

bool DX8Wrapper::Is_Render_To_Texture()
{
	return g_bgfx.render_target != nullptr && BgfxCompat_Is_Render_Target(g_bgfx.render_target);
}

void DX8Wrapper::_Copy_DX8_Rects(IDirect3DSurface8 *pSourceSurface, const RECT *pSourceRectsArray, uint32_t cRects, IDirect3DSurface8 *pDestinationSurface, const POINT *pDestPointsArray)
{
	Copy_Surface_Rectangles(pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);
}

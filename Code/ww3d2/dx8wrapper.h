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
 *                 Project Name : ww3d                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/dx8wrapper.h                           $*
 *                                                                                             *
 *              Original Author:: Jani Penttinen                                               *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 2/26/02 4:04p                                               $*
 *                                                                                             *
 *                    $Revision:: 90                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once

#include <cstdint>
#endif

#ifndef DX8_WRAPPER_H
#define DX8_WRAPPER_H

enum TransformSlot
{
	TRANSFORM_WORLD = 0,
	TRANSFORM_VIEW = 1,
	TRANSFORM_PROJECTION = 2,
	TRANSFORM_TEXTURE0 = 16
};

inline TransformSlot Texture_Transform_Slot(unsigned stage)
{
	return static_cast<TransformSlot>(TRANSFORM_TEXTURE0 + stage);
}

class RenderViewportClass
{
public:
	RenderViewportClass() : X(0), Y(0), Width(0), Height(0), MinZ(0.0f), MaxZ(1.0f) {}
	RenderViewportClass(unsigned x, unsigned y, unsigned width, unsigned height, float min_z = 0.0f, float max_z = 1.0f)
		: X(x), Y(y), Width(width), Height(height), MinZ(min_z), MaxZ(max_z) {}

	unsigned X;
	unsigned Y;
	unsigned Width;
	unsigned Height;
	float MinZ;
	float MaxZ;
};

#include "vector3.h"
#include "vector4.h"
#include "shader.h"
#include "../compat/dx8vertexbuffer.h"
#include "../compat/dx8indexbuffer.h"

#include "matrix4.h"
class Matrix3D;
struct IDirect3DSurface8;
using FLOAT = float;

class TextureClass;
class LightClass;
class LightEnvironmentClass;
class RenderDeviceDescClass;
class VertexMaterialClass;

#define MAX_TEXTURE_STAGES 2

struct RenderStateStruct {
	ShaderClass shader;
	VertexMaterialClass* material = nullptr;
	TextureClass* Textures[MAX_TEXTURE_STAGES] = {};
	Matrix4 world;
	Matrix4 view;
	Matrix4 projection;
	unsigned zbias = 0;
	VertexBufferClass* vertex_buffer = nullptr;
	IndexBufferClass* index_buffer = nullptr;
	unsigned vertex_buffer_type = 0;
	unsigned index_buffer_type = 0;
	unsigned vba_offset = 0;
	unsigned iba_offset = 0;
	unsigned index_base_offset = 0;
};
class DX8Caps {
public:
	bool Support_Render_To_Texture_Format(int) const { return true; }
	bool Support_Texture_Format(int) const { return true; }
	bool Support_NPatches() const { return false; }
	bool Support_Bump_Envmap() const { return false; }
	bool Support_Bump_Envmap_Luminance() const { return false; }
	bool Support_TnL() const { return true; }
	bool Support_DXTC() const { return true; }
	bool Support_Gamma() const { return true; }
	bool Support_ZBias() const { return true; }
	bool Is_Fog_Allowed() const { return true; }
	unsigned Get_Vendor() const { return 0; }
	unsigned Get_Device() const { return 0; }
	const char *Get_Log() const { return "bgfx renderer\n"; }
	const char *Get_Compact_Log() const { return "bgfx"; }
};

// D3D compatibility defines used throughout the renderer code.
// These map legacy D3D constants to our own renderer-agnostic values.

#define D3DTS_WORLD TRANSFORM_WORLD
#define D3DTS_VIEW TRANSFORM_VIEW
#define D3DTS_PROJECTION TRANSFORM_PROJECTION
#define D3DTS_TEXTURE0 TRANSFORM_TEXTURE0

#define D3DRS_FILLMODE 8
#define D3DRS_AMBIENT 26
#define D3DRS_FOGENABLE 28
#define D3DRS_FOGCOLOR 34
#define D3DRS_ZBIAS 47
#define D3DRS_LIGHTING 137
#define D3DRS_DIFFUSEMATERIALSOURCE 145
#define D3DRS_SPECULARMATERIALSOURCE 146
#define D3DRS_AMBIENTMATERIALSOURCE 147
#define D3DRS_EMISSIVEMATERIALSOURCE 148

#define D3DMCS_MATERIAL 0
#define D3DMCS_COLOR1 1
#define D3DMCS_COLOR2 2

#define D3DFILL_POINT 1
#define D3DFILL_WIREFRAME 2
#define D3DFILL_SOLID 3

#define D3DTSS_TEXCOORDINDEX 0
#define D3DTSS_TEXTURETRANSFORMFLAGS 1
#define D3DTSS_TCI_PASSTHRU 0x00000000
#define D3DTSS_TCI_CAMERASPACEPOSITION 0x00010000
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 0x00030000

#define D3DTTFF_DISABLE 0
#define D3DTTFF_COUNT2 2
#define D3DTTFF_COUNT3 3
#define D3DTTFF_PROJECTED 256

#define D3DTSS_BUMPENVMAT00 7
#define D3DTSS_BUMPENVMAT01 8
#define D3DTSS_BUMPENVMAT10 9
#define D3DTSS_BUMPENVMAT11 10

using D3DTRANSFORMSTATETYPE = int32_t;

enum {
	BUFFER_TYPE_DX8,
	BUFFER_TYPE_SORTING,
	BUFFER_TYPE_DYNAMIC_DX8,
	BUFFER_TYPE_DYNAMIC_SORTING,
	BUFFER_TYPE_INVALID
};

class DX8Wrapper
{
public:
	static bool &_Triangle_Draw_State()
	{
		static bool enabled = true;
		return enabled;
	}

	static bool Init(void *hwnd, bool lite = false);
	static void Shutdown(void);
	static void Begin_Scene(void);
	static void End_Scene(bool flip_frame = true);
	static void Flip_To_Primary(void);
	static void Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float z = 1.0f, uint32_t stencil = 0);
	static void Set_Viewport(const RenderViewportClass &viewport);

	static bool Set_Any_Render_Device(void);
	static bool Set_Render_Device(const char *dev_name, int width = -1, int height = -1, int bits = -1, int windowed = -1, bool resize_window = false);
	static bool Set_Render_Device(int dev = -1, int width = -1, int height = -1, int bits = -1, int windowed = -1, bool resize_window = false);
	static bool Set_Next_Render_Device(void);
	static int Get_Render_Device_Count(void);
	static int Get_Render_Device(void);
	static const char *Get_Render_Device_Name(int device_index);
	static const RenderDeviceDescClass &Get_Render_Device_Desc(int deviceidx = -1);
	static bool Set_Device_Resolution(int width = -1, int height = -1, int bits = -1, int windowed = -1, bool resize_window = false);
	static void Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed);
	static void Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed);
	static bool Registry_Save_Render_Device(const char *sub_key);
	static bool Registry_Save_Render_Device(const char *sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth);
	static bool Registry_Load_Render_Device(const char *sub_key, bool resize_window);
	static bool Registry_Load_Render_Device(const char *sub_key, char *device, int device_len, int &width, int &height, int &depth, int &windowed, int &texture_depth);
	static int Get_Device_Resolution_Width(void);
	static int Get_Device_Resolution_Height(void);
	static bool Is_Windowed(void);
	static bool Toggle_Windowed(void);
	static void Set_Swap_Interval(int swap);
	static int Get_Swap_Interval(void);
	static void Set_Texture_Bitdepth(int depth);
	static int Get_Texture_Bitdepth(void);
	static void Update_Window(void *hwnd);
	static void Refresh_Render_Device_Desc(void);

	static void Set_Light_Environment(const LightEnvironmentClass *light_environment);
	static void Set_Light(unsigned index, const LightClass *light);
	static void Set_Light(unsigned index, const LightClass &light);
	static void Draw_Strip(uint16_t start_index, uint16_t polygon_count, uint16_t min_vertex_index, uint16_t vertex_count);
	static void Set_Render_Target(TextureClass *texture);
	static void Set_Render_Target(IDirect3DSurface8 *surface);
	static void Set_Gamma(float gamma, float brightness, float contrast, bool calibrate = false, bool save = false);
	static void Set_Fog(bool enable, const Vector3 &color, float start, float end);
	static bool Get_Fog_Enable() { return _Fog_Enable_State(); }
	static unsigned Get_Fog_Color() { return Convert_Color(_Fog_Color_State(), 1.0f); }
	static const Vector3 &Get_Fog_Color_Vector() { return _Fog_Color_State(); }
	static float Get_Fog_Start() { return _Fog_Start_State(); }
	static float Get_Fog_End() { return _Fog_End_State(); }

	static void Set_Transform(TransformSlot transform, const Matrix4 &m);
	static void Set_Transform(TransformSlot transform, const Matrix3D &m);
	static void Get_Transform(TransformSlot transform, Matrix4 &m);
	static void Set_Projection_Transform_With_Z_Bias(const Matrix4 &matrix, float znear, float zfar);
	static void Set_Vertex_Buffer(const VertexBufferClass *vb);
	static void Set_Vertex_Buffer(const DynamicVBAccessClass &vba);
	static void Set_Index_Buffer(const IndexBufferClass *ib, uint16_t index_base_offset);
	static void Set_Index_Buffer(const DynamicIBAccessClass &iba, uint16_t index_base_offset);
	static void Set_Index_Buffer_Index_Offset(unsigned offset);
	static void Draw_Triangles(unsigned buffer_type, uint16_t start_index, uint16_t polygon_count, uint16_t min_vertex_index, uint16_t vertex_count);
	static void Draw_Triangles(uint16_t start_index, uint16_t polygon_count, uint16_t min_vertex_index, uint16_t vertex_count);
	static void Set_Texture(unsigned stage, TextureClass *texture);
	static void Set_Material(const VertexMaterialClass *material);
	static void Set_Shader(const ShaderClass &shader);
	static void Set_DX8_Texture_Stage_State(unsigned stage, unsigned state, unsigned value);
	static void Set_DX8_Render_State(unsigned state, unsigned value);
	static unsigned Get_DX8_Render_State(unsigned state);

	static void Get_Render_State(RenderStateStruct &state);
	static void Set_Render_State(const RenderStateStruct &state);
	static void Release_Render_State();
	static void Apply_Render_State_Changes();

	static void Set_Alpha(const float alpha, uint32_t &color)
	{
		uint8_t *component = reinterpret_cast<uint8_t *>(&color);
		component[3] = static_cast<uint8_t>(255.0f * alpha);
	}

	static void Set_World_Identity();
	static void Set_View_Identity();
	static bool Is_Device_Lost() { return false; }
	static bool Is_Initted();
	static TextureClass *Create_Render_Target(unsigned width, unsigned height, int format);
	static void Begin_Statistics() {}
	static void End_Statistics() {}
	static bool Is_Render_To_Texture();
	static DX8Caps *Get_Current_Caps()
	{
		static DX8Caps caps;
		return &caps;
	}
	static void _Enable_Triangle_Draw(bool enable) { _Triangle_Draw_State() = enable; }
	static bool _Is_Triangle_Draw_Enabled() { return _Triangle_Draw_State(); }
	static unsigned Get_Last_Frame_DX8_Calls() { return 0; }
	static unsigned Get_Last_Frame_Matrix_Changes() { return 0; }
	static unsigned Get_Last_Frame_Material_Changes() { return 0; }
	static unsigned Get_Last_Frame_Vertex_Buffer_Changes() { return 0; }
	static unsigned Get_Last_Frame_Index_Buffer_Changes() { return 0; }
	static unsigned Get_Last_Frame_Light_Changes() { return 0; }
	static void _Copy_DX8_Rects(
		IDirect3DSurface8 *pSourceSurface,
		const RECT *pSourceRectsArray,
		uint32_t cRects,
		IDirect3DSurface8 *pDestinationSurface,
		const POINT *pDestPointsArray);

	static Vector4 Convert_Color(unsigned color)
	{
		const float inv = 1.0f / 255.0f;
		return Vector4(
			static_cast<float>((color >> 16) & 0xFF) * inv,
			static_cast<float>((color >> 8) & 0xFF) * inv,
			static_cast<float>(color & 0xFF) * inv,
			static_cast<float>((color >> 24) & 0xFF) * inv);
	}

	static uint32_t Convert_Color(const Vector4 &color)
	{
		auto clamp = [](float value) -> uint32_t {
			const float scaled = value < 0.0f ? 0.0f : (value > 1.0f ? 255.0f : value * 255.0f);
			return static_cast<uint32_t>(scaled + 0.5f);
		};

		return (clamp(color.W) << 24) |
			(clamp(color.X) << 16) |
			(clamp(color.Y) << 8) |
			clamp(color.Z);
	}

	static uint32_t Convert_Color(const Vector3 &color, float alpha)
	{
		return Convert_Color(Vector4(color.X, color.Y, color.Z, alpha));
	}

	static uint32_t Convert_Color_Clamp(const Vector4 &color)
	{
		return Convert_Color(color);
	}

private:
	static bool &_Fog_Enable_State()
	{
		static bool enabled = false;
		return enabled;
	}

	static Vector3 &_Fog_Color_State()
	{
		static Vector3 color(0.0f, 0.0f, 0.0f);
		return color;
	}

	static float &_Fog_Start_State()
	{
		static float start = 0.0f;
		return start;
	}

	static float &_Fog_End_State()
	{
		static float end = 1000.0f;
		return end;
	}
};

#endif

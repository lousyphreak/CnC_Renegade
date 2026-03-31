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
#include "../compat/dx8vertexbuffer.h"
#include "../compat/dx8indexbuffer.h"

struct Matrix4;
class Matrix3D;
struct IDirect3DSurface8;
using FLOAT = float;

class TextureClass;
class LightClass;
class RenderDeviceDescClass;
class ShaderClass;
class VertexMaterialClass;
class DX8Caps {
public:
	bool Support_Render_To_Texture_Format(int) const { return false; }
	bool Support_NPatches() const { return false; }
	bool Support_Bump_Envmap() const { return false; }
	bool Support_Bump_Envmap_Luminance() const { return false; }
	const char *Get_Log() const { return "bgfx bootstrap caps unavailable\n"; }
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
#define D3DRS_ZBIAS 47

#define D3DFILL_POINT 1
#define D3DFILL_WIREFRAME 2
#define D3DFILL_SOLID 3

#define D3DTSS_TEXCOORDINDEX 0
#define D3DTSS_TEXTURETRANSFORMFLAGS 1
#define D3DTSS_TCI_PASSTHRU 0
#define D3DTSS_TCI_CAMERASPACEPOSITION 1
#define D3DTSS_TCI_CAMERASPACENORMAL 2
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 3

#define D3DTTFF_COUNT2 2
#define D3DTTFF_COUNT3 3
#define D3DTTFF_PROJECTED 256

#define D3DTSS_BUMPENVMAT00 7
#define D3DTSS_BUMPENVMAT01 8
#define D3DTSS_BUMPENVMAT10 9
#define D3DTSS_BUMPENVMAT11 10

using D3DTRANSFORMSTATETYPE = int;

#define MAX_TEXTURE_STAGES 2

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
	static void Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float z = 1.0f, unsigned int stencil = 0);
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

	template <typename... Args>
	static void Set_Light_Environment(Args&&...) {}
	template <typename... Args>
	static void Set_Light(Args&&...) {}
	template <typename... Args>
	static void Draw_Strip(Args&&...) {}
	template <typename... Args>
	static void Set_Render_Target(Args&&...) {}
	template <typename... Args>
	static void Set_Gamma(Args&&...) {}
	template <typename... Args>
	static void Set_Fog(Args&&...) {}

	static void Set_Transform(TransformSlot transform, const Matrix4 &m);
	static void Set_Transform(TransformSlot transform, const Matrix3D &m);
	static void Get_Transform(TransformSlot transform, Matrix4 &m);
	static void Set_Projection_Transform_With_Z_Bias(const Matrix4 &matrix, float znear, float zfar);
	static void Set_Vertex_Buffer(const VertexBufferClass *vb);
	static void Set_Vertex_Buffer(const DynamicVBAccessClass &vba);
	static void Set_Index_Buffer(const IndexBufferClass *ib, unsigned short index_base_offset);
	static void Set_Index_Buffer(const DynamicIBAccessClass &iba, unsigned short index_base_offset);
	static void Set_Index_Buffer_Index_Offset(unsigned offset);
	static void Draw_Triangles(unsigned buffer_type, unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count);
	static void Draw_Triangles(unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count);
	static void Set_Texture(unsigned stage, TextureClass *texture);
	static void Set_Material(const VertexMaterialClass *material);
	static void Set_Shader(const ShaderClass &shader);
	static void Set_DX8_Texture_Stage_State(unsigned stage, unsigned state, unsigned value);
	static void Set_DX8_Render_State(unsigned state, unsigned value);

	static void Set_Alpha(const float alpha, unsigned int &color)
	{
		unsigned char *component = reinterpret_cast<unsigned char *>(&color);
		component[3] = static_cast<unsigned char>(255.0f * alpha);
	}

	static void Set_World_Identity();
	static void Set_View_Identity();
	static bool Is_Device_Lost() { return false; }
	static bool Is_Initted();
	static TextureClass *Create_Render_Target(unsigned, unsigned, int) { return NULL; }
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
		UINT cRects,
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

	static unsigned int Convert_Color(const Vector4 &color)
	{
		auto clamp = [](float value) -> unsigned long {
			const float scaled = value < 0.0f ? 0.0f : (value > 1.0f ? 255.0f : value * 255.0f);
			return static_cast<unsigned long>(scaled + 0.5f);
		};

		return (clamp(color.W) << 24) |
			(clamp(color.X) << 16) |
			(clamp(color.Y) << 8) |
			clamp(color.Z);
	}

	static unsigned int Convert_Color(const Vector3 &color, float alpha)
	{
		return Convert_Color(Vector4(color.X, color.Y, color.Z, alpha));
	}
};

#endif

#pragma once

#include <cstdint>

#include "vector3.h"
#include "vector4.h"
#include "../compat/dx8vertexbuffer.h"
#include "../compat/dx8indexbuffer.h"

struct Matrix4;
using FLOAT = float;

class TextureClass;
class LightClass;
class DX8Caps {
public:
	bool Support_Render_To_Texture_Format(int) const { return false; }
	bool Support_NPatches() const { return false; }
	bool Support_Bump_Envmap() const { return false; }
	bool Support_Bump_Envmap_Luminance() const { return false; }
	const char *Get_Log() const { return "DX8 caps unavailable in headless mode\n"; }
	const char *Get_Compact_Log() const { return "headless"; }
};

struct IDirect3DSurface8;

#ifndef D3DTS_WORLD
#define D3DTS_WORLD 0
#endif
#ifndef D3DTS_VIEW
#define D3DTS_VIEW 1
#endif
#ifndef D3DTS_PROJECTION
#define D3DTS_PROJECTION 2
#endif
#ifndef D3DTS_TEXTURE0
#define D3DTS_TEXTURE0 16
#endif
#ifndef D3DRS_FILLMODE
#define D3DRS_FILLMODE 8
#endif
#ifndef D3DRS_ZBIAS
#define D3DRS_ZBIAS 47
#endif
#ifndef D3DFILL_POINT
#define D3DFILL_POINT 1
#endif
#ifndef D3DFILL_WIREFRAME
#define D3DFILL_WIREFRAME 2
#endif
#ifndef D3DFILL_SOLID
#define D3DFILL_SOLID 3
#endif
#ifndef D3DTSS_TEXCOORDINDEX
#define D3DTSS_TEXCOORDINDEX 0
#endif
#ifndef D3DTSS_TEXTURETRANSFORMFLAGS
#define D3DTSS_TEXTURETRANSFORMFLAGS 1
#endif
#ifndef D3DTSS_TCI_PASSTHRU
#define D3DTSS_TCI_PASSTHRU 0x00000000
#endif
#ifndef D3DTSS_TCI_CAMERASPACEPOSITION
#define D3DTSS_TCI_CAMERASPACEPOSITION 0x00010000
#endif
#ifndef D3DTSS_TCI_CAMERASPACENORMAL
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00020000
#endif
#ifndef D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 0x00030000
#endif
#ifndef D3DTTFF_COUNT2
#define D3DTTFF_COUNT2 2
#endif
#ifndef D3DTTFF_COUNT3
#define D3DTTFF_COUNT3 3
#endif
#ifndef D3DTTFF_PROJECTED
#define D3DTTFF_PROJECTED 256
#endif
#ifndef D3DTSS_BUMPENVMAT00
#define D3DTSS_BUMPENVMAT00 7
#endif
#ifndef D3DTSS_BUMPENVMAT01
#define D3DTSS_BUMPENVMAT01 8
#endif
#ifndef D3DTSS_BUMPENVMAT10
#define D3DTSS_BUMPENVMAT10 9
#endif
#ifndef D3DTSS_BUMPENVMAT11
#define D3DTSS_BUMPENVMAT11 10
#endif

using D3DTRANSFORMSTATETYPE = int32_t;

#ifndef MAX_TEXTURE_STAGES
#define MAX_TEXTURE_STAGES 2
#endif

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

	template <typename... Args>
	static void Set_Transform(Args&&...)
	{
	}

	template <typename... Args>
	static void Get_Transform(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Viewport(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Projection_Transform_With_Z_Bias(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Light_Environment(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Light(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Vertex_Buffer(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Index_Buffer(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Index_Buffer_Index_Offset(Args&&...)
	{
	}

	template <typename... Args>
	static void Draw_Triangles(Args&&...)
	{
	}

	template <typename... Args>
	static void Draw_Strip(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Texture(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Material(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Shader(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Render_Target(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_DX8_Texture_Stage_State(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_DX8_Render_State(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Gamma(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Fog(Args&&...)
	{
	}

	template <typename... Args>
	static void Clear(Args&&...)
	{
	}

	static void Set_Alpha(const float alpha, uint32_t & color)
	{
		uint8_t * component = reinterpret_cast<uint8_t *>(&color);
		component[3] = static_cast<uint8_t>(255.0f * alpha);
	}

	static void Set_World_Identity()
	{
	}

	static void Set_View_Identity()
	{
	}

	static bool Is_Device_Lost()
	{
		return false;
	}

	static bool Is_Initted()
	{
		return false;
	}

	static TextureClass *Create_Render_Target(unsigned, unsigned, int)
	{
		return NULL;
	}

	static DX8Caps *Get_Current_Caps()
	{
		static DX8Caps caps;
		return &caps;
	}

	static void _Enable_Triangle_Draw(bool enable)
	{
		_Triangle_Draw_State() = enable;
	}

	static bool _Is_Triangle_Draw_Enabled()
	{
		return _Triangle_Draw_State();
	}

	static unsigned Get_Last_Frame_DX8_Calls()
	{
		return 0;
	}

	static unsigned Get_Last_Frame_Matrix_Changes()
	{
		return 0;
	}

	static unsigned Get_Last_Frame_Material_Changes()
	{
		return 0;
	}

	static unsigned Get_Last_Frame_Vertex_Buffer_Changes()
	{
		return 0;
	}

	static unsigned Get_Last_Frame_Index_Buffer_Changes()
	{
		return 0;
	}

	static unsigned Get_Last_Frame_Light_Changes()
	{
		return 0;
	}

	template <typename... Args>
	static void _Copy_DX8_Rects(Args&&...)
	{
	}

	static Vector4 Convert_Color(uint32_t color)
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
};

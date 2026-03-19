#pragma once

#include "always.h"
#include "bittype.h"
#include "../compat/dx8vertexbuffer.h"
#include "../compat/dx8indexbuffer.h"

class TextureClass;
class DX8Caps {
public:
	bool Support_Render_To_Texture_Format(int) const { return false; }
};

struct IDirect3DSurface8;
using D3DCOLOR = uint32;

#ifndef D3DTS_WORLD
#define D3DTS_WORLD 0
#endif
#ifndef D3DTS_VIEW
#define D3DTS_VIEW 1
#endif
#ifndef D3DTS_PROJECTION
#define D3DTS_PROJECTION 2
#endif
#ifndef D3DRS_FILLMODE
#define D3DRS_FILLMODE 8
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
	template <typename... Args>
	static void Set_Transform(Args&&...)
	{
	}

	template <typename... Args>
	static void Get_Transform(Args&&...)
	{
	}

	template <typename... Args>
	static void Set_Light_Environment(Args&&...)
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
	static void Set_DX8_Render_State(Args&&...)
	{
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

	template <typename... Args>
	static void _Copy_DX8_Rects(Args&&...)
	{
	}

	template <typename... Args>
	static D3DCOLOR Convert_Color(Args&&...)
	{
		return 0;
	}
};

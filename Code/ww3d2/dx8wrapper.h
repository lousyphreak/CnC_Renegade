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

#include "always.h"
#include "renegade_build_config.h"
#include "dllist.h"
#include "renderer_types.h"
#include "matrix4.h"
#include "statistics.h"
#include "wwstring.h"
#include "lightenvironment.h"
#include "shader.h"
#include "vector4.h"
#include "dx8caps.h"

#include "texture.h"
#include "vertmaterial.h"

/*
** Registry value names
*/
#define	VALUE_NAME_RENDER_DEVICE_NAME					"RenderDeviceName"
#define	VALUE_NAME_RENDER_DEVICE_WIDTH				"RenderDeviceWidth"
#define	VALUE_NAME_RENDER_DEVICE_HEIGHT				"RenderDeviceHeight"
#define	VALUE_NAME_RENDER_DEVICE_DEPTH				"RenderDeviceDepth"
#define	VALUE_NAME_RENDER_DEVICE_WINDOWED			"RenderDeviceWindowed"
#define	VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH		"RenderDeviceTextureDepth"

const unsigned MAX_TEXTURE_STAGES=2;

enum {
	BUFFER_TYPE_RENDER,
	BUFFER_TYPE_SORTING,
	BUFFER_TYPE_DYNAMIC_RENDER,
	BUFFER_TYPE_DYNAMIC_SORTING,
	BUFFER_TYPE_INVALID
};

class VertexMaterialClass;
class CameraClass;
class LightEnvironmentClass;
class RenderDeviceDescClass;
class VertexBufferClass;
class DynamicVBAccessClass;
class IndexBufferClass;
class DynamicIBAccessClass;
class TextureClass;
class LightClass;
class SurfaceClass;
class DX8Caps;

#define DX8_RECORD_MATRIX_CHANGE()				matrix_changes++
#define DX8_RECORD_MATERIAL_CHANGE()			material_changes++
#define DX8_RECORD_VERTEX_BUFFER_CHANGE()		vertex_buffer_changes++
#define DX8_RECORD_INDEX_BUFFER_CHANGE()		index_buffer_changes++
#define DX8_RECORD_LIGHT_CHANGE()				light_changes++
#define DX8_RECORD_TEXTURE_CHANGE()				texture_changes++
#define DX8_RECORD_RENDER_STATE_CHANGE()		render_state_changes++
#define DX8_RECORD_TEXTURE_STAGE_STATE_CHANGE() texture_stage_state_changes++

extern unsigned number_of_DX8_calls;

struct RenderStateStruct
{
	ShaderClass shader;
	VertexMaterialClass* material;
	D3DMATERIAL8 material_state;
	unsigned long material_crc;
	bool material_state_dirty;
	TextureClass * Textures[MAX_TEXTURE_STAGES];
	D3DLIGHT8 Lights[4];
	bool LightEnable[4];
	Matrix4 world;
	Matrix4 view;
	unsigned vertex_buffer_type;
	unsigned index_buffer_type;
	unsigned short vba_offset;
	unsigned short vba_count;
	unsigned short iba_offset;
	VertexBufferClass* vertex_buffer;
	IndexBufferClass* index_buffer;
	unsigned short index_base_offset;

	RenderStateStruct();
	~RenderStateStruct();

	RenderStateStruct& operator= (const RenderStateStruct& src);
};

/**
** DX8Wrapper
**
** DX8 interface wrapper class.  This encapsulates the DX8 interface; adding redundant state
** detection, stat tracking, etc etc.  In general, we will wrap all DX8 calls with at least
** an WWINLINE function so that we can add stat tracking, etc if needed.  Direct access to the
** D3D device will require "friend" status and should be granted only in extreme circumstances :-)
*/
class DX8Wrapper
{
	enum ChangedStates {
		WORLD_CHANGED	=	1<<0,
		VIEW_CHANGED	=	1<<1,
		LIGHT0_CHANGED	=	1<<2,
		LIGHT1_CHANGED	=	1<<3,
		LIGHT2_CHANGED	=	1<<4,
		LIGHT3_CHANGED	=	1<<5,
		TEXTURE0_CHANGED=	1<<6,
		TEXTURE1_CHANGED=	1<<7,
		TEXTURE2_CHANGED=	1<<8,
		TEXTURE3_CHANGED=	1<<9,
		MATERIAL_CHANGED=	1<<14,
		SHADER_CHANGED	=	1<<15,
		VERTEX_BUFFER_CHANGED = 1<<16,
		INDEX_BUFFER_CHANGED = 1 << 17,
		WORLD_IDENTITY=	1<<18,
		VIEW_IDENTITY=		1<<19,

		TEXTURES_CHANGED=
			TEXTURE0_CHANGED|TEXTURE1_CHANGED|TEXTURE2_CHANGED|TEXTURE3_CHANGED,
		LIGHTS_CHANGED=
			LIGHT0_CHANGED|LIGHT1_CHANGED|LIGHT2_CHANGED|LIGHT3_CHANGED,
	};

public:

	static bool Init(void * hwnd, bool lite = false);
	static void Shutdown(void);

	/*
	** Some WW3D sub-systems need to be initialized after the device is created and shutdown
	** before the device is released.
	*/
	static void	Do_Onetime_Device_Dependent_Inits(void);
	static void Do_Onetime_Device_Dependent_Shutdowns(void);

	static bool Is_Device_Lost() { return IsDeviceLost; }
	static bool Is_Initted(void) { return IsInitted; }

	/*
	** Rendering
	*/
	static void Begin_Scene(void);
	static void End_Scene(bool flip_frame = true);

	// Flip until the primary buffer is visible.
	static void Flip_To_Primary(void);

	static void Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float z=1.0f, unsigned int stencil=0);

	static void	Set_Viewport(CONST D3DVIEWPORT8* pViewport);

	static void Set_Vertex_Buffer(const VertexBufferClass* vb);
	static void Set_Vertex_Buffer(const DynamicVBAccessClass& vba);
	static void Set_Index_Buffer(const IndexBufferClass* ib,unsigned short index_base_offset);
	static void Set_Index_Buffer(const DynamicIBAccessClass& iba,unsigned short index_base_offset);
	static void Set_Index_Buffer_Index_Offset(unsigned offset);

	static void Get_Render_State(RenderStateStruct& state);
	static void Set_Render_State(const RenderStateStruct& state);
	static void Release_Render_State();

	static void Set_DX8_Material(const D3DMATERIAL8* mat);

	static void Set_Gamma(float gamma,float bright,float contrast,bool calibrate=true,bool uselimit=true);

	// Set_ and Get_Transform() functions take the matrix in Westwood convention format.

	static void Set_DX8_ZBias(int zbias);
	static void	Set_Pseudo_ZBias(int zbias);
	static void Set_Projection_Transform_With_Z_Bias(const Matrix4& matrix,float znear, float zfar);	// pointer to 16 matrices

	static void Set_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix4& m);
	static void Set_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix3D& m);
	static void Get_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4& m);
	static void	Set_World_Identity();
	static void Set_View_Identity();
	static bool	Is_World_Identity();
	static bool Is_View_Identity();

	// Note that *_DX8_Transform() functions take the matrix in DX8 format - transposed from Westwood convention.

	static void _Set_DX8_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix4& m);
	static void _Set_DX8_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix3D& m);
	static void _Get_DX8_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4& m);

	static void Set_DX8_Light(int index,D3DLIGHT8* light);
	static void Set_DX8_Render_State(D3DRENDERSTATETYPE state, unsigned value);
	static unsigned Get_DX8_Render_State(D3DRENDERSTATETYPE state);
	static void Set_DX8_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state, unsigned value);
	static unsigned Get_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state);
	static void Set_Light_Environment(LightEnvironmentClass* light_env);
	static void Set_Fog(bool enable, const Vector3 &color, float start, float end);

	static WWINLINE const D3DLIGHT8& Peek_Light(unsigned index);
	static WWINLINE bool Is_Light_Enabled(unsigned index);

	// Deferred

	static void Set_Shader(const ShaderClass& shader);
	static void Get_Shader(ShaderClass& shader);
	static void Set_Texture(unsigned stage,TextureClass* texture);
	static void Set_Material(const VertexMaterialClass* material);
	static void Set_Light(unsigned index,const D3DLIGHT8* light);
	static void Set_Light(unsigned index,const LightClass &light);

	static void Apply_Render_State_Changes();	// Apply deferred render state changes before renderer-owned bgfx submission.

	static void _Update_Texture(TextureClass *system, TextureClass *video);
	static void Flush_DX8_Resource_Manager(unsigned int bytes=0);
	static unsigned int Get_Free_Texture_RAM();

	static unsigned _Get_Main_Thread_ID() { return _MainThreadID; }
	static const D3DADAPTER_IDENTIFIER8& Get_Current_Adapter_Identifier() { return CurrentAdapterIdentifier; }

	/*
	** Statistics
	*/
	static void Begin_Statistics();
	static void End_Statistics();
	static unsigned Get_Last_Frame_Matrix_Changes();
	static unsigned Get_Last_Frame_Material_Changes();
	static unsigned Get_Last_Frame_Vertex_Buffer_Changes();
	static unsigned Get_Last_Frame_Index_Buffer_Changes();
	static unsigned Get_Last_Frame_Light_Changes();
	static unsigned Get_Last_Frame_Texture_Changes();
	static unsigned Get_Last_Frame_Render_State_Changes();
	static unsigned Get_Last_Frame_Texture_Stage_State_Changes();
	static unsigned Get_Last_Frame_DX8_Calls();

	static unsigned long Get_FrameCount(void);

	// Needed by shader class
	static bool						Get_Fog_Enable() { return FogEnable; }
	static D3DCOLOR				Get_Fog_Color() { return FogColor; }

	// Utilities
	static Vector4 Convert_Color(unsigned color);
	static unsigned int Convert_Color(const Vector4& color);
	static unsigned int Convert_Color(const Vector3& color, const float alpha);
	static void Clamp_Color(Vector4& color);
	static unsigned int Convert_Color_Clamp(const Vector4& color);

	static void			  Set_Alpha (const float alpha, unsigned int &color);

	static void _Enable_Triangle_Draw(bool enable) { _EnableTriangleDraw=enable; }
	static bool _Is_Triangle_Draw_Enabled() { return _EnableTriangleDraw; }

	static const DX8Caps*	Get_Current_Caps() { WWASSERT(CurrentCaps); return CurrentCaps; }

	static void	Set_Texture_Bitdepth(int depth)	{ WWASSERT(depth==16 || depth==32); TextureBitDepth = depth; }
	static int	Get_Texture_Bitdepth(void)			{ return TextureBitDepth; }

	static bool Registry_Save_Render_Device( const char * sub_key );
	static bool Registry_Load_Render_Device( const char * sub_key, bool resize_window );

protected:

	static bool	Create_Device(void);
	static bool Reset_Device(void);
	static void Release_Device(void);

	static void Reset_Statistics();
	static void Enumerate_Devices();
	static void Set_Default_Global_Render_States(void);
	static void Invalidate_Cached_Render_States(void);

	/*
	** Device Selection Code.
	** For backward compatibility, the public interface for these functions is in the ww3d.
	** header file.  These functions are protected so that we aren't exposing two interfaces.
	*/
	static bool Set_Any_Render_Device(void);
	static bool	Set_Render_Device(const char * dev_name,int width=-1,int height=-1,int bits=-1,int windowed=-1,bool resize_window=false);
	static bool	Set_Render_Device(int dev=-1,int resx=-1,int resy=-1,int bits=-1,int windowed=-1,bool resize_window = false);
	static bool Set_Next_Render_Device(void);
	static bool Toggle_Windowed(void);

	static int	Get_Render_Device_Count(void);
	static int	Get_Render_Device(void);
	static const RenderDeviceDescClass & Get_Render_Device_Desc(int deviceidx);
	static const char * Get_Render_Device_Name(int device_index);
	static bool Set_Device_Resolution(int width=-1,int height=-1,int bits=-1,int windowed=-1, bool resize_window=false);
	static void Get_Device_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed);
	static void Get_Render_Target_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed);
	static int	Get_Device_Resolution_Width(void) { return ResolutionWidth; }
	static int	Get_Device_Resolution_Height(void) { return ResolutionHeight; }

	static bool Registry_Save_Render_Device( const char *sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth);
	static bool Registry_Load_Render_Device( const char * sub_key, char *device, int device_len, int &width, int &height, int &depth, int &windowed, int &texture_depth);
	static bool Is_Windowed(void) { return IsWindowed; }

	static void	Set_Swap_Interval(int swap);
	static int	Get_Swap_Interval(void);
	static void Set_Polygon_Mode(int mode);

	static void Compute_Caps(WW3DFormat display_format);

	/*
	** Protected Member Variables
	*/

	static RenderStateStruct			render_state;
	static unsigned						render_state_changed;

	static bool								IsInitted;
	static bool								IsDeviceLost;
	static void *							Hwnd;
	static unsigned						_MainThreadID;

	static bool								_EnableTriangleDraw;

	static int								CurRenderDevice;
	static int								ResolutionWidth;
	static int								ResolutionHeight;
	static int								BitDepth;
	static int								TextureBitDepth;
	static bool								IsWindowed;

	static D3DMATRIX						old_world;
	static D3DMATRIX						old_view;
	static D3DMATRIX						old_prj;

	static bool								world_identity;
	static unsigned						RenderStates[256];
	static unsigned						TextureStageStates[MAX_TEXTURE_STAGES][32];

	// These fog settings are constant for all objects in a given scene,
	// unlike the matching renderstates which vary based on shader settings.
	static bool								FogEnable;
	static D3DCOLOR						FogColor;

	static unsigned						matrix_changes;
	static unsigned						material_changes;
	static unsigned						vertex_buffer_changes;
	static unsigned						index_buffer_changes;
	static unsigned						light_changes;
	static unsigned						texture_changes;
	static unsigned						render_state_changes;
	static unsigned						texture_stage_state_changes;
	static bool								CurrentDX8LightEnables[4];

	static unsigned long FrameCount;

	static DX8Caps*						CurrentCaps;

	static D3DADAPTER_IDENTIFIER8		CurrentAdapterIdentifier;

	static int								ZBias;
	static float							ZNear;
	static float							ZFar;
	static Matrix4							ProjectionMatrix;
	static Matrix4							TextureMatrices[MAX_TEXTURE_STAGES];

	friend class WW3D;
	friend class RenderIndexBufferClass;
	friend class RenderVertexBufferClass;
};


void DX8Wrapper::_Set_DX8_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix4& m);


void DX8Wrapper::_Set_DX8_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix3D& m);

void DX8Wrapper::_Get_DX8_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4& m);

// ----------------------------------------------------------------------------
//
// Set the index offset for the current index buffer
//
// ----------------------------------------------------------------------------

WWINLINE void DX8Wrapper::Set_Index_Buffer_Index_Offset(unsigned offset)
{
	if (render_state.index_base_offset==offset) return;
	render_state.index_base_offset=offset;
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

// ----------------------------------------------------------------------------
// Set the fog settings. This function should be used, rather than setting the
// appropriate renderstates directly, because the shader sets some of the
// renderstates on a per-mesh / per-pass basis depending on global fog states
// (stored in the wrapper) as well as the shader settings.
// This function should be called rarely - once per scene would be appropriate.
// ----------------------------------------------------------------------------

WWINLINE void DX8Wrapper::Set_Fog(bool enable, const Vector3 &color, float start, float end)
{
	// Set global states
	FogEnable = enable;
	FogColor = Convert_Color(color,0.0f);

	// Invalidate the current shader (since the renderstates set by the shader
	// depend on the global fog settings as well as the actual shader settings)
	ShaderClass::Invalidate();

	// Set renderstates which are not affected by the shader
	Set_DX8_Render_State(D3DRS_FOGSTART, *(DWORD *)(&start));
	Set_DX8_Render_State(D3DRS_FOGEND,   *(DWORD *)(&end));
}

// ----------------------------------------------------------------------------
//
// Set vertex buffer to be used in the subsequent render calls. If there was
// a vertex buffer being used earlier, release the reference to it. Passing
// NULL just will release the vertex buffer.
//
// ----------------------------------------------------------------------------

void DX8Wrapper::Set_DX8_Material(const D3DMATERIAL8* mat);

void DX8Wrapper::Set_DX8_Light(int index, D3DLIGHT8* light);

void DX8Wrapper::Set_DX8_Render_State(D3DRENDERSTATETYPE state, unsigned value);

void DX8Wrapper::Set_DX8_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state, unsigned value);

WWINLINE Vector4 DX8Wrapper::Convert_Color(unsigned color)
{
	Vector4 col;
	col[3]=((color&0xff000000)>>24)/255.0f;
	col[0]=((color&0xff0000)>>16)/255.0f;
	col[1]=((color&0xff00)>>8)/255.0f;
	col[2]=((color&0xff)>>0)/255.0f;
//	col=Vector4(1.0f,1.0f,1.0f,1.0f);
	return col;
}

WWINLINE unsigned int DX8Wrapper::Convert_Color(const Vector3& color,float alpha)
{
	return D3DCOLOR_COLORVALUE(color.X,color.Y,color.Z,alpha);
}

// ----------------------------------------------------------------------------
//
// Clamp color vertor to [0...1] range
//
// ----------------------------------------------------------------------------

WWINLINE void DX8Wrapper::Clamp_Color(Vector4& color)
{
	for (int i=0;i<4;++i) {
		if (color[i] < 0.0f) {
			color[i] = 0.0f;
		} else if (color[i] > 1.0f) {
			color[i] = 1.0f;
		}
	}
}

// ----------------------------------------------------------------------------
//
// Convert RGBA color from float vector to 32 bit integer
//
// ----------------------------------------------------------------------------

WWINLINE unsigned int DX8Wrapper::Convert_Color(const Vector4& color)
{
	return Convert_Color(reinterpret_cast<const Vector3&>(color),color[3]);
}

WWINLINE unsigned int DX8Wrapper::Convert_Color_Clamp(const Vector4& color)
{
	Vector4 clamped_color=color;
	DX8Wrapper::Clamp_Color(clamped_color);
	return Convert_Color(reinterpret_cast<const Vector3&>(clamped_color),clamped_color[3]);
}


WWINLINE void DX8Wrapper::Set_Alpha (const float alpha, unsigned int &color)
{
	unsigned char *component = (unsigned char*) &color;

	component [3] = 255.0f * alpha;
}

WWINLINE void DX8Wrapper::Get_Render_State(RenderStateStruct& state)
{
	state=render_state;
}

WWINLINE void DX8Wrapper::Get_Shader(ShaderClass& shader)
{
	shader=render_state.shader;
}

WWINLINE unsigned DX8Wrapper::Get_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state)
{
	if (stage >= MAX_TEXTURE_STAGES || state >= 32) {
		return 0u;
	}

	return TextureStageStates[stage][state];
}

WWINLINE unsigned DX8Wrapper::Get_DX8_Render_State(D3DRENDERSTATETYPE state)
{
	if (state >= 256) {
		return 0u;
	}

	return RenderStates[state];
}

WWINLINE void DX8Wrapper::Set_Texture(unsigned stage,TextureClass* texture)
{
	WWASSERT(stage<MAX_TEXTURE_STAGES);
	if (texture==render_state.Textures[stage]) return;
	REF_PTR_SET(render_state.Textures[stage],texture);
	render_state_changed|=(TEXTURE0_CHANGED<<stage);
}

WWINLINE void DX8Wrapper::Set_Material(const VertexMaterialClass* material)
{
	const unsigned long material_crc = material != nullptr ? material->Get_CRC() : 0u;
	if (material == render_state.material && material_crc == render_state.material_crc && !render_state.material_state_dirty) return;
	REF_PTR_SET(render_state.material,const_cast<VertexMaterialClass*>(material));
	render_state.material_crc = material_crc;
	render_state_changed|=MATERIAL_CHANGED;
}

WWINLINE void DX8Wrapper::Set_Shader(const ShaderClass& shader)
{
	if (!ShaderClass::ShaderDirty && ((unsigned&)shader==(unsigned&)render_state.shader)) return;
	render_state.shader=shader;
	render_state_changed|=SHADER_CHANGED;
}

void DX8Wrapper::Set_Projection_Transform_With_Z_Bias(const Matrix4& matrix, float znear, float zfar);

void DX8Wrapper::Set_Pseudo_ZBias(int zbias);

void DX8Wrapper::Set_DX8_ZBias(int zbias);

void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix4& m);

void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE transform,const Matrix3D& m);

WWINLINE void DX8Wrapper::Set_World_Identity()
{
	if (render_state_changed&(unsigned)WORLD_IDENTITY) return;
	render_state.world.Make_Identity();
	render_state_changed|=(unsigned)WORLD_CHANGED|(unsigned)WORLD_IDENTITY;
}

WWINLINE void DX8Wrapper::Set_View_Identity()
{
	if (render_state_changed&(unsigned)VIEW_IDENTITY) return;
	render_state.view.Make_Identity();
	render_state_changed|=(unsigned)VIEW_CHANGED|(unsigned)VIEW_IDENTITY;
}

WWINLINE bool DX8Wrapper::Is_World_Identity()
{
	return !!(render_state_changed&(unsigned)WORLD_IDENTITY);
}

WWINLINE bool DX8Wrapper::Is_View_Identity()
{
	return !!(render_state_changed&(unsigned)VIEW_IDENTITY);
}

void DX8Wrapper::Get_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4& m);

WWINLINE void DX8Wrapper::Set_Light(unsigned index, const D3DLIGHT8* light)
{
	if (light) {
		render_state.Lights[index]=*light;
		render_state.LightEnable[index]=true;
	}
	else {
		render_state.LightEnable[index]=false;
	}
	render_state_changed|=(LIGHT0_CHANGED<<index);
}

WWINLINE const D3DLIGHT8& DX8Wrapper::Peek_Light(unsigned index)
{
	return render_state.Lights[index];;
}

WWINLINE bool DX8Wrapper::Is_Light_Enabled(unsigned index)
{
	return render_state.LightEnable[index];
}


#endif

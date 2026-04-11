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
 *                     $Archive:: /Commando/Code/ww3d2/render2d.cpp                           $*
 *                                                                                             *
 *                       $Author:: Byon_g                                                                                                                                                                                  $Modtime:: 1/24/01 3:54p                                               $*
 *                                                                                             *
 *                    $Revision:: 46                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "render2d.h"
#include "mutex.h"
#include "ww3d.h"
#include "refcount.h"
#include "font3d.h"
#include "rect.h"
#include "texture.h"
#include "bgfxrenderer.h"
#include "dx8wrapper.h"
#include "wwprofile.h"
#include "wwmemlog.h"
#include "assetmgr.h"

#include <cstring>
#include <vector>

RectClass							Render2DClass::ScreenResolution( 0,0,0,0 );

namespace
{
struct Render2DVertex
{
	float X;
	float Y;
	float Z;
	float NX;
	float NY;
	float NZ;
	uint32_t Diffuse;
	uint32_t Specular;
	float U0;
	float V0;
	float U1;
	float V1;
};

unsigned Sanitize_Render2D_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state)
{
	unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, state);
	if (value != 0x12345678u) {
		return value;
	}

	switch (state) {
	case D3DTSS_COLOROP:
		return stage == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
	case D3DTSS_COLORARG0:
		return D3DTA_CURRENT;
	case D3DTSS_COLORARG1:
		return D3DTA_TEXTURE;
	case D3DTSS_COLORARG2:
		return D3DTA_CURRENT;
	case D3DTSS_ALPHAOP:
		return stage == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
	case D3DTSS_ALPHAARG0:
		return D3DTA_CURRENT;
	case D3DTSS_ALPHAARG1:
		return D3DTA_TEXTURE;
	case D3DTSS_ALPHAARG2:
		return D3DTA_CURRENT;
	default:
		return 0u;
	}
}

void Populate_Render2D_Fixed_Function_Inputs(BgfxRenderer::FixedFunctionShaderInputs &shader_inputs)
{
	const unsigned texture_factor = DX8Wrapper::Get_DX8_Render_State(D3DRS_TEXTUREFACTOR);
	shader_inputs.TextureFactor = texture_factor != 0x12345678u ? texture_factor : 0xffffffffu;
	shader_inputs.Stage0Color[0] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_COLOROP));
	shader_inputs.Stage0Color[1] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_COLORARG0));
	shader_inputs.Stage0Color[2] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_COLORARG1));
	shader_inputs.Stage0Color[3] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_COLORARG2));
	shader_inputs.Stage0Alpha[0] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_ALPHAOP));
	shader_inputs.Stage0Alpha[1] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_ALPHAARG0));
	shader_inputs.Stage0Alpha[2] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_ALPHAARG1));
	shader_inputs.Stage0Alpha[3] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(0, D3DTSS_ALPHAARG2));
	shader_inputs.Stage1Color[0] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_COLOROP));
	shader_inputs.Stage1Color[1] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_COLORARG0));
	shader_inputs.Stage1Color[2] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_COLORARG1));
	shader_inputs.Stage1Color[3] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_COLORARG2));
	shader_inputs.Stage1Alpha[0] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_ALPHAOP));
	shader_inputs.Stage1Alpha[1] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_ALPHAARG0));
	shader_inputs.Stage1Alpha[2] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_ALPHAARG1));
	shader_inputs.Stage1Alpha[3] = static_cast<float>(Sanitize_Render2D_Texture_Stage_State(1, D3DTSS_ALPHAARG2));
}
}


/*
** Render2DClass
*/
Render2DClass::Render2DClass( TextureClass* tex ) :
	CoordinateScale( 1, 1 ),
	CoordinateOffset( 0, 0 ),
	Texture(0),
	ZValue(0),
	IsHidden( false ),
	Indices(sizeof(PreAllocatedIndices)/sizeof(unsigned short),PreAllocatedIndices),
	Vertices(sizeof(PreAllocatedVertices)/sizeof(Vector2),PreAllocatedVertices),
	UVCoordinates(sizeof(PreAllocatedUVCoordinates)/sizeof(Vector2),PreAllocatedUVCoordinates),
	Colors(sizeof(PreAllocatedColors)/sizeof(unsigned long),PreAllocatedColors)
{
	Set_Texture( tex );	
   Shader = Get_Default_Shader();
	return ;
}

Render2DClass::~Render2DClass()
{
	REF_PTR_RELEASE(Texture);	
}

void	Render2DClass::Set_Screen_Resolution( const RectClass & screen )	
{ 
	ScreenResolution = screen; 
#if 0
	// Fool into pixel doubling  - Byon..
	if ( screen.Width() >= 1280 ) {
		ScreenResolution.Scale( 0.5f );
	}
//	ScreenResolution = RectClass( 0, 0, 800, 600 );
#endif
}

ShaderClass
Render2DClass::Get_Default_Shader( void )
{
	ShaderClass shader;

   shader.Set_Depth_Mask( ShaderClass::DEPTH_WRITE_DISABLE );
	shader.Set_Depth_Compare( ShaderClass::PASS_ALWAYS );	
	shader.Set_Dst_Blend_Func( ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA );
	shader.Set_Src_Blend_Func( ShaderClass::SRCBLEND_SRC_ALPHA );
	shader.Set_Fog_Func( ShaderClass::FOG_DISABLE );
	shader.Set_Primary_Gradient( ShaderClass::GRADIENT_MODULATE );
	shader.Set_Texturing( ShaderClass::TEXTURING_ENABLE );

	return shader;
}

void	Render2DClass::Reset(void)
{
	Vertices.Reset_Active();
	UVCoordinates.Reset_Active();
	Colors.Reset_Active();
	Indices.Reset_Active();

	Update_Bias(); // Keep the bias updated
}

void Render2DClass::Set_Texture(TextureClass* tex)
{
	REF_PTR_SET(Texture,tex);	
}

void Render2DClass::Set_Texture( const char * filename)
{
	TextureClass * tex = WW3DAssetManager::Get_Instance()->Get_Texture( filename, TextureClass::MIP_LEVELS_1 );
	Set_Texture( tex );
	if ( tex != NULL ) {
		SET_REF_OWNER( tex );
		tex->Release_Ref();
	}
}

void Render2DClass::Enable_Alpha(bool b)
{
	if (b) {
		Shader.Set_Dst_Blend_Func( ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA );
		Shader.Set_Src_Blend_Func( ShaderClass::SRCBLEND_SRC_ALPHA );
	}
	else {
		Shader.Set_Src_Blend_Func( ShaderClass::SRCBLEND_ONE);
		Shader.Set_Dst_Blend_Func( ShaderClass::DSTBLEND_ZERO );
	}
}

void Render2DClass::Enable_Additive(bool b)
{
	if (b) {
		Shader.Set_Dst_Blend_Func( ShaderClass::DSTBLEND_ONE );
		Shader.Set_Src_Blend_Func( ShaderClass::SRCBLEND_ONE );
	}
	else {
		Shader.Set_Src_Blend_Func( ShaderClass::SRCBLEND_ONE);
		Shader.Set_Dst_Blend_Func( ShaderClass::DSTBLEND_ZERO );
	}
}

void Render2DClass::Enable_Texturing(bool b)
{
	if (b) {
		Shader.Set_Texturing( ShaderClass::TEXTURING_ENABLE );
	}
	else {
		Shader.Set_Texturing( ShaderClass::TEXTURING_DISABLE );
	}
}

void	Render2DClass::Set_Coordinate_Range( const RectClass & range )
{
	// default range is (-1,1)-(1,-1)
	CoordinateScale.X = 2 / range.Width();
	CoordinateScale.Y = -2 / range.Height();
	CoordinateOffset.X = -(CoordinateScale.X * range.Left) - 1;
	CoordinateOffset.Y = -(CoordinateScale.Y * range.Top) + 1;

	Update_Bias();
}

void	  Render2DClass::Update_Bias( void )
{

	BiasedCoordinateOffset = CoordinateOffset;

	if ( WW3D::Is_Screen_UV_Biased() ) {	// Global bais setting
		Vector2 bais_add( -0.5f ,-0.5f );	// offset by -0.5,-0.5 in pixels

		// Convert from pixels to (-1,1)-(1,-1) units
		bais_add.X = bais_add.X / (Get_Screen_Resolution().Width() * 0.5f);
		bais_add.Y = bais_add.Y / (Get_Screen_Resolution().Height() * -0.5f);

		BiasedCoordinateOffset += bais_add;
	}
}

#if 0
Vector2 Render2DClass::Convert_Vert( const Vector2 & v ) 
{
	Vector2 out;

	// Convert to (-1,1)-(1,-1)
	out.X = v.X * CoordinateScale.X + CoordinateOffset.X;
	out.Y = v.Y * CoordinateScale.Y + CoordinateOffset.Y;

	// Convert to pixels 
	out.X = (out.X + 1.0f) * (Get_Screen_Resolution().Width() * 0.5f);
	out.Y = (out.Y - 1.0f) * (Get_Screen_Resolution().Height() * -0.5f);

	// Round to nearest pixel
	out.X = WWMath::Floor( out.X + 0.5f );
	out.Y = WWMath::Floor( out.Y + 0.5f );

	// Bias
	if ( WW3D::Is_Screen_UV_Biased() ) {	// Global bais setting
		out.X -= 0.5f;
		out.Y -= 0.5f;
	}


	// Convert back to (-1,1)-(1,-1)
	out.X = out.X / (Get_Screen_Resolution().Width() * 0.5f) - 1.0f;
	out.Y = out.Y / (Get_Screen_Resolution().Height() * -0.5f) + 1.0f;	

	return out;
}
#else
/*
** Convert Vert must convert from the convention defined by Set_Coordinate_Range
** into the convention (-1,1)-(1,-1), which is needed by the renderer.
// NOPE ** In addition, it rounds all coordinates off to the nearest pixel
** Also, it offsets the coordinates as need for Screen_UV_Bias
*/
void Render2DClass::Convert_Vert( Vector2 & vert_out, const Vector2 & vert_in )
{
	// Convert to (-1,1)-(1,-1)
	vert_out.X = vert_in.X * CoordinateScale.X + BiasedCoordinateOffset.X;
	vert_out.Y = vert_in.Y * CoordinateScale.Y + BiasedCoordinateOffset.Y;
}

void Render2DClass::Convert_Vert( Vector2 & vert_out, float x_in, float y_in )
{
	// Convert to (-1,1)-(1,-1)
	vert_out.X = x_in * CoordinateScale.X + BiasedCoordinateOffset.X;
	vert_out.Y = y_in * CoordinateScale.Y + BiasedCoordinateOffset.Y;
}

#endif

void	Render2DClass::Move( const Vector2 & move )	// Move all verts 
{
	Vector2 scaled_move;
	scaled_move.X = move.X * CoordinateScale.X;
	scaled_move.Y = move.Y * CoordinateScale.Y;
	for ( int i = 0; i < Vertices.Count(); i++ ) {
		Vertices[i] += scaled_move;
	}
}

void	Render2DClass::Force_Alpha( float alpha )		// Force all alphas 
{
	unsigned long a = (unsigned)(WWMath::Clamp( alpha, 0, 1 ) * 255.0f);
	a <<= 24;
	for ( int i = 0; i < Colors.Count(); i++ ) {
		Colors[i] = (Colors[i] & 0x00FFFFFF) | a;
	}
}


void	Render2DClass::Force_Color( int color )		// Force all alphas 
{
	for ( int i = 0; i < Colors.Count(); i++ ) {
		Colors[i] = color;
	}
}


/*
** Internal Add Quad Elements
** Caller must mutex lock
*/
void	Render2DClass::Internal_Add_Quad_Vertices( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3 )
{
	Convert_Vert( *Vertices.Uninitialized_Add(), v0 );
	Convert_Vert( *Vertices.Uninitialized_Add(), v1 );
	Convert_Vert( *Vertices.Uninitialized_Add(), v2 );
	Convert_Vert( *Vertices.Uninitialized_Add(), v3 );
}

void	Render2DClass::Internal_Add_Quad_Vertices( const RectClass & screen )
{
	Convert_Vert( *Vertices.Uninitialized_Add(), screen.Left,  screen.Top );
	Convert_Vert( *Vertices.Uninitialized_Add(), screen.Left,  screen.Bottom );
	Convert_Vert( *Vertices.Uninitialized_Add(), screen.Right, screen.Top );
	Convert_Vert( *Vertices.Uninitialized_Add(), screen.Right, screen.Bottom );

}

void	Render2DClass::Internal_Add_Quad_UVs( const RectClass & uv )
{
	Vector2* uvs;

	uvs=UVCoordinates.Uninitialized_Add();
	uvs->X = uv.Left;		uvs->Y = uv.Top;
	uvs=UVCoordinates.Uninitialized_Add();
	uvs->X = uv.Left;		uvs->Y = uv.Bottom;
	uvs=UVCoordinates.Uninitialized_Add();
	uvs->X = uv.Right;	uvs->Y = uv.Top;
	uvs=UVCoordinates.Uninitialized_Add();
	uvs->X = uv.Right;	uvs->Y = uv.Bottom;	

}

void	Render2DClass::Internal_Add_Quad_Colors( unsigned long color )
{
	unsigned long* colors;

	colors=Colors.Uninitialized_Add();
	*colors=color;
	colors=Colors.Uninitialized_Add();
	*colors=color;
	colors=Colors.Uninitialized_Add();
	*colors=color;
	colors=Colors.Uninitialized_Add();
	*colors=color;
}

void	Render2DClass::Internal_Add_Quad_VColors( unsigned long color1, unsigned long color2 )
{
	unsigned long* colors;

	colors=Colors.Uninitialized_Add();
	*colors=color1;
	colors=Colors.Uninitialized_Add();
	*colors=color2;
	colors=Colors.Uninitialized_Add();
	*colors=color1;
	colors=Colors.Uninitialized_Add();
	*colors=color2;

}

void	Render2DClass::Internal_Add_Quad_HColors( unsigned long color1, unsigned long color2 )
{
	unsigned long* colors;

	colors=Colors.Uninitialized_Add();
	*colors=color1;
	colors=Colors.Uninitialized_Add();
	*colors=color1;
	colors=Colors.Uninitialized_Add();
	*colors=color2;
	colors=Colors.Uninitialized_Add();
	*colors=color2;
}


void	Render2DClass::Internal_Add_Quad_Indicies( int start_vert_index, bool backfaced )
{
	unsigned short * indices;
	
	if (backfaced ^ (CoordinateScale.X * CoordinateScale.Y > 0)) {
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 1;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 0;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 2;

		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 1;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 2;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 3;
	} else {
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 0;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 1;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 2;

		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 2;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 1;
		indices=Indices.Uninitialized_Add();
		*indices = start_vert_index + 3;
	}

}


void	Render2DClass::Add_Quad( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, const RectClass & uv, unsigned long color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( v0, v1, v2, v3 );
	Internal_Add_Quad_UVs( uv );
	Internal_Add_Quad_Colors( color );
}

void	Render2DClass::Add_Quad_Backfaced( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, const RectClass & uv, unsigned long color )
{
	Internal_Add_Quad_Indicies( Vertices.Count(), true );
	Internal_Add_Quad_Vertices( v0, v1, v2, v3 );
	Internal_Add_Quad_UVs( uv );
	Internal_Add_Quad_Colors( color );
}

void	Render2DClass::Add_Quad_VGradient( const RectClass & screen, unsigned long top_color, unsigned long bottom_color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( screen );
	Internal_Add_Quad_UVs( RectClass( 0,0,1,1 ) );
	Internal_Add_Quad_VColors( top_color, bottom_color );
}

void	Render2DClass::Add_Quad_HGradient( const RectClass & screen, unsigned long left_color, unsigned long right_color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( screen );
	Internal_Add_Quad_UVs( RectClass( 0,0,1,1 ) );
	Internal_Add_Quad_HColors( left_color, right_color );
}


void	Render2DClass::Add_Quad( const RectClass & screen, const RectClass & uv, unsigned long color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( screen );
	Internal_Add_Quad_UVs( uv );
	Internal_Add_Quad_Colors( color );
}

void	Render2DClass::Add_Quad( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, unsigned long color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( v0, v1, v2, v3 );
	Internal_Add_Quad_UVs( RectClass( 0,0,1,1 ) );
	Internal_Add_Quad_Colors( color );
}

void	Render2DClass::Add_Quad( const RectClass & screen, unsigned long color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( screen );
	Internal_Add_Quad_UVs( RectClass( 0,0,1,1 ) );
	Internal_Add_Quad_Colors( color );
}

/*
** Add Tri
*/
void	Render2DClass::Add_Tri( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & uv0, const Vector2 & uv1, const Vector2 & uv2, unsigned long color )
{
	int old_vert_count = Vertices.Count();

	// Add the verticies (translated to new coordinates)
#if 0
	Vertices.Add( Convert_Vert( v0 ), new_vert_count );
	Vertices.Add( Convert_Vert( v1 ), new_vert_count );
	Vertices.Add( Convert_Vert( v2 ), new_vert_count );
#else
	Convert_Vert( *Vertices.Uninitialized_Add(), v0 );
	Convert_Vert( *Vertices.Uninitialized_Add(), v1 );
	Convert_Vert( *Vertices.Uninitialized_Add(), v2 );
	
#endif

	// Add the uv coordinates

	*UVCoordinates.Uninitialized_Add()=uv0;
	*UVCoordinates.Uninitialized_Add()=uv1;
	*UVCoordinates.Uninitialized_Add()=uv2;

	// Add the colors
	*Colors.Uninitialized_Add()=color;
	*Colors.Uninitialized_Add()=color;
	*Colors.Uninitialized_Add()=color;

	// Add the faces
	*Indices.Uninitialized_Add()=old_vert_count + 0;
	*Indices.Uninitialized_Add()=old_vert_count + 1;
	*Indices.Uninitialized_Add()=old_vert_count + 2;

}

void	Render2DClass::Add_Line( const Vector2 & a, const Vector2 & b, float width, unsigned long color )
{
	Add_Line( a, b, width, RectClass( 0,0,1,1 ), color );
}

void	Render2DClass::Add_Line( const Vector2 & a, const Vector2 & b, float width, const RectClass & uv, unsigned long color )
{
	Vector2	corner_offset = a - b;				// get line relative to b
	float temp = corner_offset.X;					// Rotate 90
	corner_offset.X = corner_offset.Y;
	corner_offset.Y = -temp;
	corner_offset.Normalize();						// scale to length width/2
	corner_offset *= width / 2;

	Add_Quad( a - corner_offset, a + corner_offset, b - corner_offset, b + corner_offset, uv, color );
}


void	Render2DClass::Add_Rect( const RectClass & rect, float border_width, uint32 border_color, uint32 fill_color )
{
	//
	//	First add the outline
	//
	Add_Outline( rect, border_width, border_color );

	//
	//	Next, fill the contents
	//
	RectClass fill_rect = rect;
	fill_rect.Right	-= border_width;
	fill_rect.Bottom	-= border_width;
	Add_Quad (fill_rect, fill_color);
	return ;
}

void	Render2DClass::Add_Outline( const RectClass & rect, float width, unsigned long color )
{
	Add_Outline( rect, width, RectClass( 0,0,1,1 ), color );
}

void	Render2DClass::Add_Outline( const RectClass & rect, float width, const RectClass & uv, unsigned long color )
{
	//
	//	Pretty straight forward, simply add the four side of the rectangle as lines.
	//
	Add_Line (Vector2 (rect.Left, rect.Bottom),		Vector2 (rect.Left, rect.Top - 1),		width, uv, color);
	Add_Line (Vector2 (rect.Left, rect.Top),		Vector2 (rect.Right, rect.Top),			width, uv, color);
	Add_Line (Vector2 (rect.Right, rect.Top),		Vector2 (rect.Right, rect.Bottom),		width, uv, color);
	Add_Line (Vector2 (rect.Right, rect.Bottom),	Vector2 (rect.Left - 1, rect.Bottom),	width, uv, color);	
}


void Render2DClass::Render(void)
{
	if ( !Indices.Count() || IsHidden || !BgfxRenderer::Is_Initted()) {
		return;
	}

	bgfx::ProgramHandle program = BgfxRenderer::Get_Fixed_Function_Program();
	if (!bgfx::isValid(program)) {
		return;
	}

	std::vector<Render2DVertex> submission_vertices(static_cast<size_t>(Vertices.Count()));
	for (int index = 0; index < Vertices.Count(); ++index) {
		Render2DVertex &vertex = submission_vertices[static_cast<size_t>(index)];
		vertex.X = Vertices[index].X;
		vertex.Y = Vertices[index].Y;
		vertex.Z = ZValue;
		vertex.NX = 0.0f;
		vertex.NY = 0.0f;
		vertex.NZ = 1.0f;
		vertex.Diffuse = BgfxRenderer::Convert_Packed_Color(static_cast<uint32_t>(Colors[index]));
		vertex.Specular = 0u;
		vertex.U0 = UVCoordinates[index].X;
		vertex.V0 = UVCoordinates[index].Y;
		vertex.U1 = 0.0f;
		vertex.V1 = 0.0f;
	}

	BgfxRenderer::Prepare_Overlay_View();

	bgfx::TextureHandle texture_handle = BgfxRenderer::Get_White_Texture();
	uint32_t sampler_flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;
	if (Shader.Get_Texturing() != ShaderClass::TEXTURING_DISABLE && Texture != NULL) {
		texture_handle = Texture->Get_Bgfx_Texture();
		sampler_flags = Texture->Get_Bgfx_Sampler_Flags(0);
		if (!bgfx::isValid(texture_handle)) {
			texture_handle = BgfxRenderer::Get_White_Texture();
		}
	}

	const bgfx::VertexLayout &layout = BgfxRenderer::Get_Fixed_Function_Layout();
	const uint32_t vertex_count = static_cast<uint32_t>(submission_vertices.size());
	const uint32_t index_count = static_cast<uint32_t>(Indices.Count());
	const bool can_use_transient =
		bgfx::getAvailTransientVertexBuffer(vertex_count, layout) == vertex_count &&
		bgfx::getAvailTransientIndexBuffer(index_count) == index_count;

	if (can_use_transient) {
		bgfx::TransientVertexBuffer vertex_buffer;
		bgfx::TransientIndexBuffer index_buffer;
		bgfx::allocTransientVertexBuffer(&vertex_buffer, vertex_count, layout);
		bgfx::allocTransientIndexBuffer(&index_buffer, index_count);

		memcpy(vertex_buffer.data, submission_vertices.data(), submission_vertices.size() * sizeof(Render2DVertex));
		memcpy(index_buffer.data, &Indices[0], static_cast<size_t>(index_count) * sizeof(unsigned short));

		bgfx::setVertexBuffer(0, &vertex_buffer);
		bgfx::setIndexBuffer(&index_buffer);
	} else {
		const bgfx::Memory *vertex_memory = bgfx::copy(
			submission_vertices.data(),
			static_cast<uint32_t>(submission_vertices.size() * sizeof(Render2DVertex)));
		const bgfx::Memory *index_memory = bgfx::copy(
			&Indices[0],
			static_cast<uint32_t>(index_count * sizeof(unsigned short)));
		bgfx::VertexBufferHandle vertex_buffer = bgfx::createVertexBuffer(vertex_memory, layout);
		bgfx::IndexBufferHandle index_buffer = bgfx::createIndexBuffer(index_memory);

		bgfx::setVertexBuffer(0, vertex_buffer);
		bgfx::setIndexBuffer(index_buffer);
		bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), texture_handle, sampler_flags);
		bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), BgfxRenderer::Get_White_Texture(), sampler_flags);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Apply_Render_State_Changes();
		BgfxRenderer::FixedFunctionShaderInputs shader_inputs;
		Populate_Render2D_Fixed_Function_Inputs(shader_inputs);
		if (Shader.Get_Texturing() == ShaderClass::TEXTURING_DISABLE || Texture == NULL) {
			shader_inputs.Stage0Color[0] = static_cast<float>(D3DTOP_SELECTARG1);
			shader_inputs.Stage0Color[1] = static_cast<float>(D3DTA_CURRENT);
			shader_inputs.Stage0Color[2] = static_cast<float>(D3DTA_CURRENT);
			shader_inputs.Stage0Color[3] = static_cast<float>(D3DTA_CURRENT);
			shader_inputs.Stage0Alpha[0] = static_cast<float>(D3DTOP_SELECTARG1);
			shader_inputs.Stage0Alpha[1] = static_cast<float>(D3DTA_CURRENT);
			shader_inputs.Stage0Alpha[2] = static_cast<float>(D3DTA_CURRENT);
			shader_inputs.Stage0Alpha[3] = static_cast<float>(D3DTA_CURRENT);
			shader_inputs.Stage1Color[0] = static_cast<float>(D3DTOP_DISABLE);
			shader_inputs.Stage1Alpha[0] = static_cast<float>(D3DTOP_DISABLE);
		}
		BgfxRenderer::Apply_Fixed_Function_Shader_Inputs(Shader, shader_inputs);
		bgfx::setState(BgfxRenderer::Build_Render_State(Shader));
		bgfx::submit(BgfxRenderer::Get_Overlay_View_Id(), program);

		bgfx::destroy(index_buffer);
		bgfx::destroy(vertex_buffer);
		return;
	}

	bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), texture_handle, sampler_flags);
	bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), BgfxRenderer::Get_White_Texture(), sampler_flags);
	DX8Wrapper::Set_Shader(Shader);
	DX8Wrapper::Apply_Render_State_Changes();
	BgfxRenderer::FixedFunctionShaderInputs shader_inputs;
	Populate_Render2D_Fixed_Function_Inputs(shader_inputs);
	if (Shader.Get_Texturing() == ShaderClass::TEXTURING_DISABLE || Texture == NULL) {
		shader_inputs.Stage0Color[0] = static_cast<float>(D3DTOP_SELECTARG1);
		shader_inputs.Stage0Color[1] = static_cast<float>(D3DTA_CURRENT);
		shader_inputs.Stage0Color[2] = static_cast<float>(D3DTA_CURRENT);
		shader_inputs.Stage0Color[3] = static_cast<float>(D3DTA_CURRENT);
		shader_inputs.Stage0Alpha[0] = static_cast<float>(D3DTOP_SELECTARG1);
		shader_inputs.Stage0Alpha[1] = static_cast<float>(D3DTA_CURRENT);
		shader_inputs.Stage0Alpha[2] = static_cast<float>(D3DTA_CURRENT);
		shader_inputs.Stage0Alpha[3] = static_cast<float>(D3DTA_CURRENT);
		shader_inputs.Stage1Color[0] = static_cast<float>(D3DTOP_DISABLE);
		shader_inputs.Stage1Alpha[0] = static_cast<float>(D3DTOP_DISABLE);
	}
	BgfxRenderer::Apply_Fixed_Function_Shader_Inputs(Shader, shader_inputs);
	bgfx::setState(BgfxRenderer::Build_Render_State(Shader));
	bgfx::submit(BgfxRenderer::Get_Overlay_View_Id(), program);
}


/*
** Render2DTextClass
*/
Render2DTextClass::Render2DTextClass(Font3DInstanceClass *font) :
	Location(0.0f,0.0f),
	Cursor(0.0f,0.0f),
	Font(NULL),
	WrapWidth(0),
	ClipRect(0, 0, 0, 0),
	IsClippedEnabled(false)
{
	Set_Coordinate_Range( RectClass( -320, -240, 320, 240 ) );
	Set_Font( font );
	
	Reset();
}

Render2DTextClass::~Render2DTextClass()
{
	REF_PTR_RELEASE(Font);
}

void	Render2DTextClass::Reset(void)
{
	Render2DClass::Reset();
	Cursor = Location;
	WrapWidth = 0;
	DrawExtents = RectClass( 0,0,0,0 );
	TotalExtents = RectClass( 0,0,0,0 );
	ClipRect.Set (0, 0, 0, 0);
	IsClippedEnabled = false;
}

void	Render2DTextClass::Set_Font( Font3DInstanceClass *font )
{
	REF_PTR_SET(Font,font);

	if ( Font != NULL ) {
		Set_Texture( Font->Peek_Texture() );

	#define	BLOCK_CHAR	0
		BlockUV = Font->Char_UV( BLOCK_CHAR );
		// Inset it a bit to be sure we have no edge problems
		BlockUV.Inflate( Vector2(-BlockUV.Width()/4, -BlockUV.Height()/4) );
	}
}


/*
**
*/
void	Render2DTextClass::Draw_Char( WCHAR ch, unsigned long color )
{
	float char_spacing	= Font->Char_Spacing( ch );
	float char_height		= Font->Char_Height();
	
	//
	//	Check to see if this character is clipped
	//
	bool is_clipped = false;
	if (	IsClippedEnabled &&
			(Cursor.X < ClipRect.Left ||
			 Cursor.X + char_spacing > ClipRect.Right ||
			 Cursor.Y < ClipRect.Top ||
			 Cursor.Y + char_height > ClipRect.Bottom))
	{
		is_clipped = true;
	}

	if ( ch != (WCHAR)' ' && !is_clipped ) {
		RectClass screen( Cursor.X, Cursor.Y, Cursor.X + Font->Char_Width(ch), Cursor.Y + char_height );

		Internal_Add_Quad_Indicies( Vertices.Count() );
		Internal_Add_Quad_Vertices( screen );
		Internal_Add_Quad_UVs( Font->Char_UV( ch ) );
		Internal_Add_Quad_Colors( color );

		DrawExtents += screen;
		TotalExtents += screen;
	}
	Cursor.X += char_spacing;
}

void	Render2DTextClass::Draw_Text( const char * text, unsigned long color )
{
	WWMEMLOG(MEM_GEOMETRY);
	WideStringClass wide(0,true);
	wide.Convert_From( text );
	Draw_Text( wide, color );
}

void	Render2DTextClass::Draw_Text( const WCHAR * text, unsigned long color )
{
	WWMEMLOG(MEM_GEOMETRY);

	// Reset the Extents
	DrawExtents = RectClass( Location, Location );
	if ( TotalExtents.Width() == 0 ) {
		TotalExtents = RectClass( Location, Location );
	}

	while (*text) {
		WCHAR ch = *text++;

		//	Check to see if we need to move to a newline or not
		bool wrap = ( ch == (WCHAR)'\n' );

		// if the current char is a space, and the next word length puts us past our Width, wrap
		if ( ch == (WCHAR)' ' && WrapWidth > 0 ) {
			const WCHAR * word = text;
			float word_width = Font->Char_Spacing(ch);
			while ( (*word != (WCHAR)0) && (*word > (WCHAR)' ') ) {
				word_width += Font->Char_Spacing(*word++);
			}
			wrap = ( (Cursor.X + word_width) >= (Location.X + WrapWidth) );
		}

		if ( wrap ) {
			Cursor.Y += Font->Char_Height();
			Cursor.X = Location.X;
		} else {

			// Draw char at cursor, update cursor and extents
			Draw_Char( ch, color );
		}
	}
}

void	Render2DTextClass::Draw_Block( const RectClass & screen, unsigned long color )
{
	Internal_Add_Quad_Indicies( Vertices.Count() );
	Internal_Add_Quad_Vertices( screen );
	Internal_Add_Quad_UVs( BlockUV );
	Internal_Add_Quad_Colors( color );

	TotalExtents += screen;
}

Vector2	Render2DTextClass::Get_Text_Extents( const WCHAR * text )
{
	Vector2 extent (0, Font->Char_Height());

	if (text) {
		while (*text) {
			WCHAR ch = *text++;

			if ( ch != (WCHAR)'\n' ) {
				extent.X += Font->Char_Spacing( ch );
			}
		}
	}

	return extent;
}

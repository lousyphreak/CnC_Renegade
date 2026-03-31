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
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   ShaderClass::Init_From_Material3 -- Initialize a shader from a Material3 structure        *
 *   ShaderClass::Apply -- Apply the renderstates for this shader (bgfx implementation)        *
 *   ShaderClass::Invert_Backface_Culling -- Globally invert the sense of all backface culling *
 *   ShaderClass::Is_Backface_Culling_Inverted -- Is backface culling inverted?                *
 *   ShaderClass::Get_SS_Category -- Helper function for static sort system                    *
 *   ShaderClass::Guess_Sort_Level -- Guess the static sort level                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "shader.h"
#include "w3d_file.h"
#include "wwdebug.h"

namespace {

bool g_backface_culling_inverted = false;

}

bool ShaderClass::ShaderDirty = true;
unsigned long ShaderClass::CurrentShader = 0;


/*
** Definitions of the preset shaders:
*/

// Texturing, zbuffer, primary gradient, no blending
#define SC_OPAQUE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_ENABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_MODULATE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetOpaqueShader(SC_OPAQUE);

// Texturing, zbuffer, disabled zbuffer write, primary gradient, additive blending
#define SC_ADDITIVE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ONE, FOG_DISABLE, GRADIENT_MODULATE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAdditiveShader(SC_ADDITIVE);

// Texturing, zbuffer, disabled zbuffer write, primary gradient, additive blending, bumpenvmap
#define SC_BUMPENVMAP ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ONE, FOG_DISABLE, GRADIENT_BUMPENVMAP, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_ADD, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetBumpenvmapShader(SC_BUMPENVMAP);

// Texturing, zbuffer, disabled zbuffer write, primary gradient, alpha blending
#define SC_ALPHA ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_SRC_ALPHA, \
	DSTBLEND_ONE_MINUS_SRC_ALPHA, FOG_DISABLE, GRADIENT_MODULATE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAlphaShader(SC_ALPHA);

// Texturing, zbuffer, disabled zbuffer write, primary gradient, multiplicative blending
#define SC_MULTIPLICATIVE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ZERO, \
	DSTBLEND_SRC_COLOR, FOG_DISABLE, GRADIENT_MODULATE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetMultiplicativeShader(SC_MULTIPLICATIVE);

// Texturing, no zbuffer reading/writing, no gradients, no blending, no
// fogging - mostly for opaque 2D objects.
#define SC_OP_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetOpaque2DShader(SC_OP_2D);

// Texturing, default zbuffer reading, no zbuffer writing, no gradients, no blending, no
// fogging - mostly for opaque sprite objects.
#define SC_OP_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetOpaqueSpriteShader(SC_OP_SPRITE);

// Texturing, no zbuffer reading/writing, no gradients, additive blending,
// no fogging - mostly for additive 2D objects.
#define SC_ADD_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ONE, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAdditive2DShader(SC_ADD_2D);

// Texturing, no zbuffer reading/writing, no gradients, alpha blending, no
// fogging - mostly for alpha-blended 2D objects.
#define SC_ALPHA_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_SRC_ALPHA, DSTBLEND_ONE_MINUS_SRC_ALPHA, FOG_DISABLE, GRADIENT_DISABLE, \
	SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAlpha2DShader(SC_ALPHA_2D);

// Texturing, default zbuffer reading, no zbuffer writing, no gradients,
// additive blending, no fogging - mostly for use in additive sprite objects.
#define SC_ADD_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ONE, DSTBLEND_ONE, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_ENABLE, ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAdditiveSpriteShader(SC_ADD_SPRITE);

// Texturing, default zbuffer reading, no zbuffer writing, no gradients,
// alpha blending, no fogging - mostly for use in alpha-blended sprite objects.
#define SC_ALPHA_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_SRC_ALPHA, DSTBLEND_ONE_MINUS_SRC_ALPHA, FOG_DISABLE, GRADIENT_DISABLE, \
	SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAlphaSpriteShader(SC_ALPHA_SPRITE);

// No texturing, default zbuffer reading/writing, primary gradient, no
// blending, no fogging - mostly for use in solid-colored opaque objects.
#define SC_OPAQUE_SOLID ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_ENABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ONE, DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_MODULATE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_DISABLE, ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetOpaqueSolidShader(SC_OPAQUE_SOLID);

// No texturing, default zbuffer reading, no zbuffer writing, primary
// gradient, additive blending, no fogging - mostly for use in solid-colored
// additive objects.
#define SC_ADDITIVE_SOLID ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ONE, DSTBLEND_ONE, FOG_DISABLE, GRADIENT_MODULATE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_DISABLE, ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAdditiveSolidShader(SC_ADDITIVE_SOLID);

// No texturing, default zbuffer reading, no zbuffer writing, primary
// gradient, alpha blending, no fogging - mostly for use in solid-colored
// alpha objects.
#define SC_ALPHA_SOLID ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_SRC_ALPHA, DSTBLEND_ONE_MINUS_SRC_ALPHA, FOG_DISABLE, GRADIENT_MODULATE, \
	SECONDARY_GRADIENT_DISABLE, TEXTURING_DISABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetAlphaSolidShader(SC_ALPHA_SOLID);

// Texturing, no zbuffer reading/writing, no gradients, no blending, alpha
// testing, no fogging - mostly for "pure" alpha-tested 2D objects.
#define SC_ATEST_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ONE, DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_ENABLE, ALPHATEST_ENABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetATest2DShader(SC_ATEST_2D);

// Texturing, default zbuffer reading and writing, no gradients, no
// blending, alpha testing, no fogging - mostly for "pure" alpha-tested
// sprite objects.
#define SC_ATEST_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_ENABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ONE, DSTBLEND_ZERO, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_ENABLE, ALPHATEST_ENABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetATestSpriteShader(SC_ATEST_SPRITE);

// Texturing, no zbuffer reading/writing, no gradients, alpha blending AND
// alpha testing, no fogging - mostly for alpha-tested and blended 2D objects.
#define SC_ATESTBLEND_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_SRC_ALPHA, DSTBLEND_ONE_MINUS_SRC_ALPHA, FOG_DISABLE, GRADIENT_DISABLE, \
	SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_ENABLE, CULL_MODE_ENABLE, DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetATestBlend2DShader(SC_ATESTBLEND_2D);

// Texturing, default zbuffer reading and writing, no gradients, alpha
// blending AND alpha testing, no fogging - mostly for use in alpha-tested
// and blended sprite objects.
#define SC_ATESTBLEND_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_ENABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_SRC_ALPHA, DSTBLEND_ONE_MINUS_SRC_ALPHA, FOG_DISABLE, GRADIENT_DISABLE, \
	SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_ENABLE, CULL_MODE_ENABLE, DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetATestBlendSpriteShader(SC_ATESTBLEND_SPRITE);

// Texturing, no zbuffer reading/writing, no gradients, screen blending,
// no fogging - mostly for screen-blended 2D objects.
#define SC_SCREEN_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ONE, \
	DSTBLEND_ONE_MINUS_SRC_COLOR, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_ENABLE, ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetScreen2DShader(SC_SCREEN_2D);

// Texturing, default zbuffer reading, no zbuffer writing, no gradients,
// screen blending, no fogging - mostly for use in screen-blended sprite objects.
#define SC_SCREEN_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ONE, DSTBLEND_ONE_MINUS_SRC_COLOR, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_ENABLE, ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetScreenSpriteShader(SC_SCREEN_SPRITE);

// Texturing, no zbuffer reading/writing, no gradients, multiplicative
// blending, no fogging - mostly for multiplicatively blended 2D objects.
#define SC_MUL_2D ( SHADE_CNST(PASS_ALWAYS, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, SRCBLEND_ZERO, \
	DSTBLEND_SRC_COLOR, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, TEXTURING_ENABLE, \
	ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetMultiplicative2DShader(SC_MUL_2D);

// Texturing, default zbuffer reading, no zbuffer writing, no gradients,
// multiplicative blending, no fogging - mostly for use in multiplicatively
// blended sprite objects.
#define SC_MUL_SPRITE ( SHADE_CNST(PASS_LEQUAL, DEPTH_WRITE_DISABLE, COLOR_WRITE_ENABLE, \
	SRCBLEND_ZERO, DSTBLEND_SRC_COLOR, FOG_DISABLE, GRADIENT_DISABLE, SECONDARY_GRADIENT_DISABLE, \
	TEXTURING_ENABLE, ALPHATEST_DISABLE, CULL_MODE_ENABLE, \
	DETAILCOLOR_DISABLE, DETAILALPHA_DISABLE) )
ShaderClass ShaderClass::_PresetMultiplicativeSpriteShader(SC_MUL_SPRITE);


/***********************************************************************************************
 * ShaderClass::Init_From_Material3 -- Initialize a shader from a Material3 structure          *
 *=============================================================================================*/
void ShaderClass::Init_From_Material3(const W3dMaterial3Struct & mat3)
{
	if ( mat3.Attributes & W3DMATERIAL_USE_ALPHA ) {
		Set_Depth_Mask( DEPTH_WRITE_DISABLE );
		Set_Dst_Blend_Func( DSTBLEND_ONE_MINUS_SRC_ALPHA );
		Set_Src_Blend_Func( SRCBLEND_SRC_ALPHA );
	}
}


/***********************************************************************************************
 * ShaderClass::Enable_Fog -- Turn on fog for this shader.                                     *
 *                                                                                             *
 * Enable most appropriate fog mode (FOG_ENABLE, FOG_SCALE_FRAGMENT or FOG_WHITE) for given    *
 * source and destination blends.                                                              *
 *=============================================================================================*/
void ShaderClass::Enable_Fog(const char *source)
{
	switch (Get_Src_Blend_Func()) {

		case ShaderClass::SRCBLEND_ZERO:
			if (Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_SRC_COLOR) {
				Set_Fog_Func (ShaderClass::FOG_WHITE);
			} else {
				Report_Unable_To_Fog (source);
			}
			break;

		case ShaderClass::SRCBLEND_ONE:
			switch (Get_Dst_Blend_Func()) {

				case ShaderClass::DSTBLEND_ZERO:
					Set_Fog_Func (ShaderClass::FOG_ENABLE);
					break;

				case ShaderClass::DSTBLEND_ONE:
				case ShaderClass::DSTBLEND_ONE_MINUS_SRC_COLOR:
					Set_Fog_Func (ShaderClass::FOG_SCALE_FRAGMENT);
					break;

				default:
					Report_Unable_To_Fog (source);
					break;
			}
			break;

		case ShaderClass::SRCBLEND_SRC_ALPHA:
			if (Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA) {
				Set_Fog_Func (ShaderClass::FOG_ENABLE);
			} else {
				Report_Unable_To_Fog (source);
			}
			break;

		case ShaderClass::SRCBLEND_ONE_MINUS_SRC_ALPHA:
			if (Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_SRC_ALPHA) {
				Set_Fog_Func (ShaderClass::FOG_ENABLE);
			} else {
				Report_Unable_To_Fog (source);
			}
			break;
	}
}


/***********************************************************************************************
 * ShaderClass::Report_Unable_To_Fog --                                                        *
 *=============================================================================================*/
void ShaderClass::Report_Unable_To_Fog(const char *source)
{
	#ifdef WWDEBUG
	static unsigned _reportcount = 0;

	const char		*unabletofogtext		= "WARNING: Unable to fog shader in %s with given blending mode.\r\n";
	const char		*unabletofogmoretext = "WARNING: Unable to fog additional shaders (further warnings will be suppressed).\r\n";
	const unsigned  maxreportcount		= 10;

	if (_reportcount < maxreportcount) {
		WWDEBUG_SAY ((unabletofogtext, source));
		_reportcount++;
	} else {
		if (_reportcount == maxreportcount) {
			WWDEBUG_SAY ((unabletofogmoretext));
			_reportcount++;
		}
	}
	#endif
}


/***********************************************************************************************
 * ShaderClass::Apply -- Apply the renderstates for this shader (bgfx implementation)          *
 *                                                                                             *
 * In the bgfx rendering path, the actual GPU state is computed at draw time by                *
 * Build_BGFX_State() in dx8wrapper.cpp, which reads directly from the ShaderClass             *
 * stored in g_bgfx.shader (set via DX8Wrapper::Set_Shader()). Apply() just needs              *
 * to track dirty state for consistency with the D3D8 code path's expectations.                *
 *=============================================================================================*/
void ShaderClass::Apply()
{
	unsigned long diff;

	if (ShaderDirty) {
		diff = 0xffffffff;
	} else {
		diff = CurrentShader ^ ShaderBits;
	}

	if (!diff) return;

	CurrentShader = ShaderBits;
	ShaderDirty = false;
}


/***********************************************************************************************
 * ShaderClass::Invert_Backface_Culling -- Globally invert the sense of all backface culling   *
 *=============================================================================================*/
void ShaderClass::Invert_Backface_Culling(bool onoff)
{
	g_backface_culling_inverted = onoff;
	Invalidate();
}


/***********************************************************************************************
 * ShaderClass::Is_Backface_Culling_Inverted -- Is backface culling inverted?                  *
 *=============================================================================================*/
bool ShaderClass::Is_Backface_Culling_Inverted(void)
{
	return g_backface_culling_inverted;
}


/***********************************************************************************************
 * ShaderClass::Get_SS_Category -- Helper function for static sort system                      *
 *=============================================================================================*/
ShaderClass::StaticSortCategoryType ShaderClass::Get_SS_Category(void) const
{
	// category: Opaque
	if ( (ALPHATEST_DISABLE==Get_Alpha_Test()) && (DSTBLEND_ZERO==Get_Dst_Blend_Func()) )
		return SSCAT_OPAQUE;
	// category: Alpha Test
	if (ALPHATEST_ENABLE==Get_Alpha_Test())	{
		if (DSTBLEND_ZERO==Get_Dst_Blend_Func()) return SSCAT_ALPHA_TEST;
		if ( (SRCBLEND_SRC_ALPHA==Get_Src_Blend_Func()) &&
			  (DSTBLEND_ONE_MINUS_SRC_ALPHA==Get_Dst_Blend_Func()) )
			  return SSCAT_ALPHA_TEST;
	}
	// category: Additive
	if ( (SRCBLEND_ONE==Get_Src_Blend_Func()) && (DSTBLEND_ONE==Get_Dst_Blend_Func()) )
		return SSCAT_ADDITIVE;
	// category: Everything else
	return SSCAT_OTHER;
}


/***********************************************************************************************
 * ShaderClass::Guess_Sort_Level -- Guess the static sort level                                *
 *=============================================================================================*/
int ShaderClass::Guess_Sort_Level(void) const
{
	int sort_level;
	StaticSortCategoryType scat = Get_SS_Category();

	switch (scat)
	{
	case ShaderClass::SSCAT_OPAQUE:
	case ShaderClass::SSCAT_ALPHA_TEST:
		sort_level = SORT_LEVEL_NONE;
		break;
	case ShaderClass::SSCAT_ADDITIVE:
		sort_level = SORT_LEVEL_BIN3;
		break;
	default:
		sort_level = SORT_LEVEL_BIN1;
		break;
	}
	return sort_level;
}

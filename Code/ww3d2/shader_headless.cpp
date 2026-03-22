#include "shader.h"

namespace {

bool g_backface_culling_inverted = false;

}

bool ShaderClass::ShaderDirty = true;
unsigned long ShaderClass::CurrentShader = 0;

#define SC_OPAQUE ( SHADE_CNST(ShaderClass::PASS_LEQUAL, ShaderClass::DEPTH_WRITE_ENABLE, ShaderClass::COLOR_WRITE_ENABLE, ShaderClass::SRCBLEND_ONE, ShaderClass::DSTBLEND_ZERO, ShaderClass::FOG_DISABLE, ShaderClass::GRADIENT_MODULATE, ShaderClass::SECONDARY_GRADIENT_DISABLE, ShaderClass::TEXTURING_ENABLE, ShaderClass::ALPHATEST_DISABLE, ShaderClass::CULL_MODE_ENABLE, ShaderClass::DETAILCOLOR_DISABLE, ShaderClass::DETAILALPHA_DISABLE) )
#define SC_ADDITIVE ( SHADE_CNST(ShaderClass::PASS_LEQUAL, ShaderClass::DEPTH_WRITE_DISABLE, ShaderClass::COLOR_WRITE_ENABLE, ShaderClass::SRCBLEND_ONE, ShaderClass::DSTBLEND_ONE, ShaderClass::FOG_DISABLE, ShaderClass::GRADIENT_MODULATE, ShaderClass::SECONDARY_GRADIENT_DISABLE, ShaderClass::TEXTURING_ENABLE, ShaderClass::ALPHATEST_DISABLE, ShaderClass::CULL_MODE_ENABLE, ShaderClass::DETAILCOLOR_DISABLE, ShaderClass::DETAILALPHA_DISABLE) )
#define SC_ALPHA ( SHADE_CNST(ShaderClass::PASS_LEQUAL, ShaderClass::DEPTH_WRITE_DISABLE, ShaderClass::COLOR_WRITE_ENABLE, ShaderClass::SRCBLEND_SRC_ALPHA, ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA, ShaderClass::FOG_DISABLE, ShaderClass::GRADIENT_MODULATE, ShaderClass::SECONDARY_GRADIENT_DISABLE, ShaderClass::TEXTURING_ENABLE, ShaderClass::ALPHATEST_DISABLE, ShaderClass::CULL_MODE_ENABLE, ShaderClass::DETAILCOLOR_DISABLE, ShaderClass::DETAILALPHA_DISABLE) )
#define SC_MULT ( SHADE_CNST(ShaderClass::PASS_LEQUAL, ShaderClass::DEPTH_WRITE_DISABLE, ShaderClass::COLOR_WRITE_ENABLE, ShaderClass::SRCBLEND_ZERO, ShaderClass::DSTBLEND_SRC_COLOR, ShaderClass::FOG_DISABLE, ShaderClass::GRADIENT_MODULATE, ShaderClass::SECONDARY_GRADIENT_DISABLE, ShaderClass::TEXTURING_ENABLE, ShaderClass::ALPHATEST_DISABLE, ShaderClass::CULL_MODE_ENABLE, ShaderClass::DETAILCOLOR_DISABLE, ShaderClass::DETAILALPHA_DISABLE) )
#define SC_2D ( SHADE_CNST(ShaderClass::PASS_ALWAYS, ShaderClass::DEPTH_WRITE_DISABLE, ShaderClass::COLOR_WRITE_ENABLE, ShaderClass::SRCBLEND_ONE, ShaderClass::DSTBLEND_ZERO, ShaderClass::FOG_DISABLE, ShaderClass::GRADIENT_DISABLE, ShaderClass::SECONDARY_GRADIENT_DISABLE, ShaderClass::TEXTURING_ENABLE, ShaderClass::ALPHATEST_DISABLE, ShaderClass::CULL_MODE_ENABLE, ShaderClass::DETAILCOLOR_DISABLE, ShaderClass::DETAILALPHA_DISABLE) )

ShaderClass ShaderClass::_PresetOpaqueShader(SC_OPAQUE);
ShaderClass ShaderClass::_PresetAdditiveShader(SC_ADDITIVE);
ShaderClass ShaderClass::_PresetBumpenvmapShader(SC_ADDITIVE);
ShaderClass ShaderClass::_PresetAlphaShader(SC_ALPHA);
ShaderClass ShaderClass::_PresetMultiplicativeShader(SC_MULT);
ShaderClass ShaderClass::_PresetOpaque2DShader(SC_2D);
ShaderClass ShaderClass::_PresetOpaqueSpriteShader(SC_2D);
ShaderClass ShaderClass::_PresetAdditive2DShader(SC_ADDITIVE);
ShaderClass ShaderClass::_PresetAlpha2DShader(SC_ALPHA);
ShaderClass ShaderClass::_PresetAdditiveSpriteShader(SC_ADDITIVE);
ShaderClass ShaderClass::_PresetAlphaSpriteShader(SC_ALPHA);
ShaderClass ShaderClass::_PresetOpaqueSolidShader(SC_OPAQUE);
ShaderClass ShaderClass::_PresetAdditiveSolidShader(SC_ADDITIVE);
ShaderClass ShaderClass::_PresetAlphaSolidShader(SC_ALPHA);
ShaderClass ShaderClass::_PresetATest2DShader(SC_2D);
ShaderClass ShaderClass::_PresetATestSpriteShader(SC_OPAQUE);
ShaderClass ShaderClass::_PresetATestBlend2DShader(SC_ALPHA);
ShaderClass ShaderClass::_PresetATestBlendSpriteShader(SC_ALPHA);
ShaderClass ShaderClass::_PresetScreen2DShader(SC_ADDITIVE);
ShaderClass ShaderClass::_PresetScreenSpriteShader(SC_ADDITIVE);

void ShaderClass::Apply() {}
void ShaderClass::Init_From_Material3(const W3dMaterial3Struct &) {}
void ShaderClass::Enable_Fog(const char *) { if (Get_Fog_Func() == FOG_DISABLE) { Set_Fog_Func(FOG_ENABLE); } }
ShaderClass::StaticSortCategoryType ShaderClass::Get_SS_Category(void) const
{
	if (Get_Alpha_Test() != ALPHATEST_DISABLE) return SSCAT_ALPHA_TEST;
	if (Uses_Alpha()) return SSCAT_OTHER;
	if (Get_Dst_Blend_Func() == DSTBLEND_ONE && Get_Src_Blend_Func() == SRCBLEND_ONE) return SSCAT_ADDITIVE;
	return SSCAT_OPAQUE;
}
int ShaderClass::Guess_Sort_Level(void) const { return Get_SS_Category() == SSCAT_OPAQUE ? 0 : 1; }
void ShaderClass::Invert_Backface_Culling(bool onoff) { g_backface_culling_inverted = onoff; }
bool ShaderClass::Is_Backface_Culling_Inverted(void) { return g_backface_culling_inverted; }

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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/pscene_projectors.cpp                 $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 1/11/02 3:12p                                               $*
 *                                                                                             *
 *                    $Revision:: 50                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "pscene.h"
#include "colmathaabox.h"
#include "rinfo.h"
#include "staticaabtreecull.h"
#include "dynamicaabtreecull.h"
#include "physgridcull.h"
#include "lightcull.h"
#include "staticphys.h"
#include "refcount.h"
#include "camera.h"
#include "vertmaterial.h"
#include "wwprofile.h"
#include "texture.h"
#include "ww3d.h"
#include "shadowmap.h"
#include "vertexformat.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "pot.h"
#include "wwmemlog.h"
#include "vertmaterial.h"

const int		SHADOW_CLIP_FAR							= 500;

const int		DEFAULT_MAX_DYNAMIC_SHADOWS			= 6;
const int		DEFAULT_DYNAMIC_SHADOW_RESOLUTION	= 256;

/**
** DynamicShadowTexMgrClass
** This class manages a pool of render target textures which are shared by the currently
** active shadow projectors.
*/
class DynamicShadowTexMgrClass
{
public:

	DynamicShadowTexMgrClass(void);
	virtual ~DynamicShadowTexMgrClass(void);
//	void					Reset(); // Jani: Disabling reset. Re-allocating render targets if the device is out of
										// free texture memory causes problems on at least TNT2.

	void					Set_Max_Simultaneous_Shadows(uint32_t max);
	uint32_t		Get_Max_Simultaneous_Shadows(void);

	void					Set_Shadow_Resolution(uint32_t size);
	uint32_t		Get_Shadow_Resolution(void);

	void					Per_Frame_Reset(void);
	void					Assign_Render_Target_Texture(TexProjectClass * tex_proj);

private:

	TextureClass *		Allocate_Render_Target_Texture(void);

	uint32_t		CurShadow;
	uint32_t		TextureResolution;
	SimpleVecClass<TextureClass *>	ShadowTextures;

};


/*
**
** Instantiate the Shadow Texture Managers.
**
*/
static DynamicShadowTexMgrClass		_DynamicShadowTexMgr;


static TextureClass* Create_Projector_Render_Target(unsigned w,unsigned h)
{
	static const WW3DFormat preferred_formats[] = {
		WW3D_FORMAT_R5G6B5,
		WW3D_FORMAT_A4R4G4B4,
		WW3D_FORMAT_X1R5G5B5,
		WW3D_FORMAT_A1R5G5B5,
		WW3D_FORMAT_A8R8G8B8
	};

	for (unsigned int i = 0; i < sizeof(preferred_formats) / sizeof(preferred_formats[0]); ++i) {
		TextureClass *texture = WW3D::Create_Render_Target_Texture(w, h, preferred_formats[i]);
		if (texture != NULL) {
			return texture;
		}
	}

	return NULL;
}

/************************************************************************************
**
** DynamicShadowTexMgrClass Implementation
**
************************************************************************************/
DynamicShadowTexMgrClass::DynamicShadowTexMgrClass(void) :
	CurShadow(0),
	TextureResolution(DEFAULT_DYNAMIC_SHADOW_RESOLUTION)
{
	WWMEMLOG(MEM_GAMEDATA);

	ShadowTextures.Resize(DEFAULT_MAX_DYNAMIC_SHADOWS);
	for (int i=0; i<ShadowTextures.Length(); i++) {
		ShadowTextures[i] = NULL;
	}
}

DynamicShadowTexMgrClass::~DynamicShadowTexMgrClass(void)
{
//	Reset();
	for (int i=0; i<ShadowTextures.Length(); i++) {
		REF_PTR_RELEASE(ShadowTextures[i]);
	}
}
/*
void DynamicShadowTexMgrClass::Reset(void)
{
	for (int i=0; i<ShadowTextures.Length(); i++) {
		REF_PTR_RELEASE(ShadowTextures[i]);
	}
}
*/

// Set the maximum number of dynamic render targets. Allocate all
// textures at this point!
void DynamicShadowTexMgrClass::Set_Max_Simultaneous_Shadows(uint32_t max)
{
	int curlen = ShadowTextures.Length();
	for (int i=max;i<curlen; i++) {
		REF_PTR_RELEASE(ShadowTextures[i]);
	}
	ShadowTextures.Resize(max);

	if (curlen>ShadowTextures.Length()) curlen=ShadowTextures.Length();
	int i = 0;
	for (; i<curlen; i++) {
		if (!ShadowTextures[i]) {
			ShadowTextures[i] = Allocate_Render_Target_Texture();
		}
	}
	for (; i<ShadowTextures.Length(); i++) {
		ShadowTextures[i] = Allocate_Render_Target_Texture();
	}
}

uint32_t DynamicShadowTexMgrClass::Get_Max_Simultaneous_Shadows(void)
{
	return ShadowTextures.Length();
}

void DynamicShadowTexMgrClass::Set_Shadow_Resolution(uint32_t res)
{
	uint32_t oksize = ::Find_POT(res);
	if (oksize > 256) {
		oksize = 256;
	}
	if (oksize < 16) {
		oksize = 16;
	}

	if (oksize != TextureResolution) {

		int i;
		for (i=0; i<ShadowTextures.Length(); i++) {
			REF_PTR_RELEASE(ShadowTextures[i]);
		}

		TextureResolution = oksize;

		for (i=0; i<ShadowTextures.Length(); i++) {
			ShadowTextures[i] = Allocate_Render_Target_Texture();
		}
	}
}

uint32_t DynamicShadowTexMgrClass::Get_Shadow_Resolution(void)
{
	return TextureResolution;
}

void DynamicShadowTexMgrClass::Per_Frame_Reset(void)
{
	CurShadow = 0;
}

void DynamicShadowTexMgrClass::Assign_Render_Target_Texture(TexProjectClass * tex_proj)
{
	if (CurShadow < (uint32_t)ShadowTextures.Length()) {
		if (ShadowTextures[CurShadow] == NULL) {
			ShadowTextures[CurShadow] = Allocate_Render_Target_Texture();
		}
		tex_proj->Set_Render_Target(ShadowTextures[CurShadow]);
		CurShadow++;
	}
}

TextureClass * DynamicShadowTexMgrClass::Allocate_Render_Target_Texture(void)
{
	TextureClass * texture=Create_Projector_Render_Target(TextureResolution,TextureResolution);

	if (texture != NULL) {

		SET_REF_OWNER(texture);
		texture->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		texture->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
	}
	return texture;
}




/************************************************************************************
**
** PhysicsSceneClass Texture Projection Code
**
************************************************************************************/
void PhysicsSceneClass::Release_Projector_Resources(void)
{
	if (ShadowRenderContext != NULL) {
		delete ShadowRenderContext;
		ShadowRenderContext=NULL;
	}

	REF_PTR_RELEASE(ShadowMaterialPass);
	REF_PTR_RELEASE(ShadowCamera);
	REF_PTR_RELEASE(ShadowBlobTexture);
//	_DynamicShadowTexMgr.Reset();
}


void PhysicsSceneClass::Set_Shadow_Resolution(uint32_t res)
{
	_DynamicShadowTexMgr.Set_Shadow_Resolution(res);
}

uint32_t PhysicsSceneClass::Get_Shadow_Resolution(void)
{
	return _DynamicShadowTexMgr.Get_Shadow_Resolution();
}

void PhysicsSceneClass::Set_Max_Simultaneous_Shadows(uint32_t count)
{
	_DynamicShadowTexMgr.Set_Max_Simultaneous_Shadows(count);
}

uint32_t PhysicsSceneClass::Get_Max_Simultaneous_Shadows(void)
{
	return _DynamicShadowTexMgr.Get_Max_Simultaneous_Shadows();
}


ShadowRenderInfoClass *
PhysicsSceneClass::Get_Shadow_Render_Context(int width,int height)
{
	if (ShadowRenderContext == NULL) {
		/*
		** Create a camera for shadow rendering to use
		*/
		if (ShadowCamera == NULL) {
			ShadowCamera = NEW_REF(CameraClass,());
			ShadowCamera->Set_Clip_Planes(0.2f,SHADOW_CLIP_FAR);
			ShadowCamera->Set_View_Plane(DEG_TO_RAD(90.0f),DEG_TO_RAD(90.0f));
			ShadowCamera->Set_Viewport(Vector2(0,0),Vector2(1,1));
		}

		/*
		** Create the render context
		*/
		ShadowRenderContext = new ShadowRenderInfoClass(*ShadowCamera);
	}

	return ShadowRenderContext;
}

MaterialPassClass * PhysicsSceneClass::Get_Shadow_Material_Pass(void)
{
	if (ShadowMaterialPass == NULL) {

		VertexMaterialClass * vmtl = NEW_REF(VertexMaterialClass,());
		vmtl->Set_Ambient(0,0,0);
		vmtl->Set_Diffuse(0,0,0);
		vmtl->Set_Specular(0,0,0);
		vmtl->Set_Emissive(0,0,0);
		vmtl->Set_Lighting(true);

		ShaderClass shader = ShaderClass::_PresetOpaqueShader;
		shader.Set_Depth_Compare(ShaderClass::PASS_ALWAYS);
		shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
		shader.Set_Texturing(ShaderClass::TEXTURING_DISABLE);

		ShadowMaterialPass = NEW_REF(MaterialPassClass,());
		ShadowMaterialPass->Set_Material(vmtl);
		ShadowMaterialPass->Set_Shader(shader);
		ShadowMaterialPass->Enable_On_Translucent_Meshes(false);

		REF_PTR_RELEASE(vmtl);
	}

	ShadowMaterialPass->Add_Ref();
	return ShadowMaterialPass;
}

void PhysicsSceneClass::Set_Shadow_Mode(ShadowEnum shadow_mode)
{
	if (((int)shadow_mode >= 0) && ((int)shadow_mode < SHADOW_MODE_COUNT)) {
		ShadowEnum resolved_mode = (shadow_mode == SHADOW_MODE_NONE) ? SHADOW_MODE_NONE : SHADOW_MODE_HARDWARE;
		if (ShadowMode != resolved_mode) {
			ShadowMode = resolved_mode;
		}

		Set_Max_Simultaneous_Shadows(0);
		ShadowMapManager::Set_Enabled(ShadowMode != SHADOW_MODE_NONE);
	}
}

PhysicsSceneClass::ShadowEnum
PhysicsSceneClass::Get_Shadow_Mode(void)
{
	return ShadowMode;
}

void PhysicsSceneClass::Set_Shadow_Attenuation(float atten_start_distance,float atten_end_distance)
{
	if (atten_start_distance < 0.0f) {
		atten_start_distance = 0.0f;
	}
	if (atten_end_distance < atten_start_distance) {
		atten_end_distance = atten_start_distance;
	}
	ShadowAttenStart = atten_start_distance;
	ShadowAttenEnd = atten_end_distance;
}

void PhysicsSceneClass::Get_Shadow_Attenuation(float * set_atten_start,float * set_atten_end)
{
	if (set_atten_start != NULL) {
		*set_atten_start = ShadowAttenStart;
	}

	if (set_atten_end != NULL) {
		*set_atten_end = ShadowAttenEnd;
	}
}

void PhysicsSceneClass::Set_Shadow_Normal_Intensity(float normal_intensity)
{
	if (normal_intensity < 0.0f) {
		normal_intensity = 0.0f;
	}
	if (normal_intensity > 1.0f) {
		normal_intensity = 1.0f;
	}
	ShadowNormalIntensity = normal_intensity;
}

float PhysicsSceneClass::Get_Shadow_Normal_Intensity(void)
{
	return ShadowNormalIntensity;
}

void PhysicsSceneClass::Add_Static_Texture_Projector(TexProjectClass * newprojector)
{
	WWASSERT(newprojector);
	WWASSERT(!StaticProjectorList.Is_In_List(newprojector));

	StaticProjectorList.Add(newprojector);
}

void PhysicsSceneClass::Remove_Static_Texture_Projector(TexProjectClass * projector)
{
	WWASSERT(projector);
	if (StaticProjectorList.Is_In_List(projector)) {
		StaticProjectorList.Remove(projector);
	}
}

void PhysicsSceneClass::Add_Dynamic_Texture_Projector(TexProjectClass * newprojector)
{
	WWASSERT(newprojector);
	WWASSERT(!DynamicProjectorList.Is_In_List(newprojector));

	DynamicProjectorList.Add(newprojector);
}

void PhysicsSceneClass::Remove_Dynamic_Texture_Projector(TexProjectClass * projector)
{
	WWASSERT(projector);
	if (DynamicProjectorList.Is_In_List(projector)) {
		DynamicProjectorList.Remove(projector);
	}
}

void PhysicsSceneClass::Remove_Texture_Projector(TexProjectClass * projector)
{
	WWASSERT(projector);

	if (DynamicProjectorList.Is_In_List(projector)) {
		DynamicProjectorList.Remove(projector);
	} else if (StaticProjectorList.Is_In_List(projector)) {
		StaticProjectorList.Remove(projector);
	}
}

bool PhysicsSceneClass::Contains(TexProjectClass * projector)
{
	return (DynamicProjectorList.Is_In_List(projector) || StaticProjectorList.Is_In_List(projector));
}

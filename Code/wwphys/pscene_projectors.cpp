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




void PhysicsSceneClass::Set_Shadows_Enabled(bool enabled)
{
	if (ShadowsEnabled != enabled) {
		ShadowsEnabled = enabled;
	}

	_DynamicShadowTexMgr.Set_Max_Simultaneous_Shadows(0);
	ShadowMapManager::Set_Enabled(ShadowsEnabled);
}

bool PhysicsSceneClass::Are_Shadows_Enabled(void) const
{
	return ShadowsEnabled;
}

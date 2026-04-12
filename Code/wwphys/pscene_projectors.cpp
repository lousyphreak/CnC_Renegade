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
#include "staticanimphys.h"
#include "assetmgr.h"
#include "refcount.h"
#include "camera.h"
#include "quat.h"
#include "vertmaterial.h"
#include "wwprofile.h"
#include "texture.h"
#include "bgfxrenderer.h"
#include "vertexformat.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "dx8wrapper.h"
#include "pot.h"
#include "materialeffect.h"
#include "wwmemlog.h"
#include "vertmaterial.h"


#define DEBUG_SHADOW_RENDERING				0

const int		SHADOW_CLIP_FAR							= 500;
const int		STATIC_PROJECTOR_RESOLUTION			= 256; //128;
const float		STATIC_SHADOW_INTENSITY					= 0.6f;
const float		MIN_STATIC_SHADOW_COS_HALF_THETA		= cos(DEG_TO_RADF(10.0f)/2.0f);	// angle to allow shadow tex re-use

const int		DEFAULT_MAX_DYNAMIC_SHADOWS			= 6;
const int		DEFAULT_DYNAMIC_SHADOW_RESOLUTION	= 256;

/**
** StaticShadowTexMgrClass
** This object simply manages the list of unique shadow textures being used by the
** static shadow projectors.  For example, all instances of the same tree model
** re-use the same shadow texture...
*/
class StaticShadowTexMgrClass
{
public:
	StaticShadowTexMgrClass(void);
	virtual ~StaticShadowTexMgrClass(void);
	void						Reset(void);

	TextureClass *			Peek_Shadow_Texture(uint32_t obj_type_id,const Quaternion & orientation);
	void						Add_Shadow_Texture(uint32_t obj_type_id,const Quaternion & orientation,TextureClass * tex);
	void						Remove_Shadow_Texture(TextureClass * tex);

private:

	class	 ShadowTexClass
	{
	public:
		ShadowTexClass(void);
		ShadowTexClass(uint32_t obj_type_id,const Quaternion & orientation,TextureClass * tex);
		ShadowTexClass(const ShadowTexClass & that);
		~ShadowTexClass(void);
		const ShadowTexClass & operator = (const ShadowTexClass &);

		bool					operator == (const ShadowTexClass &)						{ return false; }
		bool					operator != (const ShadowTexClass & that)					{ return true; }
		uint32_t				ObjectTypeID;			// use the Definition ID to uniquely identify object types
		Quaternion			ObjectOrientation;	// orientation of the object when this shadow was generated
		TextureClass *		Texture;					// texture

	};

	DynamicVectorClass<ShadowTexClass>	ShadowTextures;
};


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
static StaticShadowTexMgrClass		_StaticShadowTexMgr;
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
		TextureClass *texture = BgfxRenderer::Create_Render_Target_Texture(w, h, preferred_formats[i]);
		if (texture != NULL) {
			return texture;
		}
	}

	return NULL;
}

static TextureClass *Create_Static_Shadow_Texture(void)
{
	TextureClass *texture = Create_Projector_Render_Target(STATIC_PROJECTOR_RESOLUTION,STATIC_PROJECTOR_RESOLUTION);
	if (texture != NULL) {
		texture->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		texture->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
	}
	return texture;
}

/************************************************************************************
**
** ShadowTexClass Implemenation
**
************************************************************************************/
StaticShadowTexMgrClass::ShadowTexClass::ShadowTexClass(void) :
	ObjectTypeID(0),
	ObjectOrientation(1),
	Texture(NULL)
{
}

StaticShadowTexMgrClass::ShadowTexClass::ShadowTexClass
(
	uint32_t obj_type_id,
	const Quaternion & orientation,
	TextureClass * tex
) :
	ObjectTypeID(obj_type_id),
	ObjectOrientation(orientation),
	Texture(NULL)
{
	REF_PTR_SET(Texture,tex);
}

StaticShadowTexMgrClass::ShadowTexClass::ShadowTexClass(const ShadowTexClass & that) :
	ObjectTypeID(0),
	ObjectOrientation(1),
	Texture(NULL)
{
	*this = that;
}

const StaticShadowTexMgrClass::ShadowTexClass &
StaticShadowTexMgrClass::ShadowTexClass::operator = (const ShadowTexClass & that)
{
	ObjectTypeID = that.ObjectTypeID;
	ObjectOrientation = that.ObjectOrientation;
	REF_PTR_SET(Texture,that.Texture);
	return *this;
}

StaticShadowTexMgrClass::ShadowTexClass::~ShadowTexClass(void)
{
	REF_PTR_RELEASE(Texture);
}


/************************************************************************************
**
** StaticShadowTexMgrClass Implemenation
**
************************************************************************************/
StaticShadowTexMgrClass::StaticShadowTexMgrClass(void)
{
}

StaticShadowTexMgrClass::~StaticShadowTexMgrClass(void)
{
}

void StaticShadowTexMgrClass::Reset(void)
{
	ShadowTextures.Delete_All();
}

TextureClass * StaticShadowTexMgrClass::Peek_Shadow_Texture
(
	uint32_t					obj_type_id,
	const Quaternion &	orientation
)
{
	for (int i=0; i<ShadowTextures.Count(); i++) {
		Quaternion deltaq = ShadowTextures[i].ObjectOrientation * Inverse(orientation);
		if (	(ShadowTextures[i].ObjectTypeID == obj_type_id) &&
				(deltaq.W > MIN_STATIC_SHADOW_COS_HALF_THETA))
		{
			return ShadowTextures[i].Texture;
		}
	}
	return NULL;
}

void StaticShadowTexMgrClass::Add_Shadow_Texture
(
	uint32_t					obj_type_id,
	const Quaternion &	orientation,
	TextureClass *			tex
)
{
	WWASSERT(Peek_Shadow_Texture(obj_type_id,orientation) == NULL);
	ShadowTexClass record(obj_type_id,orientation,tex);
	ShadowTextures.Add(record);
}

void StaticShadowTexMgrClass::Remove_Shadow_Texture
(
	TextureClass *			tex
)
{
	for (int i=0; i<ShadowTextures.Count(); i++) {
		if (ShadowTextures[i].Texture == tex) {
			ShadowTextures.Delete(i);
			return;
		}
	}
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
	_StaticShadowTexMgr.Reset();
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


SpecialRenderInfoClass *
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
		ShadowRenderContext = new SpecialRenderInfoClass(*ShadowCamera,SpecialRenderInfoClass::RENDER_SHADOW);
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

void PhysicsSceneClass::Enable_Static_Projectors(bool onoff)
{
	StaticProjectorsEnabled = onoff;
}

bool PhysicsSceneClass::Are_Static_Projectors_Enabled(void)
{
	return StaticProjectorsEnabled;
}

void PhysicsSceneClass::Enable_Dynamic_Projectors(bool onoff)
{
	DynamicProjectorsEnabled = onoff;
}

bool PhysicsSceneClass::Are_Dynamic_Projectors_Enabled(void)
{
	return DynamicProjectorsEnabled;
}

void PhysicsSceneClass::Set_Shadow_Mode(ShadowEnum shadow_mode)
{
	if (((int)shadow_mode >= 0) && ((int)shadow_mode < SHADOW_MODE_COUNT)) {
		if (ShadowMode!=shadow_mode) {
			ShadowMode = shadow_mode;

			switch (ShadowMode) {
			default:
			case SHADOW_MODE_NONE:			// no shadows at all
			case SHADOW_MODE_BLOBS:			// projected blob shadows
				Set_Max_Simultaneous_Shadows(0);
				break;
			case SHADOW_MODE_BLOBS_PLUS:	// projected blobs with main character having a rendered shadow
				Set_Max_Simultaneous_Shadows(1);
				break;
			case SHADOW_MODE_HARDWARE:		// use render-to-texture hardware
				Set_Max_Simultaneous_Shadows(4);
				break;
			}
		}
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
	StaticProjectorCullingSystem->Add_Object(newprojector);
}

void PhysicsSceneClass::Remove_Static_Texture_Projector(TexProjectClass * projector)
{
	WWASSERT(projector);

	if (projector->Get_Culling_System() == StaticProjectorCullingSystem) {
		WWASSERT(StaticProjectorList.Is_In_List(projector));
		StaticProjectorList.Remove(projector);
		StaticProjectorCullingSystem->Remove_Object(projector);
	}
}

void PhysicsSceneClass::Add_Dynamic_Texture_Projector(TexProjectClass * newprojector)
{
	WWASSERT(newprojector);
	WWASSERT(!DynamicProjectorList.Is_In_List(newprojector));

	DynamicProjectorList.Add(newprojector);
	DynamicProjectorCullingSystem->Add_Object(newprojector);
}

void PhysicsSceneClass::Remove_Dynamic_Texture_Projector(TexProjectClass * projector)
{
	WWASSERT(projector);

	if (projector->Get_Culling_System() == DynamicProjectorCullingSystem) {
		WWASSERT(DynamicProjectorList.Is_In_List(projector));
		DynamicProjectorList.Remove(projector);
		DynamicProjectorCullingSystem->Remove_Object(projector);
	}
}

void PhysicsSceneClass::Remove_Texture_Projector(TexProjectClass * projector)
{
	WWASSERT(projector);

	if (DynamicProjectorList.Is_In_List(projector)) {
		DynamicProjectorCullingSystem->Remove_Object(projector);
		DynamicProjectorList.Remove(projector);
	} else if (StaticProjectorList.Is_In_List(projector)) {
		StaticProjectorCullingSystem->Remove_Object(projector);
		StaticProjectorList.Remove(projector);
	}
}

bool PhysicsSceneClass::Contains(TexProjectClass * projector)
{
	return (DynamicProjectorList.Is_In_List(projector) || StaticProjectorList.Is_In_List(projector));
}

float PhysicsSceneClass::Compute_Projector_Attenuation(TexProjectClass * dynamic_projector,const Vector3 & view_pos,const Vector3 & view_dir)
{
	Vector3 r;
	Vector3::Subtract(dynamic_projector->Get_Bounding_Volume().Center,view_pos,&r);
	float dist = Vector3::Dot_Product(r,view_dir);
	if (dist > ShadowAttenEnd) {
		return 0.0f;
	}
	if (dist < ShadowAttenStart) {
		return 1.0f;
	}
	return 1.0f - (dist - ShadowAttenStart) / (ShadowAttenEnd - ShadowAttenStart);
}


void PhysicsSceneClass::Apply_Projectors
(
	const CameraClass &	camera
)
{
	WWPROFILE("pscene::Apply_Projectors");

	static int _dbg_frame = 0;
	_dbg_frame++;
	if (_dbg_frame == 10) {
		int vis_total = 0, vis_ws = 0;
		RefPhysListIterator vis_it(&VisibleStaticObjectList);
		for (vis_it.First(); !vis_it.Is_Done(); vis_it.Next()) vis_total++;
		RefPhysListIterator ws_it(&VisibleWSMeshList);
		for (ws_it.First(); !ws_it.Is_Done(); ws_it.Next()) vis_ws++;
		fprintf(stderr, "SHADOW F10: VisStatic=%d VisWS=%d\n", vis_total, vis_ws);
	}

	Vector3 view_pos;
	Vector3 view_dir;
	camera.Get_Transform().Get_Translation(&view_pos);
	camera.Get_Transform().Get_Z_Vector(&view_dir);
	view_dir = -view_dir;

	if (StaticProjectorsEnabled) {

		/*
		** collect the visible static projectors
		*/
		StaticProjectorCullingSystem->Reset_Collection();
		StaticProjectorCullingSystem->Collect_Objects(camera.Get_Frustum());

		TexProjectClass * static_projector = StaticProjectorCullingSystem->Get_First_Collected_Object();
		while (static_projector != NULL) {

			/*
			** only keep considering this projector if its intensity is above ZERO
			*/
			if (!static_projector->Is_Intensity_Zero()) {

				/*
				** attenuate the projector with distance from the camera (if needed)
				*/
				float attenuation = 1.0f;
				if (static_projector->Is_Attenuation_Enabled()) {
					attenuation = Compute_Projector_Attenuation(static_projector,view_pos,view_dir);
					static_projector->Set_Attenuation(attenuation);
				}

				/*
				** if the projector is completely attenuated, don't process it
				*/
				if (attenuation > 0.0f) {

					if (ProjectorDebugDisplayEnabled) {
						DEBUG_RENDER_OBBOX(static_projector->Get_Bounding_Volume(),Vector3(0,1,0),0.25f);
					}

					Apply_Projector_To_Objects(static_projector,camera);
				}
			}
			static_projector = StaticProjectorCullingSystem->Get_Next_Collected_Object(static_projector);
		}
	}

	/*
	** Build a list of the dynamic shadow textures that need to be rendered
	*/
	TexProjListClass rt_projector_list;
	uint32_t count = 0;

	if (DynamicProjectorsEnabled) {

		/*
		** Collect the visible dynamic projectors
		*/
		DynamicProjectorCullingSystem->Reset_Collection();
		DynamicProjectorCullingSystem->Collect_Objects(camera.Get_Frustum());

		TexProjectClass * dynamic_projector = DynamicProjectorCullingSystem->Get_First_Collected_Object();
		while (dynamic_projector != NULL) {

			/*
			** only keep considering this projector if its intensity is above ZERO
			*/
			if (!dynamic_projector->Is_Intensity_Zero()) {

				/*
				** attenuate the projector with distance from the camera (if needed)
				*/
				float attenuation = 1.0f;
				if (dynamic_projector->Is_Attenuation_Enabled()) {
					attenuation = Compute_Projector_Attenuation(dynamic_projector,view_pos,view_dir);
					dynamic_projector->Set_Attenuation(attenuation);
				}

				/*
				** if the projector is completely attenuated, don't process it
				*/
				if (attenuation > 0.0f) {

					if (ProjectorDebugDisplayEnabled) {
						DEBUG_RENDER_OBBOX(dynamic_projector->Get_Bounding_Volume(),Vector3(1,0,0),0.25f);
					}

					/*
					** If this projector needs to recompute its texture, add it to the list,
					** otherwise set it up now.
					*/
					if (dynamic_projector->Needs_Render_Target()) {
						rt_projector_list.Add(dynamic_projector);
						count++;
					} else {
						Apply_Projector_To_Objects(dynamic_projector,camera);
					}
				}
			}
			dynamic_projector = DynamicProjectorCullingSystem->Get_Next_Collected_Object(dynamic_projector);
		}
	}

	/*
	** Reject texture projectors until we are below the maximum allowed simultaneous shadows
	*/
	Vector3 cam_pos;
	camera.Get_Transform().Get_Translation(&cam_pos);

	TexProjListIterator it(&rt_projector_list);
	while (count > _DynamicShadowTexMgr.Get_Max_Simultaneous_Shadows()) {

		/*
		** Find the projector farthest from the camera
		*/
		it.First();
		TexProjectClass * farthest_shadow = it.Peek_Obj();
		float farthest_dist = (farthest_shadow->Get_Bounding_Volume().Center - cam_pos).Length2();
		it.Next();

		while (!it.Is_Done()) {
			float dist = (it.Peek_Obj()->Get_Bounding_Volume().Center - cam_pos).Length2();
			if (dist > farthest_dist) {
				farthest_dist = dist;
				farthest_shadow = it.Peek_Obj();
			}
			it.Next();
		}

		/*
		** Remove it from the list and disable it.
		*/
		rt_projector_list.Remove(farthest_shadow);
		farthest_shadow->Set_Render_Target(NULL);
		count--;
	}

	/*
	** Process the most "important" active dynamic shadows
	*/
	_DynamicShadowTexMgr.Per_Frame_Reset();
	it.First();

	while (!it.Is_Done()) {

		TexProjectClass * projector = it.Peek_Obj();
		_DynamicShadowTexMgr.Assign_Render_Target_Texture(projector);

		if (projector->Peek_Render_Target() != NULL) {
			Apply_Projector_To_Objects(projector,camera);
		}

		it.Next();
	}

	BgfxRenderer::Reset_Render_Target();
}

void PhysicsSceneClass::Apply_Projector_To_Objects
(
	TexProjectClass * tex_proj,
	const CameraClass &	camera
)
{
	bool projector_update_needed = false;	// Needed if just one mesh uses it
	SimpleEffectClass * effect = NEW_REF(SimpleEffectClass,(tex_proj->Peek_Material_Pass()));
	effect->Enable_Auto_Remove(true);

	/*
	** collect the static objects intersecting the projector's volume
	*/
	if (tex_proj->Is_Affect_Static_Objects_Enabled()) {
		StaticCullingSystem->Reset_Collection();
		StaticCullingSystem->Collect_Objects(tex_proj->Get_Bounding_Volume());

		StaticPhysClass * static_obj = (StaticPhysClass *)StaticCullingSystem->Get_First_Collected_Object();
		while (static_obj) {
			if (VisibleStaticObjectList.Contains(static_obj) || VisibleWSMeshList.Is_In_List(static_obj)) {
				static_obj->Add_Effect_To_Me(effect);
				projector_update_needed = true;
			}
			static_obj = (StaticPhysClass *)StaticCullingSystem->Get_Next_Collected_Object(static_obj);
		}
	}

	/*
	** collect the dynamic objects intersecting the projector's volume
	*/
	if (tex_proj->Is_Affect_Dynamic_Objects_Enabled()) {
		DynamicCullingSystem->Reset_Collection();
		DynamicCullingSystem->Collect_Objects(tex_proj->Get_Bounding_Volume());

		PhysClass * dyn_obj = DynamicCullingSystem->Get_First_Collected_Object();
		while (dyn_obj) {

			/*
			** for each dynamic object, if it is also in our visible list, add this projector
			** to it. Dynamic shadows need to reach the caster for self-shadowing as well as
			** other dynamic units in the projection volume.
			*/
			if (VisibleDynamicObjectList.Contains(dyn_obj)) {

				dyn_obj->Add_Effect_To_Me(effect);
				projector_update_needed = true;
			}
			dyn_obj = DynamicCullingSystem->Get_Next_Collected_Object(dyn_obj);
		}
	}

	// if this projector is needed, update it
	if (projector_update_needed) {
		tex_proj->Pre_Render_Update(camera.Get_Transform());
	}

	REF_PTR_RELEASE(effect);
}

void PhysicsSceneClass::Invalidate_Static_Shadow_Projectors()
{
	/*
	** Collect a list of all static objects who want to generate a shadow
	** Tell each to destroy their shadow
	*/
	RefPhysListClass shadow_gen_list;
	RefPhysListIterator static_anim_iterator(&StaticAnimList);
	for (static_anim_iterator.First(); !static_anim_iterator.Is_Done(); static_anim_iterator.Next()) {
		StaticAnimPhysClass * obj = (StaticAnimPhysClass *)static_anim_iterator.Peek_Obj();

		if (obj != NULL) {
			StaticAnimPhysDefClass * def = obj->Get_StaticAnimPhysDef();
			if (def && def->Shadow_Dynamic_Objs()) {
				obj->Set_Shadow(NULL);
				shadow_gen_list.Add(obj);
			}
		}
	}

	StaticProjectorsDirty=true;

	/*
	** Release all of the textures we were using for static shadows
	*/
	_StaticShadowTexMgr.Reset();
}

void PhysicsSceneClass::Generate_Static_Shadow_Projectors(void)
{
	if (!StaticProjectorsDirty) return;

	// Don't operate if the device is lost!
	if (DX8Wrapper::Is_Device_Lost() || !DX8Wrapper::Is_Initted()) return;

	/*
	** Collect a list of all static objects who want to generate a shadow
	** Tell each to destroy their shadow
	*/
	RefPhysListClass shadow_gen_list;
	RefPhysListIterator static_anim_iterator(&StaticAnimList);
	for (static_anim_iterator.First(); !static_anim_iterator.Is_Done(); static_anim_iterator.Next()) {
		StaticAnimPhysClass * obj = (StaticAnimPhysClass *)static_anim_iterator.Peek_Obj();

		if (obj != NULL) {
			StaticAnimPhysDefClass * def = obj->Get_StaticAnimPhysDef();
			if (def && def->Shadow_Dynamic_Objs()) {
				obj->Set_Shadow(NULL);
				shadow_gen_list.Add(obj);
			}
		}
	}

	/*
	** Release all of the textures we were using for static shadows
	*/
	_StaticShadowTexMgr.Reset();

	/*
	** Generate a new shadow for each one. Each shadow now renders directly into the
	** cached bgfx texture instead of copying from a shared render target through SurfaceClass.
	*/
	RefPhysListIterator shadow_gen_iterator(&shadow_gen_list);
	for (shadow_gen_iterator.First(); !shadow_gen_iterator.Is_Done(); shadow_gen_iterator.Next()) {

		StaticAnimPhysClass * obj = (StaticAnimPhysClass *)shadow_gen_iterator.Peek_Obj();

		/*
		** Setup the shadow projector for this object
		*/
		Vector3 sunvector;
		Get_Sun_Light_Vector(&sunvector);
		Setup_Static_Directional_Shadow(*obj,sunvector);
	}
	StaticProjectorsDirty=false;
}

void PhysicsSceneClass::Setup_Static_Directional_Shadow
(
	StaticAnimPhysClass & obj,
	const Vector3 & light_dir
)
{
	/*
	** Get the definition for this object
	*/
	StaticAnimPhysDefClass * def = obj.Get_StaticAnimPhysDef();
	if (def == NULL) {
		return;
	}

	int type_id = def->Get_ID();
	Quaternion obj_orientation(1);

	if (def->Shadow_Ignores_Z_Rotation() == false) {
		obj_orientation = Build_Quaternion(obj.Get_Transform());
	}

	/*
	** Create the projector
	*/
	PhysTexProjectClass * shadow_projector = NEW_REF(PhysTexProjectClass,());

	shadow_projector->Set_Texture_Size(STATIC_PROJECTOR_RESOLUTION);
	shadow_projector->Set_Intensity(def->Shadow_Intensity(),true);

	if (def->Shadow_Is_Additive()) {
		shadow_projector->Init_Additive();
	} else {
		shadow_projector->Init_Multiplicative();

		/*
		** (gth) override the static shadow intensity with the global setting.
		*/
		shadow_projector->Set_Intensity(Get_Shadow_Normal_Intensity(),true);
	}

	shadow_projector->Compute_Ortho_Projection(&obj,light_dir,def->Shadow_NearZ(),def->Shadow_FarZ());
	shadow_projector->Enable_Attenuation(true);
	shadow_projector->Enable_Depth_Gradient(false);
	shadow_projector->Peek_Material_Pass()->Enable_On_Translucent_Meshes(false);

	/*
	** See if we already have a suitable texture.
	*/
	TextureClass * existing_texture = _StaticShadowTexMgr.Peek_Shadow_Texture(type_id,obj_orientation);
	if (existing_texture != NULL) {
		shadow_projector->Set_Texture(existing_texture);
	}

	/*
	** If no texture was a available, we have to generate it.
	** Then add it to the cache so others can use it
	*/
	if (existing_texture == NULL) {

		TextureClass * shadow_texture = Create_Static_Shadow_Texture();
		if (shadow_texture == NULL) {
			WWDEBUG_SAY(("Failed to allocate static shadow texture for object type %d\n",type_id));
			REF_PTR_RELEASE(shadow_projector);
			return;
		}

		shadow_projector->Set_Render_Target(shadow_texture);
		if (!shadow_projector->Compute_Texture(&obj,def->Shadow_Is_Additive())) {
			WWDEBUG_SAY(("Failed to render static shadow texture for object type %d\n",type_id));
			shadow_projector->Set_Render_Target(NULL);
			REF_PTR_RELEASE(shadow_texture);
			REF_PTR_RELEASE(shadow_projector);
			return;
		}

		shadow_projector->Set_Render_Target(NULL);
		shadow_projector->Set_Texture(shadow_texture);

		_StaticShadowTexMgr.Add_Shadow_Texture(type_id,obj_orientation,shadow_projector->Peek_Texture());
		REF_PTR_RELEASE(shadow_texture);
	}

	/*
	** Give the projector to the object
	*/
	obj.Set_Shadow(shadow_projector);

	/*
	** Release resources
	*/
	REF_PTR_RELEASE(shadow_projector);
}

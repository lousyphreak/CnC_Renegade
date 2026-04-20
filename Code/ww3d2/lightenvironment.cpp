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
 *                     $Archive:: /Commando/Code/ww3d2/lightenvironment.cpp                   $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 3/01/02 3:43p                                               $*
 *                                                                                             *
 *                    $Revision:: 4                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "lightenvironment.h"
#include "matrix3d.h"
#include "camera.h"
#include "light.h"

#include <cfloat>
#include <cstring>


/*
** Constants
*/
const float DIFFUSE_TO_AMBIENT_FRACTION = 1.0f;


/*
** Static variables
*/
static float _LightingLODCutoff			= 0.5f;	
static float _LightingLODCutoff2		= 0.5f * 0.5f;

namespace
{
Vector3 Resolve_Light_Travel_Direction(const LightClass & light)
{
	return -light.Get_Transform().Get_Z_Vector();
}

void Init_Render_Light(const LightClass & light, D3DLIGHT8 & render_light)
{
	::memset(&render_light, 0, sizeof(render_light));

	switch (light.Get_Type())
	{
	case LightClass::POINT:
		render_light.Type = D3DLIGHT_POINT;
		break;
	case LightClass::DIRECTIONAL:
		render_light.Type = D3DLIGHT_DIRECTIONAL;
		break;
	case LightClass::SPOT:
		render_light.Type = D3DLIGHT_SPOT;
		break;
	}

	Vector3 color;
	light.Get_Diffuse(&color);
	color *= light.Get_Intensity();
	render_light.Diffuse.r = color.X;
	render_light.Diffuse.g = color.Y;
	render_light.Diffuse.b = color.Z;
	render_light.Diffuse.a = 1.0f;

	light.Get_Specular(&color);
	color *= light.Get_Intensity();
	render_light.Specular.r = color.X;
	render_light.Specular.g = color.Y;
	render_light.Specular.b = color.Z;
	render_light.Specular.a = 1.0f;

	light.Get_Ambient(&color);
	color *= light.Get_Intensity();
	render_light.Ambient.r = color.X;
	render_light.Ambient.g = color.Y;
	render_light.Ambient.b = color.Z;
	render_light.Ambient.a = 1.0f;

	const Vector3 position = light.Get_Position();
	render_light.Position = { position.X, position.Y, position.Z };

	if (light.Get_Type() != LightClass::POINT) {
		const Vector3 direction = Resolve_Light_Travel_Direction(light);
		render_light.Direction = { direction.X, direction.Y, direction.Z };
	}

	render_light.Range = FLT_MAX;
	render_light.Falloff = light.Get_Spot_Exponent();
	render_light.Theta = light.Get_Spot_Angle();
	render_light.Phi = light.Get_Spot_Angle();
	render_light.Attenuation0 = 1.0f;
	render_light.Attenuation1 = 0.0f;
	render_light.Attenuation2 = 0.0f;

	if (light.Get_Type() != LightClass::DIRECTIONAL) {
		if (light.Get_Flag(LightClass::FAR_ATTENUATION)) {
			double atten_start = 0.0;
			double atten_end = 0.0;
			light.Get_Far_Attenuation_Range(atten_start, atten_end);
			render_light.Range = static_cast<float>(atten_end);
			const float atten_delta = static_cast<float>(atten_end - atten_start);
			if ((atten_delta >= 1.0e-5f || atten_delta <= -1.0e-5f) && atten_start > 1.0e-5) {
				render_light.Attenuation1 = static_cast<float>(1.0 / atten_start);
			}
		}
	}
}
}


/************************************************************************************************
**
** LightEnvironmentClass::InputLightStruct Implementation
**
************************************************************************************************/

void LightEnvironmentClass::InputLightStruct::Init
(
	const LightClass & light,
	const Vector3 & object_center
)
{
	switch(light.Get_Type()) 
	{
	case LightClass::POINT:
	case LightClass::SPOT:
		Init_From_Point_Or_Spot_Light(light,object_center);
		break;
	case LightClass::DIRECTIONAL:
		Init_From_Directional_Light(light,object_center);
		break;
	};
}

void LightEnvironmentClass::InputLightStruct::Init_From_Point_Or_Spot_Light
(
	const LightClass & light,
	const Vector3 & object_center
)
{
	/*
	** Compute the direction vector and the distance to the light
	*/
	Direction = light.Get_Position() - object_center;
	float dist = Direction.Length();
	if (dist > 0.0f) {
		Direction /= dist;
	}

	/*
	** Compute the attenuation factor
	*/
	double atten_start,atten_end;
	light.Get_Far_Attenuation_Range(atten_start,atten_end);
	
	float atten = 1.0f;
	if (light.Get_Flag(LightClass::FAR_ATTENUATION)) {
		const float atten_delta = static_cast<float>(atten_end - atten_start);
		if (atten_delta >= 1.0e-5f || atten_delta <= -1.0e-5f) {
			atten = 1.0f - (dist - atten_start) / atten_delta;
			atten = WWMath::Clamp(atten,0.0f,1.0f);
		} else {
			atten = (dist <= atten_end) ? 1.0f : 0.0f;
		}
	}

	if (light.Get_Type() == LightClass::SPOT) {
		const Vector3 spot_dir = Resolve_Light_Travel_Direction(light);
		float spot_angle_cos = light.Get_Spot_Angle_Cos();
		const float spot_denominator = (1.0f - spot_angle_cos) > 1.0e-5f ? (1.0f - spot_angle_cos) : 1.0e-5f;
		atten *= (Vector3::Dot_Product(spot_dir,-Direction) - spot_angle_cos) / spot_denominator;
		atten = WWMath::Clamp(atten,0.0f,1.0f);
	}

	/*
	** Compute the ambient and diffuse values.  Rejecting the diffuse
	** component and folding it into the ambient component if it is below
	** the LOD cutoff
	*/
	light.Get_Ambient(&Ambient);
	light.Get_Diffuse(&Diffuse);
	Ambient *= light.Get_Intensity();
	Diffuse *= light.Get_Intensity();
	Init_Render_Light(light, RenderLight);
	const Vector3 attenuated_diffuse = Diffuse * atten;

	if (attenuated_diffuse.Length2() > _LightingLODCutoff2) {

		DiffuseRejected = false;
		Ambient *= atten;
		Diffuse = attenuated_diffuse;
		ContributionValue = Diffuse.Length2();
		
	} else {

		DiffuseRejected = true;
		Ambient *= atten;
		Ambient += atten * DIFFUSE_TO_AMBIENT_FRACTION * Diffuse;
		Diffuse.Set(0,0,0);
		ContributionValue = 0.0f;

	}
}


void LightEnvironmentClass::InputLightStruct::Init_From_Directional_Light
(
	const LightClass & light,
	const Vector3 & object_center
)
{
	Direction = Resolve_Light_Travel_Direction(light);

	DiffuseRejected = false;
	light.Get_Ambient(&Ambient);
	light.Get_Diffuse(&Diffuse);
	Ambient *= light.Get_Intensity();
	Diffuse *= light.Get_Intensity();
	Init_Render_Light(light, RenderLight);
	ContributionValue = Diffuse.Length2();
}


float LightEnvironmentClass::InputLightStruct::Contribution(void) const
{
	return ContributionValue;
}
	

/************************************************************************************************
**
** LightEnvironmentClass::OutputLightStruct Implementation
**
************************************************************************************************/

void LightEnvironmentClass::OutputLightStruct::Init
(
	const InputLightStruct & input,
	const Matrix3D & camera_tm
)
{
	Diffuse = input.Diffuse;
	Matrix3D::Inverse_Rotate_Vector(camera_tm,input.Direction,&Direction);
}	



/************************************************************************************************
**
** LightEnvironmentClass Implementation
**
************************************************************************************************/

LightEnvironmentClass::LightEnvironmentClass(void) :
	LightCount(0),
	ObjectCenter(0,0,0),
	OutputAmbient(0,0,0)
{
}


LightEnvironmentClass::~LightEnvironmentClass(void)
{
}


void LightEnvironmentClass::Reset(const Vector3 & object_center,const Vector3 & ambient)
{
	LightCount = 0;
	ObjectCenter = object_center;
	OutputAmbient = ambient;
}


void LightEnvironmentClass::Add_Light(const LightClass & light)
{
	/*
	** Compute the per-light shader descriptor and object-centered contribution.
	*/
	InputLightStruct new_light;
	new_light.Init(light,ObjectCenter);

	/*
	** Weak lights are folded into the global ambient term. Stronger lights are kept
	** as active per-pixel lights until we run out of hardware slots.
	*/
	if (new_light.DiffuseRejected) {
		OutputAmbient += new_light.Ambient;
		return;
	}

	if (LightCount < MAX_LIGHTS) {
		InputLights[LightCount] = new_light;
		LightCount++;
		return;
	}

	int weakest_light_index = 0;
	float weakest_contribution = InputLights[0].Contribution();
	for (int light_index = 1; light_index < LightCount; ++light_index) {
		const float contribution = InputLights[light_index].Contribution();
		if (contribution < weakest_contribution) {
			weakest_contribution = contribution;
			weakest_light_index = light_index;
		}
	}

	if (new_light.Contribution() > weakest_contribution) {
		OutputAmbient += InputLights[weakest_light_index].Ambient;
		InputLights[weakest_light_index] = new_light;
	} else {
		OutputAmbient += new_light.Ambient;
	}
}


void LightEnvironmentClass::Pre_Render_Update(const Matrix3D & camera_tm)
{
	/*
	** Transform each light into camera space
	** and add up the ambient effect of each light
	*/
	for (int light_index=0; light_index<LightCount; light_index++) {
		OutputLights[light_index].Init(InputLights[light_index],camera_tm);
	}

	OutputAmbient.X = WWMath::Clamp(OutputAmbient.X);
	OutputAmbient.Y = WWMath::Clamp(OutputAmbient.Y);
	OutputAmbient.Z = WWMath::Clamp(OutputAmbient.Z);
}

void LightEnvironmentClass::Set_Lighting_LOD_Cutoff(float inten)
{
	_LightingLODCutoff = inten;
	_LightingLODCutoff2 = _LightingLODCutoff * _LightingLODCutoff;
}

float LightEnvironmentClass::Get_Lighting_LOD_Cutoff(void)
{
	return _LightingLODCutoff;
}

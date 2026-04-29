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
 *                     $Archive:: /Commando/Code/wwphys/projectormanager.cpp                  $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 5/14/01 7:58p                                               $*
 *                                                                                             *
 *                    $Revision:: 17                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "projectormanager.h"
#include "chunkio.h"
#include "wwmath.h"
#include "wwdebug.h"


/********************************************************************************************
**
** ProjectorManagerClass Implementation
**
********************************************************************************************/

ProjectorManagerClass::ProjectorManagerClass(void)
{
}

ProjectorManagerClass::~ProjectorManagerClass(void)
{
}

void ProjectorManagerClass::Init(const ProjectorManagerDefClass & def,RenderObjClass * model)
{
	if (!def.IsEnabled || model == NULL) {
		return;
	}

	static bool warned = false;
	if (!warned) {
		WWDEBUG_SAY(("ProjectorManagerClass::Init ignoring legacy runtime texture projector definitions; use shadow maps, decals, or explicit projected-effect systems instead.\n"));
		warned = true;
	}
}

void ProjectorManagerClass::Update_From_Model(RenderObjClass * model)
{
	WWASSERT(model != NULL);
}


/********************************************************************************************
**
** ProjectorManagerDefClass Implementation
** Note, ProjectorManagerDef is not a full-fleged definition class.  It is meant to be
** embedded inside another real definition.
**
********************************************************************************************/

enum 
{
	PROJECTORMANAGERDEF_CHUNK_VARIABLES							= 0x01110004,
	
	PROJECTORMANAGERDEF_VARIABLE_ISENABLED						= 0x00,
	PROJECTORMANAGERDEF_VARIABLE_ISPERSPECTIVE,
	PROJECTORMANAGERDEF_VARIABLE_ISADDITIVE,
	PROJECTORMANAGERDEF_VARIABLE_ISANIMATED,
	PROJECTORMANAGERDEF_VARIABLE_ORTHOWIDTH,
	PROJECTORMANAGERDEF_VARIABLE_ORTHOHEIGHT,
	PROJECTORMANAGERDEF_VARIABLE_HORIZONTALFOV,
	PROJECTORMANAGERDEF_VARIABLE_VERTICALFOV,
	PROJECTORMANAGERDEF_VARIABLE_NEARZ,
	PROJECTORMANAGERDEF_VARIABLE_FARZ,
	PROJECTORMANAGERDEF_VARIABLE_TEXTURENAME,
	PROJECTORMANAGERDEF_VARIABLE_BONENAME,

	PROJECTORMANAGERDEF_VARIABLE_INTENSITY,
};


ProjectorManagerDefClass::ProjectorManagerDefClass(void) :
	IsEnabled(false),
	IsPerspective(false),
	IsAdditive(false),
	IsAnimated(false),
	OrthoWidth(10.0f),
	OrthoHeight(10.0f),
	HorizontalFOV(DEG_TO_RADF(10.0f)),
	VerticalFOV(DEG_TO_RADF(10.0f)),
	NearZ(5.0f),
	FarZ(20.0f),
	Intensity(1.0f)
{
}

ProjectorManagerDefClass::~ProjectorManagerDefClass(void)
{
}

void ProjectorManagerDefClass::Validate_Parameters(void)
{
	if (HorizontalFOV <= 0.0f) { HorizontalFOV = DEG_TO_RADF(10.0f); }
	if (VerticalFOV <= 0.0f) { VerticalFOV = DEG_TO_RADF(10.0f); }
	if (OrthoWidth <= 0.0f) { OrthoWidth = 10.0f; }
	if (OrthoHeight <= 0.0f) { OrthoHeight = 10.0f; }
	if (NearZ < 0.0f) { NearZ = 0.0f; }
	if (FarZ < NearZ) { FarZ = NearZ + 10.0f; }
}

bool ProjectorManagerDefClass::Save(ChunkSaveClass &csave)
{
	Validate_Parameters();

	csave.Begin_Chunk(PROJECTORMANAGERDEF_CHUNK_VARIABLES);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_ISENABLED,IsEnabled);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_ISPERSPECTIVE,IsPerspective);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_ISADDITIVE,IsAdditive);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_ISANIMATED,IsAnimated);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_ORTHOWIDTH,OrthoWidth);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_ORTHOHEIGHT,OrthoHeight);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_HORIZONTALFOV,HorizontalFOV);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_VERTICALFOV,VerticalFOV);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_NEARZ,NearZ);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_FARZ,FarZ);
	WRITE_MICRO_CHUNK_WWSTRING(csave,PROJECTORMANAGERDEF_VARIABLE_TEXTURENAME,TextureName);
	WRITE_MICRO_CHUNK_WWSTRING(csave,PROJECTORMANAGERDEF_VARIABLE_BONENAME,BoneName);
	WRITE_MICRO_CHUNK(csave,PROJECTORMANAGERDEF_VARIABLE_INTENSITY,Intensity);
	csave.End_Chunk();

	return true;
}

bool ProjectorManagerDefClass::Load(ChunkLoadClass &cload)
{
	while (cload.Open_Chunk()) {

		switch(cload.Cur_Chunk_ID()) {

			case PROJECTORMANAGERDEF_CHUNK_VARIABLES:
				while (cload.Open_Micro_Chunk()) {
					switch(cload.Cur_Micro_Chunk_ID()) {
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_ISENABLED,IsEnabled);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_ISPERSPECTIVE,IsPerspective);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_ISADDITIVE,IsAdditive);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_ISANIMATED,IsAnimated);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_ORTHOWIDTH,OrthoWidth);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_ORTHOHEIGHT,OrthoHeight);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_HORIZONTALFOV,HorizontalFOV);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_VERTICALFOV,VerticalFOV);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_NEARZ,NearZ);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_FARZ,FarZ);
						READ_MICRO_CHUNK_WWSTRING(cload,PROJECTORMANAGERDEF_VARIABLE_TEXTURENAME,TextureName);
						READ_MICRO_CHUNK_WWSTRING(cload,PROJECTORMANAGERDEF_VARIABLE_BONENAME,BoneName);
						READ_MICRO_CHUNK(cload,PROJECTORMANAGERDEF_VARIABLE_INTENSITY,Intensity);
					}
					cload.Close_Micro_Chunk();	
				}
				break;

			default:
				WWDEBUG_SAY(("Unhandled Chunk: 0x%X File: %s Line: %d\r\n",cload.Cur_Chunk_ID(),__FILE__,__LINE__));
				break;
		}

		cload.Close_Chunk();
	}
	return true;
}

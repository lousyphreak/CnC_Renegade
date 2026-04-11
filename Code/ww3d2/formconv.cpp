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
 *                     $Archive:: /Commando/Code/ww3d2/formconv.cpp                           $*
 *                                                                                             *
 *              Original Author:: Nathaniel Hoffman                                            *
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/16/01 1:33p                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "formconv.h"

#include "renderer_types.h"

namespace
{
constexpr uint32 kHighestSupportedRendererFormat = D3DFMT_X8L8V8U8;
}

uint32 WW3DFormatToRendererFormatConversionArray[WW3D_FORMAT_COUNT] = {
	D3DFMT_UNKNOWN,
	D3DFMT_R8G8B8,
	D3DFMT_A8R8G8B8,
	D3DFMT_X8R8G8B8,
	D3DFMT_R5G6B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_A1R5G5B5,
	D3DFMT_A4R4G4B4,
	D3DFMT_R3G3B2,
	D3DFMT_A8,
	D3DFMT_A8R3G3B2,
	D3DFMT_X4R4G4B4,
	D3DFMT_A8P8,
	D3DFMT_P8,
	D3DFMT_L8,
	D3DFMT_A8L8,
	D3DFMT_A4L4,
	D3DFMT_V8U8,		// Bumpmap
	D3DFMT_L6V5U5,		// Bumpmap
	D3DFMT_X8L8V8U8,	// Bumpmap
	D3DFMT_DXT1,
	D3DFMT_DXT2,
	D3DFMT_DXT3,
	D3DFMT_DXT4,
	D3DFMT_DXT5
};

WW3DFormat RendererFormatToWW3DFormatConversionArray[kHighestSupportedRendererFormat + 1];

uint32 WW3DFormat_To_Renderer_Format(WW3DFormat ww3d_format)
{
	if (ww3d_format >= WW3D_FORMAT_COUNT) {
		return D3DFMT_UNKNOWN;
	} else {
		return WW3DFormatToRendererFormatConversionArray[(unsigned int)ww3d_format];
	}
}

WW3DFormat Renderer_Format_To_WW3DFormat(uint32 renderer_format)
{
	switch (renderer_format) {
	// The DXT-codes are created with FOURCC macro and thus can't be placed in the conversion table
	case D3DFMT_DXT1: return WW3D_FORMAT_DXT1;
	case D3DFMT_DXT2: return WW3D_FORMAT_DXT2;
	case D3DFMT_DXT3: return WW3D_FORMAT_DXT3;
	case D3DFMT_DXT4: return WW3D_FORMAT_DXT4;
	case D3DFMT_DXT5: return WW3D_FORMAT_DXT5;
	default:
		if (renderer_format > kHighestSupportedRendererFormat) {
			return WW3D_FORMAT_UNKNOWN;
		} else {
			return RendererFormatToWW3DFormatConversionArray[(unsigned int)renderer_format];
		}
		break;
	}
}

void Init_Renderer_Format_Conversion()
{
	for (uint32 i = 0; i <= kHighestSupportedRendererFormat; ++i) {
		RendererFormatToWW3DFormatConversionArray[i] = WW3D_FORMAT_UNKNOWN;
	}

	RendererFormatToWW3DFormatConversionArray[D3DFMT_R8G8B8] = WW3D_FORMAT_R8G8B8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A8R8G8B8] = WW3D_FORMAT_A8R8G8B8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_X8R8G8B8] = WW3D_FORMAT_X8R8G8B8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_R5G6B5] = WW3D_FORMAT_R5G6B5;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_X1R5G5B5] = WW3D_FORMAT_X1R5G5B5;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A1R5G5B5] = WW3D_FORMAT_A1R5G5B5;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A4R4G4B4] = WW3D_FORMAT_A4R4G4B4;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_R3G3B2] = WW3D_FORMAT_R3G3B2;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A8] = WW3D_FORMAT_A8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A8R3G3B2] = WW3D_FORMAT_A8R3G3B2;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_X4R4G4B4] = WW3D_FORMAT_X4R4G4B4;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A8P8] = WW3D_FORMAT_A8P8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_P8] = WW3D_FORMAT_P8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_L8] = WW3D_FORMAT_L8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A8L8] = WW3D_FORMAT_A8L8;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_A4L4] = WW3D_FORMAT_A4L4;
	RendererFormatToWW3DFormatConversionArray[D3DFMT_V8U8] = WW3D_FORMAT_U8V8;				// Bumpmap
	RendererFormatToWW3DFormatConversionArray[D3DFMT_L6V5U5] = WW3D_FORMAT_L6V5U5;		// Bumpmap
	RendererFormatToWW3DFormatConversionArray[D3DFMT_X8L8V8U8] = WW3D_FORMAT_X8L8V8U8;	// Bumpmap

};

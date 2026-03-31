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

bgfx::TextureFormat::Enum WW3DFormat_To_BgfxFormat(WW3DFormat ww3d_format)
{
	switch (ww3d_format) {
		case WW3D_FORMAT_R8G8B8:    return bgfx::TextureFormat::RGB8;
		case WW3D_FORMAT_A8R8G8B8:  return bgfx::TextureFormat::BGRA8;
		case WW3D_FORMAT_X8R8G8B8:  return bgfx::TextureFormat::BGRA8;
		case WW3D_FORMAT_R5G6B5:    return bgfx::TextureFormat::R5G6B5;
		case WW3D_FORMAT_X1R5G5B5:  return bgfx::TextureFormat::RGB5A1;
		case WW3D_FORMAT_A1R5G5B5:  return bgfx::TextureFormat::RGB5A1;
		case WW3D_FORMAT_A4R4G4B4:  return bgfx::TextureFormat::RGBA4;
		case WW3D_FORMAT_R3G3B2:    return bgfx::TextureFormat::R8;       // No exact match
		case WW3D_FORMAT_A8:        return bgfx::TextureFormat::A8;
		case WW3D_FORMAT_A8R3G3B2:  return bgfx::TextureFormat::BGRA8;    // Upconvert
		case WW3D_FORMAT_X4R4G4B4:  return bgfx::TextureFormat::RGBA4;
		case WW3D_FORMAT_A8P8:      return bgfx::TextureFormat::BGRA8;    // No palette in bgfx
		case WW3D_FORMAT_P8:        return bgfx::TextureFormat::R8;       // No palette in bgfx
		case WW3D_FORMAT_L8:        return bgfx::TextureFormat::R8;
		case WW3D_FORMAT_A8L8:      return bgfx::TextureFormat::RG8;
		case WW3D_FORMAT_A4L4:      return bgfx::TextureFormat::RG8;
		case WW3D_FORMAT_U8V8:      return bgfx::TextureFormat::RG8S;     // Signed
		case WW3D_FORMAT_L6V5U5:    return bgfx::TextureFormat::RG8S;     // Closest
		case WW3D_FORMAT_X8L8V8U8:  return bgfx::TextureFormat::RGBA8S;   // Closest
		case WW3D_FORMAT_DXT1:      return bgfx::TextureFormat::BC1;
		case WW3D_FORMAT_DXT2:      return bgfx::TextureFormat::BC2;
		case WW3D_FORMAT_DXT3:      return bgfx::TextureFormat::BC2;
		case WW3D_FORMAT_DXT4:      return bgfx::TextureFormat::BC3;
		case WW3D_FORMAT_DXT5:      return bgfx::TextureFormat::BC3;
		default:                    return bgfx::TextureFormat::Unknown;
	}
}

WW3DFormat BgfxFormat_To_WW3DFormat(bgfx::TextureFormat::Enum bgfx_format)
{
	switch (bgfx_format) {
		case bgfx::TextureFormat::RGB8:    return WW3D_FORMAT_R8G8B8;
		case bgfx::TextureFormat::BGRA8:   return WW3D_FORMAT_A8R8G8B8;
		case bgfx::TextureFormat::R5G6B5:  return WW3D_FORMAT_R5G6B5;
		case bgfx::TextureFormat::RGB5A1:  return WW3D_FORMAT_A1R5G5B5;
		case bgfx::TextureFormat::RGBA4:   return WW3D_FORMAT_A4R4G4B4;
		case bgfx::TextureFormat::R8:      return WW3D_FORMAT_L8;
		case bgfx::TextureFormat::A8:      return WW3D_FORMAT_A8;
		case bgfx::TextureFormat::RG8:     return WW3D_FORMAT_A8L8;
		case bgfx::TextureFormat::RG8S:    return WW3D_FORMAT_U8V8;
		case bgfx::TextureFormat::RGBA8S:  return WW3D_FORMAT_X8L8V8U8;
		case bgfx::TextureFormat::BC1:     return WW3D_FORMAT_DXT1;
		case bgfx::TextureFormat::BC2:     return WW3D_FORMAT_DXT3;
		case bgfx::TextureFormat::BC3:     return WW3D_FORMAT_DXT5;
		default:                           return WW3D_FORMAT_UNKNOWN;
	}
}

void Init_Format_Conversion()
{
	// No-op: switch-based conversion requires no initialization.
}

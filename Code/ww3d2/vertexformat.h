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
 *                 Project Name : ww3d                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/vertexformat.h                               $*
 *                                                                                             *
 *              Original Author:: Jani Penttinen                                               *
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 3/29/01 12:44a                                              $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once
#endif

#ifndef VERTEX_FORMAT_H
#define VERTEX_FORMAT_H

#include "always.h"
#ifdef WWDEBUG
#include "wwdebug.h"
#endif

class StringClass;

enum
{
	VERTEX_FORMAT_FLAG_XYZ				= 0x0002,
	VERTEX_FORMAT_FLAG_XYZB4			= 0x000c,
	VERTEX_FORMAT_FLAG_NORMAL			= 0x0010,
	VERTEX_FORMAT_FLAG_DIFFUSE		= 0x0040,
	VERTEX_FORMAT_FLAG_SPECULAR		= 0x0080,
	VERTEX_FORMAT_FLAG_TEXCOUNT_SHIFT	= 8,
	VERTEX_FORMAT_FLAG_TEX1			= 0x0100,
	VERTEX_FORMAT_FLAG_TEX2			= 0x0200,
	VERTEX_FORMAT_FLAG_TEX3			= 0x0300,
	VERTEX_FORMAT_FLAG_TEX4			= 0x0400,
	VERTEX_FORMAT_FLAG_TEX5			= 0x0500,
	VERTEX_FORMAT_FLAG_TEX6			= 0x0600,
	VERTEX_FORMAT_FLAG_TEX7			= 0x0700,
	VERTEX_FORMAT_FLAG_TEX8			= 0x0800,
	VERTEX_FORMAT_FLAG_TEXCOUNT_MASK	= 0x0f00,
	VERTEX_FORMAT_FLAG_LASTBETA_UBYTE4 = 0x1000,
	VERTEX_FORMAT_MAX_TEXCOORD		= 8
};

WWINLINE unsigned VERTEX_FORMAT_TEXCOORDSIZE_SHIFT(unsigned index)
{
	return 16u + (index * 2u);
}

WWINLINE unsigned VERTEX_FORMAT_TEXCOORDSIZE1(unsigned index)
{
	return 3u << VERTEX_FORMAT_TEXCOORDSIZE_SHIFT(index);
}

WWINLINE unsigned VERTEX_FORMAT_TEXCOORDSIZE2(unsigned index)
{
	return 0u << VERTEX_FORMAT_TEXCOORDSIZE_SHIFT(index);
}

WWINLINE unsigned VERTEX_FORMAT_TEXCOORDSIZE3(unsigned index)
{
	return 1u << VERTEX_FORMAT_TEXCOORDSIZE_SHIFT(index);
}

WWINLINE unsigned VERTEX_FORMAT_TEXCOORDSIZE4(unsigned index)
{
	return 2u << VERTEX_FORMAT_TEXCOORDSIZE_SHIFT(index);
}

WWINLINE unsigned VERTEX_FORMAT_Get_Texcoord_Count(unsigned FVF)
{
	return (FVF & VERTEX_FORMAT_FLAG_TEXCOUNT_MASK) >> VERTEX_FORMAT_FLAG_TEXCOUNT_SHIFT;
}

WWINLINE unsigned VERTEX_FORMAT_Get_Texcoord_Size(unsigned FVF, unsigned index)
{
	const unsigned size = (FVF >> VERTEX_FORMAT_TEXCOORDSIZE_SHIFT(index)) & 0x3u;
	switch (size) {
		case 1:
			return 3;
		case 2:
			return 4;
		case 3:
			return 1;
		default:
			return 2;
	}
}

enum {
	VERTEX_FORMAT_XYZ				= VERTEX_FORMAT_FLAG_XYZ,
	VERTEX_FORMAT_XYZN			= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_NORMAL,
	VERTEX_FORMAT_XYZNUV1		= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_NORMAL | VERTEX_FORMAT_FLAG_TEX1,
	VERTEX_FORMAT_XYZNUV2		= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_NORMAL | VERTEX_FORMAT_FLAG_TEX2,
	VERTEX_FORMAT_XYZNDUV1		= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_NORMAL | VERTEX_FORMAT_FLAG_TEX1 | VERTEX_FORMAT_FLAG_DIFFUSE,
	VERTEX_FORMAT_XYZNDUV2		= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_NORMAL | VERTEX_FORMAT_FLAG_TEX2 | VERTEX_FORMAT_FLAG_DIFFUSE,
	VERTEX_FORMAT_XYZNDUV2B1	= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_NORMAL | VERTEX_FORMAT_FLAG_TEX3 | VERTEX_FORMAT_FLAG_DIFFUSE | (3u << 20u),
	VERTEX_FORMAT_XYZDUV1		= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX1 | VERTEX_FORMAT_FLAG_DIFFUSE,
	VERTEX_FORMAT_XYZDUV2		= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX2 | VERTEX_FORMAT_FLAG_DIFFUSE,
	VERTEX_FORMAT_XYZUV1			= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX1,
	VERTEX_FORMAT_XYZUV2			= VERTEX_FORMAT_FLAG_XYZ | VERTEX_FORMAT_FLAG_TEX2
};

// ----------------------------------------------------------------------------
//
// Util structures for vertex buffer handling. Cast the void pointer returned
// by the vertex buffer to one of these structures.
//
// ----------------------------------------------------------------------------

struct VertexFormatXYZ
{
	float x;
	float y;
	float z;
};

struct VertexFormatXYZNUV1
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	float u1;
	float v1;
};

struct VertexFormatXYZNUV2
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	float u1;
	float v1;
	float u2;
	float v2;
};

struct VertexFormatXYZN
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
};

struct VertexFormatXYZNDUV1
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	unsigned diffuse;
	float u1;
	float v1;
};

struct VertexFormatXYZNDUV2
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	unsigned diffuse;
	float u1;
	float v1;
	float u2;
	float v2;
};

struct VertexFormatXYZNDUV2B1
{
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	unsigned diffuse;
	float u1;
	float v1;
	float u2;
	float v2;
	float bone_index;
};

struct VertexFormatXYZDUV1
{
	float x;
	float y;
	float z;
	unsigned diffuse;
	float u1;
	float v1;
};

struct VertexFormatXYZDUV2
{
	float x;
	float y;
	float z;
	unsigned diffuse;
	float u1;
	float v1;
	float u2;
	float v2;
};

struct VertexFormatXYZUV1
{
	float x;
	float y;
	float z;
	float u1;
	float v1;
};

struct VertexFormatXYZUV2
{
	float x;
	float y;
	float z;
	float u1;
	float v1;
	float u2;
	float v2;
};

// Vertex-format info can be created for any legal format bitfield. It constructs
// offsets to the various elements in the vertex buffer.

class VertexFormatInfoClass
{
	unsigned							FVF;
	unsigned							fvf_size;

	unsigned							location_offset;
	unsigned							normal_offset;
	unsigned							blend_offset;
	unsigned							texcoord_offset[VERTEX_FORMAT_MAX_TEXCOORD];
	unsigned							diffuse_offset;
	unsigned							specular_offset;
public:
	VertexFormatInfoClass(unsigned FVF);

	inline unsigned Get_Location_Offset() const { return location_offset; }
	inline unsigned Get_Normal_Offset() const { return normal_offset; }
#ifdef WWDEBUG
	inline unsigned Get_Tex_Offset(unsigned int n) const { WWASSERT(n<VERTEX_FORMAT_MAX_TEXCOORD); return texcoord_offset[n]; }	
#else
	inline unsigned Get_Tex_Offset(unsigned int n) const { return texcoord_offset[n]; }	
#endif

	inline unsigned Get_Diffuse_Offset() const { return diffuse_offset; }
	inline unsigned Get_Specular_Offset() const { return specular_offset; }
	inline unsigned Get_Vertex_Format() const { return FVF; }
	inline unsigned Get_Vertex_Size() const { return fvf_size; }

	void Get_Vertex_Format_Name(StringClass& fvfname) const;	// For debug purposes
};


#endif

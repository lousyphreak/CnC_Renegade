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

#include "vertexformat.h"
#include "wwstring.h"

static unsigned Get_FVF_Vertex_Size(unsigned FVF)
{
	unsigned size = 0;

	if ((FVF & VERTEX_FORMAT_FLAG_XYZ) == VERTEX_FORMAT_FLAG_XYZ) {
		size += 3 * sizeof(float);
	}

	if (((FVF & VERTEX_FORMAT_FLAG_XYZB4) == VERTEX_FORMAT_FLAG_XYZB4) &&
		((FVF & VERTEX_FORMAT_FLAG_LASTBETA_UBYTE4) == VERTEX_FORMAT_FLAG_LASTBETA_UBYTE4)) {
		size += 3 * sizeof(float) + sizeof(unsigned);
	}

	if ((FVF & VERTEX_FORMAT_FLAG_NORMAL) == VERTEX_FORMAT_FLAG_NORMAL) {
		size += 3 * sizeof(float);
	}

	if ((FVF & VERTEX_FORMAT_FLAG_DIFFUSE) == VERTEX_FORMAT_FLAG_DIFFUSE) {
		size += sizeof(unsigned);
	}

	if ((FVF & VERTEX_FORMAT_FLAG_SPECULAR) == VERTEX_FORMAT_FLAG_SPECULAR) {
		size += sizeof(unsigned);
	}

	const unsigned texcoord_count = VERTEX_FORMAT_Get_Texcoord_Count(FVF);
	for (unsigned i = 0; i < texcoord_count; ++i) {
		size += VERTEX_FORMAT_Get_Texcoord_Size(FVF, i) * sizeof(float);
	}

	return size;
}

VertexFormatInfoClass::VertexFormatInfoClass(unsigned FVF_) 
	:
	FVF(FVF_),
	fvf_size(Get_FVF_Vertex_Size(FVF))
{
	location_offset=0;
	blend_offset=location_offset;
	
	if ((FVF & VERTEX_FORMAT_FLAG_XYZ) == VERTEX_FORMAT_FLAG_XYZ) blend_offset += 3 * sizeof(float);
	normal_offset=blend_offset;

	if (((FVF & VERTEX_FORMAT_FLAG_XYZB4) == VERTEX_FORMAT_FLAG_XYZB4) &&
		((FVF & VERTEX_FORMAT_FLAG_LASTBETA_UBYTE4) == VERTEX_FORMAT_FLAG_LASTBETA_UBYTE4)) {
		normal_offset += 3 * sizeof(float) + sizeof(unsigned);
	}
	diffuse_offset=normal_offset;

	if ((FVF & VERTEX_FORMAT_FLAG_NORMAL) == VERTEX_FORMAT_FLAG_NORMAL) diffuse_offset += 3 * sizeof(float);
	specular_offset=diffuse_offset;

	if ((FVF & VERTEX_FORMAT_FLAG_DIFFUSE) == VERTEX_FORMAT_FLAG_DIFFUSE) specular_offset += sizeof(unsigned);
	texcoord_offset[0]=specular_offset;

	if ((FVF & VERTEX_FORMAT_FLAG_SPECULAR) == VERTEX_FORMAT_FLAG_SPECULAR) texcoord_offset[0] += sizeof(unsigned);

	for (unsigned int i = 1; i < VERTEX_FORMAT_MAX_TEXCOORD; i++)
	{
		texcoord_offset[i] = texcoord_offset[i - 1];
		texcoord_offset[i] += VERTEX_FORMAT_Get_Texcoord_Size(FVF, i - 1) * sizeof(float);
	}
}

void VertexFormatInfoClass::Get_Vertex_Format_Name(StringClass& fvfname) const
{
	switch (Get_Vertex_Format()) {
	case VERTEX_FORMAT_XYZ: fvfname="VERTEX_FORMAT_XYZ"; break;
	case VERTEX_FORMAT_XYZN: fvfname="VERTEX_FORMAT_XYZN"; break;
	case VERTEX_FORMAT_XYZNUV1: fvfname="VERTEX_FORMAT_XYZNUV1"; break;
	case VERTEX_FORMAT_XYZNUV2: fvfname="VERTEX_FORMAT_XYZNUV2"; break;
	case VERTEX_FORMAT_XYZNDUV1: fvfname="VERTEX_FORMAT_XYZNDUV1"; break;
	case VERTEX_FORMAT_XYZNDUV2: fvfname="VERTEX_FORMAT_XYZNDUV2"; break;
	case VERTEX_FORMAT_XYZNDUV2B1: fvfname="VERTEX_FORMAT_XYZNDUV2B1"; break;
	case VERTEX_FORMAT_XYZDUV1: fvfname="VERTEX_FORMAT_XYZDUV1"; break;
	case VERTEX_FORMAT_XYZDUV2: fvfname="VERTEX_FORMAT_XYZDUV2"; break;
	case VERTEX_FORMAT_XYZUV1: fvfname="VERTEX_FORMAT_XYZUV1"; break;
	case VERTEX_FORMAT_XYZUV2: fvfname="VERTEX_FORMAT_XYZUV2"; break;
	default: fvfname="Unknown!";
	}
}

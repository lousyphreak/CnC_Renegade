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

#include "dx8fvf.h"
#include "wwstring.h"

static unsigned Get_FVF_Vertex_Size(unsigned FVF)
{
	unsigned size = 0;

	if ((FVF & DX8_FVF_FLAG_XYZ) == DX8_FVF_FLAG_XYZ) {
		size += 3 * sizeof(float);
	}

	if (((FVF & DX8_FVF_FLAG_XYZB4) == DX8_FVF_FLAG_XYZB4) &&
		((FVF & DX8_FVF_FLAG_LASTBETA_UBYTE4) == DX8_FVF_FLAG_LASTBETA_UBYTE4)) {
		size += 3 * sizeof(float) + sizeof(unsigned);
	}

	if ((FVF & DX8_FVF_FLAG_NORMAL) == DX8_FVF_FLAG_NORMAL) {
		size += 3 * sizeof(float);
	}

	if ((FVF & DX8_FVF_FLAG_DIFFUSE) == DX8_FVF_FLAG_DIFFUSE) {
		size += sizeof(unsigned);
	}

	if ((FVF & DX8_FVF_FLAG_SPECULAR) == DX8_FVF_FLAG_SPECULAR) {
		size += sizeof(unsigned);
	}

	const unsigned texcoord_count = DX8_FVF_Get_Texcoord_Count(FVF);
	for (unsigned i = 0; i < texcoord_count; ++i) {
		size += DX8_FVF_Get_Texcoord_Size(FVF, i) * sizeof(float);
	}

	return size;
}

FVFInfoClass::FVFInfoClass(unsigned FVF_) 
	:
	FVF(FVF_),
	fvf_size(Get_FVF_Vertex_Size(FVF))
{
	location_offset=0;
	blend_offset=location_offset;
	
	if ((FVF & DX8_FVF_FLAG_XYZ) == DX8_FVF_FLAG_XYZ) blend_offset += 3 * sizeof(float);
	normal_offset=blend_offset;

	if (((FVF & DX8_FVF_FLAG_XYZB4) == DX8_FVF_FLAG_XYZB4) &&
		((FVF & DX8_FVF_FLAG_LASTBETA_UBYTE4) == DX8_FVF_FLAG_LASTBETA_UBYTE4)) {
		normal_offset += 3 * sizeof(float) + sizeof(unsigned);
	}
	diffuse_offset=normal_offset;

	if ((FVF & DX8_FVF_FLAG_NORMAL) == DX8_FVF_FLAG_NORMAL) diffuse_offset += 3 * sizeof(float);
	specular_offset=diffuse_offset;

	if ((FVF & DX8_FVF_FLAG_DIFFUSE) == DX8_FVF_FLAG_DIFFUSE) specular_offset += sizeof(unsigned);
	texcoord_offset[0]=specular_offset;

	if ((FVF & DX8_FVF_FLAG_SPECULAR) == DX8_FVF_FLAG_SPECULAR) texcoord_offset[0] += sizeof(unsigned);

	for (unsigned int i = 1; i < DX8_FVF_MAX_TEXCOORD; i++)
	{
		texcoord_offset[i] = texcoord_offset[i - 1];
		texcoord_offset[i] += DX8_FVF_Get_Texcoord_Size(FVF, i - 1) * sizeof(float);
	}
}

void FVFInfoClass::Get_FVF_Name(StringClass& fvfname) const
{
	switch (Get_FVF()) {
	case DX8_FVF_XYZ: fvfname="DX8_FVF_XYZ"; break;
	case DX8_FVF_XYZN: fvfname="DX8_FVF_XYZN"; break;
	case DX8_FVF_XYZNUV1: fvfname="DX8_FVF_XYZNUV1"; break;
	case DX8_FVF_XYZNUV2: fvfname="DX8_FVF_XYZNUV2"; break;
	case DX8_FVF_XYZNDUV1: fvfname="DX8_FVF_XYZNDUV1"; break;
	case DX8_FVF_XYZNDUV2: fvfname="DX8_FVF_XYZNDUV2"; break;
	case DX8_FVF_XYZDUV1: fvfname="DX8_FVF_XYZDUV1"; break;
	case DX8_FVF_XYZDUV2: fvfname="DX8_FVF_XYZDUV2"; break;
	case DX8_FVF_XYZUV1: fvfname="DX8_FVF_XYZUV1"; break;
	case DX8_FVF_XYZUV2: fvfname="DX8_FVF_XYZUV2"; break;
	default: fvfname="Unknown!";
	}
}

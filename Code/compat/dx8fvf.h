#pragma once

#include "renegade_build_config.h"

#if RENEGADE_WITH_DX8_RENDERER

#include "../ww3d2/dx8fvf.h"

#else

#include "always.h"
#include "wwstring.h"

#ifndef D3DDP_MAXTEXCOORD
#define D3DDP_MAXTEXCOORD 8
#endif

#ifndef D3DFVF_XYZ
#define D3DFVF_XYZ 0x002
#endif
#ifndef D3DFVF_NORMAL
#define D3DFVF_NORMAL 0x010
#endif
#ifndef D3DFVF_DIFFUSE
#define D3DFVF_DIFFUSE 0x040
#endif
#ifndef D3DFVF_SPECULAR
#define D3DFVF_SPECULAR 0x080
#endif
#ifndef D3DFVF_TEXCOUNT_MASK
#define D3DFVF_TEXCOUNT_MASK 0xF00
#endif
#ifndef D3DFVF_TEX0
#define D3DFVF_TEX0 0x000
#endif
#ifndef D3DFVF_TEX1
#define D3DFVF_TEX1 0x100
#endif
#ifndef D3DFVF_TEX2
#define D3DFVF_TEX2 0x200
#endif

enum {
	DX8_FVF_XYZ				= D3DFVF_XYZ,
	DX8_FVF_XYZN			= D3DFVF_XYZ | D3DFVF_NORMAL,
	DX8_FVF_XYZNUV1		= D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1,
	DX8_FVF_XYZNUV2		= D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2,
	DX8_FVF_XYZNDUV1		= D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE,
	DX8_FVF_XYZNDUV2		= D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2 | D3DFVF_DIFFUSE,
	DX8_FVF_XYZDUV1		= D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE,
	DX8_FVF_XYZDUV2		= D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_DIFFUSE,
	DX8_FVF_XYZUV1			= D3DFVF_XYZ | D3DFVF_TEX1,
	DX8_FVF_XYZUV2			= D3DFVF_XYZ | D3DFVF_TEX2
};

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

class FVFInfoClass
{
	unsigned FVF;
	unsigned fvf_size;
	unsigned location_offset;
	unsigned normal_offset;
	unsigned blend_offset;
	unsigned texcoord_offset[D3DDP_MAXTEXCOORD];
	unsigned diffuse_offset;
	unsigned specular_offset;

	static inline unsigned Get_Texcoord_Count(unsigned fvf)
	{
		return (fvf & D3DFVF_TEXCOUNT_MASK) >> 8;
	}

	static inline unsigned Get_FVF_Vertex_Size(unsigned fvf)
	{
		switch (fvf) {
			case DX8_FVF_XYZ:
				return sizeof(VertexFormatXYZ);
			case DX8_FVF_XYZN:
				return sizeof(VertexFormatXYZN);
			case DX8_FVF_XYZNUV1:
				return sizeof(VertexFormatXYZNUV1);
			case DX8_FVF_XYZNUV2:
				return sizeof(VertexFormatXYZNUV2);
			case DX8_FVF_XYZNDUV1:
				return sizeof(VertexFormatXYZNDUV1);
			case DX8_FVF_XYZNDUV2:
				return sizeof(VertexFormatXYZNDUV2);
			case DX8_FVF_XYZDUV1:
				return sizeof(VertexFormatXYZDUV1);
			case DX8_FVF_XYZDUV2:
				return sizeof(VertexFormatXYZDUV2);
			case DX8_FVF_XYZUV1:
				return sizeof(VertexFormatXYZUV1);
			case DX8_FVF_XYZUV2:
				return sizeof(VertexFormatXYZUV2);
			default:
			{
				unsigned size = 0;
				if ((fvf & D3DFVF_XYZ) == D3DFVF_XYZ) {
					size += 3U * sizeof(float);
				}
				if ((fvf & D3DFVF_NORMAL) == D3DFVF_NORMAL) {
					size += 3U * sizeof(float);
				}
				if ((fvf & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE) {
					size += sizeof(unsigned);
				}
				if ((fvf & D3DFVF_SPECULAR) == D3DFVF_SPECULAR) {
					size += sizeof(unsigned);
				}
				size += Get_Texcoord_Count(fvf) * 2U * sizeof(float);
				return size;
			}
		}
	}

public:
	explicit FVFInfoClass(unsigned fvf)
		: FVF(fvf),
		  fvf_size(Get_FVF_Vertex_Size(fvf)),
		  location_offset(0),
		  normal_offset(0),
		  blend_offset(0),
		  diffuse_offset(0),
		  specular_offset(0)
	{
		for (unsigned i = 0; i < D3DDP_MAXTEXCOORD; ++i) {
			texcoord_offset[i] = 0;
		}

		unsigned offset = 0;
		location_offset = offset;
		blend_offset = offset;
		if ((FVF & D3DFVF_XYZ) == D3DFVF_XYZ) {
			offset += 3U * sizeof(float);
		}

		normal_offset = offset;
		if ((FVF & D3DFVF_NORMAL) == D3DFVF_NORMAL) {
			offset += 3U * sizeof(float);
		}

		diffuse_offset = offset;
		if ((FVF & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE) {
			offset += sizeof(unsigned);
		}

		specular_offset = offset;
		if ((FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR) {
			offset += sizeof(unsigned);
		}

		for (unsigned i = 0; i < D3DDP_MAXTEXCOORD; ++i) {
			texcoord_offset[i] = offset;
			offset += 2U * sizeof(float);
		}
	}

	inline unsigned Get_Location_Offset() const { return location_offset; }
	inline unsigned Get_Normal_Offset() const { return normal_offset; }
	inline unsigned Get_Tex_Offset(unsigned int n) const
	{
		WWASSERT(n < D3DDP_MAXTEXCOORD);
		return texcoord_offset[n];
	}
	inline unsigned Get_Diffuse_Offset() const { return diffuse_offset; }
	inline unsigned Get_Specular_Offset() const { return specular_offset; }
	inline unsigned Get_FVF() const { return FVF; }
	inline unsigned Get_FVF_Size() const { return fvf_size; }

	inline void Get_FVF_Name(StringClass& fvfname) const
	{
		switch (Get_FVF()) {
			case DX8_FVF_XYZ: fvfname = "D3DFVF_XYZ"; break;
			case DX8_FVF_XYZN: fvfname = "D3DFVF_XYZ|D3DFVF_NORMAL"; break;
			case DX8_FVF_XYZNUV1: fvfname = "D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1"; break;
			case DX8_FVF_XYZNUV2: fvfname = "D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX2"; break;
			case DX8_FVF_XYZNDUV1: fvfname = "D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1|D3DFVF_DIFFUSE"; break;
			case DX8_FVF_XYZNDUV2: fvfname = "D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX2|D3DFVF_DIFFUSE"; break;
			case DX8_FVF_XYZDUV1: fvfname = "D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE"; break;
			case DX8_FVF_XYZDUV2: fvfname = "D3DFVF_XYZ|D3DFVF_TEX2|D3DFVF_DIFFUSE"; break;
			case DX8_FVF_XYZUV1: fvfname = "D3DFVF_XYZ|D3DFVF_TEX1"; break;
			case DX8_FVF_XYZUV2: fvfname = "D3DFVF_XYZ|D3DFVF_TEX2"; break;
			default: fvfname = "Unknown!"; break;
		}
	}
};

#endif
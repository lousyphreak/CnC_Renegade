#pragma once

#include "refcount.h"

#include <vector>

#ifndef DX8_FVF_XYZNDUV1
#define DX8_FVF_XYZNDUV1 0
#endif

#ifndef DX8_FVF_XYZNDUV2
#define DX8_FVF_XYZNDUV2 1
#endif

const unsigned dynamic_fvf_type = DX8_FVF_XYZNDUV2;

struct VertexFormatXYZNDUV1
{
	float x, y, z;
	float nx, ny, nz;
	unsigned diffuse;
	float u1, v1;
};

struct VertexFormatXYZNDUV2
{
	float x, y, z;
	float nx, ny, nz;
	unsigned diffuse;
	float u1, v1;
	float u2, v2;
};

class DX8VertexBufferClass : public RefCountClass
{
public:
	DX8VertexBufferClass(int = 0, int = 0)
	{
	}
};

class DynamicVBAccessClass
{
public:
	DynamicVBAccessClass(int, int vertex_count)
		: Storage(static_cast<size_t>(vertex_count) * 64U, 0)
	{
	}

	DynamicVBAccessClass(int, int, int vertex_count)
		: Storage(static_cast<size_t>(vertex_count) * 64U, 0)
	{
	}

	class WriteLockClass
	{
	public:
		explicit WriteLockClass(DynamicVBAccessClass * access)
			: Access(access)
		{
		}

		void * Get_Vertex_Array()
		{
			return Access != nullptr ? Access->Storage.data() : nullptr;
		}

		VertexFormatXYZNDUV2 * Get_Formatted_Vertex_Array()
		{
			return reinterpret_cast<VertexFormatXYZNDUV2 *>(Get_Vertex_Array());
		}

	private:
		DynamicVBAccessClass * Access;
	};

private:
	std::vector<unsigned char> Storage;
};

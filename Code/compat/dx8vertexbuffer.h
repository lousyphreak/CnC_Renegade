#pragma once

#include "dx8fvf.h"
#include "refcount.h"

#include <vector>

const unsigned dynamic_fvf_type = DX8_FVF_XYZNDUV2;

class VertexBufferClass;

class VertexBufferLockClass
{
protected:
	VertexBufferClass * VertexBuffer;
	void * Vertices;

	VertexBufferLockClass(VertexBufferClass * vertex_buffer)
		: VertexBuffer(vertex_buffer),
		  Vertices(NULL)
	{
	}

public:
	void * Get_Vertex_Array()
	{
		return Vertices;
	}
};

class VertexBufferClass : public RefCountClass
{
protected:
	VertexBufferClass(unsigned buffer_type, unsigned fvf, unsigned short vertex_count)
		: type(buffer_type),
		  VertexCount(vertex_count),
		  engine_refs(0),
		  fvf_info(fvf),
		  Storage(static_cast<size_t>(vertex_count) * static_cast<size_t>(fvf_info.Get_FVF_Size()), 0)
	{
	}

public:
	static unsigned Get_Total_Allocated_Memory()
	{
		return 0;
	}

	const FVFInfoClass & FVF_Info() const
	{
		return fvf_info;
	}

	unsigned short Get_Vertex_Count() const
	{
		return VertexCount;
	}

	unsigned Type() const
	{
		return type;
	}

	void Add_Engine_Ref() const
	{
		++engine_refs;
	}

	void Release_Engine_Ref() const
	{
		if (engine_refs > 0) {
			--engine_refs;
		}
	}

	unsigned Engine_Refs() const
	{
		return engine_refs;
	}

	class WriteLockClass : public VertexBufferLockClass
	{
	public:
		explicit WriteLockClass(VertexBufferClass * vertex_buffer)
			: VertexBufferLockClass(vertex_buffer)
		{
			Vertices = vertex_buffer != NULL ? vertex_buffer->Storage.data() : NULL;
		}
	};

	class AppendLockClass : public VertexBufferLockClass
	{
	public:
		AppendLockClass(VertexBufferClass * vertex_buffer, unsigned start_index, unsigned)
			: VertexBufferLockClass(vertex_buffer)
		{
			if (vertex_buffer != NULL) {
				const size_t offset = static_cast<size_t>(start_index) * static_cast<size_t>(vertex_buffer->FVF_Info().Get_FVF_Size());
				Vertices = vertex_buffer->Storage.data() + offset;
			}
		}
	};

protected:
	unsigned type;
	unsigned short VertexCount;
	mutable unsigned engine_refs;
	FVFInfoClass fvf_info;
	std::vector<unsigned char> Storage;
};

class DX8VertexBufferClass : public VertexBufferClass
{
public:
	DX8VertexBufferClass(unsigned fvf = 0, unsigned short vertex_count = 0)
		: VertexBufferClass(0, fvf, vertex_count)
	{
	}
};

class DynamicVBAccessClass
{
public:
	DynamicVBAccessClass(int, int vertex_count)
		: FVFInfo(dynamic_fvf_type),
		  Type(0),
		  VertexCount(static_cast<unsigned short>(vertex_count)),
		  Storage(static_cast<size_t>(vertex_count) * static_cast<size_t>(FVFInfo.Get_FVF_Size()), 0)
	{
	}

	DynamicVBAccessClass(int, int, int vertex_count)
		: FVFInfo(dynamic_fvf_type),
		  Type(0),
		  VertexCount(static_cast<unsigned short>(vertex_count)),
		  Storage(static_cast<size_t>(vertex_count) * static_cast<size_t>(FVFInfo.Get_FVF_Size()), 0)
	{
	}

	const FVFInfoClass & FVF_Info() const
	{
		return FVFInfo;
	}

	unsigned Get_Type() const
	{
		return Type;
	}

	unsigned short Get_Vertex_Count() const
	{
		return VertexCount;
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
	FVFInfoClass FVFInfo;
	unsigned Type;
	unsigned short VertexCount;
	std::vector<unsigned char> Storage;
};

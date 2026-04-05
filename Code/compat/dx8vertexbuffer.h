#pragma once

#include <cstdint>

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
	VertexBufferClass(unsigned buffer_type, unsigned fvf, uint16_t vertex_count)
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

	uint16_t Get_Vertex_Count() const
	{
		return VertexCount;
	}

	const uint8_t *Get_Vertex_Data() const
	{
		return Storage.data();
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
	uint16_t VertexCount;
	mutable unsigned engine_refs;
	FVFInfoClass fvf_info;
	std::vector<uint8_t> Storage;
};

class DX8VertexBufferClass : public VertexBufferClass
{
public:
	DX8VertexBufferClass(unsigned fvf = 0, uint16_t vertex_count = 0)
		: VertexBufferClass(0, fvf, vertex_count)
	{
	}
};

class SortingVertexBufferClass : public VertexBufferClass
{
public:
	VertexFormatXYZNDUV2* VertexBuffer;

	explicit SortingVertexBufferClass(uint16_t vertex_count = 0)
		: VertexBufferClass(1, dynamic_fvf_type, vertex_count),
		  VertexBuffer(reinterpret_cast<VertexFormatXYZNDUV2*>(Storage.data()))
	{
	}
};

class DynamicVBAccessClass
{
public:
	static void _Reset(bool)
	{
	}

	DynamicVBAccessClass(int type, int vertex_count)
		: FVFInfo(dynamic_fvf_type),
		  Type(static_cast<unsigned>(type)),
		  VertexCount(static_cast<uint16_t>(vertex_count)),
		  BackingBuffer(NULL)
	{
		Attach_Shared_Buffer(type);
	}

	DynamicVBAccessClass(int type, int, int vertex_count)
		: FVFInfo(dynamic_fvf_type),
		  Type(static_cast<unsigned>(type)),
		  VertexCount(static_cast<uint16_t>(vertex_count)),
		  BackingBuffer(NULL)
	{
		Attach_Shared_Buffer(type);
	}

	~DynamicVBAccessClass()
	{
		REF_PTR_RELEASE(BackingBuffer);
	}

	const FVFInfoClass & FVF_Info() const
	{
		return FVFInfo;
	}

	unsigned Get_Type() const
	{
		return Type;
	}

	uint16_t Get_Vertex_Count() const
	{
		return VertexCount;
	}

	const uint8_t *Get_Vertex_Data() const
	{
		return BackingBuffer != NULL ? BackingBuffer->Get_Vertex_Data() : NULL;
	}

	const VertexBufferClass * Get_Vertex_Buffer() const
	{
		return BackingBuffer;
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
			return Access != nullptr && Access->BackingBuffer != NULL ? const_cast<uint8_t*>(Access->BackingBuffer->Get_Vertex_Data()) : nullptr;
		}

		VertexFormatXYZNDUV2 * Get_Formatted_Vertex_Array()
		{
			return reinterpret_cast<VertexFormatXYZNDUV2 *>(Get_Vertex_Array());
		}

	private:
		DynamicVBAccessClass * Access;
	};

private:
	static VertexBufferClass *& Shared_DX8_Buffer()
	{
		static VertexBufferClass * buffer = NULL;
		return buffer;
	}

	static VertexBufferClass *& Shared_Sorting_Buffer()
	{
		static VertexBufferClass * buffer = NULL;
		return buffer;
	}

	void Attach_Shared_Buffer(int type)
	{
		const bool sorting_buffer = (type == 1 || type == 3);
		VertexBufferClass *& shared_buffer = sorting_buffer ? Shared_Sorting_Buffer() : Shared_DX8_Buffer();

		if (shared_buffer == NULL || shared_buffer->Get_Vertex_Count() < VertexCount) {
			REF_PTR_RELEASE(shared_buffer);
			shared_buffer = sorting_buffer ?
				static_cast<VertexBufferClass *>(new SortingVertexBufferClass(VertexCount)) :
				static_cast<VertexBufferClass *>(new DX8VertexBufferClass(dynamic_fvf_type, VertexCount));
		}

		REF_PTR_SET(BackingBuffer, shared_buffer);
	}

	FVFInfoClass FVFInfo;
	unsigned Type;
	uint16_t VertexCount;
	VertexBufferClass * BackingBuffer;
};

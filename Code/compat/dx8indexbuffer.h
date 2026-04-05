#pragma once

#include "refcount.h"

#include <vector>

class IndexBufferClass : public RefCountClass
{
public:
	static unsigned Get_Total_Allocated_Memory()
	{
		return 0;
	}

	IndexBufferClass(unsigned buffer_type, unsigned short index_count)
		: engine_refs(0),
		  index_count(index_count),
		  type(buffer_type),
		  Storage(static_cast<size_t>(index_count), 0)
	{
	}

	unsigned short Get_Index_Count() const
	{
		return index_count;
	}

	const unsigned short *Get_Index_Data() const
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

	class WriteLockClass
	{
	public:
		explicit WriteLockClass(IndexBufferClass * index_buffer)
			: IndexBuffer(index_buffer)
		{
		}

		unsigned short * Get_Index_Array()
		{
			return IndexBuffer != NULL ? IndexBuffer->Storage.data() : NULL;
		}

	private:
		IndexBufferClass * IndexBuffer;
	};

	class AppendLockClass
	{
	public:
		AppendLockClass(IndexBufferClass * index_buffer, unsigned start_index, unsigned)
			: IndexBuffer(index_buffer),
			  StartIndex(start_index)
		{
		}

		unsigned short * Get_Index_Array()
		{
			return IndexBuffer != NULL ? IndexBuffer->Storage.data() + StartIndex : NULL;
		}

	private:
		IndexBufferClass * IndexBuffer;
		unsigned StartIndex;
	};

protected:
	mutable unsigned engine_refs;
	unsigned short index_count;
	unsigned type;
	std::vector<unsigned short> Storage;
};

class DX8IndexBufferClass : public IndexBufferClass
{
public:
	explicit DX8IndexBufferClass(unsigned short index_count = 0)
		: IndexBufferClass(0, index_count)
	{
	}
};

class SortingIndexBufferClass : public IndexBufferClass
{
public:
	unsigned short* index_buffer;

	explicit SortingIndexBufferClass(unsigned short index_count = 0)
		: IndexBufferClass(1, index_count),
		  index_buffer(Storage.data())
	{
	}
};

class DynamicIBAccessClass
{
public:
	static void _Reset(bool)
	{
	}

	DynamicIBAccessClass(int type, int index_count)
		: Type(static_cast<unsigned>(type)),
		  IndexCount(static_cast<unsigned short>(index_count)),
		  BackingBuffer(NULL)
	{
		Attach_Shared_Buffer(type);
	}

	~DynamicIBAccessClass()
	{
		REF_PTR_RELEASE(BackingBuffer);
	}

	unsigned Get_Type() const
	{
		return Type;
	}

	unsigned short Get_Index_Count() const
	{
		return IndexCount;
	}

	const unsigned short *Get_Index_Data() const
	{
		return BackingBuffer != NULL ? BackingBuffer->Get_Index_Data() : NULL;
	}

	const IndexBufferClass * Get_Index_Buffer() const
	{
		return BackingBuffer;
	}

	class WriteLockClass
	{
	public:
		explicit WriteLockClass(DynamicIBAccessClass * access)
			: Access(access)
		{
		}

		unsigned short * Get_Index_Array()
		{
			return Access != nullptr && Access->BackingBuffer != NULL ? const_cast<unsigned short *>(Access->BackingBuffer->Get_Index_Data()) : nullptr;
		}

	private:
		DynamicIBAccessClass * Access;
	};

private:
	static IndexBufferClass *& Shared_DX8_Buffer()
	{
		static IndexBufferClass * buffer = NULL;
		return buffer;
	}

	static IndexBufferClass *& Shared_Sorting_Buffer()
	{
		static IndexBufferClass * buffer = NULL;
		return buffer;
	}

	void Attach_Shared_Buffer(int type)
	{
		const bool sorting_buffer = (type == 1 || type == 3);
		IndexBufferClass *& shared_buffer = sorting_buffer ? Shared_Sorting_Buffer() : Shared_DX8_Buffer();

		if (shared_buffer == NULL || shared_buffer->Get_Index_Count() < IndexCount) {
			REF_PTR_RELEASE(shared_buffer);
			shared_buffer = sorting_buffer ?
				static_cast<IndexBufferClass *>(new SortingIndexBufferClass(IndexCount)) :
				static_cast<IndexBufferClass *>(new DX8IndexBufferClass(IndexCount));
		}

		REF_PTR_SET(BackingBuffer, shared_buffer);
	}

	unsigned Type;
	unsigned short IndexCount;
	IndexBufferClass * BackingBuffer;
};

#pragma once

#include "refcount.h"

#include <vector>

class IndexBufferClass : public RefCountClass
{
public:
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
	explicit SortingIndexBufferClass(unsigned short index_count = 0)
		: IndexBufferClass(1, index_count)
	{
	}
};

class DynamicIBAccessClass
{
public:
	DynamicIBAccessClass(int, int index_count)
		: Type(0),
		  IndexCount(static_cast<unsigned short>(index_count)),
		  Storage(static_cast<size_t>(index_count), 0)
	{
	}

	unsigned Get_Type() const
	{
		return Type;
	}

	unsigned short Get_Index_Count() const
	{
		return IndexCount;
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
			return Access != nullptr ? Access->Storage.data() : nullptr;
		}

	private:
		DynamicIBAccessClass * Access;
	};

private:
	unsigned Type;
	unsigned short IndexCount;
	std::vector<unsigned short> Storage;
};

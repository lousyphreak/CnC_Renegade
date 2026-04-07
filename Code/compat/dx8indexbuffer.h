#pragma once

#include <cstdint>

#include "refcount.h"

#include <vector>

class IndexBufferClass : public RefCountClass
{
public:
	static unsigned Get_Total_Allocated_Memory()
	{
		return 0;
	}

	IndexBufferClass(unsigned buffer_type, uint16_t index_count)
		: engine_refs(0),
		  index_count(index_count),
		  type(buffer_type),
		  Storage(static_cast<size_t>(index_count), 0)
	{
	}

	uint16_t Get_Index_Count() const
	{
		return index_count;
	}

	const uint16_t *Get_Index_Data() const
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

		uint16_t * Get_Index_Array()
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

		uint16_t * Get_Index_Array()
		{
			return IndexBuffer != NULL ? IndexBuffer->Storage.data() + StartIndex : NULL;
		}

	private:
		IndexBufferClass * IndexBuffer;
		unsigned StartIndex;
	};

protected:
	mutable unsigned engine_refs;
	uint16_t index_count;
	unsigned type;
	std::vector<uint16_t> Storage;
};

class DX8IndexBufferClass : public IndexBufferClass
{
public:
	explicit DX8IndexBufferClass(uint16_t index_count = 0)
		: IndexBufferClass(0, index_count)
	{
	}
};

class SortingIndexBufferClass : public IndexBufferClass
{
public:
	uint16_t* index_buffer;

	explicit SortingIndexBufferClass(uint16_t index_count = 0)
		: IndexBufferClass(1, index_count),
		  index_buffer(Storage.data())
	{
	}
};

class DynamicIBAccessClass
{
public:
	static void _Reset(bool frame_changed)
	{
		Shared_Sorting_Buffer_Offset() = 0;
		if (frame_changed) {
			Shared_DX8_Buffer_Offset() = 0;
		}
	}

	DynamicIBAccessClass(int type, int index_count)
		: Type(static_cast<unsigned>(type)),
		  IndexCount(static_cast<uint16_t>(index_count)),
		  IndexBufferOffset(0),
		  BackingBuffer(NULL)
	{
		Attach_Shared_Buffer(type);
	}

	~DynamicIBAccessClass()
	{
		if (Type == 3) {
			Shared_Sorting_Buffer_Offset() = static_cast<uint16_t>(Shared_Sorting_Buffer_Offset() + IndexCount);
		} else {
			Shared_DX8_Buffer_Offset() = static_cast<uint16_t>(Shared_DX8_Buffer_Offset() + IndexCount);
		}
		REF_PTR_RELEASE(BackingBuffer);
	}

	unsigned Get_Type() const
	{
		return Type;
	}

	uint16_t Get_Index_Count() const
	{
		return IndexCount;
	}

	const uint16_t *Get_Index_Data() const
	{
		return BackingBuffer != NULL ? BackingBuffer->Get_Index_Data() + IndexBufferOffset : NULL;
	}

	const IndexBufferClass * Get_Index_Buffer() const
	{
		return BackingBuffer;
	}

	uint16_t Get_Index_Buffer_Offset() const
	{
		return IndexBufferOffset;
	}

	class WriteLockClass
	{
	public:
		explicit WriteLockClass(DynamicIBAccessClass * access)
			: Access(access)
		{
		}

		uint16_t * Get_Index_Array()
		{
			return Access != nullptr ? const_cast<uint16_t *>(Access->Get_Index_Data()) : nullptr;
		}

	private:
		DynamicIBAccessClass * Access;
	};

private:
	static constexpr uint16_t kDefaultSharedIndexCount = 5000;

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

	static uint16_t & Shared_DX8_Buffer_Offset()
	{
		static uint16_t offset = 0;
		return offset;
	}

	static uint16_t & Shared_Sorting_Buffer_Offset()
	{
		static uint16_t offset = 0;
		return offset;
	}

	void Attach_Shared_Buffer(int type)
	{
		const bool sorting_buffer = (type == 1 || type == 3);
		IndexBufferClass *& shared_buffer = sorting_buffer ? Shared_Sorting_Buffer() : Shared_DX8_Buffer();

		if (sorting_buffer) {
			const uint32_t required_index_count = static_cast<uint32_t>(Shared_Sorting_Buffer_Offset()) + static_cast<uint32_t>(IndexCount);
			if (shared_buffer == NULL || shared_buffer->Get_Index_Count() < required_index_count) {
				REF_PTR_RELEASE(shared_buffer);
			}
			if (shared_buffer == NULL) {
				uint32_t new_index_count = required_index_count;
				if (new_index_count < kDefaultSharedIndexCount) {
					new_index_count = kDefaultSharedIndexCount;
				}
				shared_buffer = static_cast<IndexBufferClass *>(new SortingIndexBufferClass(static_cast<uint16_t>(new_index_count)));
				Shared_Sorting_Buffer_Offset() = 0;
			}
			IndexBufferOffset = Shared_Sorting_Buffer_Offset();
		} else {
			if (shared_buffer == NULL || shared_buffer->Get_Index_Count() < IndexCount) {
				REF_PTR_RELEASE(shared_buffer);
			}
			if (shared_buffer == NULL) {
				uint32_t new_index_count = IndexCount;
				if (new_index_count < kDefaultSharedIndexCount) {
					new_index_count = kDefaultSharedIndexCount;
				}
				shared_buffer = static_cast<IndexBufferClass *>(new DX8IndexBufferClass(static_cast<uint16_t>(new_index_count)));
				Shared_DX8_Buffer_Offset() = 0;
			}
			if (static_cast<uint32_t>(Shared_DX8_Buffer_Offset()) + static_cast<uint32_t>(IndexCount) > shared_buffer->Get_Index_Count()) {
				Shared_DX8_Buffer_Offset() = 0;
			}
			IndexBufferOffset = Shared_DX8_Buffer_Offset();
		}

		REF_PTR_SET(BackingBuffer, shared_buffer);
	}

	unsigned Type;
	uint16_t IndexCount;
	uint16_t IndexBufferOffset;
	IndexBufferClass * BackingBuffer;
};

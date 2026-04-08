#include "dx8indexbuffer.h"
#include "dx8wrapper.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace
{
const unsigned short kDefaultDynamicIndexCount = 5000;

SortingIndexBufferClass *g_dynamic_sorting_index_buffer = nullptr;
bool g_dynamic_sorting_index_buffer_in_use = false;
unsigned short g_dynamic_sorting_index_buffer_size = 0;
unsigned short g_dynamic_sorting_index_buffer_offset = 0;

unsigned g_index_buffer_count = 0;
unsigned g_index_buffer_total_indices = 0;
unsigned g_index_buffer_total_size = 0;

std::unordered_map<const DX8IndexBufferClass *, std::vector<unsigned short>> g_dx8_index_buffers;

std::vector<unsigned short> &Get_DX8_Index_Data(const DX8IndexBufferClass *buffer)
{
	return g_dx8_index_buffers[buffer];
}
}

IndexBufferClass::IndexBufferClass(unsigned type_, unsigned short index_count_)
	: engine_refs(0), index_count(index_count_), type(type_)
{
	++g_index_buffer_count;
	g_index_buffer_total_indices += index_count;
	g_index_buffer_total_size += index_count * sizeof(unsigned short);
}

IndexBufferClass::~IndexBufferClass()
{
	--g_index_buffer_count;
	g_index_buffer_total_indices -= index_count;
	g_index_buffer_total_size -= index_count * sizeof(unsigned short);
}

void IndexBufferClass::Add_Engine_Ref() const
{
	++engine_refs;
}

void IndexBufferClass::Release_Engine_Ref() const
{
	--engine_refs;
	WWASSERT(engine_refs >= 0);
}

unsigned IndexBufferClass::Get_Total_Buffer_Count()
{
	return g_index_buffer_count;
}

unsigned IndexBufferClass::Get_Total_Allocated_Indices()
{
	return g_index_buffer_total_indices;
}

unsigned IndexBufferClass::Get_Total_Allocated_Memory()
{
	return g_index_buffer_total_size;
}

IndexBufferClass::WriteLockClass::WriteLockClass(IndexBufferClass *index_buffer_) : index_buffer(index_buffer_), indices(nullptr)
{
	WWASSERT(index_buffer != nullptr);
	index_buffer->Add_Ref();
	switch (index_buffer->Type()) {
	case BUFFER_TYPE_DX8:
		indices = Get_DX8_Index_Data(static_cast<DX8IndexBufferClass *>(index_buffer)).data();
		break;
	case BUFFER_TYPE_SORTING:
		indices = static_cast<SortingIndexBufferClass *>(index_buffer)->index_buffer;
		break;
	default:
		WWASSERT(0);
		break;
	}
}

IndexBufferClass::WriteLockClass::~WriteLockClass()
{
	index_buffer->Release_Ref();
}

IndexBufferClass::AppendLockClass::AppendLockClass(IndexBufferClass *index_buffer_, unsigned start_index, unsigned) : index_buffer(index_buffer_), indices(nullptr)
{
	WWASSERT(index_buffer != nullptr);
	index_buffer->Add_Ref();
	switch (index_buffer->Type()) {
	case BUFFER_TYPE_DX8:
		indices = Get_DX8_Index_Data(static_cast<DX8IndexBufferClass *>(index_buffer)).data() + start_index;
		break;
	case BUFFER_TYPE_SORTING:
		indices = static_cast<SortingIndexBufferClass *>(index_buffer)->index_buffer + start_index;
		break;
	default:
		WWASSERT(0);
		break;
	}
}

IndexBufferClass::AppendLockClass::~AppendLockClass()
{
	index_buffer->Release_Ref();
}

void IndexBufferClass::Copy(unsigned int *indices_, unsigned first_index, unsigned count)
{
	if (first_index != 0) {
		AppendLockClass lock(this, first_index, count);
		unsigned short *destination = lock.Get_Index_Array();
		for (unsigned i = 0; i < count; ++i) {
			destination[i] = static_cast<unsigned short>(indices_[i]);
		}
		return;
	}

	WriteLockClass lock(this);
	unsigned short *destination = lock.Get_Index_Array();
	for (unsigned i = 0; i < count; ++i) {
		destination[i] = static_cast<unsigned short>(indices_[i]);
	}
}

void IndexBufferClass::Copy(unsigned short *indices_, unsigned first_index, unsigned count)
{
	if (first_index != 0) {
		AppendLockClass lock(this, first_index, count);
		std::copy(indices_, indices_ + count, lock.Get_Index_Array());
		return;
	}

	WriteLockClass lock(this);
	std::copy(indices_, indices_ + count, lock.Get_Index_Array());
}

DX8IndexBufferClass::DX8IndexBufferClass(unsigned short index_count_, UsageType) : IndexBufferClass(BUFFER_TYPE_DX8, index_count_), index_buffer(nullptr)
{
	g_dx8_index_buffers[this].resize(index_count_);
}

DX8IndexBufferClass::~DX8IndexBufferClass()
{
	g_dx8_index_buffers.erase(this);
}

void DX8IndexBufferClass::Copy(unsigned int *indices_, unsigned start_index, unsigned index_count_)
{
	IndexBufferClass::Copy(indices_, start_index, index_count_);
}

void DX8IndexBufferClass::Copy(unsigned short *indices_, unsigned start_index, unsigned index_count_)
{
	IndexBufferClass::Copy(indices_, start_index, index_count_);
}

SortingIndexBufferClass::SortingIndexBufferClass(unsigned short index_count_) : IndexBufferClass(BUFFER_TYPE_SORTING, index_count_), index_buffer(new unsigned short[index_count_])
{
}

SortingIndexBufferClass::~SortingIndexBufferClass()
{
	delete[] index_buffer;
}

DynamicIBAccessClass::DynamicIBAccessClass(unsigned short type_, unsigned short index_count_)
	: Type(type_ == BUFFER_TYPE_DYNAMIC_DX8 ? BUFFER_TYPE_DYNAMIC_SORTING : type_), IndexCount(index_count_), IndexBufferOffset(0), IndexBuffer(nullptr)
{
	WWASSERT(Type == BUFFER_TYPE_DYNAMIC_SORTING);
	Allocate_Sorting_Dynamic_Buffer();
}

DynamicIBAccessClass::~DynamicIBAccessClass()
{
	REF_PTR_RELEASE(IndexBuffer);
	g_dynamic_sorting_index_buffer_in_use = false;
	g_dynamic_sorting_index_buffer_offset += IndexCount;
}

void DynamicIBAccessClass::_Deinit()
{
	REF_PTR_RELEASE(g_dynamic_sorting_index_buffer);
	g_dynamic_sorting_index_buffer_in_use = false;
	g_dynamic_sorting_index_buffer_size = 0;
	g_dynamic_sorting_index_buffer_offset = 0;
}

void DynamicIBAccessClass::_Reset(bool)
{
	g_dynamic_sorting_index_buffer_offset = 0;
}

DynamicIBAccessClass::WriteLockClass::WriteLockClass(DynamicIBAccessClass *ib_access) : DynamicIBAccess(ib_access), Indices(nullptr)
{
	WWASSERT(ib_access != nullptr);
	DynamicIBAccess->IndexBuffer->Add_Ref();
	Indices = static_cast<SortingIndexBufferClass *>(DynamicIBAccess->IndexBuffer)->index_buffer + DynamicIBAccess->IndexBufferOffset;
}

DynamicIBAccessClass::WriteLockClass::~WriteLockClass()
{
	DynamicIBAccess->IndexBuffer->Release_Ref();
}

void DynamicIBAccessClass::Allocate_DX8_Dynamic_Buffer()
{
	Allocate_Sorting_Dynamic_Buffer();
}

void DynamicIBAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	WWASSERT(!g_dynamic_sorting_index_buffer_in_use);
	g_dynamic_sorting_index_buffer_in_use = true;

	const unsigned required_index_count = g_dynamic_sorting_index_buffer_offset + IndexCount;
	WWASSERT(required_index_count < 65536);
	if (required_index_count > g_dynamic_sorting_index_buffer_size) {
		REF_PTR_RELEASE(g_dynamic_sorting_index_buffer);
		g_dynamic_sorting_index_buffer_size = std::max<unsigned short>(static_cast<unsigned short>(required_index_count), kDefaultDynamicIndexCount);
	}

	if (g_dynamic_sorting_index_buffer == nullptr) {
		g_dynamic_sorting_index_buffer = NEW_REF(SortingIndexBufferClass, (g_dynamic_sorting_index_buffer_size));
		g_dynamic_sorting_index_buffer_offset = 0;
	}

	REF_PTR_SET(IndexBuffer, g_dynamic_sorting_index_buffer);
	IndexBufferOffset = g_dynamic_sorting_index_buffer_offset;
}

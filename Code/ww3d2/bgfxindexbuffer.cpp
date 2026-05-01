#include "indexbuffer.h"
#include "dx8wrapper.h"
#include "bgfxrenderer.h"

#include <algorithm>
#include <vector>

namespace
{
const unsigned short kDefaultDynamicIndexCount = 5000;

std::vector<RenderIndexBufferClass *> g_stale_dynamic_render_index_buffers;

SortingIndexBufferClass *g_dynamic_sorting_index_buffer = nullptr;
bool g_dynamic_sorting_index_buffer_in_use = false;
unsigned short g_dynamic_sorting_index_buffer_size = 0;
unsigned short g_dynamic_sorting_index_buffer_offset = 0;
std::vector<SortingIndexBufferClass *> g_stale_dynamic_sorting_index_buffers;

unsigned g_index_buffer_count = 0;
unsigned g_index_buffer_total_indices = 0;
unsigned g_index_buffer_total_size = 0;

void Release_Stale_Dynamic_Sorting_Index_Buffers()
{
	auto it = g_stale_dynamic_sorting_index_buffers.begin();
	while (it != g_stale_dynamic_sorting_index_buffers.end()) {
		SortingIndexBufferClass *buffer = *it;
		if ((buffer == nullptr) || (buffer->Engine_Refs() == 0)) {
			if (buffer != nullptr) {
				buffer->Release_Ref();
			}
			it = g_stale_dynamic_sorting_index_buffers.erase(it);
		} else {
			++it;
		}
	}
}

void Release_Stale_Dynamic_Render_Index_Buffers()
{
	auto it = g_stale_dynamic_render_index_buffers.begin();
	while (it != g_stale_dynamic_render_index_buffers.end()) {
		RenderIndexBufferClass *buffer = *it;
		if (buffer == nullptr || buffer->Engine_Refs() == 0) {
			if (buffer != nullptr) {
				buffer->Release_Ref();
			}
			it = g_stale_dynamic_render_index_buffers.erase(it);
		} else {
			++it;
		}
	}
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
	case BUFFER_TYPE_RENDER:
	case BUFFER_TYPE_DYNAMIC_RENDER:
		indices = static_cast<RenderIndexBufferClass *>(index_buffer)->Get_Source_Index_Data();
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
	case BUFFER_TYPE_RENDER:
	case BUFFER_TYPE_DYNAMIC_RENDER:
		indices = static_cast<RenderIndexBufferClass *>(index_buffer)->Get_Source_Index_Data() + start_index;
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

RenderIndexBufferClass::RenderIndexBufferClass(unsigned short index_count_, UsageType, unsigned type)
#if !RENEGADE_WITH_BGFX_RENDERER
	: IndexBufferClass(type, index_count_), index_buffer(nullptr)
#else
	: IndexBufferClass(type, index_count_),
	  BgfxIndexBuffer(BGFX_INVALID_HANDLE),
	  BgfxIndexBufferDirty(true),
	  IndexData(index_count_)
#endif
{
	WWASSERT(type == BUFFER_TYPE_RENDER || type == BUFFER_TYPE_DYNAMIC_RENDER);
}

RenderIndexBufferClass::~RenderIndexBufferClass()
{
#if RENEGADE_WITH_BGFX_RENDERER
	if (bgfx::isValid(BgfxIndexBuffer)) {
		if (BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxIndexBuffer);
		}
		BgfxIndexBuffer = BGFX_INVALID_HANDLE;
	}
#endif
}

void RenderIndexBufferClass::Copy(unsigned int *indices_, unsigned start_index, unsigned index_count_)
{
	IndexBufferClass::Copy(indices_, start_index, index_count_);
}

void RenderIndexBufferClass::Copy(unsigned short *indices_, unsigned start_index, unsigned index_count_)
{
	IndexBufferClass::Copy(indices_, start_index, index_count_);
}

#if RENEGADE_WITH_BGFX_RENDERER
unsigned short *RenderIndexBufferClass::Get_Source_Index_Data()
{
	Mark_Bgfx_Buffer_Dirty();
	return IndexData.data();
}

const unsigned short *RenderIndexBufferClass::Get_Source_Index_Data() const
{
	return IndexData.data();
}

bool RenderIndexBufferClass::Ensure_Bgfx_Buffer() const
{
	WWASSERT(Type() == BUFFER_TYPE_RENDER);
	return Sync_Bgfx_Buffer();
}

bgfx::IndexBufferHandle RenderIndexBufferClass::Get_Bgfx_Index_Buffer() const
{
	return BgfxIndexBuffer;
}

void RenderIndexBufferClass::Mark_Bgfx_Buffer_Dirty()
{
	if (bgfx::isValid(BgfxIndexBuffer)) {
		if (BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxIndexBuffer);
		}
		BgfxIndexBuffer = BGFX_INVALID_HANDLE;
	}
	BgfxIndexBufferDirty = true;
}

bool RenderIndexBufferClass::Sync_Bgfx_Buffer() const
{
	if (!BgfxRenderer::Is_Initted()) {
		return false;
	}

	if (Type() != BUFFER_TYPE_RENDER) {
		return false;
	}

	if (bgfx::isValid(BgfxIndexBuffer) && !BgfxIndexBufferDirty) {
		return true;
	}

	// Destroy any existing buffer (immutable buffers can't be updated)
	if (bgfx::isValid(BgfxIndexBuffer)) {
		bgfx::destroy(BgfxIndexBuffer);
		BgfxIndexBuffer = BGFX_INVALID_HANDLE;
	}

	const bgfx::Memory *index_memory = bgfx::copy(
		IndexData.data(),
		static_cast<uint32_t>(IndexData.size() * sizeof(unsigned short)));
	BgfxIndexBuffer = bgfx::createIndexBuffer(index_memory);
	BgfxIndexBufferDirty = false;
	return bgfx::isValid(BgfxIndexBuffer);
}
#endif

SortingIndexBufferClass::SortingIndexBufferClass(unsigned short index_count_) : IndexBufferClass(BUFFER_TYPE_SORTING, index_count_), index_buffer(new unsigned short[index_count_])
{
}

SortingIndexBufferClass::~SortingIndexBufferClass()
{
	delete[] index_buffer;
}

DynamicIBAccessClass::DynamicIBAccessClass(unsigned short type_, unsigned short index_count_)
	: Type(type_), IndexCount(index_count_), IndexBufferOffset(0), IndexBuffer(nullptr)
{
	WWASSERT(Type == BUFFER_TYPE_DYNAMIC_RENDER || Type == BUFFER_TYPE_DYNAMIC_SORTING);
	if (Type == BUFFER_TYPE_DYNAMIC_RENDER) {
		Allocate_Render_Dynamic_Buffer();
	} else {
		Allocate_Sorting_Dynamic_Buffer();
	}
}

DynamicIBAccessClass::~DynamicIBAccessClass()
{
	if (Type == BUFFER_TYPE_DYNAMIC_SORTING) {
		g_dynamic_sorting_index_buffer_in_use = false;
		g_dynamic_sorting_index_buffer_offset += IndexCount;
	} else if (IndexBuffer != nullptr && IndexBuffer->Engine_Refs() > 0) {
		g_stale_dynamic_render_index_buffers.push_back(static_cast<RenderIndexBufferClass *>(IndexBuffer));
		IndexBuffer = nullptr;
	}
	REF_PTR_RELEASE(IndexBuffer);
}

void DynamicIBAccessClass::_Deinit()
{
	for (RenderIndexBufferClass *buffer : g_stale_dynamic_render_index_buffers) {
		if (buffer != nullptr) {
			buffer->Release_Ref();
		}
	}
	g_stale_dynamic_render_index_buffers.clear();

	REF_PTR_RELEASE(g_dynamic_sorting_index_buffer);
	for (SortingIndexBufferClass *buffer : g_stale_dynamic_sorting_index_buffers) {
		if (buffer != nullptr) {
			buffer->Release_Ref();
		}
	}
	g_stale_dynamic_sorting_index_buffers.clear();
	g_dynamic_sorting_index_buffer_in_use = false;
	g_dynamic_sorting_index_buffer_size = 0;
	g_dynamic_sorting_index_buffer_offset = 0;
}

void DynamicIBAccessClass::_Reset(bool frame_changed)
{
	Release_Stale_Dynamic_Render_Index_Buffers();
	Release_Stale_Dynamic_Sorting_Index_Buffers();
	g_dynamic_sorting_index_buffer_offset = 0;
	(void)frame_changed;
}

DynamicIBAccessClass::WriteLockClass::WriteLockClass(DynamicIBAccessClass *ib_access)
	: DynamicIBAccess(ib_access), Indices(nullptr)
{
	WWASSERT(ib_access != nullptr);
	DynamicIBAccess->IndexBuffer->Add_Ref();
	switch (DynamicIBAccess->Get_Type()) {
	case BUFFER_TYPE_DYNAMIC_RENDER:
		Indices =
			static_cast<RenderIndexBufferClass *>(DynamicIBAccess->IndexBuffer)->Get_Source_Index_Data()
			+ DynamicIBAccess->IndexBufferOffset;
		break;
	case BUFFER_TYPE_DYNAMIC_SORTING:
		Indices = static_cast<SortingIndexBufferClass *>(DynamicIBAccess->IndexBuffer)->index_buffer + DynamicIBAccess->IndexBufferOffset;
		break;
	default:
		WWASSERT(0);
		break;
	}
}

DynamicIBAccessClass::WriteLockClass::~WriteLockClass()
{
	DynamicIBAccess->IndexBuffer->Release_Ref();
}

void DynamicIBAccessClass::Allocate_Render_Dynamic_Buffer()
{
	WWASSERT(IndexBuffer == nullptr);
	IndexBuffer = NEW_REF(
		RenderIndexBufferClass,
		(std::max<unsigned short>(IndexCount, static_cast<unsigned short>(1)),
		 RenderIndexBufferClass::USAGE_DEFAULT,
		 BUFFER_TYPE_DYNAMIC_RENDER));
	IndexBufferOffset = 0;
}

void DynamicIBAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	WWASSERT(!g_dynamic_sorting_index_buffer_in_use);
	g_dynamic_sorting_index_buffer_in_use = true;

	const unsigned required_index_count = g_dynamic_sorting_index_buffer_offset + IndexCount;
	WWASSERT(required_index_count < 65536);
	if (required_index_count > g_dynamic_sorting_index_buffer_size) {
		if (g_dynamic_sorting_index_buffer != nullptr) {
			if (g_dynamic_sorting_index_buffer->Engine_Refs() > 0) {
				g_stale_dynamic_sorting_index_buffers.push_back(g_dynamic_sorting_index_buffer);
			} else {
				REF_PTR_RELEASE(g_dynamic_sorting_index_buffer);
			}
			g_dynamic_sorting_index_buffer = nullptr;
		}
		g_dynamic_sorting_index_buffer_size = std::max<unsigned short>(static_cast<unsigned short>(required_index_count), kDefaultDynamicIndexCount);
	}

	if (g_dynamic_sorting_index_buffer == nullptr) {
		g_dynamic_sorting_index_buffer = NEW_REF(SortingIndexBufferClass, (g_dynamic_sorting_index_buffer_size));
		g_dynamic_sorting_index_buffer_offset = 0;
	}

	REF_PTR_SET(IndexBuffer, g_dynamic_sorting_index_buffer);
	IndexBufferOffset = g_dynamic_sorting_index_buffer_offset;
}

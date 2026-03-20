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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WWDebug                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwdebug/wwmemlog.cpp                         $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 11/21/01 2:03p                                              $*
 *                                                                                             *
 *                    $Revision:: 27                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WWMemoryLogClass::Allocate_Memory -- allocates memory                                     *
 *   WWMemoryLogClass::Release_Memory -- frees memory                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "wwmemlog.h"
#include "wwdebug.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {

constexpr int WWMEMLOG_KEY0 = (unsigned('G') << 24) | (unsigned('g') << 16) | (unsigned('0') << 8) | unsigned('l');
constexpr int WWMEMLOG_KEY1 = (unsigned('~') << 24) | (unsigned('_') << 16) | (unsigned('d') << 8) | unsigned('3');

constexpr std::array<const char *, MEM_COUNT> kMemoryCategoryNames = {
	"UNKNOWN",
	"Geometry",
	"Animation",
	"Texture",
	"Pathfind",
	"Vis",
	"Sound",
	"CullingData",
	"Strings",
	"GameData",
	"PhysicsData",
	"W3dData",
	"StaticAllocations",
	"GameInit",
	"Renderer",
	"Network",
	"BINK",
};

struct alignas(std::max_align_t) MemoryLogStruct {
	int Key0;
	int Key1;
	int Category;
	int Size;

	bool Is_Valid() const
	{
		return (Key0 == WWMEMLOG_KEY0) && (Key1 == WWMEMLOG_KEY1);
	}
};

std::array<std::atomic<int>, MEM_COUNT> g_current_allocated_memory{};
std::array<std::atomic<int>, MEM_COUNT> g_peak_allocated_memory{};
std::atomic<int> g_live_allocation_count{0};
std::atomic<int> g_current_allocated_size{0};
std::atomic<int> g_allocate_count{0};
std::atomic<int> g_free_count{0};
std::atomic<bool> g_memlog_initialized{false};
thread_local std::vector<int> g_category_stack;

int Clamp_Category(int category)
{
	if (category < 0 || category >= MEM_COUNT) {
		return MEM_UNKNOWN;
	}
	return category;
}

int Get_Default_Category()
{
	return g_memlog_initialized.load(std::memory_order_acquire) ? MEM_UNKNOWN : MEM_STATICALLOCATION;
}

int Get_Active_Category()
{
	if (g_category_stack.empty()) {
		return Get_Default_Category();
	}
	return Clamp_Category(g_category_stack.back());
}

void Update_Peak_Allocated_Memory(int category, int current_value)
{
	int observed = g_peak_allocated_memory[category].load(std::memory_order_relaxed);
	while ((current_value > observed) &&
		!g_peak_allocated_memory[category].compare_exchange_weak(
			observed,
			current_value,
			std::memory_order_relaxed,
			std::memory_order_relaxed)) {
	}
}

} // namespace

int WWMemoryLogClass::Get_Category_Count(void)
{
	return MEM_COUNT;
}

const char * WWMemoryLogClass::Get_Category_Name(int category)
{
	return (category >= 0 && category < MEM_COUNT) ? kMemoryCategoryNames[category] : "UNKNOWN";
}

int WWMemoryLogClass::Get_Current_Allocated_Memory(int category)
{
	if (category < 0 || category >= MEM_COUNT) {
		return 0;
	}
	return g_current_allocated_memory[category].load(std::memory_order_relaxed);
}

int WWMemoryLogClass::Get_Peak_Allocated_Memory(int category)
{
	if (category < 0 || category >= MEM_COUNT) {
		return 0;
	}
	return g_peak_allocated_memory[category].load(std::memory_order_relaxed);
}

int WWMemoryLogClass::Get_Current_Allocation_Count()
{
	return g_live_allocation_count.load(std::memory_order_relaxed);
}

int WWMemoryLogClass::Get_Current_Allocated_Size()
{
	return g_current_allocated_size.load(std::memory_order_relaxed);
}

int WWMemoryLogClass::Register_Memory_Allocated(int size)
{
	const int category = Get_Active_Category();
	const int current_memory = g_current_allocated_memory[category].fetch_add(size, std::memory_order_relaxed) + size;
	Update_Peak_Allocated_Memory(category, current_memory);
	g_live_allocation_count.fetch_add(1, std::memory_order_relaxed);
	g_current_allocated_size.fetch_add(size, std::memory_order_relaxed);
	return category;
}

void WWMemoryLogClass::Register_Memory_Released(int category,int size)
{
	const int clamped_category = Clamp_Category(category);
	g_current_allocated_memory[clamped_category].fetch_sub(size, std::memory_order_relaxed);
	g_live_allocation_count.fetch_sub(1, std::memory_order_relaxed);
	g_current_allocated_size.fetch_sub(size, std::memory_order_relaxed);
}

void WWMemoryLogClass::Push_Active_Category(int category)
{
	g_category_stack.push_back(Clamp_Category(category));
}

void WWMemoryLogClass::Pop_Active_Category(void)
{
	if (!g_category_stack.empty()) {
		g_category_stack.pop_back();
	}
}

void * WWMemoryLogClass::Allocate_Memory(size_t size)
{
	if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
		return nullptr;
	}

	if (size > (std::numeric_limits<size_t>::max() - sizeof(MemoryLogStruct))) {
		return nullptr;
	}

	void * raw_memory = std::malloc(size + sizeof(MemoryLogStruct));
	if (raw_memory == nullptr) {
		return nullptr;
	}

	const int category = Register_Memory_Allocated(static_cast<int>(size));
	auto * header = static_cast<MemoryLogStruct *>(raw_memory);
	header->Key0 = WWMEMLOG_KEY0;
	header->Key1 = WWMEMLOG_KEY1;
	header->Category = category;
	header->Size = static_cast<int>(size);
	g_allocate_count.fetch_add(1, std::memory_order_relaxed);
	return static_cast<void *>(header + 1);
}

void WWMemoryLogClass::Release_Memory(void * ptr)
{
	if (ptr == nullptr) {
		return;
	}

	auto * header = static_cast<MemoryLogStruct *>(ptr) - 1;
	if (header->Is_Valid()) {
		Register_Memory_Released(header->Category, header->Size);
		std::free(header);
	} else {
		std::free(ptr);
	}

	g_free_count.fetch_add(1, std::memory_order_relaxed);
}

void WWMemoryLogClass::Reset_Counters()
{
	g_allocate_count.store(0, std::memory_order_relaxed);
	g_free_count.store(0, std::memory_order_relaxed);
}

int WWMemoryLogClass::Get_Allocate_Count()
{
	return g_allocate_count.load(std::memory_order_relaxed);
}

int WWMemoryLogClass::Get_Free_Count()
{
	return g_free_count.load(std::memory_order_relaxed);
}

void WWMemoryLogClass::Init()
{
	g_memlog_initialized.store(true, std::memory_order_release);
	g_category_stack.clear();
}

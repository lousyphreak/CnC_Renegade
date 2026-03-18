#include "wwmemlog.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
constexpr int WWMEMLOG_KEY0 = (unsigned('G')<<24) | (unsigned('g')<<16) | (unsigned('0')<<8) | unsigned('l');
constexpr int WWMEMLOG_KEY1 = (unsigned('~')<<24) | (unsigned('_')<<16) | (unsigned('d')<<8) | unsigned('3');

struct MemoryLogStruct {
    int Key0;
    int Key1;
    int Category;
    int Size;
};

std::array<std::atomic<int>, MEM_COUNT> g_current{};
std::array<std::atomic<int>, MEM_COUNT> g_peak{};
std::atomic<int> g_allocate_count{0};
std::atomic<int> g_free_count{0};
thread_local std::vector<int> g_categories{MEM_STATICALLOCATION};

int Active_Category()
{
    if (g_categories.empty()) {
        return MEM_UNKNOWN;
    }
    return g_categories.back();
}

void Update_Peak(int category, int value)
{
    int observed = g_peak[category].load(std::memory_order_relaxed);
    while (value > observed && !g_peak[category].compare_exchange_weak(observed, value, std::memory_order_relaxed)) {
    }
}
}

int WWMemoryLogClass::Get_Category_Count(void)
{
    return MEM_COUNT;
}

const char * WWMemoryLogClass::Get_Category_Name(int category)
{
    static const char * kNames[MEM_COUNT] = {
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
        "BINK"
    };
    return (category >= 0 && category < MEM_COUNT) ? kNames[category] : "UNKNOWN";
}

int WWMemoryLogClass::Get_Current_Allocated_Memory(int category)
{
    return (category >= 0 && category < MEM_COUNT) ? g_current[category].load(std::memory_order_relaxed) : 0;
}

int WWMemoryLogClass::Get_Peak_Allocated_Memory(int category)
{
    return (category >= 0 && category < MEM_COUNT) ? g_peak[category].load(std::memory_order_relaxed) : 0;
}

int WWMemoryLogClass::Register_Memory_Allocated(int size)
{
    const int category = Active_Category();
    const int current = g_current[category].fetch_add(size, std::memory_order_relaxed) + size;
    Update_Peak(category, current);
    return category;
}

void WWMemoryLogClass::Register_Memory_Released(int category, int size)
{
    if (category >= 0 && category < MEM_COUNT) {
        g_current[category].fetch_sub(size, std::memory_order_relaxed);
    }
}

void WWMemoryLogClass::Push_Active_Category(int category)
{
    g_categories.push_back(category);
}

void WWMemoryLogClass::Pop_Active_Category(void)
{
    if (!g_categories.empty()) {
        g_categories.pop_back();
    }
    if (g_categories.empty()) {
        g_categories.push_back(MEM_UNKNOWN);
    }
}

void * WWMemoryLogClass::Allocate_Memory(size_t size)
{
    g_allocate_count.fetch_add(1, std::memory_order_relaxed);
    void * raw = std::malloc(size + sizeof(MemoryLogStruct));
    if (raw == nullptr) {
        return nullptr;
    }

    const int category = Register_Memory_Allocated(static_cast<int>(size));
    auto * header = static_cast<MemoryLogStruct *>(raw);
    header->Key0 = WWMEMLOG_KEY0;
    header->Key1 = WWMEMLOG_KEY1;
    header->Category = category;
    header->Size = static_cast<int>(size);
    return static_cast<void *>(header + 1);
}

void WWMemoryLogClass::Release_Memory(void * mem)
{
    if (mem == nullptr) {
        return;
    }

    auto * header = static_cast<MemoryLogStruct *>(mem) - 1;
    if (header->Key0 == WWMEMLOG_KEY0 && header->Key1 == WWMEMLOG_KEY1) {
        Register_Memory_Released(header->Category, header->Size);
        std::free(header);
    } else {
        std::free(mem);
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
    g_categories.clear();
    g_categories.push_back(MEM_UNKNOWN);
}

MemLogClass * WWMemoryLogClass::Get_Log(void)
{
    return nullptr;
}

void WWMemoryLogClass::Release_Log(void)
{
}

#include "mutex.h"

#include "wwdebug.h"
#include <chrono>
#include <mutex>

namespace {
using MutexHandle = std::timed_mutex;
using CriticalSectionHandle = std::recursive_mutex;
}

MutexClass::MutexClass(const char *) : handle(new MutexHandle()), locked(false)
{
}

MutexClass::~MutexClass()
{
    WWASSERT(!locked);
    delete static_cast<MutexHandle *>(handle);
    handle = nullptr;
}

bool MutexClass::Lock(int time)
{
    auto * mutex = static_cast<MutexHandle *>(handle);
    if (time == WAIT_INFINITE) {
        mutex->lock();
        ++locked;
        return true;
    }

    if (mutex->try_lock_for(std::chrono::milliseconds{time})) {
        ++locked;
        return true;
    }

    return false;
}

void MutexClass::Unlock()
{
    WWASSERT(locked);
    --locked;
    static_cast<MutexHandle *>(handle)->unlock();
}

MutexClass::LockClass::LockClass(MutexClass & mutex_, int time) : mutex(mutex_)
{
    failed = !mutex.Lock(time);
}

MutexClass::LockClass::~LockClass()
{
    if (!failed) {
        mutex.Unlock();
    }
}

CriticalSectionClass::CriticalSectionClass() : handle(new CriticalSectionHandle()), locked(false)
{
}

CriticalSectionClass::~CriticalSectionClass()
{
    WWASSERT(!locked);
    delete static_cast<CriticalSectionHandle *>(handle);
    handle = nullptr;
}

void CriticalSectionClass::Lock()
{
    static_cast<CriticalSectionHandle *>(handle)->lock();
    ++locked;
}

void CriticalSectionClass::Unlock()
{
    WWASSERT(locked);
    --locked;
    static_cast<CriticalSectionHandle *>(handle)->unlock();
}

CriticalSectionClass::LockClass::LockClass(CriticalSectionClass & critical_section) : CriticalSection(critical_section)
{
    CriticalSection.Lock();
}

CriticalSectionClass::LockClass::~LockClass()
{
    CriticalSection.Unlock();
}

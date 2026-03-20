#include "mutex.h"

#include "wwlib_debug.h"

#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>

namespace
{
	struct WWLibMutexHandle
	{
		SDL_Mutex * Mutex;
	};

	struct WWLibCriticalSectionHandle
	{
		SDL_Mutex * Mutex;
		SDL_ThreadID Owner;
		unsigned Recursion;
	};
}

MutexClass::MutexClass(const char *) : handle(new WWLibMutexHandle{SDL_CreateMutex()}), locked(false)
{
	WWASSERT(handle != NULL);
	WWASSERT(static_cast<WWLibMutexHandle *>(handle)->Mutex != NULL);
}

MutexClass::~MutexClass()
{
	WWASSERT(!locked);
	if (handle != NULL) {
		SDL_DestroyMutex(static_cast<WWLibMutexHandle *>(handle)->Mutex);
		delete static_cast<WWLibMutexHandle *>(handle);
		handle = NULL;
	}
}

bool MutexClass::Lock(int time)
{
	WWLibMutexHandle * mutex = static_cast<WWLibMutexHandle *>(handle);
	WWASSERT(mutex != NULL && mutex->Mutex != NULL);

	if (time == WAIT_INFINITE) {
		SDL_LockMutex(mutex->Mutex);
		locked++;
		return true;
	}

	const Uint64 start = SDL_GetTicks();
	while ((SDL_GetTicks() - start) < static_cast<Uint64>(time)) {
		if (SDL_TryLockMutex(mutex->Mutex)) {
			locked++;
			return true;
		}
		SDL_Delay(1);
	}

	return false;
}

void MutexClass::Unlock()
{
	WWASSERT(locked);
	locked--;
	SDL_UnlockMutex(static_cast<WWLibMutexHandle *>(handle)->Mutex);
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

CriticalSectionClass::CriticalSectionClass()
	: handle(new WWLibCriticalSectionHandle{SDL_CreateMutex(), 0, 0}), locked(false)
{
	WWASSERT(handle != NULL);
	WWASSERT(static_cast<WWLibCriticalSectionHandle *>(handle)->Mutex != NULL);
}

CriticalSectionClass::~CriticalSectionClass()
{
	WWASSERT(!locked);
	if (handle != NULL) {
		SDL_DestroyMutex(static_cast<WWLibCriticalSectionHandle *>(handle)->Mutex);
		delete static_cast<WWLibCriticalSectionHandle *>(handle);
		handle = NULL;
	}
}

void CriticalSectionClass::Lock()
{
	WWLibCriticalSectionHandle * critical_section = static_cast<WWLibCriticalSectionHandle *>(handle);
	const SDL_ThreadID current_thread = SDL_GetCurrentThreadID();
	if (critical_section->Owner == current_thread && critical_section->Recursion > 0) {
		critical_section->Recursion++;
		locked++;
		return;
	}

	SDL_LockMutex(critical_section->Mutex);
	critical_section->Owner = current_thread;
	critical_section->Recursion = 1;
	locked++;
}

void CriticalSectionClass::Unlock()
{
	WWASSERT(locked);
	WWLibCriticalSectionHandle * critical_section = static_cast<WWLibCriticalSectionHandle *>(handle);
	WWASSERT(critical_section->Owner == SDL_GetCurrentThreadID());
	WWASSERT(critical_section->Recursion > 0);

	locked--;
	critical_section->Recursion--;
	if (critical_section->Recursion == 0) {
		critical_section->Owner = 0;
		SDL_UnlockMutex(critical_section->Mutex);
	}
}

CriticalSectionClass::LockClass::LockClass(CriticalSectionClass & critical_section) : CriticalSection(critical_section)
{
	CriticalSection.Lock();
}

CriticalSectionClass::LockClass::~LockClass()
{
	CriticalSection.Unlock();
}

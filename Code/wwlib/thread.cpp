#include "thread.h"

#include "wwlib_debug.h"

#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>

#include <cstring>

namespace
{
	SDL_ThreadPriority WWLib_Map_Thread_Priority(int priority)
	{
		if (priority >= 2) {
			return SDL_THREAD_PRIORITY_TIME_CRITICAL;
		}
		if (priority > 0) {
			return SDL_THREAD_PRIORITY_HIGH;
		}
		if (priority < 0) {
			return SDL_THREAD_PRIORITY_LOW;
		}
		return SDL_THREAD_PRIORITY_NORMAL;
	}

	SDL_Thread * WWLib_Get_Thread(std::uintptr_t handle)
	{
		return reinterpret_cast<SDL_Thread *>(handle);
	}
}

ThreadClass::ThreadClass(const char * thread_name, ExceptionHandlerType exception_handler)
	: running(false), ThreadID(0), ExceptionHandler(exception_handler), handle(0), thread_priority(0)
{
	if (thread_name != NULL) {
		WWASSERT(std::strlen(thread_name) < sizeof(ThreadName));
		std::strcpy(ThreadName, thread_name);
	} else {
		std::strcpy(ThreadName, "No name");
	}
}

ThreadClass::~ThreadClass()
{
	Stop();
}

int ThreadClass::Internal_Thread_Function(void * params)
{
	ThreadClass * tc = reinterpret_cast<ThreadClass *>(params);
	tc->running = true;
	tc->ThreadID = static_cast<unsigned>(SDL_GetCurrentThreadID() & 0xFFFFFFFFu);
	SDL_SetCurrentThreadPriority(WWLib_Map_Thread_Priority(tc->thread_priority));
	tc->Thread_Function();
	tc->running = false;
	tc->ThreadID = 0;
	return 0;
}

void ThreadClass::Execute()
{
	if (handle != 0 && !running) {
		Stop();
	}

	WWASSERT(handle == 0);
	SDL_Thread * thread = SDL_CreateThread(&ThreadClass::Internal_Thread_Function, ThreadName, this);
	WWASSERT(thread != NULL);
	if (thread == NULL) {
		return;
	}

	handle = reinterpret_cast<std::uintptr_t>(thread);
	WWDEBUG_SAY(("ThreadClass::Execute: Started thread %s\n", ThreadName));
}

void ThreadClass::Set_Priority(int priority)
{
	thread_priority = priority;
	if (running && static_cast<unsigned>(SDL_GetCurrentThreadID() & 0xFFFFFFFFu) == ThreadID) {
		SDL_SetCurrentThreadPriority(WWLib_Map_Thread_Priority(thread_priority));
	}
}

void ThreadClass::Stop(unsigned ms)
{
	SDL_Thread * thread = WWLib_Get_Thread(handle);
	if (thread == NULL) {
		running = false;
		ThreadID = 0;
		return;
	}

	running = false;
	const Uint64 start = SDL_GetTicks();
	while (Is_Running() && (SDL_GetTicks() - start) < ms) {
		SDL_Delay(1);
	}

	WWASSERT(!Is_Running());
	SDL_WaitThread(thread, NULL);
	handle = 0;
	running = false;
	ThreadID = 0;
}

void ThreadClass::Sleep_Ms(unsigned ms)
{
	SDL_Delay(ms);
}

void ThreadClass::Switch_Thread()
{
	SDL_Delay(0);
}

unsigned ThreadClass::_Get_Current_Thread_ID()
{
	return static_cast<unsigned>(SDL_GetCurrentThreadID() & 0xFFFFFFFFu);
}

bool ThreadClass::Is_Running()
{
	return running;
}

#include "thread.h"

#include "wwdebug.h"
#include <cassert>
#include <cstring>
#include <thread>

namespace {
std::thread * Get_Thread_Handle(std::uintptr_t handle)
{
    return reinterpret_cast<std::thread *>(handle);
}
}

ThreadClass::ThreadClass(const char *thread_name, ExceptionHandlerType exception_handler)
    : running(false), ThreadID(0), ExceptionHandler(exception_handler), handle(0), thread_priority(0)
{
    if (thread_name) {
        assert(strlen(thread_name) < sizeof(ThreadName) - 1);
        strcpy(ThreadName, thread_name);
    } else {
        strcpy(ThreadName, "No name");
    }
}

ThreadClass::~ThreadClass()
{
    Stop();
}

void __cdecl ThreadClass::Internal_Thread_Function(void * params)
{
    ThreadClass * tc = reinterpret_cast<ThreadClass *>(params);
    tc->running = true;
    tc->ThreadID = GetCurrentThreadId();
    tc->Thread_Function();
    tc->running = false;
    tc->ThreadID = 0;
}

void ThreadClass::Execute()
{
    WWASSERT(!handle);
    auto * thread = new std::thread(&ThreadClass::Internal_Thread_Function, static_cast<void *>(this));
    handle = reinterpret_cast<std::uintptr_t>(thread);
}

void ThreadClass::Set_Priority(int priority)
{
    thread_priority = priority;
    (void)thread_priority;
}

void ThreadClass::Stop(unsigned)
{
    running = false;
    auto * thread = Get_Thread_Handle(handle);
    if (thread == nullptr) {
        return;
    }

    if (thread->joinable()) {
        thread->join();
    }

    delete thread;
    handle = 0;
    ThreadID = 0;
}

void ThreadClass::Sleep_Ms(unsigned ms)
{
    Sleep(ms);
}

void ThreadClass::Switch_Thread()
{
    std::this_thread::yield();
}

unsigned ThreadClass::_Get_Current_Thread_ID()
{
    return GetCurrentThreadId();
}

bool ThreadClass::Is_Running()
{
    return handle != 0;
}

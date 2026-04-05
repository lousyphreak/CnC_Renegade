#include "Except.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

uint32_t ExceptionReturnStack = 0;
uint32_t ExceptionReturnAddress = 0;
uint32_t ExceptionReturnFrame = 0;

namespace {
bool g_trying_to_exit = false;
void (*g_app_exception_callback)(void) = nullptr;
char *(*g_app_version_callback)(void) = nullptr;

std::mutex & Thread_List_Mutex()
{
    static std::mutex mutex;
    return mutex;
}

std::vector<ThreadInfoType> & Thread_List()
{
    static std::vector<ThreadInfoType> threads;
    return threads;
}

std::vector<ThreadInfoType>::iterator Find_Thread_By_Name(const char * thread_name)
{
    std::vector<ThreadInfoType> & threads = Thread_List();
    return std::find_if(threads.begin(), threads.end(), [thread_name](const ThreadInfoType & thread) {
        return std::strcmp(thread.ThreadName, thread_name) == 0;
    });
}
}

int Exception_Handler(int, EXCEPTION_POINTERS *)
{
    if (g_app_exception_callback != nullptr) {
        g_app_exception_callback();
    }
    return 0;
}

int Stack_Walk(uint32_t *, int, CONTEXT *)
{
    return 0;
}

bool Lookup_Symbol(void *, char * symbol, int & displacement)
{
    if (symbol != nullptr) {
        symbol[0] = '\0';
    }
    displacement = 0;
    return false;
}

void Load_Image_Helper(void)
{
}

void Register_Thread_ID(uint32_t thread_id, char * thread_name, bool main)
{
    if (thread_name == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(Thread_List_Mutex());
    std::vector<ThreadInfoType> & threads = Thread_List();
    std::vector<ThreadInfoType>::iterator existing = Find_Thread_By_Name(thread_name);
    if (existing != threads.end()) {
        existing->ThreadID = thread_id;
        existing->Main = main || existing->Main;
        return;
    }

    ThreadInfoType thread = {};
    thread.ThreadID = thread_id;
    std::strncpy(thread.ThreadName, thread_name, sizeof(thread.ThreadName) - 1);
    thread.ThreadName[sizeof(thread.ThreadName) - 1] = '\0';
    thread.ThreadHandle = nullptr;
    thread.Main = main;
    threads.push_back(thread);
}

void Unregister_Thread_ID(uint32_t thread_id, char * thread_name)
{
    if (thread_name == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(Thread_List_Mutex());
    std::vector<ThreadInfoType> & threads = Thread_List();
    threads.erase(std::remove_if(threads.begin(), threads.end(), [thread_id, thread_name](const ThreadInfoType & thread) {
        return std::strcmp(thread.ThreadName, thread_name) == 0 && (thread_id == 0 || thread.ThreadID == thread_id);
    }), threads.end());
}

void Register_Application_Exception_Callback(void (*app_callback)(void))
{
    g_app_exception_callback = app_callback;
}

void Register_Application_Version_Callback(char *(*app_version_callback)(void))
{
    g_app_version_callback = app_version_callback;
}

void Set_Exit_On_Exception(bool set)
{
    g_trying_to_exit = set;
}

bool Is_Trying_To_Exit(void)
{
    return g_trying_to_exit;
}

uint32_t Get_Main_Thread_ID(void)
{
    std::lock_guard<std::mutex> lock(Thread_List_Mutex());
    const std::vector<ThreadInfoType> & threads = Thread_List();
    for (const ThreadInfoType & thread : threads) {
        if (thread.Main) {
            return thread.ThreadID;
        }
    }
    return 0;
}

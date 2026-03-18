#include "Except.h"

unsigned long ExceptionReturnStack = 0;
unsigned long ExceptionReturnAddress = 0;
unsigned long ExceptionReturnFrame = 0;

namespace {
bool g_trying_to_exit = false;
void (*g_app_exception_callback)(void) = nullptr;
char *(*g_app_version_callback)(void) = nullptr;
unsigned long g_main_thread_id = 0;
}

int Exception_Handler(int, EXCEPTION_POINTERS *)
{
    if (g_app_exception_callback != nullptr) {
        g_app_exception_callback();
    }
    return 0;
}

int Stack_Walk(unsigned long *, int, CONTEXT *)
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

void Register_Thread_ID(unsigned long thread_id, char *, bool main)
{
    if (main) {
        g_main_thread_id = thread_id;
    }
}

void Unregister_Thread_ID(unsigned long, char *)
{
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

unsigned long Get_Main_Thread_ID(void)
{
    return g_main_thread_id;
}

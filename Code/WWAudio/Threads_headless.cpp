#include "Threads.h"

#include "refcount.h"

HANDLE WWAudioThreadsClass::m_hDelayedReleaseThread = (HANDLE)-1;
HANDLE WWAudioThreadsClass::m_hDelayedReleaseEvent = (HANDLE)-1;
CriticalSectionClass WWAudioThreadsClass::m_CriticalSection;
WWAudioThreadsClass::DELAYED_RELEASE_INFO *WWAudioThreadsClass::m_ReleaseListHead = NULL;
CriticalSectionClass WWAudioThreadsClass::m_ListMutex;
bool WWAudioThreadsClass::m_IsFlushing = false;

WWAudioThreadsClass::WWAudioThreadsClass(void) = default;
WWAudioThreadsClass::~WWAudioThreadsClass(void) = default;

HANDLE WWAudioThreadsClass::Create_Delayed_Release_Thread(LPVOID)
{
    return m_hDelayedReleaseThread;
}

void WWAudioThreadsClass::End_Delayed_Release_Thread(DWORD)
{
}

void WWAudioThreadsClass::Add_Delayed_Release_Object(RefCountClass *object, DWORD)
{
    REF_PTR_RELEASE(object);
}

void WWAudioThreadsClass::Flush_Delayed_Release_Objects(void)
{
}

void __cdecl WWAudioThreadsClass::Delayed_Release_Thread_Proc(LPVOID)
{
}

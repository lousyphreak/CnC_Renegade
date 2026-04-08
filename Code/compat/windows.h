#pragma once

#include "win.h"

#ifndef _WIN32

#include <cstdio>
#include <cstdlib>
#include <cwchar>

#ifndef DLL_PROCESS_ATTACH
#define DLL_PROCESS_ATTACH 1
#endif

#ifndef DLL_PROCESS_DETACH
#define DLL_PROCESS_DETACH 0
#endif

#ifndef RENEGADE_COMPAT_WINDOWS_TYPES_DEFINED
#define RENEGADE_COMPAT_WINDOWS_TYPES_DEFINED
using UINT = unsigned int;
using DWORD = unsigned long;
using ULONG = unsigned long;
#endif

#ifndef _strdup
#define _strdup strdup
#endif

inline void OutputDebugStringA(const char * text)
{
    if (text != nullptr) {
        std::fputs(text, stderr);
    }
}

inline void OutputDebugStringW(const wchar_t * text)
{
    if (text != nullptr) {
        std::fputws(text, stderr);
    }
}

#ifndef RENEGADE_OUTPUTDEBUGSTRING_DEFINED
inline void OutputDebugString(const char * text)
{
    OutputDebugStringA(text);
}
#endif

#endif

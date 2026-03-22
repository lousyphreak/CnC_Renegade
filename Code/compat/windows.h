#pragma once

#include "win.h"

#include <cstdio>
#include <cwchar>

#ifndef DLL_PROCESS_ATTACH
#define DLL_PROCESS_ATTACH 1
#endif

#ifndef DLL_PROCESS_DETACH
#define DLL_PROCESS_DETACH 0
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

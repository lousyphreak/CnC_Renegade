#pragma once

#ifndef RENEGADE_TCHAR_COMPAT_H
#define RENEGADE_TCHAR_COMPAT_H

#include "osdep.h"
#include <cstring>

using TCHAR = char;
using _TCHAR = char;

#ifndef TEXT
#define TEXT(x) x
#endif

#ifndef _T
#define _T(x) x
#endif

inline int _tcslen(const TCHAR * text)
{
    return static_cast<int>(std::strlen(text));
}

inline int _tcsclen(const TCHAR * text)
{
    return static_cast<int>(std::strlen(text));
}

inline int _tcscmp(const TCHAR * lhs, const TCHAR * rhs)
{
    return std::strcmp(lhs, rhs);
}

inline int _tcsicmp(const TCHAR * lhs, const TCHAR * rhs)
{
    return stricmp(lhs, rhs);
}

inline TCHAR * _tcscpy(TCHAR * destination, const TCHAR * source)
{
    return std::strcpy(destination, source);
}

#endif

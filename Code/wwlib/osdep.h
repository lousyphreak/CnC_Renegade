#pragma once

#ifndef RENEGADE_OSDEP_H
#define RENEGADE_OSDEP_H

#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <ctype.h>
#include <errno.h>
#include <filesystem>
#include <limits.h>
#include <strings.h>
#include <string>
#include <thread>

#include "bittype.h"

#ifndef _UNIX
#define _UNIX 1
#endif

#ifndef __cdecl
#define __cdecl
#endif

#ifndef _cdecl
#define _cdecl
#endif

#ifndef __stdcall
#define __stdcall
#endif

#ifndef __fastcall
#define __fastcall
#endif

#ifndef __forceinline
#define __forceinline inline __attribute__((always_inline))
#endif

using HANDLE = void *;
using HINSTANCE = void *;
using HWND = void *;
using LPVOID = void *;
using LPCVOID = const void *;
using LPSTR = char *;
using WCHAR = wchar_t;
using LPWSTR = WCHAR *;
using LPCWSTR = const WCHAR *;
using CHAR = char;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#ifndef _MAX_PATH
#define _MAX_PATH PATH_MAX
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#endif

#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY 0x00000001u
#endif

#ifndef CP_ACP
#define CP_ACP 0
#endif

inline DWORD GetLastError()
{
    return static_cast<DWORD>(errno);
}

inline void Sleep(DWORD milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline DWORD GetCurrentThreadId()
{
    const auto hashed = std::hash<std::thread::id>{}(std::this_thread::get_id());
    return static_cast<DWORD>(hashed & 0xFFFFFFFFu);
}

inline DWORD timeGetTime()
{
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start);
    return static_cast<DWORD>(elapsed.count() & 0xFFFFFFFFu);
}

inline int DeleteFile(const char * filename)
{
    std::error_code error;
    return std::filesystem::remove(filename, error) ? TRUE : FALSE;
}

inline DWORD GetFileAttributes(const char * filename)
{
    std::error_code error;
    const auto status = std::filesystem::status(filename, error);
    if (error || !std::filesystem::exists(status)) {
        return INVALID_FILE_ATTRIBUTES;
    }

    DWORD attributes = 0;
    if ((status.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
        attributes |= FILE_ATTRIBUTE_READONLY;
    }
    return attributes;
}

inline int stricmp(const char * lhs, const char * rhs)
{
    return ::strcasecmp(lhs, rhs);
}

inline int strnicmp(const char * lhs, const char * rhs, std::size_t count)
{
    return ::strncasecmp(lhs, rhs, count);
}

inline char * strupr(char * text)
{
    if (text == nullptr) {
        return nullptr;
    }

    for (char * cursor = text; *cursor != '\0'; ++cursor) {
        *cursor = static_cast<char>(::toupper(static_cast<unsigned char>(*cursor)));
    }
    return text;
}

inline char * strlwr(char * text)
{
    if (text == nullptr) {
        return nullptr;
    }

    for (char * cursor = text; *cursor != '\0'; ++cursor) {
        *cursor = static_cast<char>(::tolower(static_cast<unsigned char>(*cursor)));
    }
    return text;
}

inline int _vsnprintf(char * buffer, std::size_t count, const char * format, std::va_list args)
{
    return std::vsnprintf(buffer, count, format, args);
}

inline int _vsnwprintf(wchar_t * buffer, std::size_t count, const wchar_t * format, std::va_list args)
{
    return std::vswprintf(buffer, count, format, args);
}

inline int WideCharToMultiByte(
    unsigned int,
    DWORD,
    const wchar_t * source,
    int source_length,
    char * destination,
    int destination_length,
    const char *,
    BOOL * used_default_char)
{
    if (used_default_char != nullptr) {
        *used_default_char = FALSE;
    }

    if (source == nullptr) {
        return 0;
    }

    std::mbstate_t state{};
    const wchar_t * src = source;

    if (destination == nullptr || destination_length == 0) {
        return static_cast<int>(1 + std::wcsrtombs(nullptr, &src, 0, &state));
    }

    if (source_length < 0) {
        return static_cast<int>(std::wcsrtombs(destination, &src, destination_length, &state));
    }

    std::wstring temp(source, source + source_length);
    temp.push_back(L'\0');
    const wchar_t * temp_src = temp.c_str();
    return static_cast<int>(std::wcsrtombs(destination, &temp_src, destination_length, &state));
}

#endif

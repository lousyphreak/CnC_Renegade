#pragma once

#ifndef RENEGADE_OSDEP_H
#define RENEGADE_OSDEP_H

#include <algorithm>
#include <alloca.h>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
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

#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#ifndef __fastcall
#define __fastcall
#endif

#ifndef __forceinline
#define __forceinline inline __attribute__((always_inline))
#endif

#ifndef _alloca
#define _alloca(size) alloca(size)
#endif

using HANDLE = void *;
using HINSTANCE = void *;
using HWND = void *;
using HKL = void *;
using HIMC = void *;
using LPVOID = void *;
using LPCVOID = const void *;
using LPSTR = char *;
using WCHAR = wchar_t;
using LPWSTR = WCHAR *;
using LPCWSTR = const WCHAR *;
using CHAR = char;
using HFONT = void *;
using HBITMAP = void *;
using HCURSOR = void *;
using HBRUSH = void *;
using HICON = void *;
using HDC = void *;
using LONG = long;
using LPARAM = std::intptr_t;
using WPARAM = std::uintptr_t;
using LRESULT = std::intptr_t;
using LPLOGFONT = void *;

struct POINT {
    LONG x;
    LONG y;
};

struct SIZE {
    LONG cx;
    LONG cy;
};

struct RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

using COLORREF = DWORD;

inline bool GameInFocus = true;

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

#ifndef GL_LEVEL_NOGUIDELINE
#define GL_LEVEL_NOGUIDELINE 0
#endif

#ifndef VK_LBUTTON
#define VK_LBUTTON 0x01
#endif

#ifndef VK_RBUTTON
#define VK_RBUTTON 0x02
#endif

#ifndef VK_MBUTTON
#define VK_MBUTTON 0x04
#endif

#ifndef VK_BACK
#define VK_BACK 0x08
#endif

#ifndef VK_TAB
#define VK_TAB 0x09
#endif

#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif

#ifndef VK_SHIFT
#define VK_SHIFT 0x10
#endif

#ifndef VK_CONTROL
#define VK_CONTROL 0x11
#endif

#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif

#ifndef VK_SPACE
#define VK_SPACE 0x20
#endif

#ifndef VK_PRIOR
#define VK_PRIOR 0x21
#endif

#ifndef VK_NEXT
#define VK_NEXT 0x22
#endif

#ifndef VK_END
#define VK_END 0x23
#endif

#ifndef VK_HOME
#define VK_HOME 0x24
#endif

#ifndef VK_LEFT
#define VK_LEFT 0x25
#endif

#ifndef VK_UP
#define VK_UP 0x26
#endif

#ifndef VK_RIGHT
#define VK_RIGHT 0x27
#endif

#ifndef VK_DOWN
#define VK_DOWN 0x28
#endif

#ifndef VK_DELETE
#define VK_DELETE 0x2E
#endif

#ifndef WM_KEYDOWN
#define WM_KEYDOWN 0x0100
#endif

#ifndef WM_KEYUP
#define WM_KEYUP 0x0101
#endif

#ifndef WM_CHAR
#define WM_CHAR 0x0102
#endif

#ifndef BS_BITMAP
#define BS_BITMAP 0x00000080L
#endif

#ifndef BS_OWNERDRAW
#define BS_OWNERDRAW 0x0000000BL
#endif

#ifndef BS_LEFT
#define BS_LEFT 0x00000100L
#endif

#ifndef BS_CHECKBOX
#define BS_CHECKBOX 0x00000002L
#endif

#ifndef BS_AUTOCHECKBOX
#define BS_AUTOCHECKBOX 0x00000003L
#endif

#ifndef BS_FLAT
#define BS_FLAT 0x00008000L
#endif

#ifndef BN_CLICKED
#define BN_CLICKED 0
#endif

#ifndef IDOK
#define IDOK 1
#endif

#ifndef IDCANCEL
#define IDCANCEL 2
#endif

#ifndef CBS_SIMPLE
#define CBS_SIMPLE 0x0001L
#endif

#ifndef CBS_DROPDOWN
#define CBS_DROPDOWN 0x0002L
#endif

#ifndef CBS_OEMCONVERT
#define CBS_OEMCONVERT 0x0080L
#endif

#ifndef ES_CENTER
#define ES_CENTER 0x0001L
#endif

#ifndef ES_MULTILINE
#define ES_MULTILINE 0x0004L
#endif

#ifndef ES_PASSWORD
#define ES_PASSWORD 0x0020L
#endif

#ifndef ES_AUTOVSCROLL
#define ES_AUTOVSCROLL 0x0040L
#endif

#ifndef ES_NUMBER
#define ES_NUMBER 0x2000L
#endif

#ifndef ES_OEMCONVERT
#define ES_OEMCONVERT 0x0400L
#endif

#ifndef WS_BORDER
#define WS_BORDER 0x00800000L
#endif

#ifndef WS_GROUP
#define WS_GROUP 0x00020000L
#endif

#ifndef WS_VISIBLE
#define WS_VISIBLE 0x10000000L
#endif

#ifndef WS_DISABLED
#define WS_DISABLED 0x08000000L
#endif

#ifndef SS_LEFT
#define SS_LEFT 0x00000000L
#endif

#ifndef SS_CENTER
#define SS_CENTER 0x00000001L
#endif

#ifndef SS_RIGHT
#define SS_RIGHT 0x00000002L
#endif

#ifndef SS_BLACKFRAME
#define SS_BLACKFRAME 0x00000007L
#endif

#ifndef SS_ETCHEDHORZ
#define SS_ETCHEDHORZ 0x00000010L
#endif

#ifndef SS_BITMAP
#define SS_BITMAP 0x0000000EL
#endif

#ifndef SS_CENTERIMAGE
#define SS_CENTERIMAGE 0x00000200L
#endif

#ifndef SS_LEFTNOWORDWRAP
#define SS_LEFTNOWORDWRAP 0x0000000CL
#endif

#ifndef SS_TYPEMASK
#define SS_TYPEMASK 0x0000001FL
#endif

#ifndef LVS_NOCOLUMNHEADER
#define LVS_NOCOLUMNHEADER 0x4000L
#endif

#ifndef LOWORD
#define LOWORD(value) (static_cast<WORD>(static_cast<DWORD>(value) & 0xFFFF))
#endif

#ifndef HIWORD
#define HIWORD(value) (static_cast<WORD>((static_cast<DWORD>(value) >> 16) & 0xFFFF))
#endif

#ifndef MAKELONG
#define MAKELONG(low, high) (static_cast<LONG>((static_cast<WORD>(low)) | (static_cast<DWORD>(static_cast<WORD>(high)) << 16)))
#endif

#ifndef LOCALE_USER_DEFAULT
#define LOCALE_USER_DEFAULT 0
#endif

#ifndef NORM_IGNORECASE
#define NORM_IGNORECASE 0x00000001
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

inline short GetAsyncKeyState(int)
{
    return 0;
}

inline int AddFontResource(const char *)
{
    return 0;
}

inline int RemoveFontResource(const char *)
{
    return 0;
}

inline int GetKeyboardState(unsigned char * state)
{
    if (state != nullptr) {
        std::memset(state, 0, 256);
    }
    return TRUE;
}

inline void ZeroMemory(void * destination, std::size_t size)
{
    std::memset(destination, 0, size);
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

inline int _stricmp(const char * lhs, const char * rhs)
{
    return stricmp(lhs, rhs);
}

inline int strnicmp(const char * lhs, const char * rhs, std::size_t count)
{
    return ::strncasecmp(lhs, rhs, count);
}

inline int _strnicmp(const char * lhs, const char * rhs, std::size_t count)
{
    return strnicmp(lhs, rhs, count);
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

inline char * _strupr(char * text)
{
    return strupr(text);
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

inline char * _strlwr(char * text)
{
    return strlwr(text);
}

inline wchar_t * _wcsupr(wchar_t * text)
{
    if (text == nullptr) {
        return nullptr;
    }

    for (wchar_t * cursor = text; *cursor != L'\0'; ++cursor) {
        *cursor = std::towupper(*cursor);
    }
    return text;
}

inline wchar_t * _wcslwr(wchar_t * text)
{
    if (text == nullptr) {
        return nullptr;
    }

    for (wchar_t * cursor = text; *cursor != L'\0'; ++cursor) {
        *cursor = std::towlower(*cursor);
    }
    return text;
}

inline int _wcsnicmp(const wchar_t * lhs, const wchar_t * rhs, std::size_t count)
{
    if (lhs == rhs) {
        return 0;
    }
    if (lhs == nullptr) {
        return -1;
    }
    if (rhs == nullptr) {
        return 1;
    }

    for (std::size_t index = 0; index < count; ++index) {
        const wchar_t left = std::towlower(lhs[index]);
        const wchar_t right = std::towlower(rhs[index]);
        if (left != right) {
            return (left < right) ? -1 : 1;
        }
        if (lhs[index] == L'\0' || rhs[index] == L'\0') {
            break;
        }
    }

    return 0;
}

inline int _wcsicmp(const wchar_t * lhs, const wchar_t * rhs)
{
    if (lhs == rhs) {
        return 0;
    }
    if (lhs == nullptr) {
        return -1;
    }
    if (rhs == nullptr) {
        return 1;
    }

    const std::size_t lhs_length = std::wcslen(lhs);
    const std::size_t rhs_length = std::wcslen(rhs);
    return _wcsnicmp(lhs, rhs, std::max(lhs_length, rhs_length) + 1);
}

inline int _wtoi(const wchar_t * text)
{
    return (text == nullptr) ? 0 : static_cast<int>(std::wcstol(text, nullptr, 10));
}

inline unsigned long _lrotl(unsigned long value, int shift)
{
    const uint32_t narrowed = static_cast<uint32_t>(value);
    const unsigned normalized = static_cast<unsigned>(shift) & 31u;
    return static_cast<unsigned long>((narrowed << normalized) | (narrowed >> ((32u - normalized) & 31u)));
}

inline unsigned long _byteswap_ulong(unsigned long value)
{
    return static_cast<unsigned long>(__builtin_bswap32(static_cast<uint32_t>(value)));
}

inline int CompareStringW(DWORD, DWORD, const wchar_t * lhs, int lhs_length, const wchar_t * rhs, int rhs_length)
{
    const std::wstring lhs_text = (lhs == nullptr)
        ? std::wstring()
        : ((lhs_length < 0) ? std::wstring(lhs) : std::wstring(lhs, lhs + lhs_length));
    const std::wstring rhs_text = (rhs == nullptr)
        ? std::wstring()
        : ((rhs_length < 0) ? std::wstring(rhs) : std::wstring(rhs, rhs + rhs_length));

    const int comparison = _wcsicmp(lhs_text.c_str(), rhs_text.c_str());
    if (comparison < 0) {
        return 1;
    }
    if (comparison > 0) {
        return 3;
    }
    return 2;
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

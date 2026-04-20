#pragma once

#ifndef RENEGADE_OSDEP_H
#define RENEGADE_OSDEP_H

#include <algorithm>
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
#include <limits.h>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "bittype.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <malloc.h>

#include "win.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef alloca
#define alloca _alloca
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef stricmp
#define stricmp _stricmp
#endif

#ifndef strcmpi
#define strcmpi _stricmp
#endif

#ifndef strnicmp
#define strnicmp _strnicmp
#endif

#else

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_time.h>
#include <SDL3/SDL_video.h>

#include <alloca.h>
#include <strings.h>
#include <unistd.h>

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

#ifndef _stdcall
#define _stdcall __stdcall
#endif

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef APIENTRY
#define APIENTRY WINAPI
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

#ifndef __declspec
#define __declspec(x)
#endif

#ifndef _declspec
#define _declspec(x)
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
using LPTSTR = char *;
using LPCTSTR = const char *;
using TCHAR = char;
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
using HACCEL = void *;
using LARGE_INTEGER = int64_t;
using FARPROC = void *;
using intptr_t = std::intptr_t;
using uintptr_t = std::uintptr_t;
using LPLOGFONT = void *;

struct CRITICAL_SECTION {
    std::recursive_mutex mutex;
};

struct POINT {
    int32_t x;
    int32_t y;
};

struct SIZE {
    int32_t cx;
    int32_t cy;
};

struct RECT {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

#ifndef RENEGADE_COMPAT_FILETIME_DEFINED
#define RENEGADE_COMPAT_FILETIME_DEFINED
typedef struct _FILETIME {
	uint32_t dwLowDateTime;
	uint32_t dwHighDateTime;
} FILETIME, *LPFILETIME;
#endif

typedef struct _BY_HANDLE_FILE_INFORMATION {
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
} BY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

struct WIN32_FIND_DATAA {
	uint32_t dwFileAttributes;
	FILETIME ftLastWriteTime;
	char cFileName[260];
};

using WIN32_FIND_DATA = WIN32_FIND_DATAA;

using COLORREF = uint32_t;

extern bool GameInFocus;

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

#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif

#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif

#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif

#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1))
#endif

#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY 0x00000001u
#endif

#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010u
#endif

#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x00000080u
#endif

#ifndef GENERIC_WRITE
#define GENERIC_WRITE 0x40000000u
#endif

#ifndef GENERIC_READ
#define GENERIC_READ 0x80000000u
#endif

#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ 0x00000001u
#endif

#ifndef CREATE_NEW
#define CREATE_NEW 1u
#endif

#ifndef CREATE_ALWAYS
#define CREATE_ALWAYS 2u
#endif

#ifndef OPEN_EXISTING
#define OPEN_EXISTING 3u
#endif

#ifndef FILE_BEGIN
#define FILE_BEGIN 0u
#endif

#ifndef FILE_CURRENT
#define FILE_CURRENT 1u
#endif

#ifndef FILE_END
#define FILE_END 2u
#endif

#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183u
#endif

#ifndef MAX_COMPUTERNAME_LENGTH
#define MAX_COMPUTERNAME_LENGTH 15
#endif

#ifndef MB_OK
#define MB_OK 0x00000000u
#endif

#ifndef MB_ICONEXCLAMATION
#define MB_ICONEXCLAMATION 0x00000030u
#endif

#ifndef MB_SETFOREGROUND
#define MB_SETFOREGROUND 0x00010000u
#endif

#ifndef SW_SHOW
#define SW_SHOW 5
#endif

#ifndef SW_HIDE
#define SW_HIDE 0
#endif

#ifndef SW_MINIMIZE
#define SW_MINIMIZE 6
#endif

#ifndef SW_RESTORE
#define SW_RESTORE 9
#endif

#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(i) reinterpret_cast<const char *>(static_cast<uintptr_t>(static_cast<uint16_t>(i)))
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
#define LOWORD(value) (static_cast<uint16_t>(static_cast<uint32_t>(value) & 0xFFFF))
#endif

#ifndef HIWORD
#define HIWORD(value) (static_cast<uint16_t>((static_cast<uint32_t>(value) >> 16) & 0xFFFF))
#endif

#ifndef MAKELONG
#define MAKELONG(low, high) (static_cast<int32_t>((static_cast<uint16_t>(low)) | (static_cast<uint32_t>(static_cast<uint16_t>(high)) << 16)))
#endif

#ifndef LOCALE_USER_DEFAULT
#define LOCALE_USER_DEFAULT 0
#endif

#ifndef NORM_IGNORECASE
#define NORM_IGNORECASE 0x00000001
#endif

inline uint32_t GetLastError()
{
    return static_cast<uint32_t>(errno);
}

namespace renegade_osdep {

struct CompatFileHandle {
    SDL_IOStream *stream;
    std::string path;
};

struct CompatFindHandle {
    std::vector<std::string> entries;
    std::size_t index;
};

inline std::string Normalize_Path(const char * path)
{
    if (path == nullptr) {
        return std::string();
    }

    std::string normalized(path);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

inline bool Is_Path_Separator(char ch)
{
    return (ch == '/') || (ch == '\\');
}

inline std::string Trim_Trailing_Path_Separators(const std::string & path)
{
    if (path.empty()) {
        return path;
    }

    std::size_t end = path.size();
    while (end > 1 && Is_Path_Separator(path[end - 1])) {
        --end;
    }
    return path.substr(0, end);
}

inline std::string Join_Path(const std::string & parent, const std::string & child)
{
    if (parent.empty()) {
        return child;
    }
    if (child.empty()) {
        return parent;
    }
    if (parent.size() == 1 && Is_Path_Separator(parent[0])) {
        return parent + child;
    }
    if (Is_Path_Separator(parent[parent.size() - 1])) {
        return parent + child;
    }
    return parent + "/" + child;
}

inline std::string Get_Parent_Path(const std::string & path)
{
    const std::string normalized = Trim_Trailing_Path_Separators(Normalize_Path(path.c_str()));
    if (normalized.empty()) {
        return std::string();
    }

    const std::size_t separator = normalized.find_last_of('/');
    if (separator == std::string::npos) {
        return ".";
    }
    if (separator == 0) {
        return "/";
    }
    return normalized.substr(0, separator);
}

inline std::string Get_Filename_Part(const std::string & path)
{
    const std::string normalized = Trim_Trailing_Path_Separators(Normalize_Path(path.c_str()));
    const std::size_t separator = normalized.find_last_of('/');
    if (separator == std::string::npos) {
        return normalized;
    }
    if (separator + 1 >= normalized.size()) {
        return std::string();
    }
    return normalized.substr(separator + 1);
}

inline std::string Get_Path_Extension(const std::string & path)
{
    const std::string filename = Get_Filename_Part(path);
    const std::size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return std::string();
    }
    return filename.substr(dot);
}

inline std::string Get_Path_Stem(const std::string & path)
{
    const std::string filename = Get_Filename_Part(path);
    const std::size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return filename;
    }
    return filename.substr(0, dot);
}

inline std::mutex & Get_Path_Cache_Mutex()
{
    static std::mutex cache_mutex;
    return cache_mutex;
}

inline std::unordered_map<std::string, std::string> & Get_Resolved_Path_Cache()
{
    static std::unordered_map<std::string, std::string> cache;
    return cache;
}

inline std::unordered_map<std::string, std::vector<std::string>> & Get_Directory_Entry_Cache()
{
    static std::unordered_map<std::string, std::vector<std::string>> cache;
    return cache;
}

inline std::string Normalize_Directory_Cache_Key(const std::string & directory)
{
    const std::string normalized = Trim_Trailing_Path_Separators(Normalize_Path(directory.c_str()));
    return normalized.empty() ? std::string(".") : normalized;
}

inline void Cache_Resolved_Path(const std::string & normalized_path, const std::string & resolved_path)
{
    if (normalized_path.empty() || resolved_path.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(Get_Path_Cache_Mutex());
    Get_Resolved_Path_Cache()[normalized_path] = resolved_path;
}

inline bool Try_Get_Cached_Resolved_Path(const std::string & normalized_path, std::string & resolved_path)
{
    std::lock_guard<std::mutex> lock(Get_Path_Cache_Mutex());
    const auto & cache = Get_Resolved_Path_Cache();
    const auto it = cache.find(normalized_path);
    if (it == cache.end()) {
        return false;
    }

    resolved_path = it->second;
    return true;
}

inline bool Try_Get_Cached_Directory_Entries(const std::string & directory, std::vector<std::string> & entries)
{
    const std::string key = Normalize_Directory_Cache_Key(directory);

    std::lock_guard<std::mutex> lock(Get_Path_Cache_Mutex());
    const auto & cache = Get_Directory_Entry_Cache();
    const auto it = cache.find(key);
    if (it == cache.end()) {
        return false;
    }

    entries = it->second;
    return true;
}

inline void Cache_Directory_Entries(const std::string & directory, const std::vector<std::string> & entries)
{
    const std::string key = Normalize_Directory_Cache_Key(directory);

    std::lock_guard<std::mutex> lock(Get_Path_Cache_Mutex());
    Get_Directory_Entry_Cache()[key] = entries;
}

inline void Invalidate_Path_Caches()
{
    std::lock_guard<std::mutex> lock(Get_Path_Cache_Mutex());
    Get_Resolved_Path_Cache().clear();
    Get_Directory_Entry_Cache().clear();
}

inline bool Get_Path_Info(const char * path, SDL_PathInfo * info)
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }

    return SDL_GetPathInfo(path, info);
}

inline bool Path_Exists(const std::string & path)
{
    return Get_Path_Info(path.c_str(), nullptr);
}

inline bool Path_Is_Directory(const std::string & path)
{
    SDL_PathInfo info = {};
    return Get_Path_Info(path.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY;
}

inline bool Path_Is_Regular_File(const std::string & path)
{
    SDL_PathInfo info = {};
    return Get_Path_Info(path.c_str(), &info) && info.type == SDL_PATHTYPE_FILE;
}

inline bool Collect_Directory_Entries(const std::string & directory, std::vector<std::string> & entries)
{
    if (Try_Get_Cached_Directory_Entries(directory, entries)) {
        return true;
    }

    int count = 0;
    char ** matches = SDL_GlobDirectory(directory.c_str(), nullptr, static_cast<SDL_GlobFlags>(0), &count);
    if (matches == nullptr) {
        return false;
    }

    entries.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        if (matches[index] != nullptr) {
            entries.emplace_back(matches[index]);
        }
    }
    SDL_free(matches);

    Cache_Directory_Entries(directory, entries);
    return true;
}

inline bool Find_Case_Insensitive_Path_Component(const std::string & directory, const std::string & component, std::string & matched_component)
{
    std::vector<std::string> entries;
    if (!Collect_Directory_Entries(directory, entries)) {
        return false;
    }

    for (const std::string & entry : entries) {
        if (::strcasecmp(entry.c_str(), component.c_str()) == 0) {
            matched_component = entry;
            return true;
        }
    }

    return false;
}

inline bool Resolve_Path_Case(const std::string & normalized_path, bool allow_missing_leaf, std::string & resolved_path)
{
    if (normalized_path.empty()) {
        return false;
    }

    const char * path = normalized_path.c_str();
    const int path_length = static_cast<int>(normalized_path.size());

    std::string current_path;
    int cursor = 0;

    if (Is_Path_Separator(path[0])) {
        current_path = "/";
        while (cursor < path_length && Is_Path_Separator(path[cursor])) {
            ++cursor;
        }
    }

    while (cursor < path_length) {
        while (cursor < path_length && Is_Path_Separator(path[cursor])) {
            ++cursor;
        }
        if (cursor >= path_length) {
            break;
        }

        const int component_start = cursor;
        while (cursor < path_length && !Is_Path_Separator(path[cursor])) {
            ++cursor;
        }

        std::string component(path + component_start, path + cursor);
        if (component == ".") {
            continue;
        }
        if (component == "..") {
            current_path = current_path.empty() ? std::string("..") : Join_Path(current_path, component);
            continue;
        }

        int next_component = cursor;
        while (next_component < path_length && Is_Path_Separator(path[next_component])) {
            ++next_component;
        }
        const bool is_last_component = (next_component >= path_length);

        const std::string search_directory = current_path.empty() ? std::string(".") : current_path;
        if (!Path_Is_Directory(search_directory)) {
            return false;
        }

        std::string matched_component;
        if (Find_Case_Insensitive_Path_Component(search_directory, component, matched_component)) {
            current_path = Join_Path(current_path, matched_component);
        } else {
            if (allow_missing_leaf && is_last_component) {
                resolved_path = Join_Path(current_path, component);
                return true;
            }
            return false;
        }
    }

    resolved_path = current_path.empty() ? normalized_path : current_path;
    return true;
}

inline bool Resolve_Existing_Path(const char * path, std::string & resolved_path)
{
    const std::string normalized = Normalize_Path(path);
    if (normalized.empty()) {
        return false;
    }

    if (Try_Get_Cached_Resolved_Path(normalized, resolved_path)) {
        return true;
    }

    if (Path_Exists(normalized)) {
        resolved_path = normalized;
        Cache_Resolved_Path(normalized, resolved_path);
        return true;
    }

    if (!Resolve_Path_Case(normalized, false, resolved_path)) {
        return false;
    }

    Cache_Resolved_Path(normalized, resolved_path);
    return true;
}

inline bool Resolve_Path_For_Access(const char * path, bool allow_missing_leaf, std::string & resolved_path)
{
    if (Resolve_Existing_Path(path, resolved_path)) {
        return true;
    }

    if (!allow_missing_leaf) {
        return false;
    }

    const std::string normalized = Normalize_Path(path);
    if (normalized.empty()) {
        return false;
    }

    return Resolve_Path_Case(normalized, true, resolved_path);
}

inline bool Mode_Can_Create_File(const char * mode)
{
    if (mode == nullptr) {
        return false;
    }

    return std::strchr(mode, 'w') != nullptr || std::strchr(mode, 'a') != nullptr;
}

inline bool Resolve_Path_For_Mode(const char * filename, const char * mode, std::string & resolved_path)
{
    if (filename == nullptr || mode == nullptr) {
        errno = EINVAL;
        return false;
    }

    if (Resolve_Existing_Path(filename, resolved_path)) {
        return true;
    }

    if (!Mode_Can_Create_File(mode)) {
        errno = ENOENT;
        return false;
    }

    if (!Resolve_Path_For_Access(filename, true, resolved_path)) {
        errno = ENOENT;
        return false;
    }

    return true;
}

inline SDL_IOStream * Open_C_File(const char * filename, const char * mode)
{
    std::string resolved_path;
    if (!Resolve_Path_For_Mode(filename, mode, resolved_path)) {
        return nullptr;
    }

    const bool existed_before_open = Path_Exists(resolved_path);
    SDL_IOStream * file = SDL_IOFromFile(resolved_path.c_str(), mode);
    if (file == nullptr) {
        return nullptr;
    }

    if (Mode_Can_Create_File(mode) && !existed_before_open) {
        Invalidate_Path_Caches();
    }
    Cache_Resolved_Path(Normalize_Path(filename), resolved_path);
    return file;
}

inline SDL_IOStream * Open_C_File(const std::string & filename, const char * mode)
{
    return Open_C_File(filename.c_str(), mode);
}

inline SDL_IOStream * Open_C_File_Read_Write(const char * filename)
{
    std::string resolved_path;
    if (Resolve_Existing_Path(filename, resolved_path)) {
        SDL_IOStream * file = SDL_IOFromFile(resolved_path.c_str(), "rb+");
        if (file == nullptr) {
            return nullptr;
        }

        Cache_Resolved_Path(Normalize_Path(filename), resolved_path);
        return file;
    }

    if (!Resolve_Path_For_Access(filename, true, resolved_path)) {
        errno = ENOENT;
        return nullptr;
    }

    SDL_IOStream * file = SDL_IOFromFile(resolved_path.c_str(), "wb+");
    if (file == nullptr) {
        return nullptr;
    }

    Invalidate_Path_Caches();
    Cache_Resolved_Path(Normalize_Path(filename), resolved_path);
    return file;
}

inline std::size_t Read_C_File(SDL_IOStream * file, void * buffer, std::size_t byte_count)
{
    if (file == nullptr || (buffer == nullptr && byte_count != 0)) {
        errno = EINVAL;
        return 0;
    }

    return SDL_ReadIO(file, buffer, byte_count);
}

inline std::size_t Write_C_File(SDL_IOStream * file, const void * buffer, std::size_t byte_count)
{
    if (file == nullptr || (buffer == nullptr && byte_count != 0)) {
        errno = EINVAL;
        return 0;
    }

    return SDL_WriteIO(file, buffer, byte_count);
}

inline Sint64 Tell_C_File(SDL_IOStream * file)
{
    if (file == nullptr) {
        errno = EINVAL;
        return -1;
    }

    return SDL_TellIO(file);
}

inline bool Seek_C_File(SDL_IOStream * file, Sint64 offset, int origin)
{
    if (file == nullptr) {
        errno = EINVAL;
        return false;
    }

    SDL_IOWhence whence = SDL_IO_SEEK_CUR;
    switch (origin) {
        case SEEK_SET:
            whence = SDL_IO_SEEK_SET;
            break;
        case SEEK_END:
            whence = SDL_IO_SEEK_END;
            break;
        case SEEK_CUR:
        default:
            whence = SDL_IO_SEEK_CUR;
            break;
    }

    return SDL_SeekIO(file, offset, whence) >= 0;
}

inline Sint64 Get_C_File_Size(SDL_IOStream * file)
{
    if (file == nullptr) {
        errno = EINVAL;
        return -1;
    }
    return SDL_GetIOSize(file);
}

inline bool Flush_C_File(SDL_IOStream * file)
{
    if (file == nullptr) {
        errno = EINVAL;
        return false;
    }

    return SDL_FlushIO(file);
}

inline bool Is_C_File_EOF(SDL_IOStream * file)
{
    return file != nullptr && SDL_GetIOStatus(file) == SDL_IO_STATUS_EOF;
}

inline bool Has_C_File_Error(SDL_IOStream * file)
{
    if (file == nullptr) {
        return true;
    }

    const SDL_IOStatus status = SDL_GetIOStatus(file);
    return status == SDL_IO_STATUS_ERROR || status == SDL_IO_STATUS_NOT_READY || status == SDL_IO_STATUS_READONLY || status == SDL_IO_STATUS_WRITEONLY;
}

inline int Close_C_File(SDL_IOStream * file)
{
    if (file == nullptr) {
        errno = EINVAL;
        return EOF;
    }

    return SDL_CloseIO(file) ? 0 : EOF;
}

inline char * Get_C_File_Line(char * buffer, std::size_t buffer_size, SDL_IOStream * file)
{
    if (buffer == nullptr || buffer_size == 0 || file == nullptr) {
        errno = EINVAL;
        return nullptr;
    }

    std::size_t count = 0;
    while (count + 1 < buffer_size) {
        char ch = '\0';
        const std::size_t bytes_read = Read_C_File(file, &ch, 1);
        if (bytes_read == 0) {
            break;
        }

        buffer[count++] = ch;
        if (ch == '\n') {
            break;
        }
    }

    if (count == 0) {
        return nullptr;
    }

    buffer[count] = '\0';
    return buffer;
}

inline int VPrintf_C_File(SDL_IOStream * file, const char * format, va_list arguments)
{
    if (file == nullptr || format == nullptr) {
        errno = EINVAL;
        return -1;
    }

    char * text = nullptr;
    const int length = SDL_vasprintf(&text, format, arguments);
    if (length < 0 || text == nullptr) {
        return -1;
    }

    const std::size_t bytes_written = Write_C_File(file, text, static_cast<std::size_t>(length));
    SDL_free(text);
    return bytes_written == static_cast<std::size_t>(length) ? length : -1;
}

inline int Printf_C_File(SDL_IOStream * file, const char * format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int result = VPrintf_C_File(file, format, arguments);
    va_end(arguments);
    return result;
}

inline int Put_C_File_Char(int ch, SDL_IOStream * file)
{
    const unsigned char value = static_cast<unsigned char>(ch);
    return Write_C_File(file, &value, 1) == 1 ? ch : EOF;
}

inline bool Read_Entire_File(const char * filename, std::vector<uint8_t> & contents)
{
    contents.clear();

    SDL_IOStream * file = Open_C_File(filename, "rb");
    if (file == nullptr) {
        return false;
    }

    size_t data_size = 0;
    void * data = SDL_LoadFile_IO(file, &data_size, true);
    if (data == nullptr) {
        return false;
    }

    contents.resize(data_size);
    if (data_size > 0) {
        std::memcpy(contents.data(), data, data_size);
    }
    SDL_free(data);
    return true;
}

inline bool Read_Entire_File(const std::string & filename, std::vector<uint8_t> & contents)
{
    return Read_Entire_File(filename.c_str(), contents);
}

inline bool Resolve_Find_Pattern(const char * pattern, std::string & directory, std::string & wildcard)
{
    const std::string normalized = Normalize_Path(pattern);
    if (normalized.empty()) {
        return false;
    }

    wildcard = Get_Filename_Part(normalized);
    std::string raw_directory = Get_Parent_Path(normalized);
    if (raw_directory.empty()) {
        raw_directory = ".";
    }

    return Resolve_Existing_Path(raw_directory.c_str(), directory);
}

inline void Populate_Find_Data(const std::string & entry_path, WIN32_FIND_DATA * find_data)
{
    if (find_data == nullptr) {
        return;
    }

    std::memset(find_data, 0, sizeof(*find_data));
    std::snprintf(find_data->cFileName, sizeof(find_data->cFileName), "%s", Get_Filename_Part(entry_path).c_str());

    SDL_PathInfo info = {};
    if (!Get_Path_Info(entry_path.c_str(), &info)) {
        return;
    }

    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        find_data->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    SDL_TimeToWindows(info.modify_time, &find_data->ftLastWriteTime.dwLowDateTime, &find_data->ftLastWriteTime.dwHighDateTime);
}

inline bool Collect_Matching_Paths(const std::string & directory, const std::string & wildcard, std::vector<std::string> & paths)
{
    paths.clear();

    int count = 0;
    char ** matches = SDL_GlobDirectory(directory.c_str(), wildcard.c_str(), SDL_GLOB_CASEINSENSITIVE, &count);
    if (matches == nullptr) {
        return false;
    }

    paths.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        if (matches[index] != nullptr) {
            paths.push_back(Join_Path(directory, matches[index]));
        }
    }
    SDL_free(matches);
    return true;
}

inline bool Collect_Regular_Files_Recursive(const std::string & directory, std::vector<std::string> & files)
{
    std::vector<std::string> entries;
    if (!Collect_Directory_Entries(directory, entries)) {
        return false;
    }

    for (const std::string & entry : entries) {
        const std::string full_path = Join_Path(directory, entry);
        SDL_PathInfo info = {};
        if (!Get_Path_Info(full_path.c_str(), &info)) {
            continue;
        }

        if (info.type == SDL_PATHTYPE_DIRECTORY) {
            Collect_Regular_Files_Recursive(full_path, files);
        } else if (info.type == SDL_PATHTYPE_FILE) {
            files.push_back(full_path);
        }
    }

    return true;
}

inline bool Create_Directory_Tree(const std::string & path)
{
    const std::string normalized = Trim_Trailing_Path_Separators(Normalize_Path(path.c_str()));
    if (normalized.empty()) {
        errno = EINVAL;
        return false;
    }

    std::string current_path;
    std::size_t cursor = 0;
    if (Is_Path_Separator(normalized[0])) {
        current_path = "/";
        cursor = 1;
    }

    while (cursor < normalized.size()) {
        while (cursor < normalized.size() && Is_Path_Separator(normalized[cursor])) {
            ++cursor;
        }
        if (cursor >= normalized.size()) {
            break;
        }

        const std::size_t component_start = cursor;
        while (cursor < normalized.size() && !Is_Path_Separator(normalized[cursor])) {
            ++cursor;
        }

        const std::string component = normalized.substr(component_start, cursor - component_start);
        if (component == ".") {
            continue;
        }
        if (component == "..") {
            current_path = current_path.empty() ? std::string("..") : Join_Path(current_path, component);
            continue;
        }

        current_path = Join_Path(current_path, component);
        if (Path_Is_Directory(current_path)) {
            continue;
        }
        if (Path_Exists(current_path)) {
            return false;
        }
        if (!SDL_CreateDirectory(current_path.c_str())) {
            return false;
        }
    }

    return true;
}

inline CompatFileHandle * As_File_Handle(HANDLE handle)
{
    return reinterpret_cast<CompatFileHandle *>(handle);
}

inline CompatFindHandle * As_Find_Handle(HANDLE handle)
{
    return reinterpret_cast<CompatFindHandle *>(handle);
}

} // namespace renegade_osdep

inline void Sleep(uint32_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline void ExitProcess(uint32_t exit_code)
{
	std::exit(static_cast<int>(exit_code));
}

inline uint32_t GetCurrentThreadId()
{
    return static_cast<uint32_t>(SDL_GetCurrentThreadID() & 0xFFFFFFFFu);
}

inline uint32_t timeGetTime()
{
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start);
    return static_cast<uint32_t>(elapsed.count() & 0xFFFFFFFFu);
}

inline int16_t GetAsyncKeyState(int)
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

inline int GetKeyboardState(uint8_t * state)
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

inline int32_t QueryPerformanceFrequency(LARGE_INTEGER * frequency)
{
    if (frequency != nullptr) {
        *frequency = 1000000;
    }
    return TRUE;
}

inline int32_t QueryPerformanceCounter(LARGE_INTEGER * counter)
{
    if (counter != nullptr) {
        *counter = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    return TRUE;
}

inline void InitializeCriticalSection(CRITICAL_SECTION *)
{
}

inline void DeleteCriticalSection(CRITICAL_SECTION *)
{
}

inline void EnterCriticalSection(CRITICAL_SECTION * critical_section)
{
    if (critical_section != nullptr) {
        critical_section->mutex.lock();
    }
}

inline void LeaveCriticalSection(CRITICAL_SECTION * critical_section)
{
    if (critical_section != nullptr) {
        critical_section->mutex.unlock();
    }
}

inline int32_t TryEnterCriticalSection(CRITICAL_SECTION * critical_section)
{
    return (critical_section != nullptr && critical_section->mutex.try_lock()) ? TRUE : FALSE;
}

inline int DeleteFile(const char * filename)
{
    std::string resolved_path;
    if (!renegade_osdep::Resolve_Existing_Path(filename, resolved_path)) {
        return FALSE;
    }

    if (!SDL_RemovePath(resolved_path.c_str())) {
        return FALSE;
    }

    renegade_osdep::Invalidate_Path_Caches();
    return TRUE;
}

inline int MoveFile(const char * existing_filename, const char * new_filename)
{
    if (existing_filename == nullptr || new_filename == nullptr) {
        errno = EINVAL;
        return FALSE;
    }

    std::string existing_path;
    if (!renegade_osdep::Resolve_Existing_Path(existing_filename, existing_path)) {
        errno = ENOENT;
        return FALSE;
    }

    std::string new_path;
    if (!renegade_osdep::Resolve_Path_For_Access(new_filename, true, new_path)) {
        errno = ENOENT;
        return FALSE;
    }

    if (!SDL_RenamePath(existing_path.c_str(), new_path.c_str())) {
        return FALSE;
    }

    renegade_osdep::Invalidate_Path_Caches();
    return TRUE;
}

inline uint32_t GetModuleFileName(HINSTANCE, char * buffer, uint32_t size)
{
    if (buffer == nullptr || size == 0) {
        return 0;
    }

    char executable_path[PATH_MAX] = {0};
    ssize_t length = readlink("/proc/self/exe", executable_path, sizeof(executable_path) - 1);
    if (length < 0) {
        char * cwd = SDL_GetCurrentDirectory();
        if (cwd == nullptr) {
            return 0;
        }

        const std::size_t count = std::min<std::size_t>(size - 1, std::strlen(cwd));
        std::memcpy(buffer, cwd, count);
        buffer[count] = '\0';
        SDL_free(cwd);
        return static_cast<uint32_t>(count);
    }

    executable_path[length] = '\0';
    const std::string path(executable_path);
    const std::size_t count = std::min<std::size_t>(size - 1, path.size());
    std::memcpy(buffer, path.c_str(), count);
    buffer[count] = '\0';
    return static_cast<uint32_t>(count);
}

inline int32_t CreateDirectory(const char * path, void *)
{
    if (path == nullptr) {
        errno = EINVAL;
        return FALSE;
    }

    std::string existing_directory;
    if (renegade_osdep::Resolve_Existing_Path(path, existing_directory)) {
        errno = EEXIST;
        return FALSE;
    }

    std::string directory;
    if (!renegade_osdep::Resolve_Path_For_Access(path, true, directory)) {
        errno = ENOENT;
        return FALSE;
    }

    if (!renegade_osdep::Create_Directory_Tree(directory)) {
        return FALSE;
    }

    renegade_osdep::Invalidate_Path_Caches();
    return TRUE;
}

inline HANDLE CreateFile(const char * filename, uint32_t desired_access, uint32_t, void *, uint32_t creation_disposition, uint32_t, HANDLE)
{
    if (filename == nullptr) {
        errno = EINVAL;
        return INVALID_HANDLE_VALUE;
    }

    const bool wants_write = (desired_access & GENERIC_WRITE) != 0 || creation_disposition == CREATE_ALWAYS || creation_disposition == CREATE_NEW;

    std::string path;
    const bool exists = renegade_osdep::Resolve_Existing_Path(filename, path);

    if (!exists) {
        if (creation_disposition == OPEN_EXISTING) {
            errno = ENOENT;
            return INVALID_HANDLE_VALUE;
        }

        if (!renegade_osdep::Resolve_Path_For_Access(filename, wants_write, path)) {
            errno = ENOENT;
            return INVALID_HANDLE_VALUE;
        }
    }

    if (creation_disposition == CREATE_NEW && exists) {
        errno = EEXIST;
        return INVALID_HANDLE_VALUE;
    }

    const char * mode = wants_write ? "wb+" : "rb";
    if (creation_disposition == OPEN_EXISTING && wants_write) {
        mode = exists ? "rb+" : "wb+";
    }

    SDL_IOStream * file = renegade_osdep::Open_C_File(path, mode);
    if (file == nullptr) {
        return INVALID_HANDLE_VALUE;
    }

    auto * handle = new renegade_osdep::CompatFileHandle{file, path};
    return reinterpret_cast<HANDLE>(handle);
}

inline uint32_t GetFileSize(HANDLE handle, uint32_t *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->stream == nullptr) {
        return 0xFFFFFFFFu;
    }

    const Sint64 size = renegade_osdep::Get_C_File_Size(file_handle->stream);
    if (size < 0 || size > 0xFFFFFFFFll) {
        return 0xFFFFFFFFu;
    }
    return static_cast<uint32_t>(size);
}

inline int32_t WriteFile(HANDLE handle, const void * buffer, uint32_t bytes_to_write, uint32_t * bytes_written, void *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->stream == nullptr) {
        return FALSE;
    }

    const std::size_t written = renegade_osdep::Write_C_File(file_handle->stream, buffer, bytes_to_write);
    if (bytes_written != nullptr) {
        *bytes_written = static_cast<uint32_t>(written);
    }

    return written == bytes_to_write ? TRUE : FALSE;
}

inline int32_t ReadFile(HANDLE handle, void * buffer, uint32_t bytes_to_read, uint32_t * bytes_read, void *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->stream == nullptr) {
        return FALSE;
    }

    const std::size_t read = renegade_osdep::Read_C_File(file_handle->stream, buffer, bytes_to_read);
    if (bytes_read != nullptr) {
        *bytes_read = static_cast<uint32_t>(read);
    }

    return read == bytes_to_read ? TRUE : FALSE;
}

inline int32_t CloseHandle(HANDLE handle)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (handle == INVALID_HANDLE_VALUE || file_handle == nullptr) {
        return FALSE;
    }

    const int result = (file_handle->stream != nullptr) ? renegade_osdep::Close_C_File(file_handle->stream) : 0;
    delete file_handle;
    return result == 0 ? TRUE : FALSE;
}

inline HANDLE FindFirstFile(const char * pattern, WIN32_FIND_DATA * find_data)
{
    if (pattern == nullptr || find_data == nullptr) {
        return INVALID_HANDLE_VALUE;
    }

    std::string directory;
    std::string wildcard;
    if (!renegade_osdep::Resolve_Find_Pattern(pattern, directory, wildcard)) {
        return INVALID_HANDLE_VALUE;
    }

    auto * handle = new renegade_osdep::CompatFindHandle{};
    handle->index = 0;

    renegade_osdep::Collect_Matching_Paths(directory, wildcard, handle->entries);

    if (handle->entries.empty()) {
        delete handle;
        return INVALID_HANDLE_VALUE;
    }

    renegade_osdep::Populate_Find_Data(handle->entries.front(), find_data);
    return reinterpret_cast<HANDLE>(handle);
}

inline int32_t FindNextFile(HANDLE handle, WIN32_FIND_DATA * find_data)
{
    auto * find_handle = renegade_osdep::As_Find_Handle(handle);
    if (find_handle == nullptr || find_data == nullptr) {
        return FALSE;
    }

    ++find_handle->index;
    if (find_handle->index >= find_handle->entries.size()) {
        return FALSE;
    }

    renegade_osdep::Populate_Find_Data(find_handle->entries[find_handle->index], find_data);
    return TRUE;
}

inline int32_t FindClose(HANDLE handle)
{
    auto * find_handle = renegade_osdep::As_Find_Handle(handle);
    if (find_handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    delete find_handle;
    return TRUE;
}

inline int32_t GetComputerName(char * buffer, uint32_t * size)
{
    if (buffer == nullptr || size == nullptr || *size == 0) {
        return FALSE;
    }

    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return FALSE;
    }

    const std::size_t count = std::min<std::size_t>(*size - 1, std::strlen(hostname));
    std::memcpy(buffer, hostname, count);
    buffer[count] = '\0';
    *size = static_cast<uint32_t>(count);
    return TRUE;
}

inline int32_t GetUserName(char * buffer, uint32_t * size)
{
    if (buffer == nullptr || size == nullptr || *size == 0) {
        return FALSE;
    }

    const char * user = std::getenv("USER");
    if (user == nullptr || user[0] == '\0') {
        user = "player";
    }

    const std::size_t count = std::min<std::size_t>(*size - 1, std::strlen(user));
    std::memcpy(buffer, user, count);
    buffer[count] = '\0';
    *size = static_cast<uint32_t>(count);
    return TRUE;
}

inline HINSTANCE GetModuleHandle(const char *)
{
    return nullptr;
}

inline int MessageBox(HWND, const char * text, const char * caption, unsigned)
{
    std::fprintf(stderr, "%s: %s\n", caption != nullptr ? caption : "MessageBox", text != nullptr ? text : "");
    return IDOK;
}

inline int32_t ShowWindow(HWND window_handle, int command)
{
    auto * window = reinterpret_cast<SDL_Window *>(window_handle);
    if (window == nullptr) {
        return FALSE;
    }

    switch (command) {
        case SW_HIDE:
            SDL_HideWindow(window);
            SDL_SyncWindow(window);
            break;

        case SW_MINIMIZE:
            SDL_MinimizeWindow(window);
            SDL_SyncWindow(window);
            break;

        case SW_RESTORE:
            SDL_RestoreWindow(window);
            SDL_ShowWindow(window);
            SDL_RaiseWindow(window);
            SDL_SyncWindow(window);
            break;

        case SW_SHOW:
        default:
            SDL_ShowWindow(window);
            SDL_RaiseWindow(window);
            SDL_SyncWindow(window);
            break;
    }

    return TRUE;
}

inline HACCEL LoadAccelerators(HINSTANCE, const char *)
{
    return nullptr;
}

inline void Add_Accelerator(HWND, HACCEL)
{
}

inline uint32_t GetFileAttributes(const char * filename)
{
    std::string resolved_path;
    if (!renegade_osdep::Resolve_Existing_Path(filename, resolved_path)) {
        return INVALID_FILE_ATTRIBUTES;
    }

    SDL_PathInfo info = {};
    if (!renegade_osdep::Get_Path_Info(resolved_path.c_str(), &info)) {
        return INVALID_FILE_ATTRIBUTES;
    }

    uint32_t attributes = 0;
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        attributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    if (info.type == SDL_PATHTYPE_FILE) {
        SDL_IOStream * stream = SDL_IOFromFile(resolved_path.c_str(), "rb+");
        if (stream == nullptr) {
            attributes |= FILE_ATTRIBUTE_READONLY;
        } else {
            SDL_CloseIO(stream);
        }
    }

    return attributes;
}

inline uint32_t GetCurrentDirectory(uint32_t buffer_length, char * buffer)
{
    char * cwd = SDL_GetCurrentDirectory();
    if (cwd == nullptr) {
        return 0;
    }

    const std::string normalized = renegade_osdep::Trim_Trailing_Path_Separators(cwd);
    SDL_free(cwd);

    const std::string output = normalized.empty() ? std::string("/") : normalized;
    if (buffer == nullptr || buffer_length == 0) {
        return static_cast<uint32_t>(output.size());
    }

    std::snprintf(buffer, buffer_length, "%s", output.c_str());
    return static_cast<uint32_t>(std::min<std::size_t>(output.size(), buffer_length > 0 ? buffer_length - 1 : 0));
}

inline bool FileTimeToDosDateTime(const FILETIME * file_time, uint16_t * dos_date, uint16_t * dos_time)
{
    if (file_time == nullptr || dos_date == nullptr || dos_time == nullptr) {
        return false;
    }

    SDL_Time time_value = SDL_TimeFromWindows(file_time->dwLowDateTime, file_time->dwHighDateTime);
    SDL_DateTime date_time = {};
    if (!SDL_TimeToDateTime(time_value, &date_time, true)) {
        return false;
    }

    const int year = std::clamp(date_time.year, 1980, 2107);
    *dos_date =
        static_cast<uint16_t>(((year - 1980) << 9) |
        (std::clamp(date_time.month, 1, 12) << 5) |
        std::clamp(date_time.day, 1, 31));
    *dos_time =
        static_cast<uint16_t>((std::clamp(date_time.hour, 0, 23) << 11) |
        (std::clamp(date_time.minute, 0, 59) << 5) |
        std::clamp(date_time.second / 2, 0, 29));
    return true;
}

inline bool DosDateTimeToFileTime(uint16_t dos_date, uint16_t dos_time, FILETIME * file_time)
{
    if (file_time == nullptr) {
        return false;
    }

    SDL_DateTime date_time = {};
    date_time.year = 1980 + ((dos_date >> 9) & 0x7F);
    date_time.month = (dos_date >> 5) & 0x0F;
    date_time.day = dos_date & 0x1F;
    date_time.hour = (dos_time >> 11) & 0x1F;
    date_time.minute = (dos_time >> 5) & 0x3F;
    date_time.second = (dos_time & 0x1F) * 2;

    SDL_Time time_value = 0;
    if (!SDL_DateTimeToTime(&date_time, &time_value)) {
        return false;
    }

    SDL_TimeToWindows(time_value, &file_time->dwLowDateTime, &file_time->dwHighDateTime);
    return true;
}

inline int32_t SetFileTime(HANDLE handle, const FILETIME *, const FILETIME *, const FILETIME *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->stream == nullptr) {
        return FALSE;
    }

    return renegade_osdep::Flush_C_File(file_handle->stream) ? TRUE : FALSE;
}

inline int32_t GetFileInformationByHandle(HANDLE handle, BY_HANDLE_FILE_INFORMATION * info)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->stream == nullptr || info == nullptr) {
        return FALSE;
    }

    if (file_handle->path.empty()) {
        return FALSE;
    }

    SDL_PathInfo path_info = {};
    if (!renegade_osdep::Get_Path_Info(file_handle->path.c_str(), &path_info)) {
        return FALSE;
    }

    std::memset(info, 0, sizeof(*info));
    SDL_TimeToWindows(path_info.modify_time, &info->ftLastWriteTime.dwLowDateTime, &info->ftLastWriteTime.dwHighDateTime);
    return TRUE;
}

inline uint32_t SetFilePointer(HANDLE handle, int32_t distance, int32_t *, uint32_t move_method)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->stream == nullptr) {
        return 0xFFFFFFFFu;
    }

    int origin = SEEK_SET;
    switch (move_method) {
        case FILE_BEGIN:
            origin = SEEK_SET;
            break;
        case FILE_END:
            origin = SEEK_END;
            break;
        case FILE_CURRENT:
        default:
            origin = SEEK_CUR;
            break;
    }

    const bool success = renegade_osdep::Seek_C_File(file_handle->stream, distance, origin);
    if (!success) {
        return 0xFFFFFFFFu;
    }
    const Sint64 position = renegade_osdep::Tell_C_File(file_handle->stream);
    return position >= 0 ? static_cast<uint32_t>(position) : 0xFFFFFFFFu;
}

inline int stricmp(const char * lhs, const char * rhs)
{
    return ::strcasecmp(lhs, rhs);
}

inline int _stricmp(const char * lhs, const char * rhs)
{
    return stricmp(lhs, rhs);
}

inline int strcmpi(const char * lhs, const char * rhs)
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

inline char * lstrcpy(char * destination, const char * source)
{
    return std::strcpy(destination, source != nullptr ? source : "");
}

inline char * lstrcat(char * destination, const char * source)
{
    return std::strcat(destination, source != nullptr ? source : "");
}

inline char * lstrcpyn(char * destination, const char * source, int count)
{
    if (destination == nullptr || count <= 0) {
        return destination;
    }
    std::snprintf(destination, static_cast<std::size_t>(count), "%s", source != nullptr ? source : "");
    return destination;
}

inline char * _strdup(const char * text)
{
    return text != nullptr ? ::strdup(text) : nullptr;
}

inline char * itoa(int value, char * buffer, int radix)
{
    if (buffer == nullptr) {
        return nullptr;
    }

    if (radix == 16) {
        std::snprintf(buffer, 34, "%x", value);
    } else {
        std::snprintf(buffer, 34, "%d", value);
    }
    return buffer;
}

inline int _snprintf(char * buffer, std::size_t size, const char * format, ...)
{
    va_list args;
    va_start(args, format);
    const int result = std::vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

#define RENEGADE_OUTPUTDEBUGSTRING_DEFINED 1

inline void OutputDebugString(const char * text)
{
    std::fputs(text != nullptr ? text : "", stderr);
}

inline char * strupr(char * text)
{
    if (text == nullptr) {
        return nullptr;
    }

    for (char * cursor = text; *cursor != '\0'; ++cursor) {
        *cursor = static_cast<char>(::toupper(static_cast<uint8_t>(*cursor)));
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
        *cursor = static_cast<char>(::tolower(static_cast<uint8_t>(*cursor)));
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

inline int wcsicmp(const wchar_t * lhs, const wchar_t * rhs)
{
    return _wcsicmp(lhs, rhs);
}

inline int lstrcmpi(const char * lhs, const char * rhs)
{
    return stricmp(lhs, rhs);
}

inline int lstrcmpi(const wchar_t * lhs, const wchar_t * rhs)
{
    return _wcsicmp(lhs, rhs);
}

template <typename CharT>
inline void renegade_copy_path_component(CharT * destination, std::size_t capacity, const std::basic_string<CharT> & value)
{
    if (destination == nullptr || capacity == 0) {
        return;
    }

    const std::size_t count = std::min(capacity - 1, value.size());
    if (count > 0) {
        std::char_traits<CharT>::copy(destination, value.c_str(), count);
    }
    destination[count] = static_cast<CharT>(0);
}

template <typename CharT>
inline void renegade_splitpath_impl(const CharT * path, CharT * drive, CharT * dir, CharT * fname, CharT * ext)
{
    renegade_copy_path_component(drive, _MAX_DRIVE, std::basic_string<CharT>());
    renegade_copy_path_component(dir, _MAX_DIR, std::basic_string<CharT>());
    renegade_copy_path_component(fname, _MAX_FNAME, std::basic_string<CharT>());
    renegade_copy_path_component(ext, _MAX_EXT, std::basic_string<CharT>());

    if (path == nullptr) {
        return;
    }

    const std::basic_string<CharT> text(path);
    std::size_t start = 0;
    std::basic_string<CharT> drive_text;
    if (text.size() >= 2 && text[1] == static_cast<CharT>(':')) {
        drive_text = text.substr(0, 2);
        start = 2;
    }

    std::size_t last_separator = std::basic_string<CharT>::npos;
    for (std::size_t index = text.size(); index-- > start;) {
        if (text[index] == static_cast<CharT>('/') || text[index] == static_cast<CharT>('\\')) {
            last_separator = index;
            break;
        }
    }

    std::basic_string<CharT> dir_text;
    std::size_t filename_start = start;
    if (last_separator != std::basic_string<CharT>::npos) {
        dir_text = text.substr(start, last_separator + 1 - start);
        filename_start = last_separator + 1;
    }

    const std::basic_string<CharT> filename_text = text.substr(filename_start);
    std::basic_string<CharT> fname_text = filename_text;
    std::basic_string<CharT> ext_text;

    std::size_t dot = std::basic_string<CharT>::npos;
    for (std::size_t index = filename_text.size(); index-- > 0;) {
        if (filename_text[index] == static_cast<CharT>('.')) {
            dot = index;
            break;
        }
    }

    if (dot != std::basic_string<CharT>::npos && dot != 0) {
        fname_text = filename_text.substr(0, dot);
        ext_text = filename_text.substr(dot);
    }

    renegade_copy_path_component(drive, _MAX_DRIVE, drive_text);
    renegade_copy_path_component(dir, _MAX_DIR, dir_text);
    renegade_copy_path_component(fname, _MAX_FNAME, fname_text);
    renegade_copy_path_component(ext, _MAX_EXT, ext_text);
}

inline void _splitpath(const char * path, char * drive, char * dir, char * fname, char * ext)
{
    renegade_splitpath_impl(path, drive, dir, fname, ext);
}

inline void _wsplitpath(const wchar_t * path, wchar_t * drive, wchar_t * dir, wchar_t * fname, wchar_t * ext)
{
    renegade_splitpath_impl(path, drive, dir, fname, ext);
}

inline int _wtoi(const wchar_t * text)
{
    return (text == nullptr) ? 0 : static_cast<int>(std::wcstol(text, nullptr, 10));
}

inline uint32_t _lrotl(uint32_t value, int shift)
{
    const uint32_t narrowed = static_cast<uint32_t>(value);
    const unsigned normalized = static_cast<unsigned>(shift) & 31u;
    return static_cast<uint32_t>((narrowed << normalized) | (narrowed >> ((32u - normalized) & 31u)));
}

inline uint32_t _byteswap_ulong(uint32_t value)
{
    return static_cast<uint32_t>(__builtin_bswap32(static_cast<uint32_t>(value)));
}

inline int CompareStringW(uint32_t, uint32_t, const wchar_t * lhs, int lhs_length, const wchar_t * rhs, int rhs_length)
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
    uint32_t,
    uint32_t,
    const wchar_t * source,
    int source_length,
    char * destination,
    int destination_length,
    const char *,
    int32_t * used_default_char)
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

inline int MultiByteToWideChar(
    uint32_t,
    uint32_t,
    const char * source,
    int source_length,
    wchar_t * destination,
    int destination_length)
{
    if (source == nullptr) {
        return 0;
    }

    std::mbstate_t state{};
    const char * src = source;

    if (destination == nullptr || destination_length == 0) {
        return static_cast<int>(1 + std::mbsrtowcs(nullptr, &src, 0, &state));
    }

    if (source_length < 0) {
        return static_cast<int>(std::mbsrtowcs(destination, &src, destination_length, &state));
    }

    std::string temp(source, source + source_length);
    temp.push_back('\0');
    const char * temp_src = temp.c_str();
    return static_cast<int>(std::mbsrtowcs(destination, &temp_src, destination_length, &state));
}

#endif // _WIN32

#endif

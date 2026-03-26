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
#include <filesystem>
#include <limits.h>
#include <mutex>
#include <string>
#include <thread>
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
using LARGE_INTEGER = long long;
using FARPROC = void *;
using LONG = long;
using LPARAM = std::intptr_t;
using WPARAM = std::uintptr_t;
using LRESULT = std::intptr_t;
using LPLOGFONT = void *;

struct CRITICAL_SECTION {
    std::recursive_mutex mutex;
};

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

struct WIN32_FIND_DATAA {
	DWORD dwFileAttributes;
	char cFileName[260];
};

using WIN32_FIND_DATA = WIN32_FIND_DATAA;

using COLORREF = DWORD;

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
#define MAKEINTRESOURCE(i) reinterpret_cast<const char *>(static_cast<uintptr_t>(static_cast<WORD>(i)))
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

namespace renegade_osdep {

struct CompatFileHandle {
    std::FILE *file;
};

struct CompatFindHandle {
    std::vector<std::filesystem::path> entries;
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

inline bool Find_Case_Insensitive_Path_Component(const std::filesystem::path & directory, const std::string & component, std::string & matched_component)
{
    std::error_code error;
    for (const auto & entry : std::filesystem::directory_iterator(directory, error)) {
        if (error) {
            break;
        }

        const std::string filename = entry.path().filename().string();
        if (::strcasecmp(filename.c_str(), component.c_str()) == 0) {
            matched_component = filename;
            return true;
        }
    }

    return false;
}

inline bool Resolve_Path_Case(const std::string & normalized_path, bool allow_missing_leaf, std::filesystem::path & resolved_path)
{
    if (normalized_path.empty()) {
        return false;
    }

    const char * path = normalized_path.c_str();
    const int path_length = static_cast<int>(normalized_path.size());

    std::filesystem::path current_path;
    int cursor = 0;

    if (Is_Path_Separator(path[0])) {
        current_path = std::filesystem::path("/");
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
            if (current_path.empty()) {
                current_path = std::filesystem::path("..");
            } else {
                current_path /= component;
            }
            continue;
        }

        int next_component = cursor;
        while (next_component < path_length && Is_Path_Separator(path[next_component])) {
            ++next_component;
        }
        const bool is_last_component = (next_component >= path_length);

        const std::filesystem::path search_directory = current_path.empty() ? std::filesystem::path(".") : current_path;
        std::error_code status_error;
        if (!std::filesystem::exists(search_directory, status_error) || !std::filesystem::is_directory(search_directory, status_error)) {
            return false;
        }

        std::string matched_component;
        if (Find_Case_Insensitive_Path_Component(search_directory, component, matched_component)) {
            current_path /= matched_component;
        } else {
            if (allow_missing_leaf && is_last_component) {
                current_path /= component;
                resolved_path = current_path;
                return true;
            }
            return false;
        }
    }

    resolved_path = current_path.empty() ? std::filesystem::path(normalized_path) : current_path;
    return true;
}

inline bool Resolve_Existing_Path(const char * path, std::filesystem::path & resolved_path)
{
    const std::string normalized = Normalize_Path(path);
    if (normalized.empty()) {
        return false;
    }

    std::error_code error;
    const std::filesystem::path candidate(normalized);
    if (std::filesystem::exists(candidate, error)) {
        resolved_path = candidate;
        return true;
    }

    return Resolve_Path_Case(normalized, false, resolved_path);
}

inline bool Resolve_Path_For_Access(const char * path, bool allow_missing_leaf, std::filesystem::path & resolved_path)
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

inline bool Resolve_Find_Pattern(const char * pattern, std::filesystem::path & directory, std::string & wildcard)
{
    const std::string normalized = Normalize_Path(pattern);
    if (normalized.empty()) {
        return false;
    }

    const std::filesystem::path path(normalized);
    wildcard = path.filename().string();

    std::filesystem::path raw_directory = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
    if (raw_directory.empty()) {
        raw_directory = std::filesystem::path(".");
    }

    return Resolve_Existing_Path(raw_directory.string().c_str(), directory);
}

inline void Populate_Find_Data(const std::filesystem::path & entry_path, WIN32_FIND_DATA * find_data)
{
    if (find_data == nullptr) {
        return;
    }

    std::memset(find_data, 0, sizeof(*find_data));
    std::snprintf(find_data->cFileName, sizeof(find_data->cFileName), "%s", entry_path.filename().string().c_str());

    std::error_code error;
    const auto status = std::filesystem::status(entry_path, error);
    if (!error && std::filesystem::is_directory(status)) {
        find_data->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
}

inline bool Wildcard_Match(const char * pattern, const char * text)
{
    if (pattern == nullptr || text == nullptr) {
        return false;
    }

    if (*pattern == '\0') {
        return *text == '\0';
    }

    if (*pattern == '*') {
        for (const char * cursor = text; ; ++cursor) {
            if (Wildcard_Match(pattern + 1, cursor)) {
                return true;
            }
            if (*cursor == '\0') {
                break;
            }
        }
        return false;
    }

    if (*pattern == '?') {
        return (*text != '\0') && Wildcard_Match(pattern + 1, text + 1);
    }

    return (std::tolower(static_cast<unsigned char>(*pattern)) == std::tolower(static_cast<unsigned char>(*text)))
        && Wildcard_Match(pattern + 1, text + 1);
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

inline void Sleep(DWORD milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline void ExitProcess(UINT exit_code)
{
	std::exit(static_cast<int>(exit_code));
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

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER * frequency)
{
    if (frequency != nullptr) {
        *frequency = 1000000;
    }
    return TRUE;
}

inline BOOL QueryPerformanceCounter(LARGE_INTEGER * counter)
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

inline BOOL TryEnterCriticalSection(CRITICAL_SECTION * critical_section)
{
    return (critical_section != nullptr && critical_section->mutex.try_lock()) ? TRUE : FALSE;
}

inline int DeleteFile(const char * filename)
{
    std::filesystem::path resolved_path;
    if (!renegade_osdep::Resolve_Existing_Path(filename, resolved_path)) {
        return FALSE;
    }

    std::error_code error;
    return std::filesystem::remove(resolved_path, error) ? TRUE : FALSE;
}

inline int MoveFile(const char * existing_filename, const char * new_filename)
{
    if (existing_filename == nullptr || new_filename == nullptr) {
        errno = EINVAL;
        return FALSE;
    }

    std::filesystem::path existing_path;
    if (!renegade_osdep::Resolve_Existing_Path(existing_filename, existing_path)) {
        errno = ENOENT;
        return FALSE;
    }

    std::filesystem::path new_path;
    if (!renegade_osdep::Resolve_Path_For_Access(new_filename, true, new_path)) {
        errno = ENOENT;
        return FALSE;
    }

    std::error_code error;
    std::filesystem::rename(existing_path, new_path, error);
    return error ? FALSE : TRUE;
}

inline DWORD GetModuleFileName(HINSTANCE, char * buffer, DWORD size)
{
    if (buffer == nullptr || size == 0) {
        return 0;
    }

    std::error_code error;
    const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
    const std::string path = error ? std::filesystem::current_path(error).string() : executable.string();
    const std::size_t count = std::min<std::size_t>(size - 1, path.size());
    std::memcpy(buffer, path.c_str(), count);
    buffer[count] = '\0';
    return static_cast<DWORD>(count);
}

inline BOOL CreateDirectory(const char * path, void *)
{
    if (path == nullptr) {
        errno = EINVAL;
        return FALSE;
    }

    std::filesystem::path existing_directory;
    if (renegade_osdep::Resolve_Existing_Path(path, existing_directory)) {
        errno = EEXIST;
        return FALSE;
    }

    std::filesystem::path directory;
    if (!renegade_osdep::Resolve_Path_For_Access(path, true, directory)) {
        errno = ENOENT;
        return FALSE;
    }

    std::error_code error;

    return std::filesystem::create_directories(directory, error) ? TRUE : FALSE;
}

inline HANDLE CreateFile(const char * filename, DWORD desired_access, DWORD, void *, DWORD creation_disposition, DWORD, HANDLE)
{
    if (filename == nullptr) {
        errno = EINVAL;
        return INVALID_HANDLE_VALUE;
    }

    const bool wants_write = (desired_access & GENERIC_WRITE) != 0 || creation_disposition == CREATE_ALWAYS || creation_disposition == CREATE_NEW;

    std::filesystem::path path;
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

    std::FILE * file = std::fopen(path.string().c_str(), mode);
    if (file == nullptr) {
        return INVALID_HANDLE_VALUE;
    }

    auto * handle = new renegade_osdep::CompatFileHandle{file};
    return reinterpret_cast<HANDLE>(handle);
}

inline DWORD GetFileSize(HANDLE handle, DWORD *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->file == nullptr) {
        return 0xFFFFFFFFu;
    }

    const long current = std::ftell(file_handle->file);
    if (current < 0) {
        return 0xFFFFFFFFu;
    }

    if (std::fseek(file_handle->file, 0, SEEK_END) != 0) {
        return 0xFFFFFFFFu;
    }

    const long end = std::ftell(file_handle->file);
    std::fseek(file_handle->file, current, SEEK_SET);
    return end >= 0 ? static_cast<DWORD>(end) : 0xFFFFFFFFu;
}

inline BOOL WriteFile(HANDLE handle, const void * buffer, DWORD bytes_to_write, DWORD * bytes_written, void *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->file == nullptr) {
        return FALSE;
    }

    const std::size_t written = std::fwrite(buffer, 1, bytes_to_write, file_handle->file);
    if (bytes_written != nullptr) {
        *bytes_written = static_cast<DWORD>(written);
    }

    return written == bytes_to_write ? TRUE : FALSE;
}

inline BOOL ReadFile(HANDLE handle, void * buffer, DWORD bytes_to_read, DWORD * bytes_read, void *)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (file_handle == nullptr || file_handle->file == nullptr) {
        return FALSE;
    }

    const std::size_t read = std::fread(buffer, 1, bytes_to_read, file_handle->file);
    if (bytes_read != nullptr) {
        *bytes_read = static_cast<DWORD>(read);
    }

    return read == bytes_to_read ? TRUE : FALSE;
}

inline BOOL CloseHandle(HANDLE handle)
{
    auto * file_handle = renegade_osdep::As_File_Handle(handle);
    if (handle == INVALID_HANDLE_VALUE || file_handle == nullptr) {
        return FALSE;
    }

    const int result = (file_handle->file != nullptr) ? std::fclose(file_handle->file) : 0;
    delete file_handle;
    return result == 0 ? TRUE : FALSE;
}

inline HANDLE FindFirstFile(const char * pattern, WIN32_FIND_DATA * find_data)
{
    if (pattern == nullptr || find_data == nullptr) {
        return INVALID_HANDLE_VALUE;
    }

    std::filesystem::path directory;
    std::string wildcard;
    if (!renegade_osdep::Resolve_Find_Pattern(pattern, directory, wildcard)) {
        return INVALID_HANDLE_VALUE;
    }

    auto * handle = new renegade_osdep::CompatFindHandle{};
    handle->index = 0;

    std::error_code error;
    for (const auto & entry : std::filesystem::directory_iterator(directory, error)) {
        if (error) {
            break;
        }

        const std::string filename = entry.path().filename().string();
        if (renegade_osdep::Wildcard_Match(wildcard.c_str(), filename.c_str())) {
            handle->entries.push_back(entry.path());
        }
    }

    if (handle->entries.empty()) {
        delete handle;
        return INVALID_HANDLE_VALUE;
    }

    renegade_osdep::Populate_Find_Data(handle->entries.front(), find_data);
    return reinterpret_cast<HANDLE>(handle);
}

inline BOOL FindNextFile(HANDLE handle, WIN32_FIND_DATA * find_data)
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

inline BOOL FindClose(HANDLE handle)
{
    auto * find_handle = renegade_osdep::As_Find_Handle(handle);
    if (find_handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    delete find_handle;
    return TRUE;
}

inline BOOL GetComputerName(char * buffer, DWORD * size)
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
    *size = static_cast<DWORD>(count);
    return TRUE;
}

inline BOOL GetUserName(char * buffer, DWORD * size)
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
    *size = static_cast<DWORD>(count);
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

inline BOOL ShowWindow(HWND window_handle, int command)
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

inline DWORD GetFileAttributes(const char * filename)
{
    std::filesystem::path resolved_path;
    if (!renegade_osdep::Resolve_Existing_Path(filename, resolved_path)) {
        return INVALID_FILE_ATTRIBUTES;
    }

    std::error_code error;
    const auto status = std::filesystem::status(resolved_path, error);
    if (error || !std::filesystem::exists(status)) {
        return INVALID_FILE_ATTRIBUTES;
    }

    DWORD attributes = 0;
    if (std::filesystem::is_directory(status)) {
        attributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
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

inline DWORD GetCurrentDirectory(DWORD buffer_length, char * buffer)
{
    const std::string cwd = std::filesystem::current_path().string();
    if (buffer == nullptr || buffer_length == 0) {
        return static_cast<DWORD>(cwd.size());
    }
    std::snprintf(buffer, buffer_length, "%s", cwd.c_str());
    return static_cast<DWORD>(std::min<std::size_t>(cwd.size(), buffer_length > 0 ? buffer_length - 1 : 0));
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

inline int MultiByteToWideChar(
    unsigned int,
    DWORD,
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

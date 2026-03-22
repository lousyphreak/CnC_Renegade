/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*********************************************************************************************** 
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               *** 
 *********************************************************************************************** 
 *                                                                                             * 
 *                 Project Name : Command & Conquer                                            * 
 *                                                                                             * 
 *                     $Archive:: /Commando/Code/wwlib/win.h                                  $* 
 *                                                                                             * 
 *                      $Author:: Ian_l                                                       $*
 *                                                                                             * 
 *                     $Modtime:: 10/16/01 2:42p                                              $*
 *                                                                                             * 
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#pragma once

#ifndef WIN_H
#define WIN_H

#ifdef _WIN32

/*
**	This header file includes the Windows headers. If there are any special pragmas that need
**	to occur around this process, they are performed here. Typically, certain warnings will need
**	to be disabled since the Windows headers are repleat with illegal and dangerous constructs.
**
**	Within the windows headers themselves, Microsoft has disabled the warnings 4290, 4514, 
**	4069, 4200, 4237, 4103, 4001, 4035, 4164. Makes you wonder, eh?
*/

// When including windows, lets just bump the warning level back to 3...
#if (_MSC_VER >= 1200)
#pragma warning(push, 3)
#endif

// this define should also be in the DSP just in case someone includes windows stuff directly
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
//#include <mmsystem.h>
//#include	<windowsx.h>
//#include	<winnt.h>
//#include	<winuser.h>

#if (_MSC_VER >= 1200)
#pragma warning(pop)
#endif

#ifdef _WINDOWS
extern HINSTANCE	ProgramInstance;
extern HWND			MainWindow;
extern bool GameInFocus;

#ifdef _DEBUG

void __cdecl Print_Win32Error(unsigned long win32Error);

#else // _DEBUG

#define Print_Win32Error

#endif // _DEBUG

#else // _WINDOWS
//#include <unistd.h>
#endif // _WINDOWS

#else

#include "osdep.h"
#include <cstdint>
#include <chrono>
#include <cstring>
#include <ctime>
#include <unistd.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef RENEGADE_COMPAT_HKEY_DEFINED
#define RENEGADE_COMPAT_HKEY_DEFINED
typedef void *HKEY;
#endif

#ifndef RENEGADE_COMPAT_SYSTEMTIME_DEFINED
#define RENEGADE_COMPAT_SYSTEMTIME_DEFINED
typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;
#endif

#ifndef RENEGADE_COMPAT_FILETIME_DEFINED
#define RENEGADE_COMPAT_FILETIME_DEFINED
typedef struct _FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *LPFILETIME;
#endif

#ifndef RENEGADE_COMPAT_PROCESS_INFORMATION_DEFINED
#define RENEGADE_COMPAT_PROCESS_INFORMATION_DEFINED
typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
#endif

#ifndef RENEGADE_COMPAT_STARTUPINFO_DEFINED
#define RENEGADE_COMPAT_STARTUPINFO_DEFINED
typedef struct _STARTUPINFOA {
	DWORD cb;
	char *lpReserved;
	char *lpDesktop;
	char *lpTitle;
	DWORD dwX;
	DWORD dwY;
	DWORD dwXSize;
	DWORD dwYSize;
	DWORD dwXCountChars;
	DWORD dwYCountChars;
	DWORD dwFillAttribute;
	DWORD dwFlags;
	WORD wShowWindow;
	WORD cbReserved2;
	unsigned char *lpReserved2;
	HANDLE hStdInput;
	HANDLE hStdOutput;
	HANDLE hStdError;
} STARTUPINFO, *LPSTARTUPINFO;
#endif

#ifndef RENEGADE_COMPAT_RESOURCE_TYPES_DEFINED
#define RENEGADE_COMPAT_RESOURCE_TYPES_DEFINED
typedef void * HRSRC;
typedef void * HGLOBAL;
typedef void * LPVOID;

typedef struct _DLGTEMPLATE {
	DWORD style;
	DWORD dwExtendedStyle;
	WORD cdit;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATE;

typedef struct _DLGITEMTEMPLATE {
	DWORD style;
	DWORD dwExtendedStyle;
	short x;
	short y;
	short cx;
	short cy;
	WORD id;
} DLGITEMTEMPLATE;
#endif

#ifndef RT_DIALOG
#define RT_DIALOG reinterpret_cast<const char *>(5)
#endif

#ifndef DS_SETFONT
#define DS_SETFONT 0x00000040L
#endif

#ifndef CSTR_LESS_THAN
#define CSTR_LESS_THAN 1
#endif

#ifndef CSTR_EQUAL
#define CSTR_EQUAL 2
#endif

#ifndef CSTR_GREATER_THAN
#define CSTR_GREATER_THAN 3
#endif

extern HINSTANCE	ProgramInstance;
extern HWND			MainWindow;

inline HRSRC FindResource(HINSTANCE, const char *, const char *) { return nullptr; }
inline HGLOBAL LoadResource(HINSTANCE, HRSRC) { return nullptr; }
inline LPVOID LockResource(HGLOBAL) { return nullptr; }

inline DWORD GetTickCount(void)
{
	using namespace std::chrono;
	return static_cast<DWORD>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() & 0xffffffffu);
}

inline DWORD GetCurrentProcessId(void)
{
	return static_cast<DWORD>(getpid());
}

inline void GetSystemTime(LPSYSTEMTIME system_time)
{
	if (system_time == nullptr) {
		return;
	}

	std::time_t now = std::time(nullptr);
	std::tm utc_time = {};
	gmtime_r(&now, &utc_time);

	system_time->wYear = static_cast<WORD>(utc_time.tm_year + 1900);
	system_time->wMonth = static_cast<WORD>(utc_time.tm_mon + 1);
	system_time->wDayOfWeek = static_cast<WORD>(utc_time.tm_wday);
	system_time->wDay = static_cast<WORD>(utc_time.tm_mday);
	system_time->wHour = static_cast<WORD>(utc_time.tm_hour);
	system_time->wMinute = static_cast<WORD>(utc_time.tm_min);
	system_time->wSecond = static_cast<WORD>(utc_time.tm_sec);
	system_time->wMilliseconds = 0;
}

inline bool FileTimeToSystemTime(const FILETIME * file_time, LPSYSTEMTIME system_time)
{
	if (file_time == nullptr || system_time == nullptr) {
		return false;
	}

	const std::uint64_t ticks = (static_cast<std::uint64_t>(file_time->dwHighDateTime) << 32) | file_time->dwLowDateTime;
	constexpr std::uint64_t WINDOWS_TO_UNIX_EPOCH_100NS = 11644473600ull * 10000000ull;
	if (ticks < WINDOWS_TO_UNIX_EPOCH_100NS) {
		return false;
	}

	const std::uint64_t unix_100ns = ticks - WINDOWS_TO_UNIX_EPOCH_100NS;
	const std::time_t seconds = static_cast<std::time_t>(unix_100ns / 10000000ull);
	std::tm utc_time = {};
	gmtime_r(&seconds, &utc_time);

	system_time->wYear = static_cast<WORD>(utc_time.tm_year + 1900);
	system_time->wMonth = static_cast<WORD>(utc_time.tm_mon + 1);
	system_time->wDayOfWeek = static_cast<WORD>(utc_time.tm_wday);
	system_time->wDay = static_cast<WORD>(utc_time.tm_mday);
	system_time->wHour = static_cast<WORD>(utc_time.tm_hour);
	system_time->wMinute = static_cast<WORD>(utc_time.tm_min);
	system_time->wSecond = static_cast<WORD>(utc_time.tm_sec);
	system_time->wMilliseconds = static_cast<WORD>((unix_100ns % 10000000ull) / 10000ull);
	return true;
}

inline bool SystemTimeToFileTime(const SYSTEMTIME * system_time, LPFILETIME file_time)
{
	if (system_time == nullptr || file_time == nullptr) {
		return false;
	}

	std::tm utc_time = {};
	utc_time.tm_year = static_cast<int>(system_time->wYear) - 1900;
	utc_time.tm_mon = static_cast<int>(system_time->wMonth) - 1;
	utc_time.tm_mday = static_cast<int>(system_time->wDay);
	utc_time.tm_hour = static_cast<int>(system_time->wHour);
	utc_time.tm_min = static_cast<int>(system_time->wMinute);
	utc_time.tm_sec = static_cast<int>(system_time->wSecond);

	const std::time_t seconds = timegm(&utc_time);
	if (seconds < 0) {
		return false;
	}

	constexpr std::uint64_t WINDOWS_TO_UNIX_EPOCH_100NS = 11644473600ull * 10000000ull;
	const std::uint64_t ticks = WINDOWS_TO_UNIX_EPOCH_100NS + (static_cast<std::uint64_t>(seconds) * 10000000ull) + (static_cast<std::uint64_t>(system_time->wMilliseconds) * 10000ull);
	file_time->dwLowDateTime = static_cast<DWORD>(ticks & 0xFFFFFFFFull);
	file_time->dwHighDateTime = static_cast<DWORD>(ticks >> 32);
	return true;
}

inline int lstrlen(const char *text)
{
	return (text != NULL) ? static_cast<int>(std::strlen(text)) : 0;
}

#endif // _WIN32

#endif // WIN_H

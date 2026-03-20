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
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

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

inline int lstrlen(const char *text)
{
	return (text != NULL) ? static_cast<int>(std::strlen(text)) : 0;
}

#endif // _WIN32

#endif // WIN_H

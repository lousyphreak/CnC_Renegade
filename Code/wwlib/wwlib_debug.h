#pragma once

#ifndef WWLIB_DEBUG_H
#define WWLIB_DEBUG_H

#include <SDL3/SDL_assert.h>

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef WW_PRINTF_FORMAT_ATTRIBUTE
#if defined(__GNUC__) || defined(__clang__)
#define WW_PRINTF_FORMAT_ATTRIBUTE(format_index, first_arg_index) __attribute__((format(printf, format_index, first_arg_index)))
#else
#define WW_PRINTF_FORMAT_ATTRIBUTE(format_index, first_arg_index)
#endif
#endif

#ifndef MESSAGE
#define STRING_IT(a) #a
#define TOKEN_IT(a) STRING_IT(a)
#define MESSAGE(a) message(__FILE__ "(" TOKEN_IT(__LINE__) ") : " a)
#endif

enum WWLibDebugType {
	WWLIB_DEBUG_TYPE_INFORMATION,
	WWLIB_DEBUG_TYPE_WARNING,
	WWLIB_DEBUG_TYPE_ERROR,
	WWLIB_DEBUG_TYPE_USER
};

inline void WWLib_Debug_VPrintf(WWLibDebugType type, const char * format, std::va_list args)
{
	FILE * stream = (type == WWLIB_DEBUG_TYPE_WARNING || type == WWLIB_DEBUG_TYPE_ERROR) ? stderr : stdout;
	std::vfprintf(stream, format, args);
	std::fflush(stream);
}

inline void WWLib_Debug_Printf(const char * format, ...) WW_PRINTF_FORMAT_ATTRIBUTE(1, 2);
inline void WWLib_Debug_Printf_Warning(const char * format, ...) WW_PRINTF_FORMAT_ATTRIBUTE(1, 2);
inline void WWLib_Debug_Printf_Error(const char * format, ...) WW_PRINTF_FORMAT_ATTRIBUTE(1, 2);

inline void WWLib_Debug_Printf(const char * format, ...)
{
	std::va_list args;
	va_start(args, format);
	WWLib_Debug_VPrintf(WWLIB_DEBUG_TYPE_INFORMATION, format, args);
	va_end(args);
}

inline void WWLib_Debug_Printf_Warning(const char * format, ...)
{
	std::va_list args;
	va_start(args, format);
	WWLib_Debug_VPrintf(WWLIB_DEBUG_TYPE_WARNING, format, args);
	va_end(args);
}

inline void WWLib_Debug_Printf_Error(const char * format, ...)
{
	std::va_list args;
	va_start(args, format);
	WWLib_Debug_VPrintf(WWLIB_DEBUG_TYPE_ERROR, format, args);
	va_end(args);
}

inline void WWLib_Debug_Assert_Fail(const char * expr, const char * file, int line)
{
	std::fprintf(stderr, "Assertion failed: %s (%s:%d)\n", expr, file, line);
	std::fflush(stderr);
	SDL_TriggerBreakpoint();
	std::abort();
}

inline void WWLib_Debug_Assert_Fail_Print(const char * expr, const char * file, int line, const char * message)
{
	std::fprintf(stderr, "Assertion failed: %s (%s:%d): %s\n", expr, file, line, (message != nullptr) ? message : "");
	std::fflush(stderr);
	SDL_TriggerBreakpoint();
	std::abort();
}

inline void Convert_System_Error_To_String(int error_id, char * buffer, int buf_len)
{
	if (buffer == nullptr || buf_len <= 0) {
		return;
	}

	std::snprintf(buffer, static_cast<std::size_t>(buf_len), "%s", std::strerror(error_id));
}

inline int Get_Last_System_Error()
{
	return errno;
}

#ifdef WWDEBUG
#define WWDEBUG_SAY(x)							WWLib_Debug_Printf x
#define WWDEBUG_WARNING(x)					WWLib_Debug_Printf_Warning x
#define WWDEBUG_ERROR(x)					WWLib_Debug_Printf_Error x
#else
#define WWDEBUG_SAY(x)
#define WWDEBUG_WARNING(x)
#define WWDEBUG_ERROR(x)
#endif

#define WWRELEASE_SAY(x)					WWLib_Debug_Printf x
#define WWRELEASE_WARNING(x)			WWLib_Debug_Printf_Warning x
#define WWRELEASE_ERROR(x)				WWLib_Debug_Printf_Error x

#if defined(WWDEBUG) || !defined(NDEBUG)
#define WWASSERT(expr)						((expr) ? (void)0 : WWLib_Debug_Assert_Fail(#expr, __FILE__, __LINE__))
#define WWASSERT_PRINT(expr, string)	((expr) ? (void)0 : WWLib_Debug_Assert_Fail_Print(#expr, __FILE__, __LINE__, string))
#define DIE								WWLib_Debug_Assert_Fail("DIE", __FILE__, __LINE__)
#else
#define WWASSERT(expr)						((void)0)
#define WWASSERT_PRINT(expr, string)	((void)0)
#define DIE								((void)0)
#endif

#define WWDEBUG_BREAK SDL_TriggerBreakpoint()
#define WWDEBUG_TRIGGER_GENERIC0 0
#define WWDEBUG_TRIGGER_GENERIC1 1
#define WWDEBUG_TRIGGER(x) (0)
#define WWDEBUG_PROFILE_START(x) ((void)0)
#define WWDEBUG_PROFILE_STOP(x) ((void)0)

#endif

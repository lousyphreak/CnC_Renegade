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
 *                 Project Name : WWDebug                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwdebug/wwdebug.cpp                          $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 1/13/02 1:46p                                               $*
 *                                                                                             *
 *                    $Revision:: 16                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WWDebug_Install_Message_Handler -- install function for handling the debug messages       *
 *   WWDebug_Install_Assert_Handler -- Install a function for handling the assert messages     *
 *   WWDebug_Install_Trigger_Handler -- install a trigger handler function                     *
 *   WWDebug_Printf -- Internal function for passing messages to installed handler             *
 *   WWDebug_Assert_Fail -- Internal function for passing assert messages to installed handler *
 *   WWDebug_Assert_Fail_Print -- Internal function, passes assert message to handler          *
 *   WWDebug_Check_Trigger -- calls the user-installed debug trigger handler                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "wwdebug.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_messagebox.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>


static PrintFunc			_CurMessageHandler = NULL;
static AssertPrintFunc	_CurAssertHandler = NULL;
static TriggerFunc		_CurTriggerHandler = NULL;
static ProfileFunc		_CurProfileStartHandler = NULL;
static ProfileFunc		_CurProfileStopHandler = NULL;

namespace {

enum AssertDialogButtonId {
	ASSERT_BUTTON_ABORT = 1,
	ASSERT_BUTTON_RETRY = 2,
	ASSERT_BUTTON_IGNORE = 3,
};

std::string WWDebug_Format_Message(const char * format, va_list arguments)
{
	if (format == nullptr) {
		return {};
	}

	va_list copy;
	va_copy(copy, arguments);
	const int required = std::vsnprintf(nullptr, 0, format, copy);
	va_end(copy);

	if (required < 0) {
		return format;
	}

	std::string buffer(static_cast<std::size_t>(required), '\0');
	std::vsnprintf(buffer.data(), buffer.size() + 1, format, arguments);
	return buffer;
}

void WWDebug_Dispatch_Message(DebugType type, const char * format, va_list arguments)
{
	if (_CurMessageHandler == NULL) {
		return;
	}

	const std::string buffer = WWDebug_Format_Message(format, arguments);
	_CurMessageHandler(type, buffer.c_str());
}

void WWDebug_Default_Assert(const char * expr, const char * file, int line, const char * detail)
{
	char assert_buffer[4096];
	if ((detail != nullptr) && (detail[0] != '\0')) {
		std::snprintf(assert_buffer, sizeof(assert_buffer),
			"Assert failed\n\nExpression: %s\nFile: %s\nLine: %d\nDetail: %s",
			expr,
			file,
			line,
			detail);
	} else {
		std::snprintf(assert_buffer, sizeof(assert_buffer),
			"Assert failed\n\nExpression: %s\nFile: %s\nLine: %d",
			expr,
			file,
			line);
	}

	std::fprintf(stderr, "%s\n", assert_buffer);
	std::fflush(stderr);
	SDL_LogMessage(SDL_LOG_CATEGORY_ASSERT, SDL_LOG_PRIORITY_CRITICAL, "%s", assert_buffer);

	if (SDL_IsMainThread()) {
		const SDL_MessageBoxButtonData buttons[] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, ASSERT_BUTTON_RETRY, "Retry" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, ASSERT_BUTTON_IGNORE, "Ignore" },
			{ 0, ASSERT_BUTTON_ABORT, "Abort" },
		};
		const SDL_MessageBoxData message_box = {
			SDL_MESSAGEBOX_ERROR,
			NULL,
			"WWDebug Assert",
			assert_buffer,
			static_cast<int>(sizeof(buttons) / sizeof(buttons[0])),
			buttons,
			NULL,
		};

		int selected_button = ASSERT_BUTTON_RETRY;
		if (SDL_ShowMessageBox(&message_box, &selected_button)) {
			switch (selected_button) {
				case ASSERT_BUTTON_ABORT:
					raise(SIGABRT);
					std::_Exit(3);
					break;

				case ASSERT_BUTTON_IGNORE:
					return;

				case ASSERT_BUTTON_RETRY:
				default:
					WWDEBUG_BREAK;
					return;
			}
		}
	}

	WWDEBUG_BREAK;
}

} // namespace

/***********************************************************************************************
 * WWDebug_Install_Message_Handler -- install function for handling the debug messages         *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/
PrintFunc WWDebug_Install_Message_Handler(PrintFunc func)
{
	PrintFunc tmp = _CurMessageHandler;
	_CurMessageHandler = func;
	return tmp;
}


/***********************************************************************************************
 * WWDebug_Install_Assert_Handler -- Install a function for handling the assert messages       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/
AssertPrintFunc WWDebug_Install_Assert_Handler(AssertPrintFunc func)
{
	AssertPrintFunc tmp = _CurAssertHandler;
	_CurAssertHandler = func;
	return tmp;
}


/***********************************************************************************************
 * WWDebug_Install_Trigger_Handler -- install a trigger handler function                       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
TriggerFunc	WWDebug_Install_Trigger_Handler(TriggerFunc func)
{
	TriggerFunc tmp = _CurTriggerHandler;
	_CurTriggerHandler = func;
	return tmp;
}


/***********************************************************************************************
 * WWDebug_Install_Profile_Start_Handler -- install a profile handler function                 *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
ProfileFunc	WWDebug_Install_Profile_Start_Handler(ProfileFunc func)
{
	ProfileFunc tmp = _CurProfileStartHandler;
	_CurProfileStartHandler = func;
	return tmp;
}


/***********************************************************************************************
 * WWDebug_Install_Profile_Stop_Handler -- install a profile handler function                  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
ProfileFunc	WWDebug_Install_Profile_Stop_Handler(ProfileFunc func)
{
	ProfileFunc tmp = _CurProfileStopHandler;
	_CurProfileStopHandler = func;
	return tmp;
}


/***********************************************************************************************
 * WWDebug_Printf -- Internal function for passing messages to installed handler               *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/

void WWDebug_Printf(const char * format,...)
{
	va_list va;
	va_start(va, format);
	WWDebug_Dispatch_Message(WWDEBUG_TYPE_INFORMATION, format, va);
	va_end(va);
}

/***********************************************************************************************
 * WWDebug_Printf_Warning -- Internal function for passing messages to installed handler       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/

void WWDebug_Printf_Warning(const char * format,...)
{
	va_list va;
	va_start(va, format);
	WWDebug_Dispatch_Message(WWDEBUG_TYPE_WARNING, format, va);
	va_end(va);
}

/***********************************************************************************************
 * WWDebug_Printf_Error -- Internal function for passing messages to installed handler         *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/

void WWDebug_Printf_Error(const char * format,...)
{
	va_list va;
	va_start(va, format);
	WWDebug_Dispatch_Message(WWDEBUG_TYPE_ERROR, format, va);
	va_end(va);
}

/***********************************************************************************************
 * WWDebug_Assert_Fail -- Internal function for passing assert messages to installed handler   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/
#ifdef WWDEBUG
void WWDebug_Assert_Fail(const char * expr,const char * file, int line)
{
	if (_CurAssertHandler != NULL) {

		char buffer[4096];
		std::snprintf(buffer, sizeof(buffer), "%s (%d) Assert: %s\n", file, line, expr);
		_CurAssertHandler(buffer);

	} else {
		WWDebug_Default_Assert(expr, file, line, NULL);
   }
}
#endif




/***********************************************************************************************
 * _assert -- Catch all asserts by overriding lib function                                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Assert stuff                                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/11/2001 3:56PM ST : Created                                                            *
 *=============================================================================================*/
#if defined(WWDEBUG) && defined(_MSC_VER)
void __cdecl _assert(void *expr, void *filename, unsigned lineno)
{
	WWDebug_Assert_Fail((const char*)expr, (const char*)filename, lineno);
}
#endif //WWDEBUG && _MSC_VER





/***********************************************************************************************
 * WWDebug_Assert_Fail_Print -- Internal function, passes assert message to handler            *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/19/98    GTH : Created.                                                                 *
 *=============================================================================================*/
#ifdef WWDEBUG
void WWDebug_Assert_Fail_Print(const char * expr,const char * file, int line,const char * string)
{
	if (_CurAssertHandler != NULL) {

		char buffer[4096];
		std::snprintf(buffer, sizeof(buffer), "%s (%d) Assert: %s %s\n", file, line, expr, (string != NULL) ? string : "");
		_CurAssertHandler(buffer);

	} else {
		WWDebug_Default_Assert(expr, file, line, string);

	}
}
#endif


/***********************************************************************************************
 * WWDebug_Check_Trigger -- calls the user-installed debug trigger handler                     *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
bool WWDebug_Check_Trigger(int trigger_num)
{
	if (_CurTriggerHandler != NULL) {
		return _CurTriggerHandler(trigger_num);
	} else {
		return false;
	}
}


/***********************************************************************************************
 * WWDebug_Profile_Start -- calls the user-installed profile start handler                     *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
void WWDebug_Profile_Start( const char * title)
{
	if (_CurProfileStartHandler != NULL) {
		_CurProfileStartHandler( title );
	}
}


/***********************************************************************************************
 * WWDebug_Profile_Stop -- calls the user-installed profile start handler                      *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
void WWDebug_Profile_Stop( const char * title)
{
	if (_CurProfileStopHandler != NULL) {
		_CurProfileStopHandler( title );
	}
}



#ifdef WWDEBUG
/***********************************************************************************************
 * WWDebug_DBWin32_Message_Handler --                                                          *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/30/98    BMG : Created.                                                                *
 *=============================================================================================*/
void WWDebug_DBWin32_Message_Handler( const char * str )
{
	if ((str != NULL) && (str[0] != '\0')) {
		SDL_Log("%s", str);
	}
}
#endif // WWDEBUG

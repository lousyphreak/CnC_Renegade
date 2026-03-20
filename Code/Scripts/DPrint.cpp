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

/****************************************************************************
*
* FILE
*     DPrint.cpp
*
* DESCRIPTION
*     Debug printing mechanism
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* VERSION INFO
*     $Author: Byon_g $
*     $Revision: 2 $
*     $Modtime: 2/13/01 11:02a $
*     $Archive: /Commando/Code/Scripts/DPrint.cpp $
*
****************************************************************************/

#ifdef _DEBUG

#include "DPrint.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#include <array>
#include <cstdarg>
#include <cstring>
#include <string>

#include "scriptcommands.h"
extern ScriptCommands* Commands;

namespace
{
	constexpr const char * LOGFILE_NAME = "ScriptLog.txt";

	std::string Build_Log_Filename()
	{
		char * base_path = SDL_GetBasePath();
		std::string filename = (base_path != nullptr) ? base_path : "";
		if (base_path != nullptr) {
			SDL_free(base_path);
		}

		filename += LOGFILE_NAME;
		return filename;
	}

	std::string Normalize_Log_Text(const char * text)
	{
		std::string normalized;
		if (text == nullptr) {
			return normalized;
		}

		const std::size_t length = std::strlen(text);
		normalized.reserve(length + 8);

		for (std::size_t index = 0; index < length; ++index) {
			const char current = text[index];
			if (current == '\n' && (index == 0 || text[index - 1] != '\r')) {
				normalized.push_back('\r');
			}
			normalized.push_back(current);
		}

		return normalized;
	}

	void Write_Log_File(const char * text)
	{
		const std::string filename = Build_Log_Filename();
		SDL_IOStream * file = SDL_IOFromFile(filename.c_str(), "ab");
		if (file == nullptr) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to open script log '%s': %s", filename.c_str(), SDL_GetError());
			return;
		}

		const std::string normalized = Normalize_Log_Text(text);
		const std::size_t written = SDL_WriteIO(file, normalized.data(), normalized.size());
		if (written != normalized.size()) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Short write while appending to script log '%s'", filename.c_str());
		}

		SDL_CloseIO(file);
	}
}

/****************************************************************************
*
* NAME
*     DPrint(String, ArgList...)
*
* DESCRIPTION
*     Ouput debug print messages to the debugger and log file.
*
* INPUTS
*     String  - String to output.
*     ArgList - Argument list
*
* RESULT
*     NONE
*
****************************************************************************/

void DebugPrint(const char* string, ...)
{
	if (string == NULL) {
		return;
	}

	std::array<char, 1024> buffer{};
	va_list va;
	va_start(va, string);
	SDL_vsnprintf(buffer.data(), buffer.size(), string, va);
	va_end(va);

	if (Commands != NULL) {
		Commands->Debug_Message(buffer.data());
	} else {
		SDL_Log("%s", buffer.data());
	}

	Write_Log_File(buffer.data());
}

#endif // _DEBUG

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
 ***                            Confidential - Westwood Studios                              *** 
 *********************************************************************************************** 
 *                                                                                             * 
 *                 Project Name : Commando                                                     * 
 *                                                                                             * 
 *                     $Archive:: /Commando/Code/wwutil/stackdump.cpp                 $* 
 *                                                                                             * 
 *                      $Author:: Tom_s                                                       $* 
 *                                                                                             * 
 *                     $Modtime:: 9/29/01 1:13p                                               $* 
 *                                                                                             * 
 *                    $Revision:: 1                                                           $* 
 *                                                                                             * 
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "stackdump.h"

#include <SDL3/SDL_log.h>

#if defined(__has_include)
#  if __has_include(<stacktrace>)
#    include <stacktrace>
#    if defined(__cpp_lib_stacktrace) && (__cpp_lib_stacktrace >= 202011L)
#      define RENEGADE_HAS_STD_STACKTRACE 1
#    endif
#  endif
#endif

#ifndef RENEGADE_HAS_STD_STACKTRACE
#  define RENEGADE_HAS_STD_STACKTRACE 0
#endif

void cStackDump::Print_Call_Stack(void)
{
	SDL_Log("cStackDump::Print_Call_Stack:");

#if RENEGADE_HAS_STD_STACKTRACE
	const auto trace = std::stacktrace::current();
	if (trace.empty()) {
		SDL_Log("  stack trace is empty.");
		return;
	}

	for (std::size_t index = 0; index < trace.size(); ++index) {
		SDL_Log("  [%zu] %s", index, trace[index].description().c_str());
	}
#else
	SDL_Log("  std::stacktrace is not available with this toolchain; no portable call stack could be captured.");
#endif
}



















	/*
	//
	//	Determine the path to the executable
	//
	char path[MAX_PATH] = "";
	uint32_t gmf = ::GetModuleFileName(NULL, path, sizeof(path));
	
	if (gmf != 0)
	{
		//
		//	Strip off the filename
		//
		char * filename = ::strrchr(path, '\\');
		if (filename != NULL) 
		{
			filename[0] = 0;
		}

		::SetCurrentDirectory(path);
	}

	*/

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

#ifndef PROCESS_HEADER
#define PROCESS_HEADER

#include <cstdint>

#include <windows.h>
#include "wstypes.h"
#include "wdebug.h"
#include "configfile.h"

class Process
{
 public:
           Process();

  char     directory[256];
  char     command[256];
  char     args[256];
  HANDLE   hProcess;
	uint32_t dwProcessID;
  HANDLE   hThread;
	uint32_t dwThreadID;
};

int8_t Read_Process_Info(ConfigFile &config,OUT Process &info, IN char *key = NULL);
int8_t Create_Process(Process &process);
int8_t Wait_Process(Process &process, uint32_t *exit_code=NULL);


#endif
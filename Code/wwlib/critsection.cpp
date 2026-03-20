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
 *                 Project Name : critsection.cpp                                              *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwlib/critsection.cpp                        $*
 *                                                                                             *
 *              Original Author:: Hector Yee                                                   *
 *                                                                                             *
 *                      $Author:: Hector_y                                                    $*
 *                                                                                             *
 *                     $Modtime:: 3/14/01 4:04p                                               $*
 *                                                                                             *
 *                    $Revision:: 1                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "critsection.h"

#include <SDL3/SDL_mutex.h>

CriticalSectionClass::CriticalSectionClass():
Bar(SDL_CreateMutex()),
Owner(0),
Recursion(0),
inside(false)
{
	WWASSERT(Bar != NULL);
}

CriticalSectionClass::~CriticalSectionClass()
{
	if (Bar != NULL) {
		SDL_DestroyMutex(Bar);
		Bar = NULL;
	}
}

void CriticalSectionClass::Enter()
{
	SDL_ThreadID current_thread = SDL_GetCurrentThreadID();
	if (Owner == current_thread && Recursion > 0) {
		Recursion++;
		inside = true;
		return;
	}

	SDL_LockMutex(Bar);
	Owner = current_thread;
	Recursion = 1;
	inside = true;
}

void CriticalSectionClass::Exit()
{
	WWASSERT(inside == true);
	WWASSERT(Owner == SDL_GetCurrentThreadID());
	WWASSERT(Recursion > 0);

	Recursion--;
	if (Recursion == 0) {
		inside = false;
		Owner = 0;
		SDL_UnlockMutex(Bar);
	}
}

CriticalSectionClass::LockClass::LockClass(CriticalSectionClass &c):
crit(c)
{
	crit.Enter();
}

CriticalSectionClass::LockClass::~LockClass()
{
	crit.Exit();
}
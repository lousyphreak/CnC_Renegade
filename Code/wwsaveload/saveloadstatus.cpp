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

#include "saveloadstatus.h"

#include <SDL3/SDL_mutex.h>

#define MAX_STATUS_TEXT_ID 2

namespace
{
	class SaveLoadStatusMutexClass
	{
	public:
		SaveLoadStatusMutexClass()
			: Mutex(SDL_CreateMutex())
		{
		}

		~SaveLoadStatusMutexClass()
		{
			if (Mutex != NULL) {
				SDL_DestroyMutex(Mutex);
			}
		}

		void Lock()
		{
			if (Mutex != NULL) {
				SDL_LockMutex(Mutex);
			}
		}

		void Unlock()
		{
			if (Mutex != NULL) {
				SDL_UnlockMutex(Mutex);
			}
		}

	private:
		SDL_Mutex * Mutex;
	};

	class SaveLoadStatusLockClass
	{
	public:
		explicit SaveLoadStatusLockClass(SaveLoadStatusMutexClass & mutex)
			: Mutex(mutex)
		{
			Mutex.Lock();
		}

		~SaveLoadStatusLockClass()
		{
			Mutex.Unlock();
		}

	private:
		SaveLoadStatusMutexClass & Mutex;
	};
}

static SaveLoadStatusMutexClass text_mutex;
static StringClass status_text[MAX_STATUS_TEXT_ID];

void SaveLoadStatus::Set_Status_Text(const char* text,int id)
{
	SaveLoadStatusLockClass m(text_mutex);
	WWASSERT(id<MAX_STATUS_TEXT_ID);
	status_text[id]=text;
	if (id==0) status_text[1]="";
}

void SaveLoadStatus::Get_Status_Text(StringClass& text, int id)
{
	SaveLoadStatusLockClass m(text_mutex);
	WWASSERT(id<MAX_STATUS_TEXT_ID);
	text=status_text[id];
}

static	int	status_count;
void	SaveLoadStatus::Reset_Status_Count( void )
{
	status_count = 0;
}

void	SaveLoadStatus::Inc_Status_Count( void )
{
	status_count++;
}

int	SaveLoadStatus::Get_Status_Count( void )
{
	return status_count;
}


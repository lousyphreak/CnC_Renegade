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

//
// Filename:     miscutil.cpp
// Project:      wwutil
// Author:       Tom Spencer-Smith
// Date:         June 1998
// Description:
//
//-----------------------------------------------------------------------------
#include "miscutil.h" // I WANNA BE FIRST!

#include <cstring>

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_time.h>

//---------------------------------------------------------------------------
const char * cMiscUtil::Get_Text_Time(void)
{
	static char time_str[64];
	static constexpr const char * kWeekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static constexpr const char * kMonths[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	SDL_Time ticks = 0;
	SDL_DateTime date_time = {};
	if (!SDL_GetCurrentTime(&ticks) || !SDL_TimeToDateTime(ticks, &date_time, true)) {
		SDL_snprintf(time_str, sizeof(time_str), "Unknown time");
		return time_str;
	}

	const char * weekday = "???";
	if (date_time.day_of_week >= 0 && date_time.day_of_week < static_cast<int>(SDL_arraysize(kWeekdays))) {
		weekday = kWeekdays[date_time.day_of_week];
	}

	const char * month = "???";
	if (date_time.month >= 1 && date_time.month <= static_cast<int>(SDL_arraysize(kMonths))) {
		month = kMonths[date_time.month - 1];
	}

	SDL_snprintf(
		time_str,
		sizeof(time_str),
		"%s %s %2d %02d:%02d:%02d %04d",
		weekday,
		month,
		date_time.day,
		date_time.hour,
		date_time.minute,
		date_time.second,
		date_time.year
	);

	return time_str;
}

//---------------------------------------------------------------------------
void cMiscUtil::Seconds_To_Hms(float seconds, int & h, int & m, int & s)
{
	SDL_assert(seconds >= 0);

	h = static_cast<int>(seconds / 3600);
	seconds -= h * 3600;
	m = static_cast<int>(seconds / 60);
	seconds -= m * 60;
	s = static_cast<int>(seconds);

	SDL_assert(h >= 0);
	SDL_assert(m >= 0 && m < 60);
	SDL_assert(s >= 0 && s < 60);
}

//-----------------------------------------------------------------------------
bool cMiscUtil::Is_String_Same(const char * str1, const char * str2)
{
	SDL_assert(str1 != nullptr);
	SDL_assert(str2 != nullptr);

	return SDL_strcasecmp(str1, str2) == 0;
}

//-----------------------------------------------------------------------------
bool cMiscUtil::Is_String_Different(const char * str1, const char * str2)
{
	SDL_assert(str1 != nullptr);
	SDL_assert(str2 != nullptr);

	return SDL_strcasecmp(str1, str2) != 0;
}

//-----------------------------------------------------------------------------
bool cMiscUtil::File_Exists(const char * filename)
{
	SDL_assert(filename != nullptr);

	SDL_PathInfo info = {};
	return SDL_GetPathInfo(filename, &info) && info.type != SDL_PATHTYPE_NONE;
}

//-----------------------------------------------------------------------------
bool cMiscUtil::Is_Alphabetic(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

//-----------------------------------------------------------------------------
bool cMiscUtil::Is_Numeric(char c)
{
	return (c >= '0' && c <= '9');
}

//-----------------------------------------------------------------------------
bool cMiscUtil::Is_Alphanumeric(char c)
{
	return Is_Alphabetic(c) || Is_Numeric(c);
}

//-----------------------------------------------------------------------------
bool cMiscUtil::Is_Whitespace(char c)
{
	return c == ' ' || c == '\t';
}

//-----------------------------------------------------------------------------
void cMiscUtil::Trim_Trailing_Whitespace(char * text)
{
	SDL_assert(text != nullptr);

	int length = static_cast<int>(std::strlen(text));
	while (length > 0 && Is_Whitespace(text[length - 1])) {
		text[--length] = 0;
	}
}

//-----------------------------------------------------------------------------
void cMiscUtil::Remove_File(const char * filename)
{
	SDL_assert(filename != nullptr);

	SDL_RemovePath(filename);
}

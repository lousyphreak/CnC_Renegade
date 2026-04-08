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
 *                     $Archive:: /Commando/Code/wwlib/always.h                               $*
 *                                                                                             *
 *                      $Author:: Steve_t                                                     $*
 *                                                                                             *
 *                     $Modtime:: 8/28/01 3:21p                                               $*
 *                                                                                             *
 *                    $Revision:: 13                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef ALWAYS_H
#define ALWAYS_H

#include <cstdint>
#include "osdep.h"

#define WWINLINE inline

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using sint8 = std::int8_t;
using sint16 = std::int16_t;
using sint32 = std::int32_t;
using sint64 = std::int64_t;

/*
** Define the MIN and MAX macros.
** NOTE: Joe used to #include <minmax.h> in the various compiler header files.  This
** header defines 'min' and 'max' macros which conflict with the surrender code so
** I'm relpacing all occurances of 'min' and 'max with 'MIN' and 'MAX'.  For code which
** is out of our domain (e.g. Max sdk) I'm declaring template functions for 'min' and 'max'
*/
#define NOMINMAX

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

template <class T> T min(T a,T b)
{
	if (a<b) {
		return a;
	} else {
		return b;
	}
}

template <class T> T max(T a,T b)
{
	if (a>b) {
		return a;
	} else {
		return b;
	}
}


#ifndef	NULL
	#define	NULL		0
#endif

/**********************************************************************
**	This macro serves as a general way to determine the number of elements
**	within an array.
*/
#ifndef ARRAY_SIZE
#define	ARRAY_SIZE(x)		int(sizeof(x)/sizeof(x[0]))
#endif

#ifndef size_of
#define size_of(typ,id) sizeof(((typ*)0)->id)
#endif


#endif

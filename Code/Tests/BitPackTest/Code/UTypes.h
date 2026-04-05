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
*     UTypes.h
*
* DESCRIPTION
*     Generic user type definitions
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* VERSION INFO
*     $Author: Denzil_l $
*     $Revision: 1 $
*     $Modtime: 12/02/99 4:31p $
*     $Archive: /Commando/Code/Tests/BitPackTest/Code/UTypes.h $
*
****************************************************************************/

#ifndef _UTYPES_H_
#define _UTYPES_H_

#include <cstdint>

#include <cstddef>

//! TriState
enum TriState : int32_t {OFF = false, ON = true, PENDING = -1};

//! Empty pointer
#ifndef NULL
#define NULL (0)
#endif

#endif // _UTYPES_H_

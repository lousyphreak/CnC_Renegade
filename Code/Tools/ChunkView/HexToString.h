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
 *                 Project Name : ChunkView                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Tools/ChunkView/HexToString.h                $*
 *                                                                                             *
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                     $Modtime:: 9/28/99 9:48a                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#ifndef HEXTOSTRING_H
#define HEXTOSTRING_H

#include <cstdint>


#include "stdafx.h"
#include "bittype.h"


class HexToStringClass
{
public:

	HexToStringClass(const uint8_t * data,uint32_t size);

	virtual bool		Is_Done(void) = 0;
	virtual CString	Get_Next_Line(void) = 0;

protected:

	const uint8_t *	Data;
	uint32_t			Size;

};


class HexToStringByteClass : public HexToStringClass
{
public:
	HexToStringByteClass(const uint8_t * data,uint32_t size);
	
	bool				Is_Done(void);
	CString			Get_Next_Line(void);

private:
	
	void				Reset(void);

	const uint8_t *	CurPos;
};

class HexToStringShortClass : public HexToStringClass
{
public:
	HexToStringShortClass(const uint8_t * data,uint32_t size);
	
	bool				Is_Done(void);
	CString			Get_Next_Line(void);

private:
	
	void				Reset(void);

	const uint16_t *	CurPos;
};

class HexToStringLongClass : public HexToStringClass
{
public:
	HexToStringLongClass(const uint8_t * data,uint32_t size);
	
	bool				Is_Done(void);
	CString			Get_Next_Line(void);

private:
	
	void				Reset(void);

	const uint32_t *	CurPos;
};


#endif
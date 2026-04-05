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
// Filename:     nicenum.cpp
// Author:       Tom Spencer-Smith
// Date:         Dec 1999
// Description:  NIC enumeration
//

#ifndef __NICENUM_H__
#define __NICENUM_H__

#include <cstdint>

#include "bittype.h"

//-----------------------------------------------------------------------------
class cNicEnum 
{
public:
	static void			Init(void);
	static uint16_t		Get_Num_Nics(void)			{return NumNics;}
	static uint32_t *		Get_Nics(void)					{return NicList;}
	static uint16_t		Get_Num_GameSpy_Nics(void)	{return NumGSNics;}
	static uint32_t *		Get_GameSpy_Nics(void)		{return GSNicList;}

	enum					{MAX_NICS = 10};

private:
	static uint32_t		Enumerate_Nics(uint32_t * addresses, uint32_t max_addresses);

	static uint32_t		NicList[MAX_NICS];
	static uint32_t		GSNicList[MAX_NICS];
	static uint16_t		NumNics;
	static uint16_t		NumGSNics;
};

//-----------------------------------------------------------------------------

#endif // __NICENUM_H__





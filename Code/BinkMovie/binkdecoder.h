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
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**	General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Bink.h"

namespace FFBink
{

HBINK Open_Bink_Handle(const char *filename, uint32_t flags);
void Close_Bink_Handle(HBINK bink);
uint32_t Wait_For_Frame(HBINK bink);
void Decode_Frame(HBINK bink);
void Advance_Frame(HBINK bink);
void Copy_Frame_To_Buffer(HBINK bink, void *dest, int32_t dest_pitch, uint32_t dest_height,
	uint32_t dest_x, uint32_t dest_y, uint32_t flags);
bool Get_Frame_Planes(HBINK bink, BINKFRAMEPLANES *planes);

}

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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BINK
{
	uint32_t Width;
	uint32_t Height;
	uint32_t FrameRate;
	uint32_t FrameRateDiv;
	uint32_t FrameNum;
	uint32_t Frames;
	void *Internal;
} BINK;

typedef BINK *HBINK;

typedef struct BINKFRAMEPLANES
{
	const uint8_t *YPlane;
	const uint8_t *UPlane;
	const uint8_t *VPlane;
	const uint8_t *APlane;
	int32_t YStride;
	int32_t UStride;
	int32_t VStride;
	int32_t AStride;
	uint32_t LumaWidth;
	uint32_t LumaHeight;
	uint32_t ChromaWidth;
	uint32_t ChromaHeight;
	uint32_t Flags;
} BINKFRAMEPLANES;

enum
{
	BINKSURFACE565 = 0x00000002,
	BINKCOPYNOSCALING = 0x00004000,
	BINKFRAMEPLANES_FULL_RANGE = 0x00010000,
	BINKFRAMEPLANES_HAS_ALPHA = 0x00020000,
};

void BinkSoundUseDirectSound(uintptr_t direct_sound);
HBINK BinkOpen(const char *filename, uint32_t flags);
void BinkClose(HBINK bink);
uint32_t BinkWait(HBINK bink);
void BinkDoFrame(HBINK bink);
void BinkNextFrame(HBINK bink);
void BinkCopyToBuffer(HBINK bink, void *dest, int32_t dest_pitch, uint32_t dest_height,
	uint32_t dest_x, uint32_t dest_y, uint32_t flags);
int32_t BinkGetFramePlanes(HBINK bink, BINKFRAMEPLANES *planes);

#ifdef __cplusplus
}
#endif

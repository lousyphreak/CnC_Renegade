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
// rc4.cpp
// RC4 encryption / decryption
//
#include "rc4.h"
#include <memory.h>

static uint8_t RC4_Temp_Byte;
#define RC4_SWAP_BYTE(a,b) RC4_Temp_Byte=a; a=b; b=RC4_Temp_Byte

//
// Don't rely on this to zero the key
//
RC4Class::RC4Class()
{
	memset(Key.State, 0, 256);
	Key.X=0;
	Key.Y=0;
}


//
// Setup the encryption key.  This must be called before you encrypt/decrypt!
//
void RC4Class::Prepare_Key(const uint8_t *key_data_ptr, int key_data_len)
{
	uint8_t index1;
	uint8_t index2;
	uint8_t *state;
	int counter;

	state = &Key.State[0];
	for (counter = 0; counter < 256; counter++)
		state[counter] = (uint8_t)counter;
	Key.X = 0;
	Key.Y = 0;
	index1 = 0;
	index2 = 0;
	for (counter = 0; counter < 256; counter++) {
		index2 = (uint8_t)(key_data_ptr[index1] + state[counter] + index2);
		RC4_SWAP_BYTE(state[counter], state[index2]);
		index1 = (uint8_t)((index1 + 1) % key_data_len);
	}
}


//
// RC4 in standard mode.
//
// This will XOR the buffer with the RC4 stream (like a one time pad).
//
void RC4Class::RC4(uint8_t *buffer_ptr, int buffer_len)
{
	uint8_t x;
	uint8_t y;
	uint8_t *state;
	int counter;

	x = Key.X;
	y = Key.Y;

	state = &Key.State[0];
	for (counter = 0; counter < buffer_len; counter++) {
		x++;
		y = (uint8_t)(y + state[x]);
		RC4_SWAP_BYTE(state[x], state[y]);
		buffer_ptr[counter] ^= state[(state[x] + state[y]) & 255];
	}
	Key.X = x;
	Key.Y = y;
}

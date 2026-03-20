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
// Filename:     bitpacker.cpp
// Project:      wwbitpack.lib
// Author:       Tom Spencer-Smith
// Date:         June 1998
// Description:  Minimal bit encoding
//

#include "BitPacker.h"

#include <cstring>

#include "wwbitpack_platform.h"

//-----------------------------------------------------------------------------
cBitPacker::cBitPacker() :
	BitWritePosition(0),
	BitReadPosition(0)
{
	std::memset(Buffer, 0, sizeof(Buffer));
}

//-----------------------------------------------------------------------------
cBitPacker::~cBitPacker()
{
}

//-----------------------------------------------------------------------------
void cBitPacker::Reset()
{
	std::memset(Buffer, 0, sizeof(Buffer));
	BitWritePosition = 0;
	BitReadPosition = 0;
}

//-----------------------------------------------------------------------------
cBitPacker& cBitPacker::operator=(const cBitPacker& rhs)
{
	std::memcpy(Buffer, rhs.Buffer, sizeof(Buffer));
	BitReadPosition = rhs.BitReadPosition;
	BitWritePosition = rhs.BitWritePosition;

	return *this;
}

//-----------------------------------------------------------------------------
void cBitPacker::Add_Bits(std::uint32_t value, std::uint32_t num_bits)
{
	WWBITPACK_ASSERT(num_bits > 0 && num_bits <= MAX_BITS);
	WWBITPACK_ASSERT(BitWritePosition + num_bits <= MAX_BUFFER_SIZE * wwbitpack::kBitsPerByte);

	for (std::uint32_t bit_index = 0; bit_index < num_bits; ++bit_index) {
		const std::uint32_t destination_bit = BitWritePosition + bit_index;
		const std::uint32_t byte_index = destination_bit / wwbitpack::kBitsPerByte;
		const std::uint32_t bit_offset = 7u - (destination_bit % wwbitpack::kBitsPerByte);
		const std::uint8_t mask = static_cast<std::uint8_t>(1u << bit_offset);
		const std::uint32_t source_mask = 1u << (num_bits - bit_index - 1u);

		if ((value & source_mask) != 0u) {
			Buffer[byte_index] |= mask;
		} else {
			Buffer[byte_index] &= static_cast<std::uint8_t>(~mask);
		}
	}

	BitWritePosition += num_bits;
	Clear_Unused_Tail_Bits();
}

//-----------------------------------------------------------------------------
void cBitPacker::Get_Bits(std::uint32_t & value, std::uint32_t num_bits)
{
	WWBITPACK_ASSERT(num_bits > 0 && num_bits <= MAX_BITS);
	WWBITPACK_ASSERT(BitReadPosition + num_bits <= MAX_BUFFER_SIZE * wwbitpack::kBitsPerByte);
	WWBITPACK_ASSERT(BitReadPosition + num_bits <= BitWritePosition);

	value = 0;
	for (std::uint32_t bit_index = 0; bit_index < num_bits; ++bit_index) {
		const std::uint32_t source_bit = BitReadPosition + bit_index;
		const std::uint32_t byte_index = source_bit / wwbitpack::kBitsPerByte;
		const std::uint32_t bit_offset = 7u - (source_bit % wwbitpack::kBitsPerByte);
		const std::uint32_t bit = (Buffer[byte_index] >> bit_offset) & 0x1u;

		value = (value << 1u) | bit;
	}

	BitReadPosition += num_bits;
}

//-----------------------------------------------------------------------------
void cBitPacker::Set_Bit_Write_Position(unsigned int position)
{
	WWBITPACK_ASSERT(position <= MAX_BUFFER_SIZE * wwbitpack::kBitsPerByte);
	BitWritePosition = position;
	if (BitReadPosition > BitWritePosition) {
		BitReadPosition = BitWritePosition;
	}
	Clear_Unused_Tail_Bits();
}

//-----------------------------------------------------------------------------
unsigned int cBitPacker::Get_Compressed_Size_Bytes() const
{
	return (BitWritePosition + wwbitpack::kBitsPerByte - 1u) / wwbitpack::kBitsPerByte;
}

//-----------------------------------------------------------------------------
void cBitPacker::Clear_Unused_Tail_Bits()
{
	const std::uint32_t used_bits_in_last_byte = BitWritePosition % wwbitpack::kBitsPerByte;
	if (used_bits_in_last_byte == 0u) {
		return;
	}

	const std::uint32_t byte_index = BitWritePosition / wwbitpack::kBitsPerByte;
	const std::uint8_t used_mask = static_cast<std::uint8_t>(0xFFu << (wwbitpack::kBitsPerByte - used_bits_in_last_byte));
	Buffer[byte_index] &= used_mask;
}



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
// Filename:     bitpacker.h
// Project:      wwbitpack.lib
// Author:       Tom Spencer-Smith
// Date:         June 1998
// Description:  Minimal bit encoding
//

#ifndef BITPACKER_H
#define BITPACKER_H

#include <cstdint>

static constexpr std::uint32_t MAX_BITS = 32;

// 1400 is too big. Minimum MTU allowable on the internet is 576. IP Header is 20 bytes. UDP header is 8 bytes
// So our max packet size is 576 - 28 = 548
//static const int MAX_BUFFER_SIZE = 1400;
static constexpr std::uint32_t MAX_BUFFER_SIZE = 548;

class cBitPacker
{
	public:
		cBitPacker();
		virtual ~cBitPacker();

		char * Get_Data() const {return reinterpret_cast<char *>(const_cast<std::uint8_t *>(Buffer));}
		unsigned int Get_Buffer_Size() const {return MAX_BUFFER_SIZE;}
		void Flush() {BitReadPosition = BitWritePosition;}
		bool Is_Flushed() const {return (BitReadPosition == BitWritePosition);}
		void Reset();

		void Add_Bits(std::uint32_t value, std::uint32_t num_bits);
		void Get_Bits(std::uint32_t & value, std::uint32_t num_bits);

		void Set_Bit_Write_Position(unsigned int position);
		unsigned int Get_Bit_Write_Position() const {return BitWritePosition;}
		unsigned int Get_Compressed_Size_Bytes() const;

	protected:
		cBitPacker& operator=(const cBitPacker& rhs);

	private:
		cBitPacker(const cBitPacker& source); // Disallow copy constructor
		void Clear_Unused_Tail_Bits();

		std::uint8_t Buffer[MAX_BUFFER_SIZE];
		std::uint32_t BitWritePosition;
		std::uint32_t BitReadPosition;
};

#endif // BITPACKER_H
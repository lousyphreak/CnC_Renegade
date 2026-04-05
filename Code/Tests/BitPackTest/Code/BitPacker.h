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
*     $Archive: /Commando/Code/Tests/BitPackTest/Code/BitPacker.h $
*
* DESCRIPTION
*     Provide variable length bit packing.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* VERSION INFO
*     $Author: Denzil_l $
*     $Revision: 2 $
*     $Modtime: 5/31/00 9:21a $
*
****************************************************************************/

#ifndef _BITPACKER_H_
#define _BITPACKER_H_

#include <cstdint>

class BitPacker
	{
	public:
		BitPacker();
		BitPacker(void* buffer, uint32_t bufferSize);
		virtual ~BitPacker();

		// Set the buffer to use for read / write of bit packed data.
		void SetBuffer(void* buffer, uint32_t bufferSize);
		
		// Flush remainder bits to the stream.
		// (This must be called when finished writting)
		void Flush(void);

		// Reset the bitpacked stream.
		void Reset(void);

		// Retrieve the length of the packed stream (in bytes).
		uint32_t GetPackedSize(void);

		// Retrieve a bit from the stream,
		int GetBit(void);

		// Write a bit to the stream
		bool PutBit(int value);

		// Retrieve a series of bits from the stream (Max = 32)
		int GetBits(uint32_t& outBits, uint32_t numBits);

		// Write a series of bits to the stream (Max = 32)
		int PutBits(uint32_t bits, uint32_t numBits);

	private:
		uint8_t* mBuffer;
		uint32_t mBufferSize;
		uint32_t mBytePosition;
		uint32_t mBitMask;
		uint8_t mStore;
	};

#endif // _BITPACKER_H_

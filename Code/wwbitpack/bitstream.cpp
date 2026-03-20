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
 *                 Project Name : wwbitpack                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwbitpack/bitstream.cpp                      $*
 *                                                                                             *
 *              Original Author:: Tom Spencer-Smith                                            *
 *                                                                                             *
 *                      $Author:: Bhayes                                                      $*
 *                                                                                             *
 *                     $Modtime:: 2/18/02 10:49p                                              $*
 *                                                                                             *
 *                    $Revision:: 4                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "bitstream.h"

#include <cstring>
#include <cwchar>
#include <limits>


//-----------------------------------------------------------------------------
BitStreamClass::BitStreamClass() :
	cBitPacker(),
	UncompressedSizeBytes(0)
{
}

//-----------------------------------------------------------------------------
BitStreamClass& BitStreamClass::operator=(const BitStreamClass& rhs)
{
	//
	// Call operator for base class
	//
	cBitPacker::operator= (rhs);

	UncompressedSizeBytes = rhs.UncompressedSizeBytes;

   return * this;
}

//-----------------------------------------------------------------------------
void BitStreamClass::Add(bool value)
{
	if (cEncoderList::Is_Compression_Enabled()) {
		Add_Bits(value, 1);
	} else {
		Add_Bits(wwbitpack::Encode_Uncompressed(value), wwbitpack::Bit_Depth<bool>());
	}

	UncompressedSizeBytes += static_cast<unsigned int>(sizeof(bool));
}

//-----------------------------------------------------------------------------
bool BitStreamClass::Get(bool & value)
{
	std::uint32_t u_value = 0;
	if (cEncoderList::Is_Compression_Enabled()) {
		Get_Bits(u_value, 1);
	} else {
		Get_Bits(u_value, wwbitpack::Bit_Depth<bool>());
	}

	value = (u_value == 1);
	return value;
}

//-----------------------------------------------------------------------------
void BitStreamClass::Add_Raw_Data(const char * data, std::uint16_t data_size)
{
	WWBITPACK_ASSERT(data != NULL);

	for (std::uint16_t i = 0; i < data_size; ++i) {
		Add(data[i]);
	}
}

//-----------------------------------------------------------------------------
void BitStreamClass::Get_Raw_Data(char * buffer, std::uint16_t buffer_size, std::uint16_t data_size)
{
	WWBITPACK_ASSERT(buffer != NULL);
   WWBITPACK_ASSERT(buffer_size >= data_size);

	for (std::uint16_t i = 0; i < data_size; ++i) {
		Get(buffer[i]);
	}
}

//-----------------------------------------------------------------------------
void BitStreamClass::Add_Terminated_String(const char * string, bool permit_empty)
{
	WWBITPACK_ASSERT(string != NULL);

	//
	// The terminating null is not transmitted.
	//
	const std::size_t raw_len = std::strlen(string);
	WWBITPACK_ASSERT(raw_len <= std::numeric_limits<std::uint16_t>::max());
	const std::uint16_t len = static_cast<std::uint16_t>(raw_len);
	if (!permit_empty) {
		WWBITPACK_ASSERT(len > 0);
	}

	Add(len);
	for (std::uint16_t i = 0; i < len; ++i) {
		Add(string[i]);
	}
}

//-----------------------------------------------------------------------------
void BitStreamClass::Get_Terminated_String(char * buffer, std::uint16_t buffer_size, bool permit_empty)
{
	WWBITPACK_ASSERT(buffer != NULL);
	WWBITPACK_ASSERT(buffer_size > 0);

	std::uint16_t len = 0;
	Get(len);
	WWBITPACK_ASSERT(len < buffer_size);
	if (!permit_empty) {
		WWBITPACK_ASSERT(len > 0);
	}

	char temp = '?';
	std::uint16_t i = 0;
	for (i = 0; i < len; i++) {
		Get(temp);
		if (i < buffer_size - 1) {
			buffer[i] = temp;
		}
	}

	// Null-terminate it.
	if (i < buffer_size) {
		buffer[i] = 0;
	} else {
		buffer[buffer_size - 1] = 0;
	}
}


//-----------------------------------------------------------------------------
void BitStreamClass::Add_Wide_Terminated_String(const wchar_t * string, bool permit_empty)
{
	WWBITPACK_ASSERT(string != NULL);

	//
	// The terminating null is not transmitted.
	//
	const std::size_t raw_len = std::wcslen(string);
	WWBITPACK_ASSERT(raw_len <= std::numeric_limits<std::uint16_t>::max());
	const std::uint16_t len = static_cast<std::uint16_t>(raw_len);
	if (!permit_empty) {
		WWBITPACK_ASSERT(len > 0 && "Empty string not permitted");
	}

	Add(len);
	for (std::uint16_t i = 0; i < len; ++i) {
		Add(string[i]);
	}
}

//-----------------------------------------------------------------------------
void BitStreamClass::Get_Wide_Terminated_String(wchar_t * buffer, std::uint16_t buffer_len, bool permit_empty)
{
	WWBITPACK_ASSERT(buffer != NULL);
	WWBITPACK_ASSERT(buffer_len > 0);

	std::uint16_t len = 0;
	Get(len);
	WWBITPACK_ASSERT(len < buffer_len && "String length exceeds provided buffer");
	if (!permit_empty) {
		WWBITPACK_ASSERT(len > 0 && "Empty string not permitted");
	}

	wchar_t temp = L'?';
	std::uint16_t i = 0;
	for (i = 0; i < len; i++) {
		Get(temp);
		if (i < buffer_len - 1) {
			buffer[i] = temp;
		}
	}

	if (i < buffer_len - 1) {
		buffer[i] = 0; // Null-terminate it.
	} else {
		buffer[buffer_len-1] = 0;
	}
}


//-----------------------------------------------------------------------------
unsigned int BitStreamClass::Get_Compressed_Size_Bytes() const
{
	return cBitPacker::Get_Compressed_Size_Bytes();
}

//-----------------------------------------------------------------------------
unsigned int BitStreamClass::Get_Compression_Pc() const
{
	const unsigned int c_size = Get_Compressed_Size_Bytes();
	const unsigned int u_size = Get_Uncompressed_Size_Bytes();

	if (cEncoderList::Is_Compression_Enabled()) {
		WWBITPACK_ASSERT(c_size <= u_size);
	} else {
		WWBITPACK_ASSERT(c_size == u_size);
	}

	WWBITPACK_ASSERT(u_size > 0);

	const unsigned int compression_pc = static_cast<unsigned int>(
		wwbitpack::Round_To_Nearest(100.0 * c_size / static_cast<double>(u_size)));
	WWBITPACK_ASSERT(compression_pc <= 100);

	return compression_pc;
}

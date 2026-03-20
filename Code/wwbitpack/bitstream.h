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
 *                     $Archive:: /Commando/Code/wwbitpack/bitstream.h                        $*
 *                                                                                             *
 *              Original Author:: Tom Spencer-Smith                                            *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 6/13/01 9:05a                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <cmath>
#include <cstdint>

#include "BitPacker.h"
#include "encoderlist.h"
#include "wwbitpack_platform.h"


/**
** BitStreamClass
**
** Author:       Tom Spencer-Smith
** Date:         June 1998
** Description:  A class for minimal bit encoding.
**					  Notes:
**					  - Uncompressed data may be included in the bitstream.
**					  - Compression may be disabled entirely if desired.
**					  - Bools are compressed to 1 bit without requiring a precision
**					    setup.
**					  - Strings and raw data are uncompressed.
**
** (gth, 08/31/2000) - renamed this class to BitStreamClass (from cTypeEncoder) and
** cleaned it up to become the interface that all game and library code uses to
** package up their state variables for network transmission, converted to westwood 
** naming convention since it is going to propogate to a lot of other code.
*/

class BitStreamClass : public cBitPacker
{
	public:

		BitStreamClass();
      BitStreamClass& operator=(const BitStreamClass& rhs);

		unsigned int Get_Uncompressed_Size_Bytes() const {return UncompressedSizeBytes;}
		unsigned int Get_Compressed_Size_Bytes() const;
		unsigned int Get_Compression_Pc() const;

      //
      // For data which may include NULL's.
		// Data will not be compressed.
      //
      void Add_Raw_Data(const char * data, std::uint16_t data_size);
		void Get_Raw_Data(char * buffer, std::uint16_t buffer_size, std::uint16_t data_size);

      //
      // For data terminated with NULL.
		// Data will not be compressed.
		// You may permit or disallow empty strings to be passed.
      //
      void Add_Terminated_String(const char * string, bool permit_empty = false);
		void Get_Terminated_String(char * buffer, std::uint16_t buffer_size, bool permit_empty = false);

      //
      // For data terminated with NULL.
		// Data will not be compressed.
		// You may permit or disallow empty strings to be passed.
      //
      void Add_Wide_Terminated_String(const wchar_t * string, bool permit_empty = false);
		void Get_Wide_Terminated_String(wchar_t * buffer, std::uint16_t buffer_len, bool permit_empty = false);

		//
		// Bool is special-cased because we know that we can always 
		// represent it as 1 bit.
		//
		void Add(bool value);
		bool Get(bool & value);

		// 
		// For all other data types that we want to support, call into our internal 
		// template function.  
		//
		enum {NO_ENCODER = -1};

		void		Add(std::uint8_t val,int type = NO_ENCODER)					{ Internal_Add(val,type); }
		void		Add(std::uint16_t val,int type = NO_ENCODER)				{ Internal_Add(val,type); }
		void		Add(unsigned int val,int type = NO_ENCODER)				{ Internal_Add(val,type); }
		void		Add(unsigned long val,int type = NO_ENCODER)				{ Internal_Add_Wire32(static_cast<std::uint32_t>(val),type); }
		void		Add(char val,int type = NO_ENCODER)							{ Internal_Add(val,type); }
		void		Add(wchar_t val,int type = NO_ENCODER)					{ Internal_Add(val,type); }
		void		Add(int val,int type = NO_ENCODER)							{ Internal_Add(val,type); }
		void		Add(float val,int type = NO_ENCODER)						{ Internal_Add(val,type); }

		std::uint8_t		Get(std::uint8_t & set_val,int type = NO_ENCODER)		{ return Internal_Get(set_val,type); }
		std::uint16_t	Get(std::uint16_t & set_val,int type = NO_ENCODER)	{ return Internal_Get(set_val,type); }
		unsigned long	Get(unsigned long & set_val,int type = NO_ENCODER)	{ return Internal_Get_Wire32(set_val,type); }
		unsigned int	Get(unsigned int & set_val,int type = NO_ENCODER)	{ return Internal_Get(set_val,type); }
		char		Get(char & set_val,int type = NO_ENCODER)					{ return Internal_Get(set_val,type); }
		wchar_t		Get(wchar_t & set_val,int type = NO_ENCODER)			{ return Internal_Get(set_val,type); }
		int		Get(int & set_val,int type = NO_ENCODER)					{ return Internal_Get(set_val,type); }
		float		Get(float & set_val,int type = NO_ENCODER)				{ return Internal_Get(set_val,type); }

	private:
		
		//
		// Add/Get for remaining atomic data types.
		template<class T> void Internal_Add(T value, int type = NO_ENCODER) {
			static_assert(sizeof(T) <= sizeof(std::uint32_t), "wwbitpack atomics must fit in 32 bits");

			if (cEncoderList::Is_Compression_Enabled() && type != NO_ENCODER) {
				WWBITPACK_ASSERT(type >= 0 && type < MAX_ENCODERTYPES);

				cEncoderTypeEntry & entry = cEncoderList::Get_Encoder_Type_Entry(type);

				//
				// If the following assert hits then the value of the type 
				// parameter is unknown.
				//
				WWBITPACK_ASSERT(entry.Is_Valid());

				std::uint32_t scaled_value = 0;
				bool is_in_range = entry.Scale(value, scaled_value);
				(void)is_in_range;

				Add_Bits(scaled_value, entry.Get_Bit_Precision());

			} else {
				Add_Bits(wwbitpack::Encode_Uncompressed(value), wwbitpack::Bit_Depth<T>());
			}

			UncompressedSizeBytes += static_cast<unsigned int>(sizeof(T));
		}
		
      template<class T> T Internal_Get(T & value, int type = NO_ENCODER) {
			static_assert(sizeof(T) <= sizeof(std::uint32_t), "wwbitpack atomics must fit in 32 bits");

			if (cEncoderList::Is_Compression_Enabled() && type != NO_ENCODER) {
				WWBITPACK_ASSERT(type >= 0 && type < MAX_ENCODERTYPES);

				cEncoderTypeEntry & entry = cEncoderList::Get_Encoder_Type_Entry(type);

				//
				// If the following assert hits then the value of the type 
				// parameter is unknown.
				//
				WWBITPACK_ASSERT(entry.Is_Valid());

				std::uint32_t u_value = 0;
				Get_Bits(u_value, entry.Get_Bit_Precision());

				double f_value = entry.Unscale(u_value);

				if (std::fabs(f_value - static_cast<double>(static_cast<T>(f_value))) < wwbitpack::kEpsilon) {
					//
					// N.B. More error may be introduced here
					//
					value = static_cast<T>(f_value);
				} else {
					value = static_cast<T>(wwbitpack::Round_To_Nearest(f_value));
				}

				WWBITPACK_ASSERT(entry.Is_Value_In_Range(value));

			} else {
				std::uint32_t u_value = 0;
				Get_Bits(u_value, wwbitpack::Bit_Depth<T>());

				value = wwbitpack::Decode_Uncompressed<T>(u_value);
			}
			return value;
		}

		void Internal_Add_Wire32(std::uint32_t value, int type = NO_ENCODER) { Internal_Add(value, type); }
		unsigned long Internal_Get_Wire32(unsigned long & value, int type = NO_ENCODER) {
			std::uint32_t raw_value = 0;
			Internal_Get(raw_value, type);
			value = static_cast<unsigned long>(raw_value);
			return value;
		}

		unsigned int UncompressedSizeBytes; // for statistics only
};

#endif // TYPEENCODER_H

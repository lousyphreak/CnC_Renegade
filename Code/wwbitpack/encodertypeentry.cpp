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
// Filename:     encodertypeentry.cpp
// Project:      wwbitpack.lib
// Author:       Tom Spencer-Smith
// Date:         June 2000
// Description:  
//
//-----------------------------------------------------------------------------
#include "encodertypeentry.h" // I WANNA BE FIRST!

#include <cmath>
#include <limits>

#include "wwbitpack_platform.h"

static constexpr std::uint32_t MAX_BITS = 32;

//-----------------------------------------------------------------------------
cEncoderTypeEntry::cEncoderTypeEntry()
{
	Invalidate();
}

//-----------------------------------------------------------------------------
bool cEncoderTypeEntry::Is_Valid() const 
{
	return 
		((Max - Min > -wwbitpack::kEpsilon) && 
		 (Resolution > -wwbitpack::kEpsilon) && 
		 (BitPrecision > 0));
}

//-----------------------------------------------------------------------------
void cEncoderTypeEntry::Invalidate()
{
	Min = 1;
	Max = -1;
	Resolution = -1;
	BitPrecision = 0;
}

//-----------------------------------------------------------------------------
bool cEncoderTypeEntry::Is_Value_In_Range(double value) const
{
	return (value >= Min - wwbitpack::kEpsilon && value <= Max + wwbitpack::kEpsilon);
}

//-----------------------------------------------------------------------------
void cEncoderTypeEntry::Init(double min, double max, double resolution)
{
	WWBITPACK_ASSERT(!Is_Valid());

	WWBITPACK_ASSERT(max - min > -wwbitpack::kEpsilon);
	WWBITPACK_ASSERT(resolution > wwbitpack::kEpsilon);

	Min = min;
	Max = max;

	Calc_Bit_Precision(resolution);

	WWBITPACK_ASSERT(Is_Valid());
}

//-----------------------------------------------------------------------------
void cEncoderTypeEntry::Init(int num_bits)
{
	WWBITPACK_ASSERT(!Is_Valid());

	WWBITPACK_ASSERT(num_bits > 0 && num_bits <= 32);

	Min = 0;
	BitPrecision = static_cast<std::uint32_t>(num_bits);
	Resolution = 1;

	std::uint64_t max = 0;
	for (int i = 0; i < num_bits; i++) {
		max += (1ull << i);
	}

	Max = static_cast<double>(max);

	WWBITPACK_ASSERT(Is_Valid());	
}

//-----------------------------------------------------------------------------
bool cEncoderTypeEntry::Scale(double value, std::uint32_t & scaled_value)
{
	WWBITPACK_ASSERT(Is_Valid());

	bool is_in_range = Is_Value_In_Range(value);

	if (!is_in_range) {
		value = Clamp(value);
	}

	scaled_value = wwbitpack::Round_To_UInt32((value - Min) / Resolution);

	return is_in_range;
}

//-----------------------------------------------------------------------------
double cEncoderTypeEntry::Unscale(std::uint32_t u_value)
{
	WWBITPACK_ASSERT(Is_Valid());

	double value = Min + u_value * Resolution;

	WWBITPACK_ASSERT(Is_Value_In_Range(value));

	return value;
}

//-----------------------------------------------------------------------------
double cEncoderTypeEntry::Clamp(double value)
{
	WWBITPACK_ASSERT(Is_Valid());

	double retval = value;
	
	if (retval < Min) {
		retval = Min;
	} else if (retval > Max) {
		retval = Max;
	}

	return retval;
}

//-----------------------------------------------------------------------------
void cEncoderTypeEntry::Calc_Bit_Precision(double resolution)
{
	// 
	// Calculate the minimum number of bits required to encode this type with
	// the specified resolution.
	//

	WWBITPACK_ASSERT(Max - Min > -wwbitpack::kEpsilon);
	WWBITPACK_ASSERT(resolution > wwbitpack::kEpsilon);

	const double f_units = std::ceil((Max - Min) / resolution - wwbitpack::kEpsilon) + 1.0;
	WWBITPACK_ASSERT(f_units <= static_cast<double>(std::numeric_limits<std::uint32_t>::max()) + wwbitpack::kEpsilon);
	const std::uint64_t units = static_cast<std::uint64_t>(f_units);

	BitPrecision = 0;
	std::uint64_t max_units = 0;
	while (max_units < units) {
		max_units += (1ull << BitPrecision);
		BitPrecision++;
		if (BitPrecision == 1) {
			max_units++;
		}
	}	

	WWBITPACK_ASSERT(BitPrecision > 0 && BitPrecision <= MAX_BITS);
	WWBITPACK_ASSERT(max_units > 0);

	Resolution = (Max - Min) / (double) (max_units - 1);

	/*TSS2001
	if (Resolution > 0) {
		WWBITPACK_ASSERT(max_units == 
			(static_cast<std::uint64_t>(std::ceil((Max - Min) / Resolution - wwbitpack::kEpsilon)) + 1ull));
	}
	*/
}

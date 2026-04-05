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
// SecureRandomClass - Generate random values
//

#pragma warning(disable : 4514)	// unreferenced inline function removed....

#include "srandom.h"
#include <SDL3/SDL_timer.h>

#include <chrono>
#include <cstdint>
#include <random>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "sha.h"

// Static class variables
uint8_t	SecureRandomClass::Seeds[SecureRandomClass::SeedLength];
bool				SecureRandomClass::Initialized=false;
uint32_t	SecureRandomClass::RandomCache[SecureRandomClass::SHADigestBytes / sizeof(uint32_t)];
int				SecureRandomClass::RandomCacheEntries=0;
uint32_t	SecureRandomClass::Counter=0;
Random3Class	SecureRandomClass::RandomHelper;

SecureRandomClass::SecureRandomClass()
{
	if (Initialized == false)
	{
		Generate_Seed();
		Initialized=true;
	}
}

SecureRandomClass::~SecureRandomClass()
{
}


//
// Add seed values to our pool of randomness
//
void SecureRandomClass::Add_Seeds(uint8_t *values, int length)
{
	for (int i=0; i<length; i++)
	{
		Seeds[0]^=values[i];

		// Rotate the seeds to the left
		uint8_t uctemp=Seeds[SeedLength-1];
		for (int j=SeedLength-1; j>=1; j--)
			Seeds[j]=Seeds[j-1];
		Seeds[0]=uctemp;
	}

	// We have a better seed pool now so trigger new random values
	RandomCacheEntries=0;
}

//
// Get a 32bit random value
//
uint32_t SecureRandomClass::Randval(void)
{
	if (RandomCacheEntries == 0)
	{
		SHAEngine sha;
		char digest[SHADigestBytes];	// SHA produces a 20 byte hash

		sha.Hash(Seeds, SeedLength);
		sha.Result(digest);

		memcpy(RandomCache, digest, SHADigestBytes);
		RandomCacheEntries=(SHADigestBytes / sizeof(uint32_t));

		uint32_t *int_seeds=(uint32_t *)Seeds;
		int_seeds[0]^=Counter;			// remove the last counter (double xor)
		int_seeds[0]^=(Counter+1);		// put the new counter in place

		int_seeds[(SeedLength/sizeof(int))-1]^=Counter;		// remove the last counter (double xor)
		int_seeds[(SeedLength/sizeof(int))-1]^=(Counter+1);	// put the new counter in place

		Counter++;				// increment counter
	}

	uint32_t retval=RandomCache[--RandomCacheEntries];

	// SHA doesn't have the best distribution properties in the world
	//   We'll XOR the result with the output of another random number
	uint32_t helperval=RandomHelper();
	retval^=helperval;

	return(retval);
}



/////////////////////////////// Private Methods ///////////////////////////////////////


//
// Seed the random number generator.
// The seed is what makes each run of random numbers unique.  If an observer
//   can guess your seed they can predict your random numbers.
//
//	Note the use of XORs everywhere.  The XOR of a good random number and a bad random
//		number is still a good random number.
//
void SecureRandomClass::Generate_Seed(void)
{
	int i;

	// Start with some garbage values
	memset(Seeds, 0xAA, SeedLength);

	uint32_t *int_seeds=(uint32_t *)Seeds;
	int int_seed_length=SeedLength/sizeof(uint32_t);
	std::random_device random_device;
	for (i = 0; i < SeedLength; i += static_cast<int>(sizeof(uint32_t))) {
		uint32_t entropy = random_device();
		for (int byte = 0; byte < static_cast<int>(sizeof(uint32_t)) && (i + byte) < SeedLength; ++byte) {
			Seeds[i + byte] ^= static_cast<uint8_t>((entropy >> (byte * 8)) & 0xFFu);
		}
	}

	const uint32_t tick_seed = static_cast<uint32_t>(SDL_GetTicks() & 0xFFFFFFFFu);
	const uint32_t chrono_seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());

	for (i=0; i<int_seed_length; i++)
	{
		if ((i % 4) == 0)
			int_seeds[i]^=time(NULL);
		else if ((i % 4) == 1)
			int_seeds[i]^=tick_seed;
		else if ((i % 4) == 2)
			int_seeds[i]^=chrono_seed;
		else if ((i % 4) == 3) {
			const std::uintptr_t pointer_seed = reinterpret_cast<std::uintptr_t>(&int_seeds[i]);
			const uint32_t folded_pointer_seed = static_cast<uint32_t>(pointer_seed ^ (pointer_seed >> 32));
			int_seeds[i]^=folded_pointer_seed ^ static_cast<uint32_t>(i);
		}
	}
}

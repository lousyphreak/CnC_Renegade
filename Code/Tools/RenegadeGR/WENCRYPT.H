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

/****************************************************************************\
*        C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S         *
******************************************************************************
Project Name: Generic Game Results Server
File Name   : wencrypt.h
Author      : Joe Howes
Start Date  : Jul 9, 1999
Last Update :
------------------------------------------------------------------------------
 
A simple encryption system for game results packets.  Specifically designed
to work only with uint8_t buffers of least 15 bytes long.
 
\****************************************************************************/


#ifndef __WENCRYPT_H__
#define __WENCRYPT_H__


/*------------------------------------------------------------------------------.
| BUILD TYPE									|
| In the interest of smaller code and distributing as little of our		|
| encryption system as possible with game CDs, this preprocessor directive	|
| should be uncommented when building a client.  When uncommented, only those	|
| functions relevant to encrypting will be compiled, and those relevant only	|
| to decrypting will not.							|
`------------------------------------------------------------------------------*/
#define __CLIENT__


/*------------------------------------------------------------------------------.
| PROTOTYPES									|
`------------------------------------------------------------------------------*/

#ifndef __CLIENT__
uint8_t*	DecryptPacket(uint8_t* encbuf, int* size);
#endif

#include <cstdint>

// These are for munching the data
uint8_t* PrepareEncryptedPacket(uint8_t* packet, int* size);
void 		RLEncode(uint8_t* src, uint8_t* dst, int* size);
float 		ran3(int32_t* idnum);
void 		GenerateKey(uint8_t* key, int len, int32_t seed);
uint8_t* 	SimpleEncrypt(const uint8_t* src, int* len);
#ifndef __CLIENT__
uint8_t* 	RLDecode(uint8_t* src, int* size);
uint8_t*	SimpleDecrypt(uint8_t* buf, int* len);
#endif


/*------------------------------------------------------------------------------.
| MACROS									|
`------------------------------------------------------------------------------*/
// For key generation
#define MBIG    1000000000
#define MSEED   161803398
#define MZ      0
#define FAC     (1.0/MBIG)
 
// Errors
#define ERR_SPURIOUS_ESCAPE     -1      // A single escape was found at the end of a compressed
                                        // buffer.  Should never happen as escapes in the
                                        // stream are stuffed and should otherwise indicate
                                        // the start of a token.
 
#define ERR_BUFFER_TOO_SMALL    -2      // The buffer being passed to Simple Encrypt is
                                        // smaller than 15 bytes, which is too small to work
                                        // with it's seed-stuffing.  A gameres packet should
                                        // never be as small as 15 bytes anyway.

#define ERR_BAD_SEED		-3      // The seed value being passed is zero.  Not a valid
					// seed.



#endif

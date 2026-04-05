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
 *                 Project Name : Command & Conquer                                            * 
 *                                                                                             * 
 *                     $Archive:: /G/wwlib/iff.h                                              $* 
 *                                                                                             * 
 *                      $Author:: Neal_k                                                      $*
 *                                                                                             * 
 *                     $Modtime:: 9/23/99 1:46p                                               $*
 *                                                                                             * 
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#pragma once

#include <cstdint>

#ifndef IFF_H
#define IFF_H

#ifndef _WINDOWS
#define __cdecl
#endif

#include	"buff.h"
#include	<stddef.h>
#include	<cstdint>

#define LZW_SUPPORTED			FALSE

/*=========================================================================*/
/* Iff and Load Picture system defines and enumerations							*/
/*=========================================================================*/

#define 	MAKE_ID(a,b,c,d)			(static_cast<int32_t>((static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(b)) <<  8) | static_cast<uint32_t>(static_cast<uint8_t>(a))))
#define	IFFize_WORD(a)			Reverse_Word(a)
#define	IFFize_LONG(a)			Reverse_Long(a)


//lint -strong(AJX,PicturePlaneType)
typedef enum {
	BM_AMIGA,	// Bit plane format (8K per bitplane).
	BM_MCGA,		// Byte per pixel format (64K).

	BM_DEFAULT=BM_MCGA	// Default picture format.
} PicturePlaneType;

/*
**	This is the compression type code.  This value is used in the compressed
**	file header to indicate the method of compression used.  Note that the
**	LZW method may not be supported.
*/
//lint -strong(AJX,CompressionType)
typedef enum {
	NOCOMPRESS,		// No compression (raw data).
	LZW12,			// LZW 12 bit codes.
	LZW14,			// LZW 14 bit codes.
	HORIZONTAL,		// Run length encoding (RLE).
	LCW				// Westwood proprietary compression.
} CompressionType;

/*
**	Compressed blocks of data must start with this header structure.
**	Note that disk based compressed files have an additional two
**	leading bytes that indicate the size of the entire file.
*/
//lint -strong(AJX,CompHeaderType)
typedef struct {
	char	Method;		// Compression method (CompressionType).
	char	pad;			// Reserved pad byte (always 0).
	int32_t	Size;			// Size of the uncompressed data.
	int16_t	Skip;			// Number of bytes to skip before data.
} CompHeaderType;


/*=========================================================================*/
/* The following prototypes are for the file: IFF.CPP								*/
/*=========================================================================*/

int __cdecl Open_Iff_File(char const *filename);
void __cdecl Close_Iff_File(int fh);
uint32_t __cdecl Get_Iff_Chunk_Size(int fh, int32_t id);
uint32_t __cdecl Read_Iff_Chunk(int fh, int32_t id, void *buffer, uint32_t maxsize);
void __cdecl Write_Iff_Chunk(int file, int32_t id, void *buffer, int32_t length);


/*=========================================================================*/
/* The following prototypes are for the file: LOADPICT.CPP						*/
/*=========================================================================*/

//int __cdecl Load_Picture(char const *filename, BufferClass& scratchbuf, BufferClass& destbuf, uint8_t *palette=NULL, PicturePlaneType format=BM_DEFAULT);


/*=========================================================================*/
/* The following prototypes are for the file: LOAD.CPP							*/
/*=========================================================================*/

uint32_t __cdecl Load_Data(char const *name, void *ptr, uint32_t size);
uint32_t __cdecl Write_Data(char const *name, void *ptr, uint32_t size);
//void * __cdecl Load_Alloc_Data(char const *name, MemoryFlagType flags);
uint32_t __cdecl Load_Uncompress(char const *file, Buffer & uncomp_buff, Buffer & dest_buff, void *reserved_data=NULL);
uint32_t __cdecl Uncompress_Data(void const *src, void *dst);
void __cdecl Set_Uncomp_Buffer(int buffer_segment, int size_of_buffer);

/*=========================================================================*/
/* The following prototypes are for the file: WRITELBM.CPP						*/
/*=========================================================================*/

//bool Write_LBM_File(int lbmhandle, BufferClass& buff, int bitplanes, uint8_t *palette);



/*========================= Assembly Functions ============================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================================*/
/* The following prototypes are for the file: PACK2PLN.ASM						*/
/*=========================================================================*/

extern void __cdecl Pack_2_Plane(void *buffer, void * pageptr, int planebit);

/*=========================================================================*/
/* The following prototypes are for the file: LCWCOMP.ASM						*/
/*=========================================================================*/

extern uint32_t __cdecl LCW_Compress(void *source, void *dest, uint32_t length);

/*=========================================================================*/
/* The following prototypes are for the file: LCWUNCMP.ASM						*/
/*=========================================================================*/

extern uint32_t __cdecl LCW_Uncompress(void *source, void *dest, uint32_t length);

#ifdef __cplusplus
}
#endif
/*=========================================================================*/



#endif //IFF_H

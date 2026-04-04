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
 *                     $Archive:: /G/wwlib/lcw.cpp                                            $* 
 *                                                                                             * 
 *                      $Author:: Neal_k                                                      $*
 *                                                                                             * 
 *                     $Modtime:: 10/04/99 10:25a                                             $*
 *                                                                                             * 
 *                    $Revision:: 4                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 *   LCW_Comp -- Performes LCW compression on a block of data.                                 * 
 *   LCW_Uncomp -- Decompress an LCW encoded data block.                                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"
#include "lcw.h"

/***************************************************************************
 * LCW_Uncomp -- Decompress an LCW encoded data block.                     *
 *                                                                         *
 * Uncompress data to the following codes in the format b = byte, w = word *
 * n = byte code pulled from compressed data.                              *
 *                                                                         *
 *   Command code, n        |Description                                   *
 * ------------------------------------------------------------------------*
 * n=0xxxyyyy,yyyyyyyy      |short copy back y bytes and run x+3 from dest *
 * n=10xxxxxx,n1,n2,...,nx+1|med length copy the next x+1 bytes from source*
 * n=11xxxxxx,w1            |med copy from dest x+3 bytes from offset w1   *
 * n=11111111,w1,w2         |long copy from dest w1 bytes from offset w2   *
 * n=11111110,w1,b1         |long run of byte b1 for w1 bytes              *
 * n=10000000               |end of data reached                           *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      void * source ptr                                                  *
 *      void * destination ptr                                             *
 *      unsigned long length of uncompressed data                          *
 *                                                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *     unsigned long # of destination bytes written                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *     3rd argument is dummy. It exists to provide cross-platform          *
 *      compatibility. Note therefore that this implementation does not    *
 *      check for corrupt source data by testing the uncompressed length.  *
 *                                                                         *
 * HISTORY:                                                                *
 *    03/20/1995 IML : Created.                                            *
 *=========================================================================*/
int LCW_Uncomp(void const * source, void * dest, unsigned long )
{
	unsigned char * source_ptr, * dest_ptr, * copy_ptr;
	unsigned char op_code, data;
	unsigned count;
	unsigned * word_dest_ptr;
	unsigned word_data;

	/* Copy the source and destination ptrs. */
	source_ptr = (unsigned char*) source;
	dest_ptr   = (unsigned char*) dest;

	for (;;) {

		/* Read in the operation code. */
		op_code = *source_ptr++;

		if (!(op_code & 0x80)) {

			/* Do a short copy from destination. */
			count = (op_code >> 4) + 3;
			copy_ptr = dest_ptr - ((unsigned) *source_ptr++ + (((unsigned) op_code & 0x0f) << 8));

			while (count--) *dest_ptr++ = *copy_ptr++;

		} else {

			if (!(op_code & 0x40)) {

				if (op_code == 0x80) {

					/* Return # of destination bytes written. */
					return ((unsigned long) (dest_ptr - (unsigned char*) dest));

				} else {

					/* Do a medium copy from source. */
					count = op_code & 0x3f;

					while (count--) *dest_ptr++ = *source_ptr++;
				}

			} else {

				if (op_code == 0xfe) {

					/* Do a long run. */
					count = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
					word_data = data = *(source_ptr + 2);
					word_data  = (word_data << 24) + (word_data << 16) + (word_data << 8) + word_data;
					source_ptr += 3;

					copy_ptr = dest_ptr + 4 - ((unsigned) dest_ptr & 0x3);
					count -= (copy_ptr - dest_ptr);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;

					word_dest_ptr = (unsigned*) dest_ptr;

					dest_ptr += (count & 0xfffffffc);

					while (word_dest_ptr < (unsigned*) dest_ptr) {
						*word_dest_ptr		= word_data;
						*(word_dest_ptr + 1) = word_data;
						word_dest_ptr += 2;
					}

					copy_ptr = dest_ptr + (count & 0x3);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;

				} else {

					if (op_code == 0xff) {

						/* Do a long copy from destination. */
						count = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						copy_ptr = (unsigned char*) dest + *(source_ptr + 2) + ((unsigned) *(source_ptr + 3) << 8);
						source_ptr += 4;

						while (count--) *dest_ptr++ = *copy_ptr++;

					} else {

						/* Do a medium copy from destination. */
						count = (op_code & 0x3f) + 3;
						copy_ptr = (unsigned char*) dest + *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						source_ptr += 2;

						while (count--) *dest_ptr++ = *copy_ptr++;
					}
				}
			}
		}
	}
}

/***********************************************************************************************
 * LCW_Comp -- Performes LCW compression on a block of data.                                   *
 *                                                                                             *
 *    Portable C++ implementation preserving all bitstream semantics from the original x86     *
 *    assembly in lcw.cpp. Packet formats emitted (matching LCW_Uncomp expectations):          *
 *                                                                                             *
 *      0x00..0x7F  : short back-ref  (2 bytes) backward offset 1-4095, length 3-10           *
 *      0x81..0xBF  : literal packet  count bytes follow                                       *
 *      0xC0..0xFD  : medium back-ref (3 bytes) absolute offset word, length 3-64             *
 *      0xFE        : long run        (4 bytes) count word + fill byte                        *
 *      0xFF        : long back-ref   (5 bytes) count word + absolute offset word             *
 *      0x80        : end of data                                                              *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the source data to compress.                                *
 *          dest     -- Pointer to the destination buffer (must be datasize + datasize/128).   *
 *          datasize -- Number of source bytes to compress.                                    *
 *                                                                                             *
 * OUTPUT:  Number of bytes written to dest.                                                   *
 *=============================================================================================*/
int LCW_Comp(void const * source, void * dest, int datasize)
{
	if (source == nullptr || dest == nullptr || datasize <= 0) {
		return 0;
	}

	const unsigned char * src       = static_cast<const unsigned char *>(source);
	const unsigned char * src_start = src;
	const unsigned char * src_end   = src + datasize;
	unsigned char       * dst       = static_cast<unsigned char *>(dest);
	unsigned char * const dst_start = dst;

	// Write initial 1-byte literal packet, mirroring the original's "0x81, first_byte" prologue.
	unsigned char * len_ptr = dst;
	*dst++ = 0x81;
	*dst++ = *src++;
	bool in_literal = true;

	while (src < src_end) {

		// --- Long run-length check (0xFE packet) ---
		// Quick filter mirrors original: test src[0] == src[64] before doing the full scan.
		if (src + 64 < src_end && *src == src[64]) {
			const unsigned char run_byte  = *src;
			const unsigned char * run_end = src + 1;
			while (run_end < src_end && *run_end == run_byte)
				++run_end;

			// Original computes ecx = (run_end - src) - 1 and emits only when ecx >= 65,
			// leaving the final byte of the run for the next iteration.
			int emit_count = static_cast<int>(run_end - src) - 1;
			if (emit_count >= 65) {
				in_literal = false;
				*dst++ = 0xFE;
				*dst++ = static_cast<unsigned char>(emit_count & 0xFF);
				*dst++ = static_cast<unsigned char>((emit_count >> 8) & 0xFF);
				*dst++ = run_byte;
				src += emit_count;
				continue;
			}
		}

		// --- Pattern match: search [src_start, src) for the longest match ---
		const unsigned char * best_match = nullptr;
		int best_len = 2; // must beat 2 to prefer a back-ref over a literal

		const int max_len = static_cast<int>(src_end - src);
		if (max_len > best_len) {
			for (const unsigned char * s = src_start; s < src; ++s) {
				if (*s != *src)
					continue;

				// Quick-check: can this position extend past the current best length?
				// Both accesses are safe: max_len > best_len guarantees src + best_len < src_end,
				// and we skip positions where s + best_len would fall outside the buffer.
				if (static_cast<int>(src_end - s) <= best_len)
					continue;
				if (s[best_len] != src[best_len])
					continue;

				// Count the actual match length.
				int len = 1;
				while (len < max_len && s[len] == src[len])
					++len;

				if (len > best_len) {
					best_len  = len;
					best_match = s;
					if (best_len == max_len)
						break; // can't do better
				}
			}
		}

		if (best_match == nullptr) {
			// --- Emit literal ---
			if (!in_literal || (*len_ptr & 0x3Fu) == 0x3Fu) {
				// Start a new literal packet (count=0, will be incremented below).
				len_ptr  = dst;
				*dst++   = 0x80;
				in_literal = true;
			}
			++(*len_ptr); // increment count byte (0x80 -> 0x81, ..., 0xBF max)
			*dst++ = *src++;

		} else {
			// --- Emit back-reference ---
			in_literal = false;
			unsigned int back    = static_cast<unsigned int>(src - best_match);
			unsigned int abs_off = static_cast<unsigned int>(best_match - src_start);

			if (best_len <= 10 && back <= 0xFFFu) {
				// Short back-ref: 2 bytes, backward offset (1-4095), length 3-10.
				*dst++ = static_cast<unsigned char>(((best_len - 3) << 4) | (back >> 8));
				*dst++ = static_cast<unsigned char>(back & 0xFFu);
			} else if (best_len <= 64) {
				// Medium back-ref: 3 bytes, absolute offset, length 3-64.
				*dst++ = static_cast<unsigned char>(0xC0u | static_cast<unsigned int>(best_len - 3));
				*dst++ = static_cast<unsigned char>(abs_off & 0xFFu);
				*dst++ = static_cast<unsigned char>((abs_off >> 8) & 0xFFu);
			} else {
				// Long back-ref: 5 bytes, absolute offset, length 65+.
				*dst++ = 0xFF;
				*dst++ = static_cast<unsigned char>(best_len & 0xFF);
				*dst++ = static_cast<unsigned char>((best_len >> 8) & 0xFF);
				*dst++ = static_cast<unsigned char>(abs_off & 0xFFu);
				*dst++ = static_cast<unsigned char>((abs_off >> 8) & 0xFFu);
			}
			src += best_len;
		}
	}

	*dst++ = 0x80; // end-of-data marker
	return static_cast<int>(dst - dst_start);
}

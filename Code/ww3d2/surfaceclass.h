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
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/surfaceclass.h                         $*
 *                                                                                             *
 *              Original Author:: Nathaniel Hoffman                                            *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 2/26/02 6:14p                                               $*
 *                                                                                             *
 *                    $Revision:: 18                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include <cstdint>

#ifndef SURFACECLASS_H
#define SURFACECLASS_H

#include "ww3dformat.h"
#include "refcount.h"

struct IDirect3DSurface8;
class Vector2i;
class Vector3;

/*************************************************************************
**                             SurfaceClass
**
** This is our surface class, which wraps IDirect3DSurface8.
**
** Hector Yee 2/12/01 - added in fills, blits etc for font3d class
**
*************************************************************************/
class SurfaceClass : public RefCountClass
{
	public:

		struct SurfaceDescription {
			WW3DFormat		Format;	// Surface format
			uint32_t	Width;	// Surface width in pixels
			uint32_t	Height;	// Surface height in pixels
		};

		// Create surface with desired height, width and format.
		SurfaceClass(unsigned width, unsigned height, WW3DFormat format);

		// Create surface from a file.
		SurfaceClass(const char *filename);

		// Create the surface from a D3D pointer
		SurfaceClass(IDirect3DSurface8 *d3d_surface);

		~SurfaceClass(void);

		// Get surface description
		 void Get_Description(SurfaceDescription &surface_desc);

		// Lock / unlock the surface
		void * Lock(int * pitch);
		void Unlock(void);

		// HY -- The following functions are support functions for font3d
		// zaps the surface memory to zero
		void Clear();

		// copies the contents of one surface to another		
		void Copy(
			uint32_t dstx, uint32_t dsty,
			uint32_t srcx, uint32_t srcy, 
			uint32_t width, uint32_t height,
			const SurfaceClass *other);

		// support for copying from a byte array
		void Copy(const uint8_t *other);

		// support for copying from a byte array
		void Copy(Vector2i &min,Vector2i &max, const uint8_t *other);

		// copies the contents of one surface to another, stretches
		void Stretch_Copy(
			uint32_t dstx, uint32_t dsty,uint32_t dstwidth, uint32_t dstheight,
			uint32_t srcx, uint32_t srcy, uint32_t srcwidth, uint32_t srcheight,
			const SurfaceClass *source);

		// finds the bounding box of non-zero pixels, used in font3d
		void FindBB(Vector2i *min,Vector2i*max);

		// tests a column to see if the alpha is nonzero, used in font3d
		bool Is_Transparent_Column(uint32_t column);		

		// makes a copy of the surface into a byte array
		uint8_t *CreateCopy(int *width,int *height,int*size,bool flip=false);

			// For use by TextureClass:
		IDirect3DSurface8 *Peek_D3D_Surface(void) { return D3DSurface; }

		// Attaching and detaching a surface pointer
		void	Attach (IDirect3DSurface8 *surface);
		void	Detach (void);

		// draws a horizontal line
		void DrawHLine(const uint32_t y,const uint32_t x1, const uint32_t x2, uint32_t color);

		void DrawPixel(const uint32_t x,const uint32_t y, uint32_t color);

		// get pixel function .. to be used infrequently
		void Get_Pixel(Vector3 &rgb, int x,int y);

		void Hue_Shift(const Vector3 &hsv_shift);

		bool Is_Monochrome(void);

		WW3DFormat Get_Surface_Format() const { return SurfaceFormat; }

		//
		//	Handy utility functions
		//
		uint32_t PixelSize(const SurfaceDescription &sd);
		void Convert_Pixel(Vector3 &rgb, const SurfaceClass::SurfaceDescription &sd, const uint8_t * pixel);
		void Convert_Pixel(uint8_t * pixel,const SurfaceClass::SurfaceDescription &sd, const Vector3 &rgb);

	private:

		// Direct3D surface object
		IDirect3DSurface8 *D3DSurface;

		WW3DFormat SurfaceFormat;
	friend class TextureClass;	
};

#endif


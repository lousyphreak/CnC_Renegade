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
 *                     $Archive:: /Commando/Code/ww3d2/surfaceclass.cpp                       $*
 *                                                                                             *
 *              Original Author:: Nathaniel Hoffman                                            *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 2/26/02 6:14p                                               $*
 *                                                                                             *
 *                    $Revision:: 26                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   SurfaceClass::Clear -- Clears a surface to 0                                              *
 *   SurfaceClass::Copy -- Copies a region from one surface to another of the same format      *
 *   SurfaceClass::FindBBAlpha -- Finds the bounding box of non zero pixels in the region (x0, *
 *   PixelSize -- Helper Function to find the size in bytes of a pixel                         *
 *   SurfaceClass::Is_Transparent_Column -- Tests to see if the column is transparent or not   *
 *   SurfaceClass::Copy -- Copies from a byte array to the surface                             *
 *   SurfaceClass::CreateCopy -- Creates a byte array copy of the surface                      *
 *   SurfaceClass::DrawHLine -- draws a horizontal line                                        *
 *   SurfaceClass::DrawPixel -- draws a pixel                                                  *
 *   SurfaceClass::Copy -- Copies a block of system ram to the surface                         *
 *   SurfaceClass::Hue_Shift -- changes the hue of the surface                                 *
 *   SurfaceClass::Is_Monochrome -- Checks if surface is monochrome or not                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "surfaceclass.h"
#include "renegade_build_config.h"
#include "formconv.h"
#if !RENEGADE_WITH_BGFX_RENDERER
#include "dx8wrapper.h"
#endif
#include "textureloader.h"
#include "vector2i.h"
#include "vector3.h"
#include "wwstring.h"
#include "colorspace.h"
#include "bound.h"

/***********************************************************************************************
 * PixelSize -- Helper Function to find the size in bytes of a pixel                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/

unsigned int SurfaceClass::PixelSize(const SurfaceClass::SurfaceDescription &sd)
{
	unsigned int size=0;

	switch (sd.Format)
	{	
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
		size=4;
		break;
	case WW3D_FORMAT_R8G8B8:
		size=3;
		break;
	case WW3D_FORMAT_R5G6B5:
	case WW3D_FORMAT_X1R5G5B5:
	case WW3D_FORMAT_A1R5G5B5:
	case WW3D_FORMAT_A4R4G4B4:
	case WW3D_FORMAT_A8R3G3B2:
	case WW3D_FORMAT_X4R4G4B4:
	case WW3D_FORMAT_A8P8:	
	case WW3D_FORMAT_A8L8:
		size=2;
		break;
	case WW3D_FORMAT_R3G3B2:
	case WW3D_FORMAT_A8:
	case WW3D_FORMAT_P8:
	case WW3D_FORMAT_L8:
	case WW3D_FORMAT_A4L4:
		size=1;
		break;
	}

	return size;
}

static unsigned Get_Compressed_Row_Size(const SurfaceClass::SurfaceDescription &sd)
{
	switch (sd.Format)
	{
	case WW3D_FORMAT_DXT1:
		return ((sd.Width + 3) / 4) * 8;
	case WW3D_FORMAT_DXT2:
	case WW3D_FORMAT_DXT3:
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5:
		return ((sd.Width + 3) / 4) * 16;
	default:
		return 0;
	}
}

static unsigned Get_Compressed_Copy_Size(const SurfaceClass::SurfaceDescription &sd)
{
	unsigned row_size = Get_Compressed_Row_Size(sd);
	if (row_size == 0) {
		return 0;
	}
	return row_size * ((sd.Height + 3) / 4);
}

static unsigned Calculate_Surface_Pitch(unsigned width, WW3DFormat format)
{
	SurfaceClass::SurfaceDescription desc;
	desc.Width = width;
	desc.Height = 1;
	desc.Format = format;

	unsigned compressed_pitch = Get_Compressed_Row_Size(desc);
	if (compressed_pitch != 0) {
		return compressed_pitch;
	}

	return width * Get_Bytes_Per_Pixel(format);
}

static unsigned Calculate_Surface_Size(unsigned width, unsigned height, WW3DFormat format)
{
	SurfaceClass::SurfaceDescription desc;
	desc.Width = width;
	desc.Height = height;
	desc.Format = format;

	unsigned compressed_size = Get_Compressed_Copy_Size(desc);
	if (compressed_size != 0) {
		return compressed_size;
	}

	return Calculate_Surface_Pitch(width, format) * height;
}

void SurfaceClass::Convert_Pixel(Vector3 &rgb, const SurfaceClass::SurfaceDescription &sd, const unsigned char * pixel)
{
	const float scale=1/255.0f;
	switch (sd.Format)
	{	
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
	case WW3D_FORMAT_R8G8B8:
		{
			rgb.X=pixel[2]; // R
			rgb.Y=pixel[1]; // G
			rgb.Z=pixel[0]; // B
		}
		break;
	case WW3D_FORMAT_A4R4G4B4:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];
			rgb.X=((tmp&0x0f00)>>4);   // R
			rgb.Y=((tmp&0x00f0));		// G
			rgb.Z=((tmp&0x000f)<<4);	// B			
		}
		break;
	case WW3D_FORMAT_A1R5G5B5:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];			
			rgb.X=(tmp>>7)&0xf8; // R
			rgb.Y=(tmp>>2)&0xf8; // G
			rgb.Z=(tmp<<3)&0xf8; // B			
		}
		break;
	case WW3D_FORMAT_R5G6B5:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];			
			rgb.X=(tmp>>8)&0xf8;
			rgb.Y=(tmp>>3)&0xfc;
			rgb.Z=(tmp<<3)&0xf8;
		}
		break;

	default:
		// TODO: Implement other pixel formats
		WWASSERT(0);
	}
	rgb*=scale;
}

// Note: This function must never overwrite the original alpha
void SurfaceClass::Convert_Pixel(unsigned char * pixel,const SurfaceClass::SurfaceDescription &sd, const Vector3 &rgb)
{
	unsigned char r,g,b;
	r=(unsigned char) (rgb.X*255.0f);
	g=(unsigned char) (rgb.Y*255.0f);
	b=(unsigned char) (rgb.Z*255.0f);
	switch (sd.Format)
	{	
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
	case WW3D_FORMAT_R8G8B8:
		pixel[0]=b;
		pixel[1]=g;
		pixel[2]=r;
		break;
	case WW3D_FORMAT_A4R4G4B4:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];
			tmp&=0xF000;
			tmp|=(r&0xF0) << 4;
			tmp|=(g&0xF0);
			tmp|=(b&0xF0) >> 4;			
			*(unsigned short*)&pixel[0]=tmp;
		}
		break;
	case WW3D_FORMAT_A1R5G5B5:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];
			tmp&=0x8000;
			tmp|=(r&0xF8) << 7;
			tmp|=(g&0xF8) << 2;
			tmp|=(b&0xF8) >> 3;			
			*(unsigned short*)&pixel[0]=tmp;
		}
		break;
	case WW3D_FORMAT_R5G6B5:
		{
			unsigned short tmp;			
			tmp=(r&0xf8) << 8;
			tmp|=(g&0xfc) << 3;
			tmp|=(b&0xf8) >> 3;
			*(unsigned short*)&pixel[0]=tmp;
		}
		break;
	default:
		// TODO: Implement other pixel formats
		WWASSERT(0);
	}
}

/*************************************************************************
**                             SurfaceClass
*************************************************************************/
SurfaceClass::SurfaceClass(unsigned width, unsigned height, WW3DFormat format):
#if !RENEGADE_WITH_BGFX_RENDERER
	DX8Surface(NULL),
#endif
	SurfaceWidth(width),
	SurfaceHeight(height),
	SurfacePitch(Calculate_Surface_Pitch(width, format)),
	SurfaceLocked(false),
	SurfaceFormat(format)
{
	WWASSERT(width);
	WWASSERT(height);
	SurfaceMemory.resize(Calculate_Surface_Size(width, height, format));
}

SurfaceClass::SurfaceClass(const char *filename):
#if !RENEGADE_WITH_BGFX_RENDERER
	DX8Surface(NULL),
#endif
	SurfaceWidth(0),
	SurfaceHeight(0),
	SurfacePitch(0),
	SurfaceLocked(false),
	SurfaceFormat(WW3D_FORMAT_UNKNOWN)
{
#if RENEGADE_WITH_BGFX_RENDERER
	WWASSERT(filename != NULL && filename[0] != '\0');
	if (filename != NULL && filename[0] != '\0') {
		StringClass filename_string(filename, true);
		SurfaceClass *loaded_surface = TextureLoader::Load_Surface_Immediate(
			filename_string,
			WW3D_FORMAT_UNKNOWN,
			true);
		WWASSERT(loaded_surface != NULL);
		if (loaded_surface != NULL) {
			SurfaceDescription desc;
			loaded_surface->Get_Description(desc);
			SurfaceWidth = desc.Width;
			SurfaceHeight = desc.Height;
			SurfaceFormat = desc.Format;
			SurfacePitch = Calculate_Surface_Pitch(SurfaceWidth, SurfaceFormat);
			SurfaceMemory.resize(Calculate_Surface_Size(SurfaceWidth, SurfaceHeight, SurfaceFormat));
			Copy(0, 0, 0, 0, SurfaceWidth, SurfaceHeight, loaded_surface);
			loaded_surface->Release_Ref();
		}
	}
#else
	DX8Surface = DX8Wrapper::_Create_DX8_Surface(filename);
	SurfaceDescription desc;
	Get_Description(desc);
	SurfaceFormat=desc.Format;
#endif
}

#if !RENEGADE_WITH_BGFX_RENDERER
SurfaceClass::SurfaceClass(IDirect3DSurface8 *d3d_surface)	:
	DX8Surface (NULL),
	SurfaceWidth(0),
	SurfaceHeight(0),
	SurfacePitch(0),
	SurfaceLocked(false)
{
	Attach (d3d_surface);
	SurfaceDescription desc;
	Get_Description(desc);
	SurfaceFormat=desc.Format;
}
#endif

SurfaceClass::~SurfaceClass(void)
{
#if !RENEGADE_WITH_BGFX_RENDERER
	if (DX8Surface) {
		DX8Surface->Release();
		DX8Surface = NULL;
	}
#endif
}

void SurfaceClass::Get_Description(SurfaceDescription &surface_desc)
{
#if RENEGADE_WITH_BGFX_RENDERER
	surface_desc.Format = SurfaceFormat;
	surface_desc.Height = SurfaceHeight;
	surface_desc.Width = SurfaceWidth;
#else
	if (!SurfaceMemory.empty()) {
		surface_desc.Format = SurfaceFormat;
		surface_desc.Height = SurfaceHeight;
		surface_desc.Width = SurfaceWidth;
		return;
	}

	D3DSURFACE_DESC d3d_desc;
	::ZeroMemory(&d3d_desc, sizeof(D3DSURFACE_DESC));
	DX8_ErrorCode(DX8Surface->GetDesc(&d3d_desc));
	surface_desc.Format = D3DFormat_To_WW3DFormat(d3d_desc.Format);
	surface_desc.Height = d3d_desc.Height;
	surface_desc.Width = d3d_desc.Width;
#endif
}

void * SurfaceClass::Lock(int * pitch)
{
#if RENEGADE_WITH_BGFX_RENDERER
	WWASSERT(!SurfaceMemory.empty());
	WWASSERT(!SurfaceLocked);
	SurfaceLocked = true;
	*pitch = SurfacePitch;
	return SurfaceMemory.data();
#else
	if (!SurfaceMemory.empty()) {
		WWASSERT(!SurfaceLocked);
		SurfaceLocked = true;
		*pitch = SurfacePitch;
		return SurfaceMemory.data();
	}

	D3DLOCKED_RECT lock_rect;	
	::ZeroMemory(&lock_rect, sizeof(D3DLOCKED_RECT));
	DX8_ErrorCode(DX8Surface->LockRect(&lock_rect, 0, 0));
	*pitch = lock_rect.Pitch;
	return (void *)lock_rect.pBits;
#endif
}

void SurfaceClass::Unlock(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
	WWASSERT(SurfaceLocked);
	SurfaceLocked = false;
#else
	if (!SurfaceMemory.empty()) {
		WWASSERT(SurfaceLocked);
		SurfaceLocked = false;
		return;
	}

	DX8_ErrorCode(DX8Surface->UnlockRect());
#endif
}

#if !RENEGADE_WITH_BGFX_RENDERER
IDirect3DSurface8 *SurfaceClass::Acquire_DX8_Surface(void)
{
	Materialize_DX8_Surface();
	if (DX8Surface != NULL) {
		DX8Surface->AddRef();
	}

	return DX8Surface;
}

IDirect3DSurface8 *SurfaceClass::Peek_DX8_Surface(void)
{
	Materialize_DX8_Surface();
	return DX8Surface;
}
#endif

/***********************************************************************************************
 * SurfaceClass::Clear -- Clears a surface to 0                                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Clear()
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int size=PixelSize(sd);
#if RENEGADE_WITH_BGFX_RENDERER
	memset(SurfaceMemory.data(), 0, Calculate_Surface_Size(sd.Width, sd.Height, sd.Format));
#else
	if (!SurfaceMemory.empty()) {
		memset(SurfaceMemory.data(), 0, Calculate_Surface_Size(sd.Width, sd.Height, sd.Format));
		return;
	}

	D3DLOCKED_RECT lock_rect;	
	::ZeroMemory(&lock_rect, sizeof(D3DLOCKED_RECT));
	DX8_ErrorCode(DX8Surface->LockRect(&lock_rect,0,0));
	unsigned int i;
	unsigned char *mem=(unsigned char *) lock_rect.pBits;

	for (i=0; i<sd.Height; i++)
	{
		memset(mem,0,size*sd.Width);
		mem+=lock_rect.Pitch;
	}
	
	DX8_ErrorCode(DX8Surface->UnlockRect());
#endif
}


/***********************************************************************************************
 * SurfaceClass::Copy -- Copies from a byte array to the surface                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/15/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Copy(const unsigned char *other)
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int size=PixelSize(sd);
#if RENEGADE_WITH_BGFX_RENDERER
	unsigned char *mem = SurfaceMemory.data();
	for (unsigned int i = 0; i < sd.Height; i++) {
		memcpy(mem, &other[i * sd.Width * size], size * sd.Width);
		mem += SurfacePitch;
	}
#else
	if (!SurfaceMemory.empty()) {
		unsigned char *mem = SurfaceMemory.data();
		for (unsigned int i = 0; i < sd.Height; i++) {
			memcpy(mem, &other[i * sd.Width * size], size * sd.Width);
			mem += SurfacePitch;
		}
		return;
	}

	D3DLOCKED_RECT lock_rect;	
	::ZeroMemory(&lock_rect, sizeof(D3DLOCKED_RECT));
	DX8_ErrorCode(DX8Surface->LockRect(&lock_rect,0,0));
	unsigned int i;
	unsigned char *mem=(unsigned char *) lock_rect.pBits;

	for (i=0; i<sd.Height; i++)
	{
		memcpy(mem,&other[i*sd.Width*size],size*sd.Width);		
		mem+=lock_rect.Pitch;
	}
	
	DX8_ErrorCode(DX8Surface->UnlockRect());
#endif
}


/***********************************************************************************************
 * SurfaceClass::Copy -- Copies a block of system ram to the surface                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   5/2/2001   hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Copy(Vector2i &min,Vector2i &max, const unsigned char *other)
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int size=PixelSize(sd);
#if RENEGADE_WITH_BGFX_RENDERER
	unsigned char *mem = SurfaceMemory.data() + min.J * SurfacePitch + min.I * size;
	int dx=max.I-min.I;
	for (int i=min.J; i<max.J; i++) {
		memcpy(mem,&other[(i*sd.Width+min.I)*size],size*dx);
		mem += SurfacePitch;
	}
#else
	if (!SurfaceMemory.empty()) {
		unsigned char *mem = SurfaceMemory.data() + min.J * SurfacePitch + min.I * size;
		int dx=max.I-min.I;
		for (int i=min.J; i<max.J; i++) {
			memcpy(mem,&other[(i*sd.Width+min.I)*size],size*dx);
			mem += SurfacePitch;
		}
		return;
	}

	D3DLOCKED_RECT lock_rect;	
	::ZeroMemory(&lock_rect, sizeof(D3DLOCKED_RECT));
	RECT rect;
	rect.left=min.I;
	rect.right=max.I;
	rect.top=min.J;
	rect.bottom=max.J;
	DX8_ErrorCode(DX8Surface->LockRect(&lock_rect,&rect,0));
	int i;
	unsigned char *mem=(unsigned char *) lock_rect.pBits;	
	int dx=max.I-min.I;

	for (i=min.J; i<max.J; i++)
	{
		memcpy(mem,&other[(i*sd.Width+min.I)*size],size*dx);		
		mem+=lock_rect.Pitch;
	}
	
	DX8_ErrorCode(DX8Surface->UnlockRect());
#endif
}


/***********************************************************************************************
 * SurfaceClass::CreateCopy -- Creates a byte array copy of the surface                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/16/2001  hy : Created.                                                                  *
 *=============================================================================================*/
unsigned char *SurfaceClass::CreateCopy(int *width,int *height,int*size,bool flip)
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int mysize=PixelSize(sd);

	*width=sd.Width;
	*height=sd.Height;
	*size=mysize;
	const unsigned compressed_copy_size = Get_Compressed_Copy_Size(sd);
	const unsigned copy_size = compressed_copy_size ? compressed_copy_size : (sd.Height * sd.Width * mysize);

	if (copy_size == 0) {
		return NULL;
	}

	unsigned char *other=new unsigned char [copy_size];
#if RENEGADE_WITH_BGFX_RENDERER
	const unsigned char *mem = SurfaceMemory.data();
	if (compressed_copy_size) {
		unsigned row_size = Get_Compressed_Row_Size(sd);
		unsigned row_count = (sd.Height + 3) / 4;
		for (unsigned i = 0; i < row_count; ++i) {
			memcpy(&other[i * row_size], mem, row_size);
			mem += SurfacePitch;
		}
	} else {
		for (unsigned int i = 0; i < sd.Height; i++) {
			if (flip) {
				memcpy(&other[(sd.Height-i-1)*sd.Width*mysize],mem,mysize*sd.Width);
			} else {
				memcpy(&other[i*sd.Width*mysize],mem,mysize*sd.Width);
			}
			mem += SurfacePitch;
		}
	}
	return other;
#else
	if (!SurfaceMemory.empty()) {
		const unsigned char *mem = SurfaceMemory.data();
		if (compressed_copy_size) {
			unsigned row_size = Get_Compressed_Row_Size(sd);
			unsigned row_count = (sd.Height + 3) / 4;
			for (unsigned i = 0; i < row_count; ++i) {
				memcpy(&other[i * row_size], mem, row_size);
				mem += SurfacePitch;
			}
		} else {
			for (unsigned int i = 0; i < sd.Height; i++) {
				if (flip) {
					memcpy(&other[(sd.Height-i-1)*sd.Width*mysize],mem,mysize*sd.Width);
				} else {
					memcpy(&other[i*sd.Width*mysize],mem,mysize*sd.Width);
				}
				mem += SurfacePitch;
			}
		}
		return other;
	}

	D3DLOCKED_RECT lock_rect;	
	::ZeroMemory(&lock_rect, sizeof(D3DLOCKED_RECT));
	DX8_ErrorCode(DX8Surface->LockRect(&lock_rect,0,D3DLOCK_READONLY));
	unsigned char *mem=(unsigned char *) lock_rect.pBits;

	if (compressed_copy_size) {
		unsigned row_size = Get_Compressed_Row_Size(sd);
		unsigned row_count = (sd.Height + 3) / 4;
		for (unsigned i = 0; i < row_count; ++i) {
			memcpy(&other[i * row_size], mem, row_size);
			mem += lock_rect.Pitch;
		}
	}
	else {
		unsigned int i;
		for (i=0; i<sd.Height; i++)
		{
			if (flip)
			{
				memcpy(&other[(sd.Height-i-1)*sd.Width*mysize],mem,mysize*sd.Width);		
			} else
			{
				memcpy(&other[i*sd.Width*mysize],mem,mysize*sd.Width);		
			}
			mem+=lock_rect.Pitch;
		}
	}
	
	DX8_ErrorCode(DX8Surface->UnlockRect());

	return other;
#endif
}


/***********************************************************************************************
 * SurfaceClass::Copy -- Copies a region from one surface to another                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Copy(
	unsigned int dstx, unsigned int dsty,
	unsigned int srcx, unsigned int srcy, 
	unsigned int width, unsigned int height,
	const SurfaceClass *other)
{
	WWASSERT(other);
	WWASSERT(width);
	WWASSERT(height);

	SurfaceDescription sd,osd;
	Get_Description(sd);
	const_cast <SurfaceClass*>(other)->Get_Description(osd);

	WWASSERT(sd.Format == osd.Format);
	if (sd.Format != osd.Format) {
		return;
	}

	const unsigned compressed_row_size = Get_Compressed_Row_Size(sd);
	const bool is_compressed = (compressed_row_size != 0);
	const unsigned pixel_size = is_compressed ? 0 : PixelSize(sd);

	if (srcx >= osd.Width || srcy >= osd.Height || dstx >= sd.Width || dsty >= sd.Height) {
		return;
	}

	width = MIN(width, osd.Width - srcx);
	height = MIN(height, osd.Height - srcy);
	width = MIN(width, sd.Width - dstx);
	height = MIN(height, sd.Height - dsty);

	if (width == 0 || height == 0) {
		return;
	}

	if (is_compressed) {
		WWASSERT((srcx % 4) == 0 && (srcy % 4) == 0 && (dstx % 4) == 0 && (dsty % 4) == 0);
		WWASSERT((width % 4) == 0 && (height % 4) == 0);

		const unsigned src_block_x = srcx / 4;
		const unsigned src_block_y = srcy / 4;
		const unsigned dst_block_x = dstx / 4;
		const unsigned dst_block_y = dsty / 4;
		const unsigned block_count_x = width / 4;
		const unsigned block_count_y = height / 4;
		const unsigned block_size = compressed_row_size / ((sd.Width + 3) / 4);

		int src_pitch = 0;
		unsigned char *src_mem = static_cast<unsigned char *>(const_cast<SurfaceClass *>(other)->Lock(&src_pitch));
		int dst_pitch = 0;
		unsigned char *dst_mem = static_cast<unsigned char *>(Lock(&dst_pitch));

		for (unsigned y = 0; y < block_count_y; ++y) {
			memcpy(
				dst_mem + (dst_block_y + y) * dst_pitch + dst_block_x * block_size,
				src_mem + (src_block_y + y) * src_pitch + src_block_x * block_size,
				block_count_x * block_size);
		}

		Unlock();
		const_cast<SurfaceClass *>(other)->Unlock();
		return;
	}

	int src_pitch = 0;
	unsigned char *src_mem = static_cast<unsigned char *>(const_cast<SurfaceClass *>(other)->Lock(&src_pitch));
	int dst_pitch = 0;
	unsigned char *dst_mem = static_cast<unsigned char *>(Lock(&dst_pitch));

	for (unsigned y = 0; y < height; ++y) {
		memcpy(
			dst_mem + (dsty + y) * dst_pitch + dstx * pixel_size,
			src_mem + (srcy + y) * src_pitch + srcx * pixel_size,
			width * pixel_size);
	}

	Unlock();
	const_cast<SurfaceClass *>(other)->Unlock();
}

/***********************************************************************************************
 * SurfaceClass::Copy -- Copies a region from one surface to another                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Stretch_Copy(
	unsigned int dstx, unsigned int dsty, unsigned int dstwidth, unsigned int dstheight,
	unsigned int srcx, unsigned int srcy, unsigned int srcwidth, unsigned int srcheight,
	const SurfaceClass *other)
{
	WWASSERT(other);

	SurfaceDescription sd,osd;
	Get_Description(sd);
	const_cast <SurfaceClass*>(other)->Get_Description(osd);

	WWASSERT(sd.Format == osd.Format);
	WWASSERT(Get_Compressed_Row_Size(sd) == 0);
	if (sd.Format != osd.Format || Get_Compressed_Row_Size(sd) != 0) {
		return;
	}

	if (srcx >= osd.Width || srcy >= osd.Height || dstx >= sd.Width || dsty >= sd.Height) {
		return;
	}

	srcwidth = MIN(srcwidth, osd.Width - srcx);
	srcheight = MIN(srcheight, osd.Height - srcy);
	dstwidth = MIN(dstwidth, sd.Width - dstx);
	dstheight = MIN(dstheight, sd.Height - dsty);

	if (srcwidth == 0 || srcheight == 0 || dstwidth == 0 || dstheight == 0) {
		return;
	}

	const unsigned pixel_size = PixelSize(sd);
	int src_pitch = 0;
	unsigned char *src_mem = static_cast<unsigned char *>(const_cast<SurfaceClass *>(other)->Lock(&src_pitch));
	int dst_pitch = 0;
	unsigned char *dst_mem = static_cast<unsigned char *>(Lock(&dst_pitch));

	for (unsigned y = 0; y < dstheight; ++y) {
		const unsigned sample_y = srcy + (y * srcheight) / dstheight;
		for (unsigned x = 0; x < dstwidth; ++x) {
			const unsigned sample_x = srcx + (x * srcwidth) / dstwidth;
			memcpy(
				dst_mem + (dsty + y) * dst_pitch + (dstx + x) * pixel_size,
				src_mem + sample_y * src_pitch + sample_x * pixel_size,
				pixel_size);
		}
	}

	Unlock();
	const_cast<SurfaceClass *>(other)->Unlock();
}

/***********************************************************************************************
 * SurfaceClass::FindBB -- Finds the bounding box of non zero pixels in the region             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::FindBB(Vector2i *min,Vector2i*max)
{
	SurfaceDescription sd;
	Get_Description(sd);

	WWASSERT(Has_Alpha(sd.Format));

	int alphabits=Alpha_Bits(sd.Format);
	int mask=0;
	switch (alphabits)
	{
	case 1: mask=1;
		break;
	case 4: mask=0xf;
		break;
	case 8: mask=0xff;
		break;
	}

	int pitch = 0;
	unsigned char *bits = static_cast<unsigned char *>(Lock(&pitch));

	int x,y;
	unsigned int size=PixelSize(sd);
	Vector2i realmin=*max;
	Vector2i realmax=*min;	
	
	// the assumption here is that whenever a pixel has alpha it's in the MSB
	for (y = min->J; y < max->J; y++) {
		for (x = min->I; x < max->I; x++) {

			// HY - this is not endian safe
			unsigned char *alpha = bits + y * pitch + x * size;
			unsigned char myalpha=alpha[size-1];
			myalpha=(myalpha>>(8-alphabits)) & mask;
			if (myalpha) {
				realmin.I = MIN(realmin.I, x);
				realmax.I = MAX(realmax.I, x);
				realmin.J = MIN(realmin.J, y);
				realmax.J = MAX(realmax.J, y);
			}
		}
	}

	Unlock();

	*max=realmax;
	*min=realmin;
}


/***********************************************************************************************
 * SurfaceClass::Is_Transparent_Column -- Tests to see if the column is transparent or not     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
bool SurfaceClass::Is_Transparent_Column(unsigned int column)
{
	SurfaceDescription sd;
	Get_Description(sd);

	WWASSERT(column<sd.Width);
	WWASSERT(Has_Alpha(sd.Format));

	int alphabits=Alpha_Bits(sd.Format);
	int mask=0;
	switch (alphabits)
	{
	case 1: mask=1;
		break;
	case 4: mask=0xf;
		break;
	case 8: mask=0xff;
		break;
	}

	unsigned int size=PixelSize(sd);
	int pitch = 0;
	unsigned char *bits = static_cast<unsigned char *>(Lock(&pitch));

	int y;	
	
	// the assumption here is that whenever a pixel has alpha it's in the MSB
	for (y = 0; y < (int) sd.Height; y++)
	{
		// HY - this is not endian safe
		unsigned char *alpha = bits + y * pitch + column * size;
		unsigned char myalpha=alpha[size-1];		
		myalpha=(myalpha>>(8-alphabits)) & mask;		
		if (myalpha) {
			Unlock();
			return false;			
		}		
	}

	Unlock();
	return true;
}

/***********************************************************************************************
 * SurfaceClass::Get_Pixel -- Returns the pixel's RGB valus to the caller							  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Get_Pixel(Vector3 &rgb, int x,int y)
{
	SurfaceDescription sd;
	Get_Description(sd);

	x = min(x,(int)sd.Width - 1);
	y = min(y,(int)sd.Height - 1);
	int pitch = 0;
	unsigned char *bits = static_cast<unsigned char *>(Lock(&pitch));
	const unsigned pixel_size = PixelSize(sd);
	Convert_Pixel(rgb,sd,bits + y * pitch + x * pixel_size);
	Unlock();	
}

#if !RENEGADE_WITH_BGFX_RENDERER
void SurfaceClass::Materialize_DX8_Surface()
{
	if (DX8Surface != NULL || SurfaceMemory.empty()) {
		return;
	}

	DX8Surface = DX8Wrapper::_Create_DX8_Surface(SurfaceWidth, SurfaceHeight, SurfaceFormat);
	WWASSERT(DX8Surface != NULL);

	D3DLOCKED_RECT lock_rect;
	::ZeroMemory(&lock_rect, sizeof(D3DLOCKED_RECT));
	DX8_ErrorCode(DX8Surface->LockRect(&lock_rect, NULL, 0));

	SurfaceDescription desc;
	Get_Description(desc);

	const unsigned compressed_row_size = Get_Compressed_Row_Size(desc);
	if (compressed_row_size != 0) {
		const unsigned row_count = (desc.Height + 3) / 4;
		const unsigned char *src = SurfaceMemory.data();
		unsigned char *dst = static_cast<unsigned char *>(lock_rect.pBits);
		for (unsigned i = 0; i < row_count; ++i) {
			memcpy(dst, src, compressed_row_size);
			src += SurfacePitch;
			dst += lock_rect.Pitch;
		}
	}
	else {
		const unsigned row_size = SurfaceWidth * Get_Bytes_Per_Pixel(SurfaceFormat);
		const unsigned char *src = SurfaceMemory.data();
		unsigned char *dst = static_cast<unsigned char *>(lock_rect.pBits);
		for (unsigned i = 0; i < SurfaceHeight; ++i) {
			memcpy(dst, src, row_size);
			src += SurfacePitch;
			dst += lock_rect.Pitch;
		}
	}

	DX8_ErrorCode(DX8Surface->UnlockRect());
}

/***********************************************************************************************
 * SurfaceClass::Attach -- Attaches a surface pointer to the object, releasing the current ptr.*
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/27/2001  pds : Created.                                                                 *
 *=============================================================================================*/
void SurfaceClass::Attach (IDirect3DSurface8 *surface)
{
	Detach ();
	DX8Surface = surface;
	SurfaceMemory.clear();

	//
	//	Lock a reference onto the object
	//
	if (DX8Surface != NULL) {
		DX8Surface->AddRef ();

		D3DSURFACE_DESC d3d_desc;
		::ZeroMemory(&d3d_desc, sizeof(D3DSURFACE_DESC));
		DX8_ErrorCode(DX8Surface->GetDesc(&d3d_desc));
		SurfaceWidth = d3d_desc.Width;
		SurfaceHeight = d3d_desc.Height;
		SurfaceFormat = D3DFormat_To_WW3DFormat(d3d_desc.Format);
		SurfacePitch = Calculate_Surface_Pitch(SurfaceWidth, SurfaceFormat);
	}

	return ;
}


/***********************************************************************************************
 * SurfaceClass::Detach -- Releases the reference on the internal surface ptr, and NULLs it.	 .*
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/27/2001  pds : Created.                                                                 *
 *=============================================================================================*/
void SurfaceClass::Detach (void)
{
	//
	//	Release the hold we have on the D3D object
	//
	if (DX8Surface != NULL) {
		DX8Surface->Release ();
	}

	DX8Surface = NULL;
	return ;
}
#endif


/***********************************************************************************************
 * SurfaceClass::DrawPixel -- draws a pixel                                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
void SurfaceClass::DrawPixel(const unsigned int x,const unsigned int y, unsigned int color)
{
	SurfaceDescription sd;
	Get_Description(sd);

	unsigned int size=PixelSize(sd);
	int pitch = 0;
	unsigned char *bits = static_cast<unsigned char *>(Lock(&pitch));
	unsigned char *cptr = bits + y * pitch + x * size;
	unsigned short *sptr = reinterpret_cast<unsigned short *>(cptr);
	unsigned int *lptr = reinterpret_cast<unsigned int *>(cptr);

	switch (size)
	{
	case 1:
		*cptr=(unsigned char) (color & 0xFF);
		break;
	case 2:
		*sptr=(unsigned short) (color & 0xFFFF);
		break;
	case 4:
		*lptr=color;
		break;
	}

	Unlock();
}

/***********************************************************************************************
 * SurfaceClass::DrawHLine -- draws a horizontal line                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/9/2001   hy : Created.                                                                  *
 *   4/9/2001   hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::DrawHLine(const unsigned int y,const unsigned int x1, const unsigned int x2, unsigned int color)
{ 
	SurfaceDescription sd;
	Get_Description(sd);

	unsigned int size=PixelSize(sd);
	int pitch = 0;
	unsigned char *bits = static_cast<unsigned char *>(Lock(&pitch));
	unsigned char *cptr = bits + y * pitch + x1 * size;
	unsigned short *sptr = reinterpret_cast<unsigned short *>(cptr);
	unsigned int *lptr = reinterpret_cast<unsigned int *>(cptr);

	unsigned int x;
	// the assumption here is that whenever a pixel has alpha it's in the MSB
	for (x=x1; x<=x2; x++)
	{		
		switch (size)
		{
		case 1:
			*cptr++=(unsigned char) (color & 0xFF);
			break;
		case 2:
			*sptr++=(unsigned short) (color & 0xFFFF);
			break;
		case 4:
			*lptr++=color;
			break;
		}
	}

	Unlock();
}


/***********************************************************************************************
 * SurfaceClass::Is_Monochrome -- Checks if surface is monochrome or not                       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   7/5/2001   hy : Created.                                                                  *
 *=============================================================================================*/
bool SurfaceClass::Is_Monochrome(void)
{
	unsigned int x,y;
	SurfaceDescription sd;
	Get_Description(sd);

	switch (sd.Format)
	{
		case WW3D_FORMAT_A8L8:	
		case WW3D_FORMAT_A8:		
		case WW3D_FORMAT_L8:
		case WW3D_FORMAT_A4L4:
			return true;
		break;
	}

	int pitch,size;

	size=PixelSize(sd);
	unsigned char *bits=(unsigned char*) Lock(&pitch);

	Vector3 rgb;
	bool mono=true;

	for (y=0; y<sd.Height; y++)
	{
		for (x=0; x<sd.Width; x++)
		{
			Convert_Pixel(rgb,sd,&bits[x*size]);
			mono&=(rgb.X==rgb.Y);
			mono&=(rgb.X==rgb.Z);
			mono&=(rgb.Z==rgb.Y);
			if (!mono)
			{
				Unlock();
				return false;
			}
		}
		bits+=pitch;
	}

	Unlock();

	return true;
}

/***********************************************************************************************
 * SurfaceClass::Hue_Shift -- changes the hue of the surface                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   7/3/2001   hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Hue_Shift(const Vector3 &hsv_shift)
{
	unsigned int x,y;
	SurfaceDescription sd;
	Get_Description(sd);
	int pitch,size;

	size=PixelSize(sd);
	unsigned char *bits=(unsigned char*) Lock(&pitch);

	Vector3 rgb;

	for (y=0; y<sd.Height; y++)
	{
		for (x=0; x<sd.Width; x++)
		{
			Convert_Pixel(rgb,sd,&bits[x*size]);
			Recolor(rgb,hsv_shift);
			rgb.X=Bound(rgb.X,0.0f,1.0f);
			rgb.Y=Bound(rgb.Y,0.0f,1.0f);
			rgb.Z=Bound(rgb.Z,0.0f,1.0f);
			Convert_Pixel(&bits[x*size],sd,rgb);
		}
		bits+=pitch;
	}

	Unlock();
}

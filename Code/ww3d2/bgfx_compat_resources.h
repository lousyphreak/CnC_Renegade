#pragma once

#include "ww3dformat.h"

#include <vector>

#include <bgfx/bgfx.h>

class TextureClass;
struct IDirect3DSurface8;
struct IDirect3DTexture8;

struct BgfxCompatSurface
{
	unsigned width = 1;
	unsigned height = 1;
	WW3DFormat format = WW3D_FORMAT_A8R8G8B8;
	std::vector<unsigned char> bytes;
};

struct BgfxCompatTexture
{
	int width = 1;
	int height = 1;
	WW3DFormat format = WW3D_FORMAT_A8R8G8B8;
	std::vector<unsigned char> bytes;
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
	bool dirty = true;
};

inline BgfxCompatSurface *BgfxCompat_To_Surface(IDirect3DSurface8 *surface)
{
	return reinterpret_cast<BgfxCompatSurface *>(surface);
}

inline const BgfxCompatSurface *BgfxCompat_To_Surface(const IDirect3DSurface8 *surface)
{
	return reinterpret_cast<const BgfxCompatSurface *>(surface);
}

inline BgfxCompatTexture *BgfxCompat_To_Texture(IDirect3DTexture8 *texture)
{
	return reinterpret_cast<BgfxCompatTexture *>(texture);
}

inline const BgfxCompatTexture *BgfxCompat_To_Texture(const IDirect3DTexture8 *texture)
{
	return reinterpret_cast<const BgfxCompatTexture *>(texture);
}

unsigned BgfxCompat_Get_Pixel_Size(WW3DFormat format);
std::vector<unsigned char> BgfxCompat_Convert_Surface_To_RGBA8(const BgfxCompatSurface &surface);
bgfx::TextureHandle BgfxCompat_Get_Texture_Handle(TextureClass *texture);
uint64_t BgfxCompat_Get_Sampler_Flags(const TextureClass *texture);
bgfx::TextureHandle BgfxCompat_Get_White_Texture();
void BgfxCompat_Shutdown_Texture_System();

#include "surfaceclass.h"

#if !RENEGADE_WITH_DX8_RENDERER && RENEGADE_WITH_BGFX_RENDERER

#include "bgfx_compat_resources.h"

#include "vector2i.h"
#include "vector3.h"
#include "textureloader.h"
#include "ww3dformat.h"

#include "wwdebug.h"

#include <algorithm>
#include <cstring>

namespace {

BgfxCompatSurface *Create_Surface(unsigned width, unsigned height, WW3DFormat format)
{
	BgfxCompatSurface *surface = new BgfxCompatSurface();
	surface->width = std::max(width, 1U);
	surface->height = std::max(height, 1U);
	surface->format = format;
	surface->bytes.resize(static_cast<size_t>(surface->width) * static_cast<size_t>(surface->height) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(format)), 0);
	return surface;
}

void Destroy_Surface(IDirect3DSurface8 *surface)
{
	delete BgfxCompat_To_Surface(surface);
}

uint8_t Expand_4_To_8(uint8_t value)
{
	return static_cast<uint8_t>((value << 4) | value);
}

uint8_t Expand_5_To_8(uint8_t value)
{
	return static_cast<uint8_t>((value << 3) | (value >> 2));
}

uint8_t Expand_6_To_8(uint8_t value)
{
	return static_cast<uint8_t>((value << 2) | (value >> 4));
}

void Decode_Pixel(const unsigned char *pixel, WW3DFormat format, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
{
	r = g = b = 0;
	a = 255;
	if (pixel == NULL) {
		return;
	}

	switch (format) {
		case WW3D_FORMAT_A8R8G8B8:
			r = pixel[2];
			g = pixel[1];
			b = pixel[0];
			a = pixel[3];
			break;
		case WW3D_FORMAT_X8R8G8B8:
			r = pixel[2];
			g = pixel[1];
			b = pixel[0];
			break;
		case WW3D_FORMAT_R8G8B8:
			r = pixel[2];
			g = pixel[1];
			b = pixel[0];
			break;
		case WW3D_FORMAT_R5G6B5:
		{
			const uint16_t value = *reinterpret_cast<const uint16_t *>(pixel);
			r = Expand_5_To_8(static_cast<uint8_t>((value >> 11) & 0x1F));
			g = Expand_6_To_8(static_cast<uint8_t>((value >> 5) & 0x3F));
			b = Expand_5_To_8(static_cast<uint8_t>(value & 0x1F));
			break;
		}
		case WW3D_FORMAT_A1R5G5B5:
		{
			const uint16_t value = *reinterpret_cast<const uint16_t *>(pixel);
			a = (value & 0x8000U) ? 255 : 0;
			r = Expand_5_To_8(static_cast<uint8_t>((value >> 10) & 0x1F));
			g = Expand_5_To_8(static_cast<uint8_t>((value >> 5) & 0x1F));
			b = Expand_5_To_8(static_cast<uint8_t>(value & 0x1F));
			break;
		}
		case WW3D_FORMAT_A4R4G4B4:
		{
			const uint16_t value = *reinterpret_cast<const uint16_t *>(pixel);
			a = Expand_4_To_8(static_cast<uint8_t>((value >> 12) & 0x0F));
			r = Expand_4_To_8(static_cast<uint8_t>((value >> 8) & 0x0F));
			g = Expand_4_To_8(static_cast<uint8_t>((value >> 4) & 0x0F));
			b = Expand_4_To_8(static_cast<uint8_t>(value & 0x0F));
			break;
		}
		case WW3D_FORMAT_A8:
			a = pixel[0];
			r = g = b = 255;
			break;
		case WW3D_FORMAT_L8:
			r = g = b = pixel[0];
			break;
		case WW3D_FORMAT_A8L8:
			a = pixel[1];
			r = g = b = pixel[0];
			break;
		case WW3D_FORMAT_A4L4:
			a = Expand_4_To_8(static_cast<uint8_t>((pixel[0] >> 4) & 0x0F));
			r = g = b = Expand_4_To_8(static_cast<uint8_t>(pixel[0] & 0x0F));
			break;
		default:
			break;
	}
}

void Encode_Pixel(unsigned char *pixel, WW3DFormat format, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if (pixel == NULL) {
		return;
	}

	switch (format) {
		case WW3D_FORMAT_A8R8G8B8:
			pixel[0] = b;
			pixel[1] = g;
			pixel[2] = r;
			pixel[3] = a;
			break;
		case WW3D_FORMAT_X8R8G8B8:
			pixel[0] = b;
			pixel[1] = g;
			pixel[2] = r;
			pixel[3] = 0xFF;
			break;
		case WW3D_FORMAT_R8G8B8:
			pixel[0] = b;
			pixel[1] = g;
			pixel[2] = r;
			break;
		case WW3D_FORMAT_R5G6B5:
		{
			const uint16_t value = static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
			*reinterpret_cast<uint16_t *>(pixel) = value;
			break;
		}
		case WW3D_FORMAT_A1R5G5B5:
		{
			const uint16_t value = static_cast<uint16_t>(((a >= 128 ? 1U : 0U) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
			*reinterpret_cast<uint16_t *>(pixel) = value;
			break;
		}
		case WW3D_FORMAT_A4R4G4B4:
		{
			const uint16_t value = static_cast<uint16_t>(((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4));
			*reinterpret_cast<uint16_t *>(pixel) = value;
			break;
		}
		case WW3D_FORMAT_A8:
			pixel[0] = a;
			break;
		case WW3D_FORMAT_L8:
			pixel[0] = static_cast<uint8_t>((static_cast<unsigned>(r) + static_cast<unsigned>(g) + static_cast<unsigned>(b)) / 3U);
			break;
		case WW3D_FORMAT_A8L8:
			pixel[0] = static_cast<uint8_t>((static_cast<unsigned>(r) + static_cast<unsigned>(g) + static_cast<unsigned>(b)) / 3U);
			pixel[1] = a;
			break;
		case WW3D_FORMAT_A4L4:
		{
			const uint8_t luminance = static_cast<uint8_t>((static_cast<unsigned>(r) + static_cast<unsigned>(g) + static_cast<unsigned>(b)) / 3U);
			pixel[0] = static_cast<uint8_t>(((a >> 4) << 4) | (luminance >> 4));
			break;
		}
		default:
			break;
	}
}

unsigned char *Pixel_At(BgfxCompatSurface *surface, unsigned x, unsigned y)
{
	const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(surface->width) + static_cast<size_t>(x)) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(surface->format));
	return surface->bytes.data() + offset;
}

const unsigned char *Pixel_At(const BgfxCompatSurface *surface, unsigned x, unsigned y)
{
	const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(surface->width) + static_cast<size_t>(x)) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(surface->format));
	return surface->bytes.data() + offset;
}

void Initialize_Loaded_Surface(BgfxCompatSurface *surface, const char *filename)
{
	if (surface == NULL || filename == NULL || filename[0] == 0) {
		WWRELEASE_SAY(("BGFX Surface: invalid filename for surface load\n"));
		return;
	}

	StringClass filename_string(filename, true);
	IDirect3DSurface8 *loaded_surface = TextureLoader::Load_Surface_Immediate(filename_string, WW3D_FORMAT_UNKNOWN, true);
	const BgfxCompatSurface *loaded = BgfxCompat_To_Surface(loaded_surface);
	if (loaded == NULL) {
		WWRELEASE_SAY(("BGFX Surface: failed to load %s via TextureLoader\n", filename));
		return;
	}

	surface->width = loaded->width;
	surface->height = loaded->height;
	surface->format = loaded->format;
	surface->bytes = loaded->bytes;

	delete loaded;
}

} // namespace

SurfaceClass::SurfaceClass(unsigned width, unsigned height, WW3DFormat format)
	: D3DSurface(reinterpret_cast<IDirect3DSurface8 *>(Create_Surface(width, height, format))),
	  SurfaceFormat(format)
{
}

SurfaceClass::SurfaceClass(const char *filename)
	: D3DSurface(reinterpret_cast<IDirect3DSurface8 *>(Create_Surface(1, 1, WW3D_FORMAT_A8R8G8B8))),
	  SurfaceFormat(WW3D_FORMAT_A8R8G8B8)
{
	Initialize_Loaded_Surface(BgfxCompat_To_Surface(D3DSurface), filename);
	SurfaceFormat = BgfxCompat_To_Surface(D3DSurface)->format;
}

SurfaceClass::SurfaceClass(IDirect3DSurface8 *d3d_surface)
	: D3DSurface(d3d_surface),
	  SurfaceFormat(WW3D_FORMAT_A8R8G8B8)
{
	if (BgfxCompat_To_Surface(D3DSurface) != NULL) {
		SurfaceFormat = BgfxCompat_To_Surface(D3DSurface)->format;
	}
}

SurfaceClass::~SurfaceClass(void)
{
	if (D3DSurface != NULL) {
		Destroy_Surface(D3DSurface);
		D3DSurface = NULL;
	}
}

void SurfaceClass::Get_Description(SurfaceDescription &surface_desc)
{
	const BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	surface_desc.Format = surface != NULL ? surface->format : SurfaceFormat;
	surface_desc.Width = surface != NULL ? surface->width : 0;
	surface_desc.Height = surface != NULL ? surface->height : 0;
}

void *SurfaceClass::Lock(int *pitch)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (pitch != NULL) {
		*pitch = surface != NULL ? static_cast<int>(surface->width * BgfxCompat_Get_Pixel_Size(surface->format)) : 0;
	}
	return surface != NULL ? surface->bytes.data() : NULL;
}

void SurfaceClass::Unlock(void)
{
}

void SurfaceClass::Clear()
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface != NULL) {
		std::fill(surface->bytes.begin(), surface->bytes.end(), 0);
	}
}

void SurfaceClass::Copy(
	unsigned int dstx,
	unsigned int dsty,
	unsigned int srcx,
	unsigned int srcy,
	unsigned int width,
	unsigned int height,
	const SurfaceClass *other)
{
	if (other == NULL) {
		return;
	}

	BgfxCompatSurface *dst_surface = BgfxCompat_To_Surface(D3DSurface);
	const BgfxCompatSurface *src_surface = BgfxCompat_To_Surface(other->Peek_D3D_Surface());
	if (dst_surface == NULL || src_surface == NULL) {
		return;
	}

	const unsigned copy_width = std::min(width, std::min(dst_surface->width > dstx ? dst_surface->width - dstx : 0U, src_surface->width > srcx ? src_surface->width - srcx : 0U));
	const unsigned copy_height = std::min(height, std::min(dst_surface->height > dsty ? dst_surface->height - dsty : 0U, src_surface->height > srcy ? src_surface->height - srcy : 0U));
	if (copy_width == 0 || copy_height == 0) {
		return;
	}

	if (dst_surface->format == src_surface->format) {
		const size_t row_bytes = static_cast<size_t>(copy_width) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(dst_surface->format));
		for (unsigned row = 0; row < copy_height; ++row) {
			std::memmove(Pixel_At(dst_surface, dstx, dsty + row), Pixel_At(src_surface, srcx, srcy + row), row_bytes);
		}
		return;
	}

	for (unsigned row = 0; row < copy_height; ++row) {
		for (unsigned col = 0; col < copy_width; ++col) {
			uint8_t r, g, b, a;
			Decode_Pixel(Pixel_At(src_surface, srcx + col, srcy + row), src_surface->format, r, g, b, a);
			Encode_Pixel(Pixel_At(dst_surface, dstx + col, dsty + row), dst_surface->format, r, g, b, a);
		}
	}
}

void SurfaceClass::Copy(const unsigned char *other)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface != NULL && other != NULL) {
		std::memcpy(surface->bytes.data(), other, surface->bytes.size());
	}
}

void SurfaceClass::Copy(Vector2i &min, Vector2i &max, const unsigned char *other)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL || other == NULL) {
		return;
	}

	const unsigned pixel_size = BgfxCompat_Get_Pixel_Size(surface->format);
	const unsigned left = static_cast<unsigned>(std::max(min.I, 0));
	const unsigned top = static_cast<unsigned>(std::max(min.J, 0));
	const unsigned right = static_cast<unsigned>(std::min(max.I, static_cast<int>(surface->width)));
	const unsigned bottom = static_cast<unsigned>(std::min(max.J, static_cast<int>(surface->height)));
	for (unsigned y = top; y < bottom; ++y) {
		const size_t row_offset = (static_cast<size_t>(y) * static_cast<size_t>(surface->width) + static_cast<size_t>(left)) * pixel_size;
		const size_t row_size = static_cast<size_t>(right - left) * pixel_size;
		std::memcpy(surface->bytes.data() + row_offset, other + row_offset, row_size);
	}
}

void SurfaceClass::Stretch_Copy(
	unsigned int dstx,
	unsigned int dsty,
	unsigned int dstwidth,
	unsigned int dstheight,
	unsigned int srcx,
	unsigned int srcy,
	unsigned int srcwidth,
	unsigned int srcheight,
	const SurfaceClass *source)
{
	if (source == NULL) {
		return;
	}

	BgfxCompatSurface *dst_surface = BgfxCompat_To_Surface(D3DSurface);
	const BgfxCompatSurface *src_surface = BgfxCompat_To_Surface(source->Peek_D3D_Surface());
	if (dst_surface == NULL || src_surface == NULL || dstwidth == 0 || dstheight == 0 || srcwidth == 0 || srcheight == 0) {
		return;
	}

	for (unsigned y = 0; y < dstheight; ++y) {
		const unsigned sample_y = srcy + std::min((y * srcheight) / dstheight, srcheight - 1U);
		for (unsigned x = 0; x < dstwidth; ++x) {
			const unsigned sample_x = srcx + std::min((x * srcwidth) / dstwidth, srcwidth - 1U);
			if (dstx + x >= dst_surface->width || dsty + y >= dst_surface->height || sample_x >= src_surface->width || sample_y >= src_surface->height) {
				continue;
			}
			uint8_t r, g, b, a;
			Decode_Pixel(Pixel_At(src_surface, sample_x, sample_y), src_surface->format, r, g, b, a);
			Encode_Pixel(Pixel_At(dst_surface, dstx + x, dsty + y), dst_surface->format, r, g, b, a);
		}
	}
}

void SurfaceClass::FindBB(Vector2i *min, Vector2i *max)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL || min == NULL || max == NULL) {
		return;
	}

	Vector2i real_min(static_cast<int>(surface->width), static_cast<int>(surface->height));
	Vector2i real_max(-1, -1);
	for (unsigned y = 0; y < surface->height; ++y) {
		for (unsigned x = 0; x < surface->width; ++x) {
			uint8_t r, g, b, a;
			Decode_Pixel(Pixel_At(surface, x, y), surface->format, r, g, b, a);
			if (a != 0) {
				real_min.I = std::min(real_min.I, static_cast<int>(x));
				real_min.J = std::min(real_min.J, static_cast<int>(y));
				real_max.I = std::max(real_max.I, static_cast<int>(x));
				real_max.J = std::max(real_max.J, static_cast<int>(y));
			}
		}
	}

	*min = real_min;
	*max = real_max;
}

bool SurfaceClass::Is_Transparent_Column(unsigned int column)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL || column >= surface->width) {
		return true;
	}

	for (unsigned y = 0; y < surface->height; ++y) {
		uint8_t r, g, b, a;
		Decode_Pixel(Pixel_At(surface, column, y), surface->format, r, g, b, a);
		if (a != 0) {
			return false;
		}
	}

	return true;
}

unsigned char *SurfaceClass::CreateCopy(int *width, int *height, int *size, bool flip)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL) {
		return NULL;
	}

	const unsigned pixel_size = BgfxCompat_Get_Pixel_Size(surface->format);
	if (width != NULL) {
		*width = static_cast<int>(surface->width);
	}
	if (height != NULL) {
		*height = static_cast<int>(surface->height);
	}
	if (size != NULL) {
		*size = static_cast<int>(pixel_size);
	}

	const size_t byte_count = surface->bytes.size();
	unsigned char *copy = new unsigned char[byte_count];
	if (!flip) {
		std::memcpy(copy, surface->bytes.data(), byte_count);
		return copy;
	}

	const size_t row_bytes = static_cast<size_t>(surface->width) * pixel_size;
	for (unsigned y = 0; y < surface->height; ++y) {
		std::memcpy(copy + static_cast<size_t>(surface->height - y - 1U) * row_bytes, surface->bytes.data() + static_cast<size_t>(y) * row_bytes, row_bytes);
	}
	return copy;
}

void SurfaceClass::Attach(IDirect3DSurface8 *surface)
{
	Detach();
	D3DSurface = surface;
	if (BgfxCompat_To_Surface(D3DSurface) != NULL) {
		SurfaceFormat = BgfxCompat_To_Surface(D3DSurface)->format;
	}
}

void SurfaceClass::Detach(void)
{
	if (D3DSurface != NULL) {
		Destroy_Surface(D3DSurface);
		D3DSurface = NULL;
	}
}

void SurfaceClass::DrawHLine(const unsigned int y, const unsigned int x1, const unsigned int x2, unsigned int color)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL || y >= surface->height) {
		return;
	}

	const unsigned start = std::min(x1, x2);
	const unsigned end = std::min(std::max(x1, x2), surface->width > 0 ? surface->width - 1U : 0U);
	for (unsigned x = start; x <= end; ++x) {
		DrawPixel(x, y, color);
	}
}

void SurfaceClass::DrawPixel(const unsigned int x, const unsigned int y, unsigned int color)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL || x >= surface->width || y >= surface->height) {
		return;
	}

	const uint8_t a = static_cast<uint8_t>((color >> 24) & 0xFF);
	const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
	const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
	const uint8_t b = static_cast<uint8_t>(color & 0xFF);
	Encode_Pixel(Pixel_At(surface, x, y), surface->format, r, g, b, a);
}

void SurfaceClass::Get_Pixel(Vector3 &rgb, int x, int y)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL || surface->width == 0 || surface->height == 0) {
		rgb.Set(0.0f, 0.0f, 0.0f);
		return;
	}

	x = std::max(0, std::min(x, static_cast<int>(surface->width) - 1));
	y = std::max(0, std::min(y, static_cast<int>(surface->height) - 1));
	uint8_t r, g, b, a;
	Decode_Pixel(Pixel_At(surface, static_cast<unsigned>(x), static_cast<unsigned>(y)), surface->format, r, g, b, a);
	rgb.Set(r / 255.0f, g / 255.0f, b / 255.0f);
}

void SurfaceClass::Hue_Shift(const Vector3 &)
{
}

bool SurfaceClass::Is_Monochrome(void)
{
	BgfxCompatSurface *surface = BgfxCompat_To_Surface(D3DSurface);
	if (surface == NULL) {
		return true;
	}

	for (unsigned y = 0; y < surface->height; ++y) {
		for (unsigned x = 0; x < surface->width; ++x) {
			uint8_t r, g, b, a;
			Decode_Pixel(Pixel_At(surface, x, y), surface->format, r, g, b, a);
			if (r != g || g != b) {
				return false;
			}
		}
	}
	return true;
}

unsigned int SurfaceClass::PixelSize(const SurfaceDescription &sd)
{
	return BgfxCompat_Get_Pixel_Size(sd.Format);
}

void SurfaceClass::Convert_Pixel(Vector3 &rgb, const SurfaceDescription &sd, const unsigned char *pixel)
{
	uint8_t r, g, b, a;
	Decode_Pixel(pixel, sd.Format, r, g, b, a);
	rgb.Set(r / 255.0f, g / 255.0f, b / 255.0f);
}

void SurfaceClass::Convert_Pixel(unsigned char *pixel, const SurfaceDescription &sd, const Vector3 &rgb)
{
	Encode_Pixel(
		pixel,
		sd.Format,
		static_cast<uint8_t>(std::clamp(rgb.X, 0.0f, 1.0f) * 255.0f),
		static_cast<uint8_t>(std::clamp(rgb.Y, 0.0f, 1.0f) * 255.0f),
		static_cast<uint8_t>(std::clamp(rgb.Z, 0.0f, 1.0f) * 255.0f),
		255);
}

#endif

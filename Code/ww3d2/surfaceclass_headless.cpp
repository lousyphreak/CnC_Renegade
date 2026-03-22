#include "surfaceclass.h"

#include "vector2i.h"
#include "vector3.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace {

struct HeadlessSurfaceData {
	unsigned width = 1;
	unsigned height = 1;
	WW3DFormat format = WW3D_FORMAT_A8R8G8B8;
	std::vector<unsigned char> bytes;
};

std::unordered_map<const SurfaceClass *, HeadlessSurfaceData> g_surfaces;

unsigned Pixel_Size(WW3DFormat format)
{
	switch (format) {
		case WW3D_FORMAT_R8G8B8:
			return 3;
		case WW3D_FORMAT_R5G6B5:
		case WW3D_FORMAT_A1R5G5B5:
		case WW3D_FORMAT_A4R4G4B4:
			return 2;
		default:
			return 4;
	}
}

HeadlessSurfaceData & Surface_Data(const SurfaceClass * surface)
{
	return g_surfaces[surface];
}

} // namespace

SurfaceClass::SurfaceClass(unsigned width, unsigned height, WW3DFormat format) : D3DSurface(NULL), SurfaceFormat(format)
{
	HeadlessSurfaceData & data = Surface_Data(this);
	data.width = std::max(width, 1U);
	data.height = std::max(height, 1U);
	data.format = format;
	data.bytes.resize(data.width * data.height * Pixel_Size(format), 0);
}

SurfaceClass::SurfaceClass(const char *) : SurfaceClass(1, 1, WW3D_FORMAT_A8R8G8B8) {}
SurfaceClass::SurfaceClass(IDirect3DSurface8 * d3d_surface) : D3DSurface(d3d_surface), SurfaceFormat(WW3D_FORMAT_A8R8G8B8) { Surface_Data(this).bytes.resize(4, 0); }
SurfaceClass::~SurfaceClass(void) { g_surfaces.erase(this); }

void SurfaceClass::Get_Description(SurfaceDescription & surface_desc)
{
	HeadlessSurfaceData & data = Surface_Data(this);
	surface_desc.Format = data.format;
	surface_desc.Width = data.width;
	surface_desc.Height = data.height;
}

void * SurfaceClass::Lock(int * pitch)
{
	HeadlessSurfaceData & data = Surface_Data(this);
	if (pitch != NULL) {
		*pitch = static_cast<int>(data.width * Pixel_Size(data.format));
	}
	return data.bytes.data();
}

void SurfaceClass::Unlock(void) {}
void SurfaceClass::Clear() { std::fill(Surface_Data(this).bytes.begin(), Surface_Data(this).bytes.end(), 0); }
void SurfaceClass::Copy(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, const SurfaceClass * other)
{
	if (other != NULL) {
		Surface_Data(this).bytes = Surface_Data(other).bytes;
	}
}
void SurfaceClass::Copy(const unsigned char * other)
{
	if (other != NULL) {
		std::memcpy(Surface_Data(this).bytes.data(), other, Surface_Data(this).bytes.size());
	}
}
void SurfaceClass::Copy(Vector2i &, Vector2i &, const unsigned char * other) { Copy(other); }
void SurfaceClass::Stretch_Copy(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, const SurfaceClass * source) { Copy(0,0,0,0,0,0,source); }
void SurfaceClass::FindBB(Vector2i * min,Vector2i * max) { if (min) min->Set(0,0); if (max) { SurfaceDescription sd; Get_Description(sd); max->Set(sd.Width ? sd.Width - 1 : 0, sd.Height ? sd.Height - 1 : 0); } }
bool SurfaceClass::Is_Transparent_Column(unsigned int) { return false; }
unsigned char * SurfaceClass::CreateCopy(int * width,int * height,int * size,bool)
{
	HeadlessSurfaceData & data = Surface_Data(this);
	if (width) *width = static_cast<int>(data.width);
	if (height) *height = static_cast<int>(data.height);
	if (size) *size = static_cast<int>(data.bytes.size());
	unsigned char * copy = new unsigned char[data.bytes.size()];
	std::memcpy(copy, data.bytes.data(), data.bytes.size());
	return copy;
}
void SurfaceClass::Attach(IDirect3DSurface8 * surface) { D3DSurface = surface; }
void SurfaceClass::Detach(void) { D3DSurface = NULL; }
void SurfaceClass::DrawHLine(const unsigned int,const unsigned int,const unsigned int,unsigned int) {}
void SurfaceClass::DrawPixel(const unsigned int,const unsigned int,unsigned int) {}
void SurfaceClass::Get_Pixel(Vector3 & rgb, int, int) { rgb.Set(0.0f, 0.0f, 0.0f); }
void SurfaceClass::Hue_Shift(const Vector3 &) {}
bool SurfaceClass::Is_Monochrome(void) { return true; }
unsigned int SurfaceClass::PixelSize(const SurfaceDescription & sd) { return Pixel_Size(sd.Format); }
void SurfaceClass::Convert_Pixel(Vector3 & rgb, const SurfaceDescription &, const unsigned char * pixel) { rgb.Set(pixel != NULL ? pixel[0] / 255.0f : 0.0f, pixel != NULL ? pixel[1] / 255.0f : 0.0f, pixel != NULL ? pixel[2] / 255.0f : 0.0f); }
void SurfaceClass::Convert_Pixel(unsigned char * pixel,const SurfaceDescription &, const Vector3 & rgb) { if (pixel != NULL) { pixel[0] = static_cast<unsigned char>(rgb.X * 255.0f); pixel[1] = static_cast<unsigned char>(rgb.Y * 255.0f); pixel[2] = static_cast<unsigned char>(rgb.Z * 255.0f); } }

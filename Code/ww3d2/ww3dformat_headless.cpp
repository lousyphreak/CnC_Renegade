#include "ww3dformat.h"

#include "vector4.h"

void Vector4_to_Color(uint32_t *outc, const Vector4 &inc, const WW3DFormat)
{
    if (outc == nullptr) {
        return;
    }

    const auto clamp = [](float value) -> uint32_t {
        if (value <= 0.0f) {
            return 0;
        }
        if (value >= 1.0f) {
            return 255;
        }
        return static_cast<uint32_t>(value * 255.0f + 0.5f);
    };

    const uint32_t a = clamp(inc.W);
    const uint32_t r = clamp(inc.X);
    const uint32_t g = clamp(inc.Y);
    const uint32_t b = clamp(inc.Z);
    *outc = (a << 24) | (r << 16) | (g << 8) | b;
}

void Color_to_Vector4(Vector4 *outc, const uint32_t inc, const WW3DFormat)
{
    if (outc == nullptr) {
        return;
    }

    outc->Set(
        static_cast<float>((inc >> 16) & 0xFF) / 255.0f,
        static_cast<float>((inc >> 8) & 0xFF) / 255.0f,
        static_cast<float>(inc & 0xFF) / 255.0f,
        static_cast<float>((inc >> 24) & 0xFF) / 255.0f);
}

void Get_WW3D_Format(WW3DFormat &dest_format, WW3DFormat &src_format, unsigned &src_bpp, const Targa &)
{
    dest_format = WW3D_FORMAT_A8R8G8B8;
    src_format = WW3D_FORMAT_A8R8G8B8;
    src_bpp = 4;
}

void Get_WW3D_Format(WW3DFormat &src_format, unsigned &src_bpp, const Targa &)
{
    src_format = WW3D_FORMAT_A8R8G8B8;
    src_bpp = 4;
}

WW3DFormat Get_Valid_Texture_Format(WW3DFormat format, bool)
{
    return format == WW3D_FORMAT_UNKNOWN ? WW3D_FORMAT_A8R8G8B8 : format;
}

unsigned Get_Bytes_Per_Pixel(WW3DFormat format)
{
    switch (format) {
    case WW3D_FORMAT_R8G8B8:
        return 3;
    case WW3D_FORMAT_R5G6B5:
    case WW3D_FORMAT_X1R5G5B5:
    case WW3D_FORMAT_A1R5G5B5:
    case WW3D_FORMAT_A4R4G4B4:
    case WW3D_FORMAT_X4R4G4B4:
    case WW3D_FORMAT_A8P8:
    case WW3D_FORMAT_A8L8:
    case WW3D_FORMAT_U8V8:
    case WW3D_FORMAT_L6V5U5:
        return 2;
    case WW3D_FORMAT_A8R8G8B8:
    case WW3D_FORMAT_X8R8G8B8:
    case WW3D_FORMAT_X8L8V8U8:
        return 4;
    case WW3D_FORMAT_R3G3B2:
    case WW3D_FORMAT_A8:
    case WW3D_FORMAT_A8R3G3B2:
    case WW3D_FORMAT_P8:
    case WW3D_FORMAT_L8:
    case WW3D_FORMAT_A4L4:
        return 1;
    case WW3D_FORMAT_DXT1:
        return 8;
    case WW3D_FORMAT_DXT2:
    case WW3D_FORMAT_DXT3:
    case WW3D_FORMAT_DXT4:
    case WW3D_FORMAT_DXT5:
        return 16;
    case WW3D_FORMAT_UNKNOWN:
    case WW3D_FORMAT_COUNT:
    default:
        return 0;
    }
}

void Get_WW3D_Format_Name(WW3DFormat format, StringClass &name)
{
    switch (format) {
    case WW3D_FORMAT_A8R8G8B8:
        name = "A8R8G8B8";
        break;
    case WW3D_FORMAT_X8R8G8B8:
        name = "X8R8G8B8";
        break;
    case WW3D_FORMAT_R5G6B5:
        name = "R5G6B5";
        break;
    case WW3D_FORMAT_A4R4G4B4:
        name = "A4R4G4B4";
        break;
    case WW3D_FORMAT_DXT1:
        name = "DXT1";
        break;
    case WW3D_FORMAT_DXT5:
        name = "DXT5";
        break;
    default:
        name = "UNKNOWN";
        break;
    }
}

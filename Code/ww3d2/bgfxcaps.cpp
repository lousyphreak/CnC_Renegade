#include "dx8caps.h"

#include <bgfx/bgfx.h>
#include <cstdio>
#include <cstring>

namespace
{
D3DCAPS8 Build_Default_Caps()
{
    D3DCAPS8 caps = {};
    caps.AdapterOrdinal = 0;
    caps.DeviceType = D3DDEVTYPE_HAL;
    caps.DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
    caps.RasterCaps = D3DPRASTERCAPS_ZBIAS;
    caps.TextureFilterCaps = D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MAGFANISOTROPIC;
    caps.TextureOpCaps = D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_MODULATE | D3DTEXOPCAPS_ADD | D3DTEXOPCAPS_SUBTRACT | D3DTEXOPCAPS_ADDSMOOTH | D3DTEXOPCAPS_BLENDTEXTUREALPHA;
    caps.MaxTextureWidth = 16384;
    caps.MaxTextureHeight = 16384;
    caps.MaxSimultaneousTextures = 2;
    caps.VertexShaderVersion = 0;
    caps.PixelShaderVersion = 0;
    return caps;
}

D3DADAPTER_IDENTIFIER8 Build_Default_Adapter()
{
    D3DADAPTER_IDENTIFIER8 adapter = {};
    const bgfx::RendererType::Enum renderer_type = bgfx::getRendererType();
    const char *description = bgfx::getRendererName(renderer_type);
    if (description == nullptr) {
        description = "bgfx";
    }
    std::snprintf(adapter.Driver, sizeof(adapter.Driver), "bgfx");
    std::snprintf(adapter.Description, sizeof(adapter.Description), "%s", description);
    std::snprintf(adapter.DeviceName, sizeof(adapter.DeviceName), "bgfx");
    adapter.DriverVersion.HighPart = 1;
    adapter.DriverVersion.LowPart = 0;
    return adapter;
}

bool Is_Supported_Texture_Format(WW3DFormat format)
{
    switch (format) {
    case WW3D_FORMAT_UNKNOWN:
    case WW3D_FORMAT_R8G8B8:
    case WW3D_FORMAT_A8R8G8B8:
    case WW3D_FORMAT_X8R8G8B8:
    case WW3D_FORMAT_R5G6B5:
    case WW3D_FORMAT_X1R5G5B5:
    case WW3D_FORMAT_A1R5G5B5:
    case WW3D_FORMAT_A4R4G4B4:
    case WW3D_FORMAT_R3G3B2:
    case WW3D_FORMAT_A8:
    case WW3D_FORMAT_A8R3G3B2:
    case WW3D_FORMAT_X4R4G4B4:
    case WW3D_FORMAT_A4L4:
    case WW3D_FORMAT_L8:
    case WW3D_FORMAT_A8L8:
    case WW3D_FORMAT_DXT1:
    case WW3D_FORMAT_DXT2:
    case WW3D_FORMAT_DXT3:
    case WW3D_FORMAT_DXT4:
    case WW3D_FORMAT_DXT5:
        return true;
    default:
        return false;
    }
}
}

DX8Caps::DX8Caps(IDirect3D8*, const D3DCAPS8& caps, WW3DFormat display_format, const D3DADAPTER_IDENTIFIER8& adapter_id)
    : MaxDisplayWidth(16384),
      MaxDisplayHeight(16384),
      Caps(caps),
      SupportTnL(true),
      SupportDXTC(true),
      supportGamma(false),
      SupportNPatches(false),
      SupportBumpEnvmap(false),
      SupportBumpEnvmapLuminance(false),
      SupportZBias(true),
      SupportAnisotropicFiltering(true),
      CanDoMultiPass(true),
      IsFogAllowed(true),
      MaxTexturesPerPass(2),
      VertexShaderVersion(0),
      PixelShaderVersion(0),
      DeviceId(0),
      DriverBuildVersion(0),
      DriverVersionStatus(DRIVER_STATUS_GOOD),
      VendorId(VENDOR_UNKNOWN),
      Direct3D(nullptr)
{
    std::memset(SupportTextureFormat, 0, sizeof(SupportTextureFormat));
    std::memset(SupportRenderToTextureFormat, 0, sizeof(SupportRenderToTextureFormat));
    for (int i = 0; i < WW3D_FORMAT_COUNT; ++i) {
        const bool supported = Is_Supported_Texture_Format(static_cast<WW3DFormat>(i));
        SupportTextureFormat[i] = supported;
        SupportRenderToTextureFormat[i] = supported;
    }

    Caps = caps;
    Compute_Caps(display_format, adapter_id);
}

DX8Caps::DX8Caps(IDirect3D8* direct3d, IDirect3DDevice8*, WW3DFormat display_format, const D3DADAPTER_IDENTIFIER8& adapter_id)
    : DX8Caps(direct3d, Build_Default_Caps(), display_format, adapter_id)
{
}

void DX8Caps::Compute_Caps(WW3DFormat, const D3DADAPTER_IDENTIFIER8& adapter_id)
{
    DriverDLL = adapter_id.Driver;
    DeviceId = adapter_id.DeviceId;
    DriverBuildVersion = adapter_id.DriverVersion.LowPart;
    VendorId = Define_Vendor(adapter_id.VendorId);
    CapsLog = adapter_id.Description;
    CompactLog = adapter_id.Description;
}

bool DX8Caps::Is_Valid_Display_Format(int width, int height, WW3DFormat format)
{
    return width > 0 && height > 0 && Is_Supported_Texture_Format(format);
}

DX8Caps::VendorIdType DX8Caps::Define_Vendor(unsigned vendor_id)
{
    switch (vendor_id) {
    case 0x10de: return VENDOR_NVIDIA;
    case 0x1002: return VENDOR_ATI;
    case 0x8086: return VENDOR_INTEL;
    default: return VENDOR_UNKNOWN;
    }
}

#include "dx8caps.h"

#include <bgfx/bgfx.h>
#include <cstdio>
#include <cstring>

#include "bgfxrenderer.h"

namespace
{
}

DX8Caps::DX8Caps(const D3DCAPS8& caps, WW3DFormat display_format, const D3DADAPTER_IDENTIFIER8& adapter_id)
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
      VendorId(VENDOR_UNKNOWN)
{
    std::memset(SupportTextureFormat, 0, sizeof(SupportTextureFormat));
    std::memset(SupportRenderToTextureFormat, 0, sizeof(SupportRenderToTextureFormat));
    for (int i = 0; i < WW3D_FORMAT_COUNT; ++i) {
        const WW3DFormat format = static_cast<WW3DFormat>(i);
        SupportTextureFormat[i] = BgfxRenderer::Supports_Texture_Format(format);
        SupportRenderToTextureFormat[i] = BgfxRenderer::Supports_Render_Target_Format(format);
    }

    Caps = caps;
    Compute_Caps(display_format, adapter_id);
}

void DX8Caps::Compute_Caps(WW3DFormat, const D3DADAPTER_IDENTIFIER8& adapter_id)
{
    DriverDLL = adapter_id.Driver;
    DeviceId = adapter_id.DeviceId;
    DriverBuildVersion = adapter_id.DriverVersion.LowPart;
    VendorId = Define_Vendor(adapter_id.VendorId);
    CapsLog = adapter_id.Description;
    CompactLog = adapter_id.Description;

    SupportTnL = (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == D3DDEVCAPS_HWTRANSFORMANDLIGHT;
    SupportZBias = (Caps.RasterCaps & D3DPRASTERCAPS_ZBIAS) == D3DPRASTERCAPS_ZBIAS;
    SupportAnisotropicFiltering =
        (Caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0 &&
        (Caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0;
    SupportBumpEnvmap = (Caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP) != 0;
    SupportBumpEnvmapLuminance = (Caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE) != 0;
    SupportDXTC =
        SupportTextureFormat[WW3D_FORMAT_DXT1] ||
        SupportTextureFormat[WW3D_FORMAT_DXT2] ||
        SupportTextureFormat[WW3D_FORMAT_DXT3] ||
        SupportTextureFormat[WW3D_FORMAT_DXT4] ||
        SupportTextureFormat[WW3D_FORMAT_DXT5];
    MaxTexturesPerPass = static_cast<int>(Caps.MaxSimultaneousTextures);
    VertexShaderVersion = Caps.VertexShaderVersion;
    PixelShaderVersion = Caps.PixelShaderVersion;
}

bool DX8Caps::Is_Valid_Display_Format(int width, int height, WW3DFormat format)
{
    return width > 0 && height > 0 && BgfxRenderer::Supports_Texture_Format(format);
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

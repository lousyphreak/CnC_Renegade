#include "bgfxrenderer.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "surfaceclass.h"
#include "texture.h"
#include "wwdebug.h"

bool BgfxRenderer::IsInitted = false;
uint32_t BgfxRenderer::Width = 0;
uint32_t BgfxRenderer::Height = 0;
uint32_t BgfxRenderer::ActiveWidth = 0;
uint32_t BgfxRenderer::ActiveHeight = 0;
uint32_t BgfxRenderer::BitDepth = 32;
bool BgfxRenderer::Windowed = true;
void *BgfxRenderer::WindowHandle = nullptr;
bgfx::PlatformData BgfxRenderer::PlatformData = {};
bgfx::VertexLayout BgfxRenderer::PosColorTexcoordLayout;
bgfx::TextureHandle BgfxRenderer::WhiteTexture = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::ColorTextureUniform = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle BgfxRenderer::ColorTextureProgram = BGFX_INVALID_HANDLE;
Matrix4 BgfxRenderer::CurrentViewMatrix(true);
Matrix4 BgfxRenderer::CurrentProjectionMatrix(true);

namespace
{
constexpr uint16_t MainViewId = 0;
constexpr uint16_t OverlayViewId = 1;
const float IdentityMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f};

const char *Get_Shader_Profile_Directory()
{
    switch (bgfx::getRendererType()) {
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12:
        return "dxbc";
    case bgfx::RendererType::Metal:
        return "metal";
    case bgfx::RendererType::OpenGLES:
        return "essl";
    case bgfx::RendererType::OpenGL:
        return "glsl";
    case bgfx::RendererType::Vulkan:
        return "spirv";
    default:
        return "spirv";
    }
}

uint64_t Convert_Blend_Factor(ShaderClass::SrcBlendFuncType factor)
{
    switch (factor) {
    case ShaderClass::SRCBLEND_ZERO:
        return BGFX_STATE_BLEND_ZERO;
    case ShaderClass::SRCBLEND_ONE:
        return BGFX_STATE_BLEND_ONE;
    case ShaderClass::SRCBLEND_SRC_ALPHA:
        return BGFX_STATE_BLEND_SRC_ALPHA;
    case ShaderClass::SRCBLEND_ONE_MINUS_SRC_ALPHA:
        return BGFX_STATE_BLEND_INV_SRC_ALPHA;
    default:
        return BGFX_STATE_BLEND_ONE;
    }
}

uint64_t Convert_Blend_Factor(ShaderClass::DstBlendFuncType factor)
{
    switch (factor) {
    case ShaderClass::DSTBLEND_ZERO:
        return BGFX_STATE_BLEND_ZERO;
    case ShaderClass::DSTBLEND_ONE:
        return BGFX_STATE_BLEND_ONE;
    case ShaderClass::DSTBLEND_SRC_COLOR:
        return BGFX_STATE_BLEND_SRC_COLOR;
    case ShaderClass::DSTBLEND_ONE_MINUS_SRC_COLOR:
        return BGFX_STATE_BLEND_INV_SRC_COLOR;
    case ShaderClass::DSTBLEND_SRC_ALPHA:
        return BGFX_STATE_BLEND_SRC_ALPHA;
    case ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA:
        return BGFX_STATE_BLEND_INV_SRC_ALPHA;
    default:
        return BGFX_STATE_BLEND_ZERO;
    }
}

uint64_t Convert_Depth_Test(ShaderClass::DepthCompareType compare)
{
    switch (compare) {
    case ShaderClass::PASS_NEVER:
        return BGFX_STATE_DEPTH_TEST_NEVER;
    case ShaderClass::PASS_LESS:
        return BGFX_STATE_DEPTH_TEST_LESS;
    case ShaderClass::PASS_EQUAL:
        return BGFX_STATE_DEPTH_TEST_EQUAL;
    case ShaderClass::PASS_LEQUAL:
        return BGFX_STATE_DEPTH_TEST_LEQUAL;
    case ShaderClass::PASS_GREATER:
        return BGFX_STATE_DEPTH_TEST_GREATER;
    case ShaderClass::PASS_NOTEQUAL:
        return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
    case ShaderClass::PASS_GEQUAL:
        return BGFX_STATE_DEPTH_TEST_GEQUAL;
    case ShaderClass::PASS_ALWAYS:
        return BGFX_STATE_DEPTH_TEST_ALWAYS;
    default:
        return BGFX_STATE_DEPTH_TEST_LEQUAL;
    }
}

bool Query_Native_Window(SDL_Window *window, bgfx::PlatformData &platform_data)
{
    if (window == nullptr) {
        return false;
    }

    const SDL_PropertiesID window_properties = SDL_GetWindowProperties(window);
    if (window_properties == 0) {
        return false;
    }

    if (void *wayland_display = SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr)) {
        void *wayland_surface = SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        if (wayland_surface == nullptr) {
            return false;
        }

        platform_data.ndt = wayland_display;
        platform_data.nwh = wayland_surface;
        return true;
    }

    if (void *x11_display = SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr)) {
        const uintptr_t x11_window = static_cast<uintptr_t>(SDL_GetNumberProperty(window_properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
        if (x11_window == 0) {
            return false;
        }

        platform_data.ndt = x11_display;
        platform_data.nwh = reinterpret_cast<void *>(x11_window);
        return true;
    }

    if (void *win32_window = SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr)) {
        platform_data.nwh = win32_window;
        return true;
    }

    if (void *cocoa_window = SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr)) {
        platform_data.nwh = cocoa_window;
        return true;
    }

    return false;
}

uint8_t Expand_4_To_8(uint8_t value)
{
    return static_cast<uint8_t>((value << 4) | value);
}

uint8_t Expand_5_To_8(uint8_t value)
{
    return static_cast<uint8_t>((value << 3) | (value >> 2));
}

bool Convert_Surface_Copy_To_BGRA8(
    const SurfaceClass::SurfaceDescription &description,
    const uint8_t *source_pixels,
    std::vector<uint8_t> &converted_pixels)
{
    if (source_pixels == nullptr || description.Width == 0 || description.Height == 0) {
        return false;
    }

    const size_t pixel_count = static_cast<size_t>(description.Width) * static_cast<size_t>(description.Height);
    converted_pixels.resize(pixel_count * 4);

    for (size_t pixel_index = 0; pixel_index < pixel_count; ++pixel_index) {
        const uint8_t *source = source_pixels;
        uint8_t *destination = &converted_pixels[pixel_index * 4];

        switch (description.Format) {
        case WW3D_FORMAT_A8R8G8B8:
            destination[0] = source[0];
            destination[1] = source[1];
            destination[2] = source[2];
            destination[3] = source[3];
            source_pixels += 4;
            break;
        case WW3D_FORMAT_X8R8G8B8:
            destination[0] = source[0];
            destination[1] = source[1];
            destination[2] = source[2];
            destination[3] = 0xff;
            source_pixels += 4;
            break;
        case WW3D_FORMAT_R8G8B8:
            destination[0] = source[0];
            destination[1] = source[1];
            destination[2] = source[2];
            destination[3] = 0xff;
            source_pixels += 3;
            break;
        case WW3D_FORMAT_R5G6B5: {
            const uint16_t packed = static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
            destination[0] = Expand_5_To_8(static_cast<uint8_t>(packed & 0x1f));
            destination[1] = static_cast<uint8_t>(((packed >> 5) & 0x3f) * 255 / 63);
            destination[2] = Expand_5_To_8(static_cast<uint8_t>((packed >> 11) & 0x1f));
            destination[3] = 0xff;
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_X1R5G5B5:
        case WW3D_FORMAT_A1R5G5B5: {
            const uint16_t packed = static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
            destination[0] = Expand_5_To_8(static_cast<uint8_t>(packed & 0x1f));
            destination[1] = Expand_5_To_8(static_cast<uint8_t>((packed >> 5) & 0x1f));
            destination[2] = Expand_5_To_8(static_cast<uint8_t>((packed >> 10) & 0x1f));
            destination[3] = (description.Format == WW3D_FORMAT_A1R5G5B5 && (packed & 0x8000u) == 0) ? 0x00 : 0xff;
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_A4R4G4B4: {
            const uint16_t packed = static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
            destination[0] = Expand_4_To_8(static_cast<uint8_t>(packed & 0x000f));
            destination[1] = Expand_4_To_8(static_cast<uint8_t>((packed >> 4) & 0x000f));
            destination[2] = Expand_4_To_8(static_cast<uint8_t>((packed >> 8) & 0x000f));
            destination[3] = Expand_4_To_8(static_cast<uint8_t>((packed >> 12) & 0x000f));
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_X4R4G4B4: {
            const uint16_t packed = static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
            destination[0] = Expand_4_To_8(static_cast<uint8_t>(packed & 0x000f));
            destination[1] = Expand_4_To_8(static_cast<uint8_t>((packed >> 4) & 0x000f));
            destination[2] = Expand_4_To_8(static_cast<uint8_t>((packed >> 8) & 0x000f));
            destination[3] = 0xff;
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_A8: {
            destination[0] = 0xff;
            destination[1] = 0xff;
            destination[2] = 0xff;
            destination[3] = source[0];
            source_pixels += 1;
            break;
        }
        case WW3D_FORMAT_L8: {
            destination[0] = source[0];
            destination[1] = source[0];
            destination[2] = source[0];
            destination[3] = 0xff;
            source_pixels += 1;
            break;
        }
        case WW3D_FORMAT_A8L8: {
            destination[0] = source[0];
            destination[1] = source[0];
            destination[2] = source[0];
            destination[3] = source[1];
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_A4L4: {
            const uint8_t luminance = Expand_4_To_8(static_cast<uint8_t>(source[0] & 0x0f));
            destination[0] = luminance;
            destination[1] = luminance;
            destination[2] = luminance;
            destination[3] = Expand_4_To_8(static_cast<uint8_t>((source[0] >> 4) & 0x0f));
            source_pixels += 1;
            break;
        }
        case WW3D_FORMAT_R3G3B2: {
            const uint8_t packed = source[0];
            destination[0] = static_cast<uint8_t>((packed & 0x03) * 255 / 3);
            destination[1] = static_cast<uint8_t>(((packed >> 2) & 0x07) * 255 / 7);
            destination[2] = static_cast<uint8_t>(((packed >> 5) & 0x07) * 255 / 7);
            destination[3] = 0xff;
            source_pixels += 1;
            break;
        }
        case WW3D_FORMAT_A8R3G3B2: {
            const uint8_t packed_color = source[0];
            destination[0] = static_cast<uint8_t>((packed_color & 0x03) * 255 / 3);
            destination[1] = static_cast<uint8_t>(((packed_color >> 2) & 0x07) * 255 / 7);
            destination[2] = static_cast<uint8_t>(((packed_color >> 5) & 0x07) * 255 / 7);
            destination[3] = source[1];
            source_pixels += 2;
            break;
        }
        default:
            return false;
        }
    }

    return true;
}

bool Is_Compressed_Format(WW3DFormat format)
{
    switch (format) {
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

uint32_t Get_Compressed_Level_Size(WW3DFormat format, uint32_t width, uint32_t height)
{
    const uint32_t block_width = (width + 3u) / 4u;
    const uint32_t block_height = (height + 3u) / 4u;

    switch (format) {
    case WW3D_FORMAT_DXT1:
        return block_width * block_height * 8u;
    case WW3D_FORMAT_DXT2:
    case WW3D_FORMAT_DXT3:
    case WW3D_FORMAT_DXT4:
    case WW3D_FORMAT_DXT5:
        return block_width * block_height * 16u;
    default:
        return 0u;
    }
}

bool Get_Bgfx_Texture_Format(WW3DFormat format, bgfx::TextureFormat::Enum &bgfx_format, bool &direct_copy)
{
    direct_copy = true;

    switch (format) {
    case WW3D_FORMAT_A8R8G8B8:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        return true;
    case WW3D_FORMAT_X8R8G8B8:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        return true;
    case WW3D_FORMAT_DXT1:
        bgfx_format = bgfx::TextureFormat::BC1;
        return true;
    case WW3D_FORMAT_DXT2:
    case WW3D_FORMAT_DXT3:
        bgfx_format = bgfx::TextureFormat::BC2;
        return true;
    case WW3D_FORMAT_DXT4:
    case WW3D_FORMAT_DXT5:
        bgfx_format = bgfx::TextureFormat::BC3;
        return true;
    default:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    }
}
}

bool BgfxRenderer::Init(void *window_handle, bool lite)
{
    if (lite) {
        return true;
    }

    if (IsInitted) {
        return true;
    }

    if (!Update_Platform_Window(window_handle)) {
        WWDEBUG_SAY(("BgfxRenderer::Init failed to query native window data\n"));
        return false;
    }

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.platformData = PlatformData;
    init.resolution.width = Width;
    init.resolution.height = Height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        WWDEBUG_SAY(("BgfxRenderer::Init bgfx::init failed\n"));
        return false;
    }

    if (!Init_Render_Resources()) {
        WWDEBUG_SAY(("BgfxRenderer::Init failed to initialize renderer resources\n"));
        bgfx::shutdown();
        return false;
    }

    bgfx::setViewClear(MainViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    bgfx::setViewRect(MainViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    ActiveWidth = Width;
    ActiveHeight = Height;

    IsInitted = true;
    return true;
}

void BgfxRenderer::Shutdown()
{
    if (!IsInitted) {
        return;
    }

    Shutdown_Render_Resources();
    bgfx::shutdown();
    Width = 0;
    Height = 0;
    ActiveWidth = 0;
    ActiveHeight = 0;
    BitDepth = 32;
    Windowed = true;
    WindowHandle = nullptr;
    PlatformData = {};
    IsInitted = false;
}

bool BgfxRenderer::Reset()
{
    if (!IsInitted) {
        return false;
    }

    bgfx::reset(Width, Height, BGFX_RESET_VSYNC);
    ActiveWidth = Width;
    ActiveHeight = Height;
    bgfx::setViewFrameBuffer(MainViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewFrameBuffer(OverlayViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(MainViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    return true;
}

bool BgfxRenderer::Begin_Frame(bool clear_color, bool clear_depth, float red, float green, float blue)
{
    if (!IsInitted) {
        return false;
    }

    Apply_Clear(clear_color, clear_depth, red, green, blue);
    return true;
}

void BgfxRenderer::Clear_View(bool clear_color, bool clear_depth, const Vector3 &color)
{
    if (!IsInitted) {
        return;
    }

    Apply_Clear(clear_color, clear_depth, color.X, color.Y, color.Z);
}

void BgfxRenderer::Set_Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!IsInitted) {
        return;
    }

    bgfx::setViewRect(
        MainViewId,
        static_cast<uint16_t>(x),
        static_cast<uint16_t>(y),
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height));
}

bool BgfxRenderer::Set_Render_Target(TextureClass &texture)
{
    if (!IsInitted) {
        return false;
    }

    bgfx::FrameBufferHandle frame_buffer = texture.Get_Bgfx_Frame_Buffer();
    if (!bgfx::isValid(frame_buffer)) {
        return false;
    }

    ActiveWidth = static_cast<uint32_t>(texture.Get_Width());
    ActiveHeight = static_cast<uint32_t>(texture.Get_Height());
    bgfx::setViewFrameBuffer(MainViewId, frame_buffer);
    bgfx::setViewFrameBuffer(OverlayViewId, frame_buffer);
    bgfx::setViewRect(MainViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    return true;
}

void BgfxRenderer::Reset_Render_Target()
{
    if (!IsInitted) {
        return;
    }

    ActiveWidth = Width;
    ActiveHeight = Height;
    bgfx::setViewFrameBuffer(MainViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewFrameBuffer(OverlayViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(MainViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
}

void BgfxRenderer::Set_Camera(const Matrix3D &view, const Matrix4 &projection)
{
    if (!IsInitted) {
        return;
    }

    CurrentViewMatrix = Matrix4(view);
    CurrentProjectionMatrix = projection;
    bgfx::setViewTransform(MainViewId, &CurrentViewMatrix[0][0], &CurrentProjectionMatrix[0][0]);
}

void BgfxRenderer::Prepare_Overlay_View()
{
    if (!IsInitted) {
        return;
    }

    bgfx::setViewMode(OverlayViewId, bgfx::ViewMode::Sequential);
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    bgfx::setViewTransform(OverlayViewId, IdentityMatrix, IdentityMatrix);
    bgfx::setViewClear(OverlayViewId, 0, 0, 1.0f, 0);
}

void BgfxRenderer::End_Frame()
{
    if (!IsInitted) {
        return;
    }

    bgfx::frame();
}

void BgfxRenderer::Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed)
{
    width = static_cast<int>(ActiveWidth);
    height = static_cast<int>(ActiveHeight);
    bits = static_cast<int>(BitDepth);
    windowed = Windowed;
}

void BgfxRenderer::Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed)
{
    Get_Render_Target_Resolution(width, height, bits, windowed);
}

const bgfx::VertexLayout &BgfxRenderer::Get_Pos_Color_Texcoord_Layout()
{
    return PosColorTexcoordLayout;
}

uint16_t BgfxRenderer::Get_Main_View_Id()
{
    return MainViewId;
}

uint16_t BgfxRenderer::Get_Overlay_View_Id()
{
    return OverlayViewId;
}

const Matrix4 &BgfxRenderer::Get_Current_View_Matrix()
{
    return CurrentViewMatrix;
}

const Matrix4 &BgfxRenderer::Get_Current_Projection_Matrix()
{
    return CurrentProjectionMatrix;
}

bgfx::TextureHandle BgfxRenderer::Get_White_Texture()
{
    return WhiteTexture;
}

bgfx::UniformHandle BgfxRenderer::Get_Color_Texture_Uniform()
{
    return ColorTextureUniform;
}

bgfx::ProgramHandle BgfxRenderer::Get_Color_Texture_Program()
{
    return ColorTextureProgram;
}

bgfx::TextureHandle BgfxRenderer::Create_Texture_From_Surface(SurfaceClass &surface)
{
    int width = 0;
    int height = 0;
    int source_pixel_size = 0;
    uint8_t *source_pixels = surface.CreateCopy(&width, &height, &source_pixel_size, false);
    if (source_pixels == nullptr || width <= 0 || height <= 0) {
        delete[] source_pixels;
        return BGFX_INVALID_HANDLE;
    }

    SurfaceClass::SurfaceDescription description;
    surface.Get_Description(description);

    std::vector<uint8_t> converted_pixels;
    const bool converted = Convert_Surface_Copy_To_BGRA8(description, source_pixels, converted_pixels);
    delete[] source_pixels;

    if (!converted) {
        WWDEBUG_SAY(("BgfxRenderer::Create_Texture_From_Surface unsupported surface format %d\n", description.Format));
        return BGFX_INVALID_HANDLE;
    }

    const bgfx::Memory *texture_memory = bgfx::copy(converted_pixels.data(), static_cast<uint32_t>(converted_pixels.size()));
    return bgfx::createTexture2D(
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height),
        false,
        1,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_NONE,
        texture_memory);
}

bgfx::TextureHandle BgfxRenderer::Create_Texture(TextureClass &texture)
{
    bgfx::TextureFormat::Enum texture_format = bgfx::TextureFormat::BGRA8;
    bool direct_copy = false;
    if (texture.Is_Render_Target_Texture()) {
        if (!Get_Bgfx_Texture_Format(texture.Get_Texture_Format(), texture_format, direct_copy)) {
            return BGFX_INVALID_HANDLE;
        }

        bgfx::TextureHandle handle = bgfx::createTexture2D(
            static_cast<uint16_t>(texture.Get_Width()),
            static_cast<uint16_t>(texture.Get_Height()),
            false,
            1,
            texture_format,
            BGFX_TEXTURE_RT);
        if (!bgfx::isValid(handle)) {
            return BGFX_INVALID_HANDLE;
        }

        bgfx::Attachment attachment;
        attachment.init(handle);
        texture.BgfxFrameBuffer = bgfx::createFrameBuffer(1, &attachment, false);
        if (!bgfx::isValid(texture.BgfxFrameBuffer)) {
            bgfx::destroy(handle);
            return BGFX_INVALID_HANDLE;
        }
        return handle;
    }

    SurfaceClass *base_surface = texture.Get_Surface_Level(0);
    if (base_surface == nullptr) {
        return BGFX_INVALID_HANDLE;
    }

    SurfaceClass::SurfaceDescription base_description;
    base_surface->Get_Description(base_description);

    if (!Get_Bgfx_Texture_Format(base_description.Format, texture_format, direct_copy)) {
        base_surface->Release_Ref();
        return BGFX_INVALID_HANDLE;
    }

    const unsigned mip_level_count = texture.Get_Mip_Level_Count();
    const bool has_mips = mip_level_count > 1;
    bgfx::TextureHandle handle = bgfx::createTexture2D(
        static_cast<uint16_t>(base_description.Width),
        static_cast<uint16_t>(base_description.Height),
        has_mips,
        1,
        texture_format,
        BGFX_TEXTURE_NONE);
    base_surface->Release_Ref();

    if (!bgfx::isValid(handle)) {
        return handle;
    }

    for (unsigned level = 0; level < mip_level_count; ++level) {
        SurfaceClass *surface = texture.Get_Surface_Level(level);
        if (surface == nullptr) {
            bgfx::destroy(handle);
            return BGFX_INVALID_HANDLE;
        }

        SurfaceClass::SurfaceDescription description;
        surface->Get_Description(description);

        int width = 0;
        int height = 0;
        int source_pixel_size = 0;
        uint8_t *source_pixels = surface->CreateCopy(&width, &height, &source_pixel_size, false);
        surface->Release_Ref();

        if (source_pixels == nullptr || width <= 0 || height <= 0) {
            delete[] source_pixels;
            bgfx::destroy(handle);
            return BGFX_INVALID_HANDLE;
        }

        const bgfx::Memory *memory = nullptr;
        std::vector<uint8_t> converted_pixels;
        if (direct_copy) {
            const uint32_t data_size = Is_Compressed_Format(description.Format)
                ? Get_Compressed_Level_Size(description.Format, static_cast<uint32_t>(width), static_cast<uint32_t>(height))
                : static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * static_cast<uint32_t>(source_pixel_size);
            memory = bgfx::copy(source_pixels, data_size);
        } else {
            if (!Convert_Surface_Copy_To_BGRA8(description, source_pixels, converted_pixels)) {
                delete[] source_pixels;
                bgfx::destroy(handle);
                return BGFX_INVALID_HANDLE;
            }
            memory = bgfx::copy(converted_pixels.data(), static_cast<uint32_t>(converted_pixels.size()));
        }

        delete[] source_pixels;
        bgfx::updateTexture2D(
            handle,
            0,
            static_cast<uint8_t>(level),
            0,
            0,
            static_cast<uint16_t>(width),
            static_cast<uint16_t>(height),
            memory);
    }

    return handle;
}

bgfx::ProgramHandle BgfxRenderer::Load_Program(const char *vertex_shader_name, const char *fragment_shader_name)
{
    bgfx::ShaderHandle vertex_shader = Load_Shader(vertex_shader_name);
    if (!bgfx::isValid(vertex_shader)) {
        return BGFX_INVALID_HANDLE;
    }

    bgfx::ShaderHandle fragment_shader = Load_Shader(fragment_shader_name);
    if (!bgfx::isValid(fragment_shader)) {
        bgfx::destroy(vertex_shader);
        return BGFX_INVALID_HANDLE;
    }

    return bgfx::createProgram(vertex_shader, fragment_shader, true);
}

void BgfxRenderer::Destroy_Program(bgfx::ProgramHandle &program)
{
    if (bgfx::isValid(program)) {
        bgfx::destroy(program);
        program = BGFX_INVALID_HANDLE;
    }
}

uint64_t BgfxRenderer::Build_Render_State(const ShaderClass &shader)
{
    uint64_t state = BGFX_STATE_MSAA;

    if (shader.Get_Color_Mask() == ShaderClass::COLOR_WRITE_ENABLE) {
        state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
    }

    if (shader.Get_Depth_Mask() == ShaderClass::DEPTH_WRITE_ENABLE) {
        state |= BGFX_STATE_WRITE_Z;
    }

    state |= Convert_Depth_Test(shader.Get_Depth_Compare());

    if (shader.Get_Cull_Mode() == ShaderClass::CULL_MODE_ENABLE) {
        state |= BGFX_STATE_CULL_CW;
    }

    if (shader.Get_Src_Blend_Func() != ShaderClass::SRCBLEND_ONE
        || shader.Get_Dst_Blend_Func() != ShaderClass::DSTBLEND_ZERO) {
        state |= BGFX_STATE_BLEND_FUNC(
            Convert_Blend_Factor(shader.Get_Src_Blend_Func()),
            Convert_Blend_Factor(shader.Get_Dst_Blend_Func()));
    }

    return state;
}

bool BgfxRenderer::Init_Render_Resources()
{
    PosColorTexcoordLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    if (!bgfx::isValid(ColorTextureUniform)) {
        ColorTextureUniform = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    }

    if (!bgfx::isValid(ColorTextureUniform)) {
        return false;
    }

    if (!bgfx::isValid(WhiteTexture)) {
        constexpr uint32_t white_pixel = 0xffffffffu;
        const bgfx::Memory *texture_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
        WhiteTexture = bgfx::createTexture2D(
            1,
            1,
            false,
            1,
            bgfx::TextureFormat::BGRA8,
            BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
            texture_memory);
    }

    if (!bgfx::isValid(WhiteTexture)) {
        return false;
    }

    if (!bgfx::isValid(ColorTextureProgram)) {
        ColorTextureProgram = Load_Program("vs_color_tex", "fs_color_tex");
    }

    return bgfx::isValid(ColorTextureProgram);
}

void BgfxRenderer::Shutdown_Render_Resources()
{
    Destroy_Program(ColorTextureProgram);

    if (bgfx::isValid(WhiteTexture)) {
        bgfx::destroy(WhiteTexture);
        WhiteTexture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(ColorTextureUniform)) {
        bgfx::destroy(ColorTextureUniform);
        ColorTextureUniform = BGFX_INVALID_HANDLE;
    }
}

bool BgfxRenderer::Update_Platform_Window(void *window_handle)
{
    SDL_Window *window = reinterpret_cast<SDL_Window *>(window_handle);
    bgfx::PlatformData platform_data = {};
    if (!Query_Native_Window(window, platform_data)) {
        return false;
    }

    uint32_t drawable_width = 0;
    uint32_t drawable_height = 0;
    if (!Query_Drawable_Size(window_handle, drawable_width, drawable_height)) {
        return false;
    }

    WindowHandle = window_handle;
    Width = drawable_width;
    Height = drawable_height;
    const SDL_PixelFormatDetails *pixel_details = SDL_GetPixelFormatDetails(SDL_GetWindowPixelFormat(window));
    if (pixel_details != nullptr && pixel_details->bits_per_pixel > 0) {
        BitDepth = pixel_details->bits_per_pixel;
    } else {
        BitDepth = 32;
    }
    Windowed = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == 0;
    PlatformData = platform_data;
    bgfx::setPlatformData(platform_data);
    return true;
}

bool BgfxRenderer::Query_Drawable_Size(void *window_handle, uint32_t &width, uint32_t &height)
{
    SDL_Window *window = reinterpret_cast<SDL_Window *>(window_handle);
    int pixel_width = 0;
    int pixel_height = 0;
    if (window == nullptr || !SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height)) {
        return false;
    }

    if (pixel_width <= 0 || pixel_height <= 0) {
        return false;
    }

    width = static_cast<uint32_t>(pixel_width);
    height = static_cast<uint32_t>(pixel_height);
    return true;
}

void BgfxRenderer::Apply_Clear(bool clear_color, bool clear_depth, float red, float green, float blue)
{
    uint16_t clear_flags = 0;
    if (clear_color) {
        clear_flags |= BGFX_CLEAR_COLOR;
    }
    if (clear_depth) {
        clear_flags |= BGFX_CLEAR_DEPTH;
    }

    const uint8_t clear_r = static_cast<uint8_t>(red * 255.0f);
    const uint8_t clear_g = static_cast<uint8_t>(green * 255.0f);
    const uint8_t clear_b = static_cast<uint8_t>(blue * 255.0f);
    const uint32_t clear_rgba = (static_cast<uint32_t>(clear_r) << 24)
        | (static_cast<uint32_t>(clear_g) << 16)
        | (static_cast<uint32_t>(clear_b) << 8)
        | 0xffu;

    bgfx::setViewClear(MainViewId, clear_flags, clear_rgba, 1.0f, 0);
    bgfx::touch(MainViewId);
}

bgfx::ShaderHandle BgfxRenderer::Load_Shader(const char *shader_name)
{
    if (shader_name == nullptr || shader_name[0] == '\0') {
        return BGFX_INVALID_HANDLE;
    }

#ifndef RENEGADE_WW3D2_SHADER_DIR
#define RENEGADE_WW3D2_SHADER_DIR ""
#endif

    const char *profile_directory = Get_Shader_Profile_Directory();
    const std::string shader_directory = std::string(RENEGADE_WW3D2_SHADER_DIR)
        + "/"
        + profile_directory
        + "/";

    std::string shader_path = shader_directory + shader_name + ".bin";
    std::FILE *shader_file = std::fopen(shader_path.c_str(), "rb");
    if (shader_file == nullptr) {
        shader_path = shader_directory + shader_name + ".sc.bin";
        shader_file = std::fopen(shader_path.c_str(), "rb");
    }
    if (shader_file == nullptr) {
        WWDEBUG_SAY(("BgfxRenderer::Load_Shader unable to open '%s'\n", shader_path.c_str()));
        return BGFX_INVALID_HANDLE;
    }

    std::fseek(shader_file, 0, SEEK_END);
    const long shader_size = std::ftell(shader_file);
    std::fseek(shader_file, 0, SEEK_SET);
    if (shader_size <= 0) {
        std::fclose(shader_file);
        WWDEBUG_SAY(("BgfxRenderer::Load_Shader invalid shader size for '%s'\n", shader_path.c_str()));
        return BGFX_INVALID_HANDLE;
    }

    std::vector<uint8_t> shader_data(static_cast<size_t>(shader_size));
    const size_t read_size = std::fread(shader_data.data(), 1, shader_data.size(), shader_file);
    std::fclose(shader_file);
    if (read_size != shader_data.size()) {
        WWDEBUG_SAY(("BgfxRenderer::Load_Shader short read for '%s'\n", shader_path.c_str()));
        return BGFX_INVALID_HANDLE;
    }

    const bgfx::Memory *shader_memory = bgfx::copy(shader_data.data(), static_cast<uint32_t>(shader_data.size()));
    return bgfx::createShader(shader_memory);
}

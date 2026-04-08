#include "bgfxrenderer.h"

#include <cstdint>

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "wwdebug.h"

bool BgfxRenderer::IsInitted = false;
uint32_t BgfxRenderer::Width = 0;
uint32_t BgfxRenderer::Height = 0;

namespace
{
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
}

bool BgfxRenderer::Init(void *window_handle, bool lite)
{
    if (lite || IsInitted) {
        return lite;
    }

    if (!Update_Platform_Window(window_handle)) {
        WWDEBUG_SAY(("BgfxRenderer::Init failed to query native window data\n"));
        return false;
    }

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.resolution.width = Width;
    init.resolution.height = Height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        WWDEBUG_SAY(("BgfxRenderer::Init bgfx::init failed\n"));
        return false;
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));

    IsInitted = true;
    return true;
}

void BgfxRenderer::Shutdown()
{
    if (!IsInitted) {
        return;
    }

    bgfx::shutdown();
    Width = 0;
    Height = 0;
    IsInitted = false;
}

bool BgfxRenderer::Reset()
{
    if (!IsInitted) {
        return false;
    }

    bgfx::reset(Width, Height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    return true;
}

bool BgfxRenderer::Begin_Frame(bool clear_color, bool clear_depth, float red, float green, float blue)
{
    if (!IsInitted) {
        return false;
    }

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

    bgfx::setViewClear(0, clear_flags, clear_rgba, 1.0f, 0);
    bgfx::touch(0);
    return true;
}

void BgfxRenderer::Set_Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!IsInitted) {
        return;
    }

    bgfx::setViewRect(
        0,
        static_cast<uint16_t>(x),
        static_cast<uint16_t>(y),
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height));
}

void BgfxRenderer::Set_Camera(const Matrix3D &view, const Matrix4 &projection)
{
    if (!IsInitted) {
        return;
    }

    const Matrix4 view_matrix(view);
    bgfx::setViewTransform(0, &view_matrix[0][0], &projection[0][0]);
}

void BgfxRenderer::End_Frame()
{
    if (!IsInitted) {
        return;
    }

    bgfx::frame();
}

bool BgfxRenderer::Update_Platform_Window(void *window_handle)
{
    bgfx::PlatformData platform_data = {};
    SDL_Window *window = reinterpret_cast<SDL_Window *>(window_handle);
    if (!Query_Native_Window(window, platform_data)) {
        return false;
    }

    uint32_t drawable_width = 0;
    uint32_t drawable_height = 0;
    if (!Query_Drawable_Size(window_handle, drawable_width, drawable_height)) {
        return false;
    }

    Width = drawable_width;
    Height = drawable_height;
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

#include "bgfxrenderer.h"

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <algorithm>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "dx8wrapper.h"
#include "rawfile.h"
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
bgfx::VertexLayout BgfxRenderer::FixedFunctionLayout;
bgfx::TextureHandle BgfxRenderer::WhiteTexture = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::Texture0Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::Texture1Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionConfig1Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionFogColorUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionTextureFactorUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionStage0ColorUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionStage0AlphaUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionStage1ColorUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionStage1AlphaUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionBumpEnvMatrixUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionBumpEnvParamsUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionMaterialAmbientUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionMaterialDiffuseUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionMaterialSpecularUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionMaterialEmissiveUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionMaterialParamsUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionCameraPositionUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionSceneAmbientUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightingConfigUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionMaterialSourceConfigUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightPositionsUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightDirectionsUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightAmbientUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightDiffuseUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightSpecularUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightAttenuationUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::FixedFunctionLightSpotParamsUniform = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle BgfxRenderer::FixedFunctionProgram = BGFX_INVALID_HANDLE;
Matrix4 BgfxRenderer::CurrentViewMatrix(true);
Matrix4 BgfxRenderer::CurrentProjectionMatrix(true);

namespace
{
struct ViewTransformState
{
    Matrix4 View;
    Matrix4 Projection;
    uint16_t ViewId = 0;
};

struct CapturedMovieFrame
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t Pitch = 0;
    bool YFlip = false;
    std::vector<uint8_t> Pixels;
    uint64_t Sequence = 0;
};

constexpr uint16_t ClearViewId = 0;
constexpr uint16_t MainViewBaseId = 1;
constexpr uint16_t MaxMainViewId = 254;
constexpr uint16_t OverlayViewId = 255;
const float IdentityMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f};
bool AutoScreenshotEnabled = false;
bool AutoScreenshotRequested = false;
uint32_t AutoScreenshotDelayMs = 0;
uint32_t AutoScreenshotStartTicks = 0;
std::string AutoScreenshotPath;
uint16_t CurrentMainViewId = MainViewBaseId;
uint16_t NextMainViewId = MainViewBaseId;
uint32_t PendingViewportX = 0;
uint32_t PendingViewportY = 0;
uint32_t PendingViewportWidth = 0;
uint32_t PendingViewportHeight = 0;
bgfx::FrameBufferHandle CurrentFrameBuffer = BGFX_INVALID_HANDLE;
std::vector<ViewTransformState> ConfiguredViews;
std::mutex MovieCaptureMutex;
bool MovieCaptureActive = false;
bool MovieCaptureConfigured = false;
bgfx::TextureFormat::Enum MovieCaptureFormat = bgfx::TextureFormat::Count;
float MovieCaptureFrameRate = 0.0f;
std::string MovieCaptureBasePath;
uint32_t MovieCaptureFrameIndex = 0;
uint64_t MovieCaptureSequence = 0;
uint64_t MovieCaptureConsumedSequence = 0;
CapturedMovieFrame LatestMovieFrame;

bool Matrices_Are_Equal(const Matrix4 &a, const Matrix4 &b)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            if (a[row][column] != b[row][column]) {
                return false;
            }
        }
    }
    return true;
}

void Extract_Camera_Position(const Matrix4 &view_matrix, float *camera_position)
{
    const float tx = view_matrix[0][3];
    const float ty = view_matrix[1][3];
    const float tz = view_matrix[2][3];

    camera_position[0] = -(view_matrix[0][0] * tx + view_matrix[1][0] * ty + view_matrix[2][0] * tz);
    camera_position[1] = -(view_matrix[0][1] * tx + view_matrix[1][1] * ty + view_matrix[2][1] * tz);
    camera_position[2] = -(view_matrix[0][2] * tx + view_matrix[1][2] * ty + view_matrix[2][2] * tz);
    camera_position[3] = 1.0f;
}

void Reset_Main_View_State(uint32_t width, uint32_t height)
{
    CurrentMainViewId = MainViewBaseId;
    NextMainViewId = MainViewBaseId;
    PendingViewportX = 0;
    PendingViewportY = 0;
    PendingViewportWidth = width;
    PendingViewportHeight = height;
    ConfiguredViews.clear();
}

uint16_t Acquire_Main_View()
{
    if (NextMainViewId > MaxMainViewId) {
        return CurrentMainViewId;
    }

    CurrentMainViewId = NextMainViewId++;
    bgfx::setViewMode(CurrentMainViewId, bgfx::ViewMode::Sequential);
    bgfx::setViewFrameBuffer(CurrentMainViewId, CurrentFrameBuffer);
    bgfx::setViewRect(
        CurrentMainViewId,
        static_cast<uint16_t>(PendingViewportX),
        static_cast<uint16_t>(PendingViewportY),
        static_cast<uint16_t>(PendingViewportWidth),
        static_cast<uint16_t>(PendingViewportHeight));
    bgfx::setViewClear(CurrentMainViewId, 0, 0, 1.0f, 0);
    return CurrentMainViewId;
}

uint16_t Configure_View(const Matrix4 &view, const Matrix4 &projection)
{
    for (const ViewTransformState &configured_view : ConfiguredViews) {
        if (Matrices_Are_Equal(configured_view.View, view) &&
            Matrices_Are_Equal(configured_view.Projection, projection)) {
            CurrentMainViewId = configured_view.ViewId;
            return configured_view.ViewId;
        }
    }

    const uint16_t view_id = Acquire_Main_View();
    const Matrix4 bgfx_view = view.Transpose();
    const Matrix4 bgfx_projection = projection.Transpose();
    bgfx::setViewTransform(view_id, &bgfx_view[0][0], &bgfx_projection[0][0]);
    ConfiguredViews.push_back({view, projection, view_id});
    return view_id;
}

bool Write_BGRA_TGA(const char *file_path, uint32_t width, uint32_t height, uint32_t pitch, const void *data, bool yflip)
{
    if (file_path == nullptr || *file_path == '\0' || data == nullptr || width == 0 || height == 0 || pitch < width * 4u) {
        return false;
    }

    RawFileClass file(file_path);
    if (!file.Create() || !file.Open(FileClass::WRITE)) {
        return false;
    }

    uint8_t header[18] = {};
    header[2] = 2;
    header[12] = static_cast<uint8_t>(width & 0xffu);
    header[13] = static_cast<uint8_t>((width >> 8) & 0xffu);
    header[14] = static_cast<uint8_t>(height & 0xffu);
    header[15] = static_cast<uint8_t>((height >> 8) & 0xffu);
    header[16] = 32;
    header[17] = 8 | 0x20;

    if (file.Write(header, sizeof(header)) != static_cast<int>(sizeof(header))) {
        file.Close();
        return false;
    }

    const uint8_t *source = static_cast<const uint8_t *>(data);
    const int row_bytes = static_cast<int>(width * 4u);

    for (uint32_t row = 0; row < height; ++row) {
        const uint32_t source_row = yflip ? (height - 1u - row) : row;
        if (file.Write(source + source_row * pitch, row_bytes) != row_bytes) {
            file.Close();
            return false;
        }
    }

    file.Close();
    return true;
}

uint32_t Get_Reset_Flags()
{
    std::lock_guard<std::mutex> lock(MovieCaptureMutex);
    return BGFX_RESET_VSYNC | (MovieCaptureActive ? BGFX_RESET_CAPTURE : 0u);
}

bool Convert_Capture_Frame_To_BGRA8(
    bgfx::TextureFormat::Enum format,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    const void *data,
    CapturedMovieFrame &frame)
{
    if (data == nullptr || width == 0 || height == 0) {
        return false;
    }

    const uint8_t *source = static_cast<const uint8_t *>(data);
    frame.Width = width;
    frame.Height = height;

    switch (format) {
    case bgfx::TextureFormat::BGRA8:
        frame.Pitch = pitch;
        frame.Pixels.assign(source, source + static_cast<size_t>(pitch) * height);
        return true;

    case bgfx::TextureFormat::RGBA8:
        frame.Pitch = width * 4u;
        frame.Pixels.resize(static_cast<size_t>(frame.Pitch) * height);
        for (uint32_t row = 0; row < height; ++row) {
            const uint8_t *src_row = source + static_cast<size_t>(row) * pitch;
            uint8_t *dst_row = frame.Pixels.data() + static_cast<size_t>(row) * frame.Pitch;
            for (uint32_t column = 0; column < width; ++column) {
                const uint8_t *src_pixel = src_row + column * 4u;
                uint8_t *dst_pixel = dst_row + column * 4u;
                dst_pixel[0] = src_pixel[2];
                dst_pixel[1] = src_pixel[1];
                dst_pixel[2] = src_pixel[0];
                dst_pixel[3] = src_pixel[3];
            }
        }
        return true;

    case bgfx::TextureFormat::RGB8:
        frame.Pitch = width * 4u;
        frame.Pixels.resize(static_cast<size_t>(frame.Pitch) * height);
        for (uint32_t row = 0; row < height; ++row) {
            const uint8_t *src_row = source + static_cast<size_t>(row) * pitch;
            uint8_t *dst_row = frame.Pixels.data() + static_cast<size_t>(row) * frame.Pitch;
            for (uint32_t column = 0; column < width; ++column) {
                const uint8_t *src_pixel = src_row + column * 3u;
                uint8_t *dst_pixel = dst_row + column * 4u;
                dst_pixel[0] = src_pixel[2];
                dst_pixel[1] = src_pixel[1];
                dst_pixel[2] = src_pixel[0];
                dst_pixel[3] = 0xffu;
            }
        }
        return true;

    default:
        return false;
    }
}

std::string Build_Movie_Frame_Path(const std::string &base_path, uint32_t frame_index)
{
    char path[512];
    std::snprintf(path, sizeof(path), "%s%06u.tga", base_path.c_str(), frame_index);
    return std::string(path);
}

class BgfxCallback final : public bgfx::CallbackI
{
public:
    void fatal(const char *file_path, uint16_t line, bgfx::Fatal::Enum code, const char *message) override
    {
        WWDEBUG_SAY(("bgfx fatal at %s:%u code=%u message=%s\n",
            file_path != nullptr ? file_path : "<unknown>",
            static_cast<unsigned>(line),
            static_cast<unsigned>(code),
            message != nullptr ? message : "<none>"));

        if (code != bgfx::Fatal::DebugCheck) {
            std::abort();
        }
    }

    void traceVargs(const char *file_path, uint16_t line, const char *format, va_list arg_list) override
    {
        (void)file_path;
        (void)line;
        (void)format;
        (void)arg_list;
    }

    void profilerBegin(const char *, uint32_t, const char *, uint16_t) override {}
    void profilerBeginLiteral(const char *, uint32_t, const char *, uint16_t) override {}
    void profilerEnd() override {}
    uint32_t cacheReadSize(uint64_t) override { return 0; }
    bool cacheRead(uint64_t, void *, uint32_t) override { return false; }
    void cacheWrite(uint64_t, const void *, uint32_t) override {}

    void screenShot(
        const char *file_path,
        uint32_t width,
        uint32_t height,
        uint32_t pitch,
        bgfx::TextureFormat::Enum,
        const void *data,
        uint32_t,
        bool yflip) override
    {
        const bool saved = Write_BGRA_TGA(file_path, width, height, pitch, data, yflip);
        WWDEBUG_SAY(("bgfx screenshot %s %ux%u %s\n",
            file_path != nullptr ? file_path : "<null>",
            static_cast<unsigned>(width),
            static_cast<unsigned>(height),
            saved ? "saved" : "failed"));
    }

    void captureBegin(
        uint32_t width,
        uint32_t height,
        uint32_t pitch,
        bgfx::TextureFormat::Enum format,
        bool yflip) override
    {
        std::lock_guard<std::mutex> lock(MovieCaptureMutex);
        LatestMovieFrame = {};
        LatestMovieFrame.Width = width;
        LatestMovieFrame.Height = height;
        LatestMovieFrame.Pitch = pitch;
        LatestMovieFrame.YFlip = yflip;
        MovieCaptureFormat = format;
        MovieCaptureConfigured = true;
    }

    void captureEnd() override
    {
        std::lock_guard<std::mutex> lock(MovieCaptureMutex);
        MovieCaptureConfigured = false;
        MovieCaptureFormat = bgfx::TextureFormat::Count;
        LatestMovieFrame = {};
    }

    void captureFrame(const void *data, uint32_t) override
    {
        std::lock_guard<std::mutex> lock(MovieCaptureMutex);
        if (!MovieCaptureActive || !MovieCaptureConfigured) {
            return;
        }

        CapturedMovieFrame frame;
        frame.Width = LatestMovieFrame.Width;
        frame.Height = LatestMovieFrame.Height;
        frame.Pitch = LatestMovieFrame.Pitch;
        frame.YFlip = LatestMovieFrame.YFlip;
        if (!Convert_Capture_Frame_To_BGRA8(MovieCaptureFormat, frame.Width, frame.Height, frame.Pitch, data, frame)) {
            WWDEBUG_SAY(("bgfx movie capture skipped unsupported format %u\n", static_cast<unsigned>(MovieCaptureFormat)));
            return;
        }

        frame.Sequence = ++MovieCaptureSequence;
        LatestMovieFrame = std::move(frame);
    }
};

BgfxCallback Callback;

void Configure_Auto_Screenshot()
{
    AutoScreenshotEnabled = false;
    AutoScreenshotRequested = false;
    AutoScreenshotDelayMs = 0;
    AutoScreenshotStartTicks = 0;
    AutoScreenshotPath.clear();

    const char *delay_value = std::getenv("RENEGADE_BGFX_SCREENSHOT_AFTER_MS");
    if (delay_value == nullptr || *delay_value == '\0') {
        return;
    }

    char *end = nullptr;
    const unsigned long parsed_delay = std::strtoul(delay_value, &end, 10);
    if (end == delay_value || (end != nullptr && *end != '\0')) {
        WWDEBUG_SAY(("BgfxRenderer invalid RENEGADE_BGFX_SCREENSHOT_AFTER_MS value '%s'\n", delay_value));
        return;
    }

    const char *path_value = std::getenv("RENEGADE_BGFX_SCREENSHOT_PATH");
    AutoScreenshotPath = (path_value != nullptr && *path_value != '\0') ? path_value : "BgfxScreenShot.tga";
    AutoScreenshotDelayMs = static_cast<uint32_t>(parsed_delay);
    AutoScreenshotStartTicks = static_cast<uint32_t>(SDL_GetTicks() & 0xffffffffu);
    AutoScreenshotEnabled = true;
}

void Maybe_Request_Auto_Screenshot()
{
    if (!AutoScreenshotEnabled || AutoScreenshotRequested) {
        return;
    }

    const uint32_t now = static_cast<uint32_t>(SDL_GetTicks() & 0xffffffffu);
    if ((now - AutoScreenshotStartTicks) < AutoScreenshotDelayMs) {
        return;
    }

    BgfxRenderer::Request_Screen_Shot(AutoScreenshotPath.c_str());
    AutoScreenshotRequested = true;
}

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

    platform_data.type = bgfx::NativeWindowHandleType::Default;

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
        platform_data.type = bgfx::NativeWindowHandleType::Wayland;
        return true;
    }

    if (void *x11_display = SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr)) {
        const uintptr_t x11_window = static_cast<uintptr_t>(SDL_GetNumberProperty(window_properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
        if (x11_window == 0) {
            return false;
        }

        platform_data.ndt = x11_display;
        platform_data.nwh = reinterpret_cast<void *>(x11_window);
        platform_data.type = bgfx::NativeWindowHandleType::Default;
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

bool Should_Use_Single_Threaded_Bgfx(SDL_Window *window)
{
    if (window == nullptr) {
        return false;
    }

    const SDL_PropertiesID window_properties = SDL_GetWindowProperties(window);
    if (window_properties == 0) {
        return false;
    }

    return SDL_GetPointerProperty(window_properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr) != nullptr;
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

uint32_t Convert_ARGB_To_ABGR(uint32_t argb_color)
{
    const uint32_t alpha = argb_color & 0xff000000u;
    const uint32_t red = (argb_color >> 16) & 0xffu;
    const uint32_t green = (argb_color >> 8) & 0xffu;
    const uint32_t blue = argb_color & 0xffu;
    return alpha | (blue << 16) | (green << 8) | red;
}

uint32_t Expand_RGB565_To_ARGB8888(uint16_t rgb)
{
    const uint32_t blue = static_cast<uint32_t>((rgb & 0x001fu) << 3);
    const uint32_t green = static_cast<uint32_t>((rgb & 0x07e0u) << 5);
    const uint32_t red = static_cast<uint32_t>((rgb & 0xf800u) << 8);
    return red | green | blue;
}

uint32_t Interpolate_RGB888(uint32_t color1, uint32_t color2, uint32_t weight1)
{
    const uint32_t weight2 = 255u - weight1;

    const uint32_t red_blue_mask = 0x00ff00ffu;
    const uint32_t green_mask = 0x0000ff00u;

    uint32_t red_blue_1 = (color1 & red_blue_mask) * weight1;
    uint32_t red_blue_2 = (color2 & red_blue_mask) * weight2;
    uint32_t green_1 = (color1 & green_mask) * weight1;
    uint32_t green_2 = (color2 & green_mask) * weight2;

    red_blue_1 = (red_blue_1 + red_blue_2) >> 8;
    green_1 = (green_1 + green_2) >> 8;
    return (red_blue_1 & red_blue_mask) | (green_1 & green_mask);
}

void Write_ARGB8888_To_BGRA8(std::vector<uint8_t> &destination, uint32_t width, uint32_t x, uint32_t y, uint32_t argb)
{
    const size_t pixel_index = (static_cast<size_t>(y) * width + x) * 4u;
    destination[pixel_index + 0] = static_cast<uint8_t>(argb & 0xffu);
    destination[pixel_index + 1] = static_cast<uint8_t>((argb >> 8) & 0xffu);
    destination[pixel_index + 2] = static_cast<uint8_t>((argb >> 16) & 0xffu);
    destination[pixel_index + 3] = static_cast<uint8_t>((argb >> 24) & 0xffu);
}

bool Convert_Compressed_Copy_To_BGRA8(
    WW3DFormat format,
    uint32_t width,
    uint32_t height,
    const uint8_t *source_pixels,
    std::vector<uint8_t> &converted_pixels)
{
    if (source_pixels == nullptr || width == 0 || height == 0) {
        return false;
    }

    const uint32_t block_width = (width + 3u) / 4u;
    const uint32_t block_height = (height + 3u) / 4u;
    const uint32_t block_size =
        format == WW3D_FORMAT_DXT1 ? 8u :
        format == WW3D_FORMAT_DXT3 || format == WW3D_FORMAT_DXT5 ? 16u : 0u;
    if (block_size == 0u) {
        return false;
    }

    converted_pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0u);

    for (uint32_t block_y = 0; block_y < block_height; ++block_y) {
        for (uint32_t block_x = 0; block_x < block_width; ++block_x) {
            const uint8_t *block = source_pixels + ((static_cast<size_t>(block_y) * block_width + block_x) * block_size);

            uint32_t alphas[16] = {};
            if (format == WW3D_FORMAT_DXT1) {
                std::fill(std::begin(alphas), std::end(alphas), 255u);
            } else if (format == WW3D_FORMAT_DXT3) {
                for (uint32_t row = 0; row < 4u; ++row) {
                    const uint16_t alpha_row = static_cast<uint16_t>(block[row * 2u]) |
                        (static_cast<uint16_t>(block[row * 2u + 1u]) << 8);
                    for (uint32_t column = 0; column < 4u; ++column) {
                        const uint32_t alpha4 = (alpha_row >> (column * 4u)) & 0x0fu;
                        alphas[row * 4u + column] = (alpha4 << 4) | alpha4;
                    }
                }
            } else if (format == WW3D_FORMAT_DXT5) {
                const uint8_t alpha0 = block[0];
                const uint8_t alpha1 = block[1];
                uint32_t alpha_palette[8] = {};
                alpha_palette[0] = alpha0;
                alpha_palette[1] = alpha1;
                if (alpha0 > alpha1) {
                    alpha_palette[2] = (6u * alpha0 + 1u * alpha1 + 3u) / 7u;
                    alpha_palette[3] = (5u * alpha0 + 2u * alpha1 + 3u) / 7u;
                    alpha_palette[4] = (4u * alpha0 + 3u * alpha1 + 3u) / 7u;
                    alpha_palette[5] = (3u * alpha0 + 4u * alpha1 + 3u) / 7u;
                    alpha_palette[6] = (2u * alpha0 + 5u * alpha1 + 3u) / 7u;
                    alpha_palette[7] = (1u * alpha0 + 6u * alpha1 + 3u) / 7u;
                } else {
                    alpha_palette[2] = (4u * alpha0 + 1u * alpha1 + 2u) / 5u;
                    alpha_palette[3] = (3u * alpha0 + 2u * alpha1 + 2u) / 5u;
                    alpha_palette[4] = (2u * alpha0 + 3u * alpha1 + 2u) / 5u;
                    alpha_palette[5] = (1u * alpha0 + 4u * alpha1 + 2u) / 5u;
                    alpha_palette[6] = 0u;
                    alpha_palette[7] = 255u;
                }

                uint64_t alpha_bits = 0u;
                for (uint32_t index = 0; index < 6u; ++index) {
                    alpha_bits |= static_cast<uint64_t>(block[2u + index]) << (index * 8u);
                }
                for (uint32_t index = 0; index < 16u; ++index) {
                    alphas[index] = alpha_palette[(alpha_bits >> (index * 3u)) & 0x7u];
                }
            }

            const uint8_t *color_block = block + (block_size - 8u);
            const uint16_t color0_565 = static_cast<uint16_t>(color_block[0]) |
                (static_cast<uint16_t>(color_block[1]) << 8);
            const uint16_t color1_565 = static_cast<uint16_t>(color_block[2]) |
                (static_cast<uint16_t>(color_block[3]) << 8);

            uint32_t colors[4] = {};
            colors[0] = Expand_RGB565_To_ARGB8888(color0_565);
            colors[1] = Expand_RGB565_To_ARGB8888(color1_565);

            const bool has_dxt1_transparency = format == WW3D_FORMAT_DXT1 && color0_565 <= color1_565;
            if (has_dxt1_transparency) {
                colors[2] = Interpolate_RGB888(colors[0], colors[1], 128u);
                colors[3] = 0u;
            } else {
                colors[2] = Interpolate_RGB888(colors[1], colors[0], 85u);
                colors[3] = Interpolate_RGB888(colors[0], colors[1], 85u);
            }

            for (uint32_t row = 0; row < 4u; ++row) {
                uint8_t color_indices = color_block[4u + row];
                for (uint32_t column = 0; column < 4u; ++column) {
                    const uint32_t pixel_x = block_x * 4u + column;
                    const uint32_t pixel_y = block_y * 4u + row;
                    if (pixel_x >= width || pixel_y >= height) {
                        color_indices >>= 2;
                        continue;
                    }

                    const uint32_t color_index = color_indices & 0x3u;
                    color_indices >>= 2;

                    uint32_t alpha = alphas[row * 4u + column];
                    if (format == WW3D_FORMAT_DXT1 && has_dxt1_transparency && color_index == 3u) {
                        alpha = 0u;
                    }

                    const uint32_t argb = (alpha << 24) | colors[color_index];
                    Write_ARGB8888_To_BGRA8(converted_pixels, width, pixel_x, pixel_y, argb);
                }
            }
        }
    }

    return true;
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
        case WW3D_FORMAT_U8V8: {
            destination[0] = 0x00;
            destination[1] = source[1];
            destination[2] = source[0];
            destination[3] = 0xff;
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_L6V5U5: {
            const uint16_t packed = static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
            destination[0] = 0x00;
            destination[1] = Expand_5_To_8(static_cast<uint8_t>((packed >> 5) & 0x1f));
            destination[2] = Expand_5_To_8(static_cast<uint8_t>(packed & 0x1f));
            destination[3] = Expand_6_To_8(static_cast<uint8_t>((packed >> 10) & 0x3f));
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_X8L8V8U8: {
            destination[0] = source[3];
            destination[1] = source[1];
            destination[2] = source[0];
            destination[3] = source[2];
            source_pixels += 4;
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
    case WW3D_FORMAT_UNKNOWN:
    case WW3D_FORMAT_A8R8G8B8:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        return true;
    case WW3D_FORMAT_X8R8G8B8:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    case WW3D_FORMAT_R5G6B5:
        bgfx_format = bgfx::TextureFormat::R5G6B5;
        return true;
    case WW3D_FORMAT_A1R5G5B5:
        bgfx_format = bgfx::TextureFormat::BGR5A1;
        return true;
    case WW3D_FORMAT_X1R5G5B5:
        bgfx_format = bgfx::TextureFormat::BGR5A1;
        direct_copy = false;
        return true;
    case WW3D_FORMAT_A4R4G4B4:
        bgfx_format = bgfx::TextureFormat::BGRA4;
        return true;
    case WW3D_FORMAT_X4R4G4B4:
        bgfx_format = bgfx::TextureFormat::BGRA4;
        direct_copy = false;
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

const char *Get_Renderer_Name(bgfx::RendererType::Enum renderer_type)
{
    switch (renderer_type) {
    case bgfx::RendererType::Direct3D11:
        return "Direct3D11";
    case bgfx::RendererType::Direct3D12:
        return "Direct3D12";
    case bgfx::RendererType::Metal:
        return "Metal";
    case bgfx::RendererType::OpenGLES:
        return "OpenGLES";
    case bgfx::RendererType::OpenGL:
        return "OpenGL";
    case bgfx::RendererType::Vulkan:
        return "Vulkan";
    default:
        return "auto";
    }
}

bool Is_Texture_Format_Supported(WW3DFormat format, uint32_t capability_flags)
{
    bgfx::TextureFormat::Enum bgfx_format = bgfx::TextureFormat::Count;
    bool direct_copy = false;
    if (!Get_Bgfx_Texture_Format(format, bgfx_format, direct_copy)) {
        return false;
    }

    const bgfx::Caps *caps = bgfx::getCaps();
    if (caps == nullptr) {
        return true;
    }

    return (caps->formats[bgfx_format] & capability_flags) != 0;
}

bgfx::RendererType::Enum Choose_Preferred_Renderer(void)
{
    return bgfx::RendererType::Count;
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

    if (Should_Use_Single_Threaded_Bgfx(reinterpret_cast<SDL_Window *>(window_handle))) {
        WWDEBUG_SAY(("BgfxRenderer::Init using bgfx render thread on X11 to avoid slow single-threaded startup\n"));
    }

    bgfx::Init init;
    init.callback = &Callback;
    init.platformData = PlatformData;
    init.resolution.width = Width;
    init.resolution.height = Height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    bool initialized = false;
    const bgfx::RendererType::Enum preferred_renderer = Choose_Preferred_Renderer();
    if (preferred_renderer != bgfx::RendererType::Count) {
        WWDEBUG_SAY(("BgfxRenderer::Init preferring %s on SDL video driver '%s'\n",
            Get_Renderer_Name(preferred_renderer),
            SDL_GetCurrentVideoDriver()));
        init.type = preferred_renderer;
        initialized = bgfx::init(init);
        if (!initialized) {
            WWDEBUG_SAY(("BgfxRenderer::Init preferred %s backend failed, falling back to auto selection\n",
                Get_Renderer_Name(preferred_renderer)));
        }
    }

    if (!initialized) {
        init.type = bgfx::RendererType::Count;
        initialized = bgfx::init(init);
    }

    if (!initialized) {
        WWDEBUG_SAY(("BgfxRenderer::Init bgfx::init failed\n"));
        return false;
    }

    if (!Init_Render_Resources()) {
        WWDEBUG_SAY(("BgfxRenderer::Init failed to initialize renderer resources\n"));
        bgfx::shutdown();
        return false;
    }

    CurrentFrameBuffer = BGFX_INVALID_HANDLE;
    bgfx::setViewClear(ClearViewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    bgfx::setViewFrameBuffer(ClearViewId, CurrentFrameBuffer);
    bgfx::setViewRect(ClearViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    ActiveWidth = Width;
    ActiveHeight = Height;
    Reset_Main_View_State(ActiveWidth, ActiveHeight);
    Configure_Auto_Screenshot();

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
    AutoScreenshotEnabled = false;
    AutoScreenshotRequested = false;
    AutoScreenshotDelayMs = 0;
    AutoScreenshotStartTicks = 0;
    AutoScreenshotPath.clear();
    CurrentFrameBuffer = BGFX_INVALID_HANDLE;
    Reset_Main_View_State(0, 0);
}

bool BgfxRenderer::Reset()
{
    if (!IsInitted) {
        return false;
    }

    Apply_Reset_State();
    return true;
}

void BgfxRenderer::Apply_Reset_State()
{
    bgfx::reset(Width, Height, Get_Reset_Flags());
    ActiveWidth = Width;
    ActiveHeight = Height;
    CurrentFrameBuffer = BGFX_INVALID_HANDLE;
    bgfx::setViewFrameBuffer(ClearViewId, CurrentFrameBuffer);
    bgfx::setViewFrameBuffer(OverlayViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(ClearViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    Reset_Main_View_State(ActiveWidth, ActiveHeight);
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

    PendingViewportX = x;
    PendingViewportY = y;
    PendingViewportWidth = width;
    PendingViewportHeight = height;
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
    CurrentFrameBuffer = frame_buffer;
    bgfx::setViewFrameBuffer(ClearViewId, CurrentFrameBuffer);
    bgfx::setViewFrameBuffer(OverlayViewId, frame_buffer);
    bgfx::setViewRect(ClearViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    Reset_Main_View_State(ActiveWidth, ActiveHeight);
    return true;
}

void BgfxRenderer::Reset_Render_Target()
{
    if (!IsInitted) {
        return;
    }

    ActiveWidth = Width;
    ActiveHeight = Height;
    CurrentFrameBuffer = BGFX_INVALID_HANDLE;
    bgfx::setViewFrameBuffer(ClearViewId, CurrentFrameBuffer);
    bgfx::setViewFrameBuffer(OverlayViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(ClearViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    Reset_Main_View_State(ActiveWidth, ActiveHeight);
}

void BgfxRenderer::Set_Camera(const Matrix3D &view, const Matrix4 &projection)
{
    if (!IsInitted) {
        return;
    }

    CurrentViewMatrix = Matrix4(view);
    CurrentProjectionMatrix = projection;
    Configure_View(CurrentViewMatrix, CurrentProjectionMatrix);
}

void BgfxRenderer::Prepare_Overlay_View()
{
    if (!IsInitted) {
        return;
    }

    bgfx::setViewMode(OverlayViewId, bgfx::ViewMode::Sequential);
    bgfx::setViewFrameBuffer(OverlayViewId, CurrentFrameBuffer);
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    bgfx::setViewTransform(OverlayViewId, IdentityMatrix, IdentityMatrix);
    bgfx::setViewClear(OverlayViewId, 0, 0, 1.0f, 0);
}

void BgfxRenderer::End_Frame()
{
    if (!IsInitted) {
        return;
    }

    Maybe_Request_Auto_Screenshot();
    bgfx::frame();
}

void BgfxRenderer::Request_Screen_Shot(const char *file_path)
{
    if (!IsInitted) {
        return;
    }

    const char *resolved_path = (file_path != nullptr && *file_path != '\0') ? file_path : "ScreenShot.tga";
    bgfx::requestScreenShot(BGFX_INVALID_HANDLE, resolved_path);
}

bool BgfxRenderer::Start_Movie_Capture(const char *file_path_base, float frame_rate)
{
    if (!IsInitted) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(MovieCaptureMutex);
        MovieCaptureActive = true;
        MovieCaptureConfigured = false;
        MovieCaptureFormat = bgfx::TextureFormat::Count;
        MovieCaptureFrameRate = frame_rate;
        MovieCaptureBasePath = (file_path_base != nullptr && *file_path_base != '\0') ? file_path_base : "Movie";
        MovieCaptureFrameIndex = 0;
        MovieCaptureSequence = 0;
        MovieCaptureConsumedSequence = 0;
        LatestMovieFrame = {};
    }

    Apply_Reset_State();
    return true;
}

void BgfxRenderer::Stop_Movie_Capture()
{
    if (!IsInitted) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(MovieCaptureMutex);
        if (!MovieCaptureActive) {
            return;
        }
        MovieCaptureActive = false;
        MovieCaptureConfigured = false;
        MovieCaptureFormat = bgfx::TextureFormat::Count;
        MovieCaptureFrameRate = 0.0f;
        MovieCaptureBasePath.clear();
        MovieCaptureFrameIndex = 0;
        MovieCaptureSequence = 0;
        MovieCaptureConsumedSequence = 0;
        LatestMovieFrame = {};
    }

    Apply_Reset_State();
}

bool BgfxRenderer::Write_Latest_Movie_Frame()
{
    CapturedMovieFrame frame;
    std::string base_path;
    uint32_t frame_index = 0;

    {
        std::lock_guard<std::mutex> lock(MovieCaptureMutex);
        if (!MovieCaptureActive || LatestMovieFrame.Sequence == 0 || LatestMovieFrame.Sequence == MovieCaptureConsumedSequence) {
            return false;
        }

        frame = LatestMovieFrame;
        MovieCaptureConsumedSequence = LatestMovieFrame.Sequence;
        base_path = MovieCaptureBasePath;
        frame_index = MovieCaptureFrameIndex++;
    }

    const std::string file_path = Build_Movie_Frame_Path(base_path, frame_index);
    const bool saved = Write_BGRA_TGA(file_path.c_str(), frame.Width, frame.Height, frame.Pitch, frame.Pixels.data(), frame.YFlip);
    if (!saved) {
        WWDEBUG_SAY(("bgfx movie capture failed to write %s\n", file_path.c_str()));
    }
    return saved;
}

bool BgfxRenderer::Is_Movie_Capture_Active()
{
    std::lock_guard<std::mutex> lock(MovieCaptureMutex);
    return MovieCaptureActive;
}

float BgfxRenderer::Get_Movie_Capture_Frame_Rate()
{
    std::lock_guard<std::mutex> lock(MovieCaptureMutex);
    return MovieCaptureFrameRate;
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

const bgfx::VertexLayout &BgfxRenderer::Get_Fixed_Function_Layout()
{
    return FixedFunctionLayout;
}

uint16_t BgfxRenderer::Get_Main_View_Id()
{
    return CurrentMainViewId;
}

uint16_t BgfxRenderer::Get_View_Id(const Matrix4 &view, const Matrix4 &projection)
{
    if (!IsInitted) {
        return CurrentMainViewId;
    }

    return Configure_View(view, projection);
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

bgfx::UniformHandle BgfxRenderer::Get_Texture0_Uniform()
{
    return Texture0Uniform;
}

bgfx::UniformHandle BgfxRenderer::Get_Texture1_Uniform()
{
    return Texture1Uniform;
}

bgfx::ProgramHandle BgfxRenderer::Get_Fixed_Function_Program()
{
    return FixedFunctionProgram;
}

bool BgfxRenderer::Supports_Texture_Format(WW3DFormat format)
{
    return Is_Texture_Format_Supported(format, BGFX_CAPS_FORMAT_TEXTURE_2D);
}

bool BgfxRenderer::Supports_Render_Target_Format(WW3DFormat format)
{
    return Is_Texture_Format_Supported(format, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER);
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
        if (!Get_Bgfx_Texture_Format(texture.Get_Texture_Format(), texture_format, direct_copy) ||
            !Supports_Render_Target_Format(texture.Get_Texture_Format())) {
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

uint64_t BgfxRenderer::Build_Render_State(const ShaderClass &shader, unsigned cull_mode)
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
        state |= (cull_mode == D3DCULL_CCW) ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW;
    }

    if (shader.Get_Src_Blend_Func() != ShaderClass::SRCBLEND_ONE
        || shader.Get_Dst_Blend_Func() != ShaderClass::DSTBLEND_ZERO) {
        state |= BGFX_STATE_BLEND_FUNC(
            Convert_Blend_Factor(shader.Get_Src_Blend_Func()),
            Convert_Blend_Factor(shader.Get_Dst_Blend_Func()));
    }

    return state;
}

void BgfxRenderer::Apply_Fixed_Function_Shader_Inputs(const ShaderClass &shader, const FixedFunctionShaderInputs &inputs)
{
    const unsigned alpha_reference_state = DX8Wrapper::Get_DX8_Render_State(D3DRS_ALPHAREF);
    const unsigned alpha_function_state = DX8Wrapper::Get_DX8_Render_State(D3DRS_ALPHAFUNC);
    float alpha_test_function = -1.0f;
    float alpha_reference = static_cast<float>(alpha_reference_state & 0xffu) / 255.0f;
    if (shader.Get_Alpha_Test() == ShaderClass::ALPHATEST_ENABLE) {
        unsigned alpha_function = alpha_function_state;
        if (alpha_function > D3DCMP_ALWAYS || (alpha_function == 0u && alpha_reference_state == 0u)) {
            alpha_function = D3DCMP_GREATEREQUAL;
            alpha_reference = 0x60 / 255.0f;
        }

        alpha_test_function = static_cast<float>(alpha_function);
    }

    float config1[4] = {
        alpha_test_function,
        alpha_reference,
        inputs.FogEnabled ? static_cast<float>(shader.Get_Fog_Func()) : 0.0f,
        shader.Get_Secondary_Gradient() == ShaderClass::SECONDARY_GRADIENT_ENABLE ? 1.0f : 0.0f};

    float fog_color[4] = {
        static_cast<float>((inputs.FogColor >> 16) & 0xffu) / 255.0f,
        static_cast<float>((inputs.FogColor >> 8) & 0xffu) / 255.0f,
        static_cast<float>(inputs.FogColor & 0xffu) / 255.0f,
        static_cast<float>((inputs.FogColor >> 24) & 0xffu) / 255.0f};

    float texture_factor[4] = {
        static_cast<float>((inputs.TextureFactor >> 16) & 0xffu) / 255.0f,
        static_cast<float>((inputs.TextureFactor >> 8) & 0xffu) / 255.0f,
        static_cast<float>(inputs.TextureFactor & 0xffu) / 255.0f,
        static_cast<float>((inputs.TextureFactor >> 24) & 0xffu) / 255.0f};

    float bump_env_matrix[4] = {
        inputs.BumpEnvMatrix[0],
        inputs.BumpEnvMatrix[1],
        inputs.BumpEnvMatrix[2],
        inputs.BumpEnvMatrix[3]};

    float bump_env_params[4] = {
        inputs.BumpEnvLuminanceScale,
        inputs.BumpEnvLuminanceOffset,
        0.0f,
        0.0f};
    float camera_position[4];
    Extract_Camera_Position(CurrentViewMatrix, camera_position);

    bgfx::setUniform(FixedFunctionConfig1Uniform, config1);
    bgfx::setUniform(FixedFunctionFogColorUniform, fog_color);
    bgfx::setUniform(FixedFunctionTextureFactorUniform, texture_factor);
    bgfx::setUniform(FixedFunctionStage0ColorUniform, inputs.Stage0Color);
    bgfx::setUniform(FixedFunctionStage0AlphaUniform, inputs.Stage0Alpha);
    bgfx::setUniform(FixedFunctionStage1ColorUniform, inputs.Stage1Color);
    bgfx::setUniform(FixedFunctionStage1AlphaUniform, inputs.Stage1Alpha);
    bgfx::setUniform(FixedFunctionBumpEnvMatrixUniform, bump_env_matrix);
    bgfx::setUniform(FixedFunctionBumpEnvParamsUniform, bump_env_params);
    bgfx::setUniform(FixedFunctionMaterialAmbientUniform, inputs.MaterialAmbient);
    bgfx::setUniform(FixedFunctionMaterialDiffuseUniform, inputs.MaterialDiffuse);
    bgfx::setUniform(FixedFunctionMaterialSpecularUniform, inputs.MaterialSpecular);
    bgfx::setUniform(FixedFunctionMaterialEmissiveUniform, inputs.MaterialEmissive);
    bgfx::setUniform(FixedFunctionMaterialParamsUniform, inputs.MaterialParams);
    bgfx::setUniform(FixedFunctionSceneAmbientUniform, inputs.SceneAmbient);
    bgfx::setUniform(FixedFunctionLightingConfigUniform, inputs.LightingConfig);
    bgfx::setUniform(FixedFunctionMaterialSourceConfigUniform, inputs.MaterialSourceConfig);
    bgfx::setUniform(FixedFunctionLightPositionsUniform, inputs.LightPositions, 4);
    bgfx::setUniform(FixedFunctionLightDirectionsUniform, inputs.LightDirections, 4);
    bgfx::setUniform(FixedFunctionLightAmbientUniform, inputs.LightAmbient, 4);
    bgfx::setUniform(FixedFunctionLightDiffuseUniform, inputs.LightDiffuse, 4);
    bgfx::setUniform(FixedFunctionLightSpecularUniform, inputs.LightSpecular, 4);
    bgfx::setUniform(FixedFunctionLightAttenuationUniform, inputs.LightAttenuation, 4);
    bgfx::setUniform(FixedFunctionLightSpotParamsUniform, inputs.LightSpotParams, 4);
    bgfx::setUniform(FixedFunctionCameraPositionUniform, camera_position);
}

std::uint32_t BgfxRenderer::Convert_Packed_Color(std::uint32_t argb_color)
{
    return Convert_ARGB_To_ABGR(argb_color);
}

bool BgfxRenderer::Init_Render_Resources()
{
    FixedFunctionLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
        .end();

    if (!bgfx::isValid(Texture0Uniform)) {
        Texture0Uniform = bgfx::createUniform("s_texColor0", bgfx::UniformType::Sampler);
    }

    if (!bgfx::isValid(Texture1Uniform)) {
        Texture1Uniform = bgfx::createUniform("s_texColor1", bgfx::UniformType::Sampler);
    }

    if (!bgfx::isValid(FixedFunctionConfig1Uniform)) {
        FixedFunctionConfig1Uniform = bgfx::createUniform("u_ffpConfig1", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionFogColorUniform)) {
        FixedFunctionFogColorUniform = bgfx::createUniform("u_ffpFogColor", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionTextureFactorUniform)) {
        FixedFunctionTextureFactorUniform = bgfx::createUniform("u_ffpTextureFactor", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionStage0ColorUniform)) {
        FixedFunctionStage0ColorUniform = bgfx::createUniform("u_ffpStage0Color", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionStage0AlphaUniform)) {
        FixedFunctionStage0AlphaUniform = bgfx::createUniform("u_ffpStage0Alpha", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionStage1ColorUniform)) {
        FixedFunctionStage1ColorUniform = bgfx::createUniform("u_ffpStage1Color", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionStage1AlphaUniform)) {
        FixedFunctionStage1AlphaUniform = bgfx::createUniform("u_ffpStage1Alpha", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionBumpEnvMatrixUniform)) {
        FixedFunctionBumpEnvMatrixUniform = bgfx::createUniform("u_ffpBumpEnvMatrix", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionBumpEnvParamsUniform)) {
        FixedFunctionBumpEnvParamsUniform = bgfx::createUniform("u_ffpBumpEnvParams", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionMaterialAmbientUniform)) {
        FixedFunctionMaterialAmbientUniform = bgfx::createUniform("u_ffpMaterialAmbient", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionMaterialDiffuseUniform)) {
        FixedFunctionMaterialDiffuseUniform = bgfx::createUniform("u_ffpMaterialDiffuse", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionMaterialSpecularUniform)) {
        FixedFunctionMaterialSpecularUniform = bgfx::createUniform("u_ffpMaterialSpecular", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionMaterialEmissiveUniform)) {
        FixedFunctionMaterialEmissiveUniform = bgfx::createUniform("u_ffpMaterialEmissive", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionMaterialParamsUniform)) {
        FixedFunctionMaterialParamsUniform = bgfx::createUniform("u_ffpMaterialParams", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionCameraPositionUniform)) {
        FixedFunctionCameraPositionUniform = bgfx::createUniform("u_ffpCameraPosition", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionSceneAmbientUniform)) {
        FixedFunctionSceneAmbientUniform = bgfx::createUniform("u_ffpSceneAmbient", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionLightingConfigUniform)) {
        FixedFunctionLightingConfigUniform = bgfx::createUniform("u_ffpLightingConfig", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionMaterialSourceConfigUniform)) {
        FixedFunctionMaterialSourceConfigUniform = bgfx::createUniform("u_ffpMaterialSourceConfig", bgfx::UniformType::Vec4);
    }

    if (!bgfx::isValid(FixedFunctionLightPositionsUniform)) {
        FixedFunctionLightPositionsUniform = bgfx::createUniform("u_ffpLightPositions", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(FixedFunctionLightDirectionsUniform)) {
        FixedFunctionLightDirectionsUniform = bgfx::createUniform("u_ffpLightDirections", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(FixedFunctionLightAmbientUniform)) {
        FixedFunctionLightAmbientUniform = bgfx::createUniform("u_ffpLightAmbient", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(FixedFunctionLightDiffuseUniform)) {
        FixedFunctionLightDiffuseUniform = bgfx::createUniform("u_ffpLightDiffuse", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(FixedFunctionLightSpecularUniform)) {
        FixedFunctionLightSpecularUniform = bgfx::createUniform("u_ffpLightSpecular", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(FixedFunctionLightAttenuationUniform)) {
        FixedFunctionLightAttenuationUniform = bgfx::createUniform("u_ffpLightAttenuation", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(FixedFunctionLightSpotParamsUniform)) {
        FixedFunctionLightSpotParamsUniform = bgfx::createUniform("u_ffpLightSpotParams", bgfx::UniformType::Vec4, 4);
    }

    if (!bgfx::isValid(Texture0Uniform)
        || !bgfx::isValid(Texture1Uniform)
        || !bgfx::isValid(FixedFunctionConfig1Uniform)
        || !bgfx::isValid(FixedFunctionFogColorUniform)
        || !bgfx::isValid(FixedFunctionTextureFactorUniform)
        || !bgfx::isValid(FixedFunctionStage0ColorUniform)
        || !bgfx::isValid(FixedFunctionStage0AlphaUniform)
        || !bgfx::isValid(FixedFunctionStage1ColorUniform)
        || !bgfx::isValid(FixedFunctionStage1AlphaUniform)
        || !bgfx::isValid(FixedFunctionBumpEnvMatrixUniform)
        || !bgfx::isValid(FixedFunctionBumpEnvParamsUniform)
        || !bgfx::isValid(FixedFunctionMaterialAmbientUniform)
        || !bgfx::isValid(FixedFunctionMaterialDiffuseUniform)
        || !bgfx::isValid(FixedFunctionMaterialSpecularUniform)
        || !bgfx::isValid(FixedFunctionMaterialEmissiveUniform)
        || !bgfx::isValid(FixedFunctionMaterialParamsUniform)
        || !bgfx::isValid(FixedFunctionCameraPositionUniform)
        || !bgfx::isValid(FixedFunctionSceneAmbientUniform)
        || !bgfx::isValid(FixedFunctionLightingConfigUniform)
        || !bgfx::isValid(FixedFunctionMaterialSourceConfigUniform)
        || !bgfx::isValid(FixedFunctionLightPositionsUniform)
        || !bgfx::isValid(FixedFunctionLightDirectionsUniform)
        || !bgfx::isValid(FixedFunctionLightAmbientUniform)
        || !bgfx::isValid(FixedFunctionLightDiffuseUniform)
        || !bgfx::isValid(FixedFunctionLightSpecularUniform)
        || !bgfx::isValid(FixedFunctionLightAttenuationUniform)
        || !bgfx::isValid(FixedFunctionLightSpotParamsUniform)) {
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

    if (!bgfx::isValid(FixedFunctionProgram)) {
        FixedFunctionProgram = Load_Program("vs_fixed_function", "fs_fixed_function");
    }

    return bgfx::isValid(FixedFunctionProgram);
}

void BgfxRenderer::Shutdown_Render_Resources()
{
    Destroy_Program(FixedFunctionProgram);

    if (bgfx::isValid(FixedFunctionLightSpotParamsUniform)) {
        bgfx::destroy(FixedFunctionLightSpotParamsUniform);
        FixedFunctionLightSpotParamsUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightAttenuationUniform)) {
        bgfx::destroy(FixedFunctionLightAttenuationUniform);
        FixedFunctionLightAttenuationUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightSpecularUniform)) {
        bgfx::destroy(FixedFunctionLightSpecularUniform);
        FixedFunctionLightSpecularUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightDiffuseUniform)) {
        bgfx::destroy(FixedFunctionLightDiffuseUniform);
        FixedFunctionLightDiffuseUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightAmbientUniform)) {
        bgfx::destroy(FixedFunctionLightAmbientUniform);
        FixedFunctionLightAmbientUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightDirectionsUniform)) {
        bgfx::destroy(FixedFunctionLightDirectionsUniform);
        FixedFunctionLightDirectionsUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightPositionsUniform)) {
        bgfx::destroy(FixedFunctionLightPositionsUniform);
        FixedFunctionLightPositionsUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionMaterialSourceConfigUniform)) {
        bgfx::destroy(FixedFunctionMaterialSourceConfigUniform);
        FixedFunctionMaterialSourceConfigUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionLightingConfigUniform)) {
        bgfx::destroy(FixedFunctionLightingConfigUniform);
        FixedFunctionLightingConfigUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionSceneAmbientUniform)) {
        bgfx::destroy(FixedFunctionSceneAmbientUniform);
        FixedFunctionSceneAmbientUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionMaterialEmissiveUniform)) {
        bgfx::destroy(FixedFunctionMaterialEmissiveUniform);
        FixedFunctionMaterialEmissiveUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionMaterialParamsUniform)) {
        bgfx::destroy(FixedFunctionMaterialParamsUniform);
        FixedFunctionMaterialParamsUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionCameraPositionUniform)) {
        bgfx::destroy(FixedFunctionCameraPositionUniform);
        FixedFunctionCameraPositionUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionMaterialSpecularUniform)) {
        bgfx::destroy(FixedFunctionMaterialSpecularUniform);
        FixedFunctionMaterialSpecularUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionMaterialDiffuseUniform)) {
        bgfx::destroy(FixedFunctionMaterialDiffuseUniform);
        FixedFunctionMaterialDiffuseUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionMaterialAmbientUniform)) {
        bgfx::destroy(FixedFunctionMaterialAmbientUniform);
        FixedFunctionMaterialAmbientUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(WhiteTexture)) {
        bgfx::destroy(WhiteTexture);
        WhiteTexture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionBumpEnvParamsUniform)) {
        bgfx::destroy(FixedFunctionBumpEnvParamsUniform);
        FixedFunctionBumpEnvParamsUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionBumpEnvMatrixUniform)) {
        bgfx::destroy(FixedFunctionBumpEnvMatrixUniform);
        FixedFunctionBumpEnvMatrixUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionFogColorUniform)) {
        bgfx::destroy(FixedFunctionFogColorUniform);
        FixedFunctionFogColorUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionConfig1Uniform)) {
        bgfx::destroy(FixedFunctionConfig1Uniform);
        FixedFunctionConfig1Uniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionStage1AlphaUniform)) {
        bgfx::destroy(FixedFunctionStage1AlphaUniform);
        FixedFunctionStage1AlphaUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionStage1ColorUniform)) {
        bgfx::destroy(FixedFunctionStage1ColorUniform);
        FixedFunctionStage1ColorUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionStage0AlphaUniform)) {
        bgfx::destroy(FixedFunctionStage0AlphaUniform);
        FixedFunctionStage0AlphaUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionStage0ColorUniform)) {
        bgfx::destroy(FixedFunctionStage0ColorUniform);
        FixedFunctionStage0ColorUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(FixedFunctionTextureFactorUniform)) {
        bgfx::destroy(FixedFunctionTextureFactorUniform);
        FixedFunctionTextureFactorUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(Texture1Uniform)) {
        bgfx::destroy(Texture1Uniform);
        Texture1Uniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(Texture0Uniform)) {
        bgfx::destroy(Texture0Uniform);
        Texture0Uniform = BGFX_INVALID_HANDLE;
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

    Reset_Main_View_State(ActiveWidth, ActiveHeight);
    bgfx::setViewClear(ClearViewId, clear_flags, clear_rgba, 1.0f, 0);
    bgfx::setViewFrameBuffer(ClearViewId, CurrentFrameBuffer);
    bgfx::setViewRect(ClearViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    bgfx::touch(ClearViewId);
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

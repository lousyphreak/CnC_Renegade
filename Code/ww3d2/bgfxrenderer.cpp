#include "bgfxrenderer.h"

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
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
#include "indexbuffer.h"
#include "pot.h"
#include "rawfile.h"
#include "shadowmap.h"
#include "surfaceclass.h"
#include "texture.h"
#include "textureloader.h"
#include "vertmaterial.h"
#include "vertexbuffer.h"
#include "vertexformat.h"
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
bgfx::VertexLayout BgfxRenderer::OverlayVertexLayout;
bgfx::TextureHandle BgfxRenderer::WhiteTexture = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::Texture0Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::Texture1Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::OverlayConfigUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshFogConfigUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshFogColorUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshFragConfigUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshFragConfig2Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshLitConfigUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshMaterialAmbientUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshMaterialDiffuseUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshMaterialEmissiveUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshSceneAmbientUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshLightDirUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshLightColorUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshBumpEnvMatUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshBumpEnvLumUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshTexgenModeUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshTexTransformFlagsUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshTexTransform0Uniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle BgfxRenderer::MeshTexTransform1Uniform = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle BgfxRenderer::OverlayProgram = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle BgfxRenderer::MeshProgram = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle BgfxRenderer::MeshTexgenProgram = BGFX_INVALID_HANDLE;
Matrix4 BgfxRenderer::CurrentViewMatrix(true);
Matrix4 BgfxRenderer::CurrentProjectionMatrix(true);

namespace
{
constexpr unsigned kUnsetRenderState = 0x12345678u;

constexpr uint16_t InvalidFrameBufferIndex = UINT16_MAX;

struct ViewTransformState
{
    Matrix4 View;
    Matrix4 Projection;
    uint16_t FrameBufferIndex = InvalidFrameBufferIndex;
    uint16_t ViewportX = 0;
    uint16_t ViewportY = 0;
    uint16_t ViewportWidth = 0;
    uint16_t ViewportHeight = 0;
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

constexpr uint16_t FirstDynamicViewId = 3; // 0-2 reserved for shadow cascades
constexpr uint16_t MaxDynamicViewId = 254;
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
bgfx::TextureHandle BlackTexture = BGFX_INVALID_HANDLE;
uint16_t CurrentMainViewId = FirstDynamicViewId;
uint16_t NextDynamicViewId = FirstDynamicViewId;
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

float Decode_Dword_Float(unsigned value)
{
    float decoded = 0.0f;
    std::memcpy(&decoded, &value, sizeof(decoded));
    return decoded;
}

void Reset_Frame_View_Allocation()
{
    CurrentMainViewId = FirstDynamicViewId;
    NextDynamicViewId = FirstDynamicViewId;
    ConfiguredViews.clear();
}

void Invalidate_Configured_Views()
{
    ConfiguredViews.clear();
}

void Reset_Default_Viewport(uint32_t width, uint32_t height)
{
    PendingViewportX = 0;
    PendingViewportY = 0;
    PendingViewportWidth = width;
    PendingViewportHeight = height;
}

void Reset_Frame_State(uint32_t width, uint32_t height)
{
    Reset_Frame_View_Allocation();
    Reset_Default_Viewport(width, height);
}

uint16_t Acquire_View()
{
    if (NextDynamicViewId > MaxDynamicViewId) {
        return CurrentMainViewId;
    }

    const uint16_t view_id = NextDynamicViewId++;
    bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
    bgfx::setViewFrameBuffer(view_id, CurrentFrameBuffer);
    bgfx::setViewRect(
        view_id,
        static_cast<uint16_t>(PendingViewportX),
        static_cast<uint16_t>(PendingViewportY),
        static_cast<uint16_t>(PendingViewportWidth),
        static_cast<uint16_t>(PendingViewportHeight));
    bgfx::setViewClear(view_id, 0, 0, 1.0f, 0);
    return view_id;
}

uint16_t Configure_View(const Matrix4 &view, const Matrix4 &projection)
{
    for (const ViewTransformState &configured_view : ConfiguredViews) {
        if (Matrices_Are_Equal(configured_view.View, view) &&
            Matrices_Are_Equal(configured_view.Projection, projection) &&
            configured_view.FrameBufferIndex == CurrentFrameBuffer.idx &&
            configured_view.ViewportX == PendingViewportX &&
            configured_view.ViewportY == PendingViewportY &&
            configured_view.ViewportWidth == PendingViewportWidth &&
            configured_view.ViewportHeight == PendingViewportHeight) {
            CurrentMainViewId = configured_view.ViewId;
            return configured_view.ViewId;
        }
    }

    const uint16_t view_id = Acquire_View();
    const Matrix4 bgfx_view = view.Transpose();
    const Matrix4 bgfx_projection = projection.Transpose();
    bgfx::setViewTransform(view_id, &bgfx_view[0][0], &bgfx_projection[0][0]);
    ConfiguredViews.push_back({
        view,
        projection,
        CurrentFrameBuffer.idx,
        static_cast<uint16_t>(PendingViewportX),
        static_cast<uint16_t>(PendingViewportY),
        static_cast<uint16_t>(PendingViewportWidth),
        static_cast<uint16_t>(PendingViewportHeight),
        view_id});
    CurrentMainViewId = view_id;
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

uint64_t Convert_Blend_Factor(D3DBLEND factor)
{
    switch (factor) {
    case D3DBLEND_ZERO:
        return BGFX_STATE_BLEND_ZERO;
    case D3DBLEND_ONE:
        return BGFX_STATE_BLEND_ONE;
    case D3DBLEND_SRCCOLOR:
        return BGFX_STATE_BLEND_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR:
        return BGFX_STATE_BLEND_INV_SRC_COLOR;
    case D3DBLEND_SRCALPHA:
    case D3DBLEND_BOTHSRCALPHA:
        return BGFX_STATE_BLEND_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA:
    case D3DBLEND_BOTHINVSRCALPHA:
        return BGFX_STATE_BLEND_INV_SRC_ALPHA;
    case D3DBLEND_DESTALPHA:
        return BGFX_STATE_BLEND_DST_ALPHA;
    case D3DBLEND_INVDESTALPHA:
        return BGFX_STATE_BLEND_INV_DST_ALPHA;
    case D3DBLEND_DESTCOLOR:
        return BGFX_STATE_BLEND_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR:
        return BGFX_STATE_BLEND_INV_DST_COLOR;
    case D3DBLEND_SRCALPHASAT:
        return BGFX_STATE_BLEND_SRC_ALPHA_SAT;
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

uint64_t Convert_Blend_Equation(D3DBLENDOP operation)
{
    switch (operation) {
    case D3DBLENDOP_SUBTRACT:
        return BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_SUB);
    case D3DBLENDOP_REVSUBTRACT:
        return BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_REVSUB);
    case D3DBLENDOP_MIN:
        return BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MIN);
    case D3DBLENDOP_MAX:
        return BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MAX);
    case D3DBLENDOP_ADD:
    default:
        return BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD);
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

uint64_t Convert_Depth_Test(D3DCMPFUNC compare)
{
    switch (compare) {
    case D3DCMP_NEVER:
        return BGFX_STATE_DEPTH_TEST_NEVER;
    case D3DCMP_LESS:
        return BGFX_STATE_DEPTH_TEST_LESS;
    case D3DCMP_EQUAL:
        return BGFX_STATE_DEPTH_TEST_EQUAL;
    case D3DCMP_LESSEQUAL:
        return BGFX_STATE_DEPTH_TEST_LEQUAL;
    case D3DCMP_GREATER:
        return BGFX_STATE_DEPTH_TEST_GREATER;
    case D3DCMP_NOTEQUAL:
        return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
    case D3DCMP_GREATEREQUAL:
        return BGFX_STATE_DEPTH_TEST_GEQUAL;
    case D3DCMP_ALWAYS:
    default:
        return BGFX_STATE_DEPTH_TEST_ALWAYS;
    }
}

uint32_t Convert_Stencil_Test(D3DCMPFUNC compare)
{
    switch (compare) {
    case D3DCMP_NEVER:
        return BGFX_STENCIL_TEST_NEVER;
    case D3DCMP_LESS:
        return BGFX_STENCIL_TEST_LESS;
    case D3DCMP_EQUAL:
        return BGFX_STENCIL_TEST_EQUAL;
    case D3DCMP_LESSEQUAL:
        return BGFX_STENCIL_TEST_LEQUAL;
    case D3DCMP_GREATER:
        return BGFX_STENCIL_TEST_GREATER;
    case D3DCMP_NOTEQUAL:
        return BGFX_STENCIL_TEST_NOTEQUAL;
    case D3DCMP_GREATEREQUAL:
        return BGFX_STENCIL_TEST_GEQUAL;
    case D3DCMP_ALWAYS:
    default:
        return BGFX_STENCIL_TEST_ALWAYS;
    }
}

uint32_t Convert_Stencil_Fail_Operation(D3DSTENCILOP operation)
{
    switch (operation) {
    case D3DSTENCILOP_ZERO:
        return BGFX_STENCIL_OP_FAIL_S_ZERO;
    case D3DSTENCILOP_REPLACE:
        return BGFX_STENCIL_OP_FAIL_S_REPLACE;
    case D3DSTENCILOP_INCRSAT:
        return BGFX_STENCIL_OP_FAIL_S_INCRSAT;
    case D3DSTENCILOP_DECRSAT:
        return BGFX_STENCIL_OP_FAIL_S_DECRSAT;
    case D3DSTENCILOP_INVERT:
        return BGFX_STENCIL_OP_FAIL_S_INVERT;
    case D3DSTENCILOP_INCR:
        return BGFX_STENCIL_OP_FAIL_S_INCR;
    case D3DSTENCILOP_DECR:
        return BGFX_STENCIL_OP_FAIL_S_DECR;
    case D3DSTENCILOP_KEEP:
    default:
        return BGFX_STENCIL_OP_FAIL_S_KEEP;
    }
}

uint32_t Convert_Stencil_Depth_Fail_Operation(D3DSTENCILOP operation)
{
    switch (operation) {
    case D3DSTENCILOP_ZERO:
        return BGFX_STENCIL_OP_FAIL_Z_ZERO;
    case D3DSTENCILOP_REPLACE:
        return BGFX_STENCIL_OP_FAIL_Z_REPLACE;
    case D3DSTENCILOP_INCRSAT:
        return BGFX_STENCIL_OP_FAIL_Z_INCRSAT;
    case D3DSTENCILOP_DECRSAT:
        return BGFX_STENCIL_OP_FAIL_Z_DECRSAT;
    case D3DSTENCILOP_INVERT:
        return BGFX_STENCIL_OP_FAIL_Z_INVERT;
    case D3DSTENCILOP_INCR:
        return BGFX_STENCIL_OP_FAIL_Z_INCR;
    case D3DSTENCILOP_DECR:
        return BGFX_STENCIL_OP_FAIL_Z_DECR;
    case D3DSTENCILOP_KEEP:
    default:
        return BGFX_STENCIL_OP_FAIL_Z_KEEP;
    }
}

uint32_t Convert_Stencil_Pass_Operation(D3DSTENCILOP operation)
{
    switch (operation) {
    case D3DSTENCILOP_ZERO:
        return BGFX_STENCIL_OP_PASS_Z_ZERO;
    case D3DSTENCILOP_REPLACE:
        return BGFX_STENCIL_OP_PASS_Z_REPLACE;
    case D3DSTENCILOP_INCRSAT:
        return BGFX_STENCIL_OP_PASS_Z_INCRSAT;
    case D3DSTENCILOP_DECRSAT:
        return BGFX_STENCIL_OP_PASS_Z_DECRSAT;
    case D3DSTENCILOP_INVERT:
        return BGFX_STENCIL_OP_PASS_Z_INVERT;
    case D3DSTENCILOP_INCR:
        return BGFX_STENCIL_OP_PASS_Z_INCR;
    case D3DSTENCILOP_DECR:
        return BGFX_STENCIL_OP_PASS_Z_DECR;
    case D3DSTENCILOP_KEEP:
    default:
        return BGFX_STENCIL_OP_PASS_Z_KEEP;
    }
}

unsigned Resolve_Render_State(D3DRENDERSTATETYPE state, unsigned default_value)
{
    const unsigned value = DX8Wrapper::Get_DX8_Render_State(state);
    return value != kUnsetRenderState ? value : default_value;
}

bool Resolve_Render_State_Bool(D3DRENDERSTATETYPE state, bool default_value)
{
    return Resolve_Render_State(state, default_value ? TRUE : FALSE) != FALSE;
}

uint32_t Build_Stencil_State()
{
    if (!Resolve_Render_State_Bool(D3DRS_STENCILENABLE, false)) {
        return BGFX_STENCIL_NONE;
    }

    const unsigned reference = Resolve_Render_State(D3DRS_STENCILREF, 0u) & 0xffu;
    const unsigned read_mask = Resolve_Render_State(D3DRS_STENCILMASK, 0xffu) & 0xffu;
    return Convert_Stencil_Test(static_cast<D3DCMPFUNC>(Resolve_Render_State(D3DRS_STENCILFUNC, D3DCMP_ALWAYS)))
        | BGFX_STENCIL_FUNC_REF(reference)
        | BGFX_STENCIL_FUNC_RMASK(read_mask)
        | Convert_Stencil_Fail_Operation(static_cast<D3DSTENCILOP>(Resolve_Render_State(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP)))
        | Convert_Stencil_Depth_Fail_Operation(static_cast<D3DSTENCILOP>(Resolve_Render_State(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP)))
        | Convert_Stencil_Pass_Operation(static_cast<D3DSTENCILOP>(Resolve_Render_State(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP)));
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
            destination[1] = source[1] ^ 0x80;
            destination[2] = source[0] ^ 0x80;
            destination[3] = 0xff;
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_L6V5U5: {
            const uint16_t packed = static_cast<uint16_t>(source[0]) | (static_cast<uint16_t>(source[1]) << 8);
            destination[0] = 0x00;
            destination[1] = Expand_5_To_8(static_cast<uint8_t>((packed >> 5) & 0x1f)) ^ 0x80;
            destination[2] = Expand_5_To_8(static_cast<uint8_t>(packed & 0x1f)) ^ 0x80;
            destination[3] = Expand_6_To_8(static_cast<uint8_t>((packed >> 10) & 0x3f));
            source_pixels += 2;
            break;
        }
        case WW3D_FORMAT_X8L8V8U8: {
            destination[0] = source[3];
            destination[1] = source[1] ^ 0x80;
            destination[2] = source[0] ^ 0x80;
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
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    case WW3D_FORMAT_A1R5G5B5:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    case WW3D_FORMAT_X1R5G5B5:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    case WW3D_FORMAT_A4R4G4B4:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    case WW3D_FORMAT_X4R4G4B4:
        bgfx_format = bgfx::TextureFormat::BGRA8;
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
    case WW3D_FORMAT_R8G8B8:
    case WW3D_FORMAT_R3G3B2:
    case WW3D_FORMAT_A8:
    case WW3D_FORMAT_A8R3G3B2:
    case WW3D_FORMAT_L8:
    case WW3D_FORMAT_A8L8:
    case WW3D_FORMAT_A4L4:
    case WW3D_FORMAT_U8V8:
    case WW3D_FORMAT_L6V5U5:
    case WW3D_FORMAT_X8L8V8U8:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        direct_copy = false;
        return true;
    default:
        bgfx_format = bgfx::TextureFormat::Count;
        direct_copy = false;
        return false;
    }
}

bool Get_Bgfx_Render_Target_Format(WW3DFormat format, bgfx::TextureFormat::Enum &bgfx_format)
{
    switch (format) {
    case WW3D_FORMAT_A8R8G8B8:
    case WW3D_FORMAT_X8R8G8B8:
        bgfx_format = bgfx::TextureFormat::BGRA8;
        return true;
    case WW3D_FORMAT_R5G6B5:
        bgfx_format = bgfx::TextureFormat::R5G6B5;
        return true;
    case WW3D_FORMAT_A1R5G5B5:
    case WW3D_FORMAT_X1R5G5B5:
        bgfx_format = bgfx::TextureFormat::BGR5A1;
        return true;
    case WW3D_FORMAT_A4R4G4B4:
    case WW3D_FORMAT_X4R4G4B4:
        bgfx_format = bgfx::TextureFormat::BGRA4;
        return true;
    default:
        bgfx_format = bgfx::TextureFormat::Count;
        return false;
    }
}

bool Is_Bgfx_Texture_Format_Supported(bgfx::TextureFormat::Enum bgfx_format, uint64_t texture_flags, uint32_t capability_flags)
{
    if (bgfx_format == bgfx::TextureFormat::Count) {
        return false;
    }

    const bgfx::Caps *caps = bgfx::getCaps();
    if (caps == nullptr) {
        return true;
    }

    return (caps->formats[bgfx_format] & capability_flags) != 0
        && bgfx::isTextureValid(0, false, 1, bgfx_format, texture_flags);
}

bool Get_Render_Target_Depth_Format(bgfx::TextureFormat::Enum &depth_format)
{
    static constexpr bgfx::TextureFormat::Enum kDepthFormats[] = {
        bgfx::TextureFormat::D24S8,
        bgfx::TextureFormat::D32,
        bgfx::TextureFormat::D24,
        bgfx::TextureFormat::D16,
        bgfx::TextureFormat::D32F,
        bgfx::TextureFormat::D24F,
        bgfx::TextureFormat::D16F};

    for (bgfx::TextureFormat::Enum candidate : kDepthFormats) {
        if (Is_Bgfx_Texture_Format_Supported(candidate, BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER)) {
            depth_format = candidate;
            return true;
        }
    }

    depth_format = bgfx::TextureFormat::Count;
    return false;
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

bool Is_Runtime_Texture_Format_Supported(WW3DFormat format)
{
    bgfx::TextureFormat::Enum bgfx_format = bgfx::TextureFormat::Count;
    bool direct_copy = false;
    if (!Get_Bgfx_Texture_Format(format, bgfx_format, direct_copy)) {
        return false;
    }

    // Most legacy runtime texture formats are normalized into the preferred
    // 32-bit upload format before they reach bgfx, so they do not require
    // native support for the original packed source format.
    if (!direct_copy && !Is_Compressed_Format(format)) {
        return true;
    }

    return Is_Bgfx_Texture_Format_Supported(bgfx_format, BGFX_TEXTURE_NONE, BGFX_CAPS_FORMAT_TEXTURE_2D);
}

bool Is_Render_Target_Format_Supported(WW3DFormat format)
{
    bgfx::TextureFormat::Enum color_format = bgfx::TextureFormat::Count;
    if (!Get_Bgfx_Render_Target_Format(format, color_format)) {
        return false;
    }

    bgfx::TextureFormat::Enum depth_format = bgfx::TextureFormat::Count;
    return Get_Render_Target_Depth_Format(depth_format)
        && Is_Bgfx_Texture_Format_Supported(color_format, BGFX_TEXTURE_RT, BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER);
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
    ActiveWidth = Width;
    ActiveHeight = Height;
    Reset_Frame_State(ActiveWidth, ActiveHeight);
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
    Reset_Frame_State(0, 0);
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
    bgfx::setViewFrameBuffer(OverlayViewId, BGFX_INVALID_HANDLE);
    Reset_Frame_State(ActiveWidth, ActiveHeight);
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
    bgfx::setViewFrameBuffer(OverlayViewId, frame_buffer);
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(ActiveWidth), static_cast<uint16_t>(ActiveHeight));
    Reset_Default_Viewport(ActiveWidth, ActiveHeight);
    Invalidate_Configured_Views();
    return true;
}

TextureClass *BgfxRenderer::Create_Render_Target_Texture(uint32_t width, uint32_t height, WW3DFormat format)
{
    if (!IsInitted || width == 0 || height == 0 || !Supports_Render_Target_Format(format)) {
        return nullptr;
    }

    const bgfx::Caps *caps = bgfx::getCaps();
    if (caps == nullptr || caps->limits.maxTextureSize == 0) {
        return nullptr;
    }

    uint32_t size = width;
    if (height < width) {
        size = height;
    }

    size = static_cast<uint32_t>(::Find_POT(static_cast<int>(size)));
    size = std::min(size, caps->limits.maxTextureSize);
    if (size == 0) {
        return nullptr;
    }

    TextureClass *texture = NEW_REF(
        TextureClass,
        (size, size, format, TextureClass::MIP_LEVELS_1, TextureClass::POOL_DEFAULT, true));
    if (texture == nullptr) {
        return nullptr;
    }

    if (!bgfx::isValid(texture->Get_Bgfx_Texture()) || !bgfx::isValid(texture->Get_Bgfx_Frame_Buffer())) {
        REF_PTR_RELEASE(texture);
        return nullptr;
    }

    return texture;
}

void BgfxRenderer::Reset_Render_Target()
{
    if (!IsInitted) {
        return;
    }

    ActiveWidth = Width;
    ActiveHeight = Height;
    CurrentFrameBuffer = BGFX_INVALID_HANDLE;
    bgfx::setViewFrameBuffer(OverlayViewId, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(OverlayViewId, 0, 0, static_cast<uint16_t>(Width), static_cast<uint16_t>(Height));
    Reset_Default_Viewport(ActiveWidth, ActiveHeight);
    Invalidate_Configured_Views();
}

bool BgfxRenderer::Has_Render_Target()
{
    return IsInitted && bgfx::isValid(CurrentFrameBuffer);
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
    Reset_Frame_View_Allocation();
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

bgfx::ProgramHandle BgfxRenderer::Get_Overlay_Program()
{
    return OverlayProgram;
}

bgfx::ProgramHandle BgfxRenderer::Get_Mesh_Program(MeshShaderProgram program)
{
    switch (program) {
    case MeshShaderProgram::Mesh:        return MeshProgram;
    case MeshShaderProgram::MeshTexgen:  return MeshTexgenProgram;
    default:                             return MeshProgram;
    }
}

bgfx::UniformHandle BgfxRenderer::Get_Fog_Config_Uniform() { return MeshFogConfigUniform; }
bgfx::UniformHandle BgfxRenderer::Get_Fog_Color_Uniform() { return MeshFogColorUniform; }
bgfx::UniformHandle BgfxRenderer::Get_Frag_Config_Uniform() { return MeshFragConfigUniform; }
bgfx::UniformHandle BgfxRenderer::Get_Frag_Config2_Uniform() { return MeshFragConfig2Uniform; }

const bgfx::VertexLayout &BgfxRenderer::Get_Overlay_Layout()
{
    return OverlayVertexLayout;
}

bool BgfxRenderer::Supports_Texture_Format(WW3DFormat format)
{
    return format != WW3D_FORMAT_UNKNOWN && Is_Runtime_Texture_Format_Supported(format);
}

bool BgfxRenderer::Supports_Render_Target_Format(WW3DFormat format)
{
    return format != WW3D_FORMAT_UNKNOWN && Is_Render_Target_Format_Supported(format);
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
        if (!Get_Bgfx_Render_Target_Format(texture.Get_Texture_Format(), texture_format) ||
            !Supports_Render_Target_Format(texture.Get_Texture_Format())) {
            return BGFX_INVALID_HANDLE;
        }

        bgfx::TextureFormat::Enum depth_format = bgfx::TextureFormat::Count;
        if (!Get_Render_Target_Depth_Format(depth_format)) {
            WWDEBUG_SAY(("BgfxRenderer::Create_Texture could not find a depth format for render target textures\n"));
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

        texture.BgfxDepthTexture = bgfx::createTexture2D(
            static_cast<uint16_t>(texture.Get_Width()),
            static_cast<uint16_t>(texture.Get_Height()),
            false,
            1,
            depth_format,
            BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);
        if (!bgfx::isValid(texture.BgfxDepthTexture)) {
            bgfx::destroy(handle);
            return BGFX_INVALID_HANDLE;
        }

        bgfx::Attachment attachments[2];
        attachments[0].init(handle);
        attachments[1].init(texture.BgfxDepthTexture);
        if (!bgfx::isFrameBufferValid(2, attachments)) {
            bgfx::destroy(texture.BgfxDepthTexture);
            texture.BgfxDepthTexture = BGFX_INVALID_HANDLE;
            bgfx::destroy(handle);
            return BGFX_INVALID_HANDLE;
        }

        texture.BgfxFrameBuffer = bgfx::createFrameBuffer(2, attachments, false);
        if (!bgfx::isValid(texture.BgfxFrameBuffer)) {
            bgfx::destroy(texture.BgfxDepthTexture);
            texture.BgfxDepthTexture = BGFX_INVALID_HANDLE;
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

    // In the active bgfx build, ShaderClass is still the authoritative owner of
    // color/depth/blend pipeline state. DX8Wrapper's cache does not yet populate
    // the broader D3DRS override surface consistently enough to drive bgfx state
    // directly without regressing core world/menu rendering.
    if (shader.Get_Color_Mask() == ShaderClass::COLOR_WRITE_ENABLE) {
        state |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
    }

    if (shader.Get_Depth_Mask() == ShaderClass::DEPTH_WRITE_ENABLE) {
        state |= BGFX_STATE_WRITE_Z;
    }

    state |= Convert_Depth_Test(shader.Get_Depth_Compare());

    const unsigned resolved_cull_mode = shader.Get_Cull_Mode() == ShaderClass::CULL_MODE_ENABLE ? cull_mode : D3DCULL_NONE;
    if (resolved_cull_mode == D3DCULL_CCW) {
        state |= BGFX_STATE_CULL_CCW;
    } else if (resolved_cull_mode == D3DCULL_CW) {
        state |= BGFX_STATE_CULL_CW;
    }

    if (shader.Get_Src_Blend_Func() != ShaderClass::SRCBLEND_ONE
        || shader.Get_Dst_Blend_Func() != ShaderClass::DSTBLEND_ZERO) {
        const uint64_t src_factor = Convert_Blend_Factor(shader.Get_Src_Blend_Func());
        const uint64_t dst_factor = Convert_Blend_Factor(shader.Get_Dst_Blend_Func());
        state |= BGFX_STATE_BLEND_FUNC(
            src_factor,
            dst_factor);
    }

    return state;
}

void BgfxRenderer::Apply_Render_State(const ShaderClass &shader, unsigned cull_mode, uint64_t extra_state)
{
    bgfx::setState(Build_Render_State(shader, cull_mode) | extra_state);
    bgfx::setStencil(Build_Stencil_State());
}

void BgfxRenderer::Apply_Overlay_Config(bool has_texture)
{
    float overlay_config[4] = {has_texture ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
    bgfx::setUniform(OverlayConfigUniform, overlay_config);
}

namespace
{
StageColorOp Map_Stage0_Color_Op(const ShaderClass &shader)
{
    if (shader.Get_Texturing() == ShaderClass::TEXTURING_DISABLE) {
        return shader.Get_Primary_Gradient() == ShaderClass::GRADIENT_DISABLE
            ? STAGE_DISABLE : STAGE_SELECT_CURRENT;
    }
    switch (shader.Get_Primary_Gradient()) {
    case ShaderClass::GRADIENT_DISABLE:              return STAGE_SELECT_TEXTURE;
    case ShaderClass::GRADIENT_ADD:                   return STAGE_ADD;
    case ShaderClass::GRADIENT_BUMPENVMAP:            return STAGE_BUMPENVMAP;
    case ShaderClass::GRADIENT_BUMPENVMAPLUMINANCE:   return STAGE_BUMPENVMAP_LUM;
    case ShaderClass::GRADIENT_DOTPRODUCT3:           return STAGE_DOTPRODUCT3;
    default:                                          return STAGE_MODULATE;
    }
}

StageColorOp Map_Stage0_Alpha_Op(const ShaderClass &shader)
{
    if (shader.Get_Texturing() == ShaderClass::TEXTURING_DISABLE) {
        return shader.Get_Primary_Gradient() == ShaderClass::GRADIENT_DISABLE
            ? STAGE_DISABLE : STAGE_SELECT_CURRENT;
    }
    switch (shader.Get_Primary_Gradient()) {
    case ShaderClass::GRADIENT_DISABLE: return STAGE_SELECT_TEXTURE;
    case ShaderClass::GRADIENT_BUMPENVMAP:
    case ShaderClass::GRADIENT_BUMPENVMAPLUMINANCE:
    case ShaderClass::GRADIENT_DOTPRODUCT3:     return STAGE_DISABLE;
    default:                            return STAGE_MODULATE;
    }
}

StageColorOp Map_Stage1_Color_Op(const ShaderClass &shader)
{
    if (shader.Get_Texturing() == ShaderClass::TEXTURING_DISABLE) return STAGE_DISABLE;
    switch (shader.Get_Post_Detail_Color_Func()) {
    case ShaderClass::DETAILCOLOR_DETAIL:      return STAGE_SELECT_TEXTURE;
    case ShaderClass::DETAILCOLOR_SCALE:       return STAGE_MODULATE;
    case ShaderClass::DETAILCOLOR_INVSCALE:    return STAGE_ADDSMOOTH;
    case ShaderClass::DETAILCOLOR_ADD:         return STAGE_ADD;
    case ShaderClass::DETAILCOLOR_SUB:         return STAGE_SUBTRACT;
    case ShaderClass::DETAILCOLOR_BLEND:       return STAGE_BLEND_TEX_ALPHA;
    case ShaderClass::DETAILCOLOR_DETAILBLEND: return STAGE_BLEND_CUR_ALPHA;
    default:                                   return STAGE_DISABLE;
    }
}

StageColorOp Map_Stage1_Alpha_Op(const ShaderClass &shader)
{
    if (shader.Get_Texturing() == ShaderClass::TEXTURING_DISABLE) return STAGE_DISABLE;
    switch (shader.Get_Post_Detail_Alpha_Func()) {
    case ShaderClass::DETAILALPHA_DETAIL:   return STAGE_SELECT_TEXTURE;
    case ShaderClass::DETAILALPHA_SCALE:    return STAGE_MODULATE;
    case ShaderClass::DETAILALPHA_INVSCALE: return STAGE_ADDSMOOTH;
    default:                                return STAGE_DISABLE;
    }
}

float Resolve_Texgen_Mode(unsigned tci_flags)
{
    const unsigned mode = tci_flags & 0xffff0000u;
    if (mode == D3DTSS_TCI_CAMERASPACENORMAL) return 1.0f;
    if (mode == D3DTSS_TCI_CAMERASPACEPOSITION) return 2.0f;
    if (mode == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR) return 3.0f;
    return 0.0f;
}

float Resolve_Tex_Transform_Flags(unsigned flags)
{
    if (flags == D3DTTFF_DISABLE) return 0.0f;
    if (flags == D3DTTFF_COUNT2) return 1.0f;
    return 2.0f; // D3DTTFF_COUNT3 | D3DTTFF_PROJECTED
}

unsigned Sanitize_Texcoord_Index(unsigned stage)
{
    const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, D3DTSS_TEXCOORDINDEX);
    return value != kUnsetRenderState ? value : (D3DTSS_TCI_PASSTHRU | stage);
}

unsigned Sanitize_Tex_Transform_Flags(unsigned stage)
{
    const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, D3DTSS_TEXTURETRANSFORMFLAGS);
    return value != kUnsetRenderState ? value : D3DTTFF_DISABLE;
}

}

MaterialClassification BgfxRenderer::Classify_Material(const ShaderClass &shader, const VertexMaterialClass *material, bool has_normals)
{
    MaterialClassification c{};

    // Determine program variant: Mesh or MeshTexgen
    bool needs_texgen = false;
    if (material != nullptr) {
        VertexMaterialClass *mutable_material = const_cast<VertexMaterialClass *>(material);
        for (int i = 0; i < 2; ++i) {
            if (mutable_material->Peek_Mapper(i) != nullptr) {
                needs_texgen = true;
                break;
            }
        }
    }
    c.program = needs_texgen ? MeshShaderProgram::MeshTexgen : MeshShaderProgram::Mesh;

    // Pre-compute fragment config (stage ops from shader)
    c.frag_config[0] = static_cast<float>(Map_Stage0_Color_Op(shader));
    c.frag_config[1] = static_cast<float>(Map_Stage1_Color_Op(shader));

    float alpha_test_ref = -1.0f;
    if (shader.Get_Alpha_Test() == ShaderClass::ALPHATEST_ENABLE) {
        unsigned char alphareference = 0x60;
        if (shader.Get_Src_Blend_Func() == ShaderClass::SRCBLEND_ONE_MINUS_SRC_ALPHA) {
            alphareference = 0xff - 0x60;
        }
        alpha_test_ref = static_cast<float>(alphareference) / 255.0f;
    }
    c.frag_config[2] = alpha_test_ref;
    c.frag_config[3] = static_cast<float>(Map_Stage0_Alpha_Op(shader));

    float fog_mode = 0.0f;
    switch (shader.Get_Fog_Func()) {
    case ShaderClass::FOG_ENABLE:         fog_mode = 1.0f; break;
    case ShaderClass::FOG_SCALE_FRAGMENT: fog_mode = 2.0f; break;
    case ShaderClass::FOG_WHITE:          fog_mode = 3.0f; break;
    default: break;
    }

    c.frag_config2[0] = static_cast<float>(Map_Stage1_Alpha_Op(shader));
    c.frag_config2[1] = fog_mode;
    c.frag_config2[2] = 0.0f;
    c.frag_config2[3] = 0.0f;

    // Pre-compute lighting config and material colors
    bool uses_lighting = material != nullptr && material->Get_Lighting();

    if (uses_lighting) {
        // Lighting mode: 1 = emissive only (no normals), 2 = full per-pixel lighting
        c.lit_config[0] = has_normals ? 2.0f : 1.0f;

        // Color sources from material (COLOR1 = vertex color, MATERIAL = material color)
        auto resolve_source = [](unsigned src) -> float {
            return (src == VertexMaterialClass::COLOR1) ? 1.0f : 0.0f;
        };
        c.lit_config[1] = resolve_source(material->Get_Diffuse_Color_Source());
        c.lit_config[2] = resolve_source(material->Get_Ambient_Color_Source());
        c.lit_config[3] = resolve_source(material->Get_Emissive_Color_Source());

        // Material colors
        Vector3 ambient(1.0f, 1.0f, 1.0f);
        Vector3 diffuse(1.0f, 1.0f, 1.0f);
        Vector3 emissive(0.0f, 0.0f, 0.0f);
        material->Get_Ambient(&ambient);
        material->Get_Diffuse(&diffuse);
        material->Get_Emissive(&emissive);

        c.material_ambient[0] = ambient.X;
        c.material_ambient[1] = ambient.Y;
        c.material_ambient[2] = ambient.Z;
        c.material_ambient[3] = 1.0f;

        c.material_diffuse[0] = diffuse.X;
        c.material_diffuse[1] = diffuse.Y;
        c.material_diffuse[2] = diffuse.Z;
        c.material_diffuse[3] = material->Get_Opacity();

        c.material_emissive[0] = emissive.X;
        c.material_emissive[1] = emissive.Y;
        c.material_emissive[2] = emissive.Z;
        c.material_emissive[3] = 1.0f;
    } else {
        // Unlit: mode 0, material colors don't matter (vertex color is used directly)
        c.lit_config[0] = 0.0f;
        c.lit_config[1] = 0.0f;
        c.lit_config[2] = 0.0f;
        c.lit_config[3] = 0.0f;

        c.material_ambient[0] = 1.0f; c.material_ambient[1] = 1.0f;
        c.material_ambient[2] = 1.0f; c.material_ambient[3] = 1.0f;
        c.material_diffuse[0] = 1.0f; c.material_diffuse[1] = 1.0f;
        c.material_diffuse[2] = 1.0f; c.material_diffuse[3] = 1.0f;
        c.material_emissive[0] = 0.0f; c.material_emissive[1] = 0.0f;
        c.material_emissive[2] = 0.0f; c.material_emissive[3] = 1.0f;
    }

    return c;
}

void BgfxRenderer::Apply_Lighting_Uniforms(const MaterialClassification &classification)
{
    bgfx::setUniform(MeshLitConfigUniform, classification.lit_config);
    const bool lighting_active = classification.lit_config[0] > 0.5f;
    if (lighting_active) {
        bgfx::setUniform(MeshMaterialAmbientUniform, classification.material_ambient);
        bgfx::setUniform(MeshMaterialDiffuseUniform, classification.material_diffuse);
        bgfx::setUniform(MeshMaterialEmissiveUniform, classification.material_emissive);

        // Scene ambient (per-mesh, from DX8Wrapper)
        const unsigned ambient_color = DX8Wrapper::Get_DX8_Render_State(D3DRS_AMBIENT);
        float scene_ambient[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (ambient_color != kUnsetRenderState) {
            scene_ambient[0] = static_cast<float>((ambient_color >> 16) & 0xffu) / 255.0f;
            scene_ambient[1] = static_cast<float>((ambient_color >> 8) & 0xffu) / 255.0f;
            scene_ambient[2] = static_cast<float>(ambient_color & 0xffu) / 255.0f;
        }
        bgfx::setUniform(MeshSceneAmbientUniform, scene_ambient);

        // Directional lights (per-mesh, from DX8Wrapper)
        float light_dir[16] = {};
        float light_color[16] = {};
        for (unsigned i = 0; i < 4u; ++i) {
            const D3DLIGHT8 &light = DX8Wrapper::Peek_Light(i);
            const size_t off = static_cast<size_t>(i) * 4u;
            light_dir[off + 0] = light.Direction.x;
            light_dir[off + 1] = light.Direction.y;
            light_dir[off + 2] = light.Direction.z;
            light_dir[off + 3] = DX8Wrapper::Is_Light_Enabled(i) ? 1.0f : 0.0f;
            light_color[off + 0] = light.Diffuse.r;
            light_color[off + 1] = light.Diffuse.g;
            light_color[off + 2] = light.Diffuse.b;
            light_color[off + 3] = 0.0f;
        }
        bgfx::setUniform(MeshLightDirUniform, light_dir, 4);
        bgfx::setUniform(MeshLightColorUniform, light_color, 4);
    }
}

void BgfxRenderer::Apply_Bump_Env_Uniforms(const MaterialClassification &classification)
{
    float stage0_op = classification.frag_config[0];
    bool is_bump = (stage0_op > 8.5f && stage0_op < 11.5f);
    if (!is_bump) return;

    // Read bump env matrix from DX8Wrapper texture stage state (set by BumpEnvTextureMapperClass::Apply)
    auto dw2f = [](unsigned dw) -> float {
        float f;
        std::memcpy(&f, &dw, sizeof(f));
        return f;
    };
    float bump_mat[4] = {
        dw2f(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00)),
        dw2f(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01)),
        dw2f(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10)),
        dw2f(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11)),
    };
    bgfx::setUniform(MeshBumpEnvMatUniform, bump_mat);

    // Match the original DX8 wrapper state layout: luminance scale/offset live on stage 1,
    // while the bump rotation matrix is stored on stage 0.
    float bump_lum[4] = {
        dw2f(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE)),
        dw2f(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET)),
        0.0f, 0.0f
    };
    bgfx::setUniform(MeshBumpEnvLumUniform, bump_lum);
}

void BgfxRenderer::Apply_Texgen_Uniforms()
{
    const unsigned tci0 = Sanitize_Texcoord_Index(0);
    const unsigned tci1 = Sanitize_Texcoord_Index(1);
    float texgen_mode[4] = {
        Resolve_Texgen_Mode(tci0),
        static_cast<float>(tci0 & 0xffffu),
        Resolve_Texgen_Mode(tci1),
        static_cast<float>(tci1 & 0xffffu)};
    bgfx::setUniform(MeshTexgenModeUniform, texgen_mode);

    const unsigned ttf0 = Sanitize_Tex_Transform_Flags(0);
    const unsigned ttf1 = Sanitize_Tex_Transform_Flags(1);
    float tex_transform_flags[4] = {
        Resolve_Tex_Transform_Flags(ttf0),
        Resolve_Tex_Transform_Flags(ttf1),
        0.0f, 0.0f};
    bgfx::setUniform(MeshTexTransformFlagsUniform, tex_transform_flags);

    Matrix4 tex_transform0(true);
    Matrix4 tex_transform1(true);
    if (ttf0 != D3DTTFF_DISABLE) DX8Wrapper::Get_Transform(D3DTS_TEXTURE0, tex_transform0);
    if (ttf1 != D3DTTFF_DISABLE) DX8Wrapper::Get_Transform(D3DTS_TEXTURE1, tex_transform1);

    const Matrix4 bgfx_tex_transform0 = tex_transform0.Transpose();
    const Matrix4 bgfx_tex_transform1 = tex_transform1.Transpose();

    float tex_mat0[16];
    float tex_mat1[16];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            tex_mat0[r * 4 + c] = bgfx_tex_transform0[r][c];
            tex_mat1[r * 4 + c] = bgfx_tex_transform1[r][c];
        }
    }
    bgfx::setUniform(MeshTexTransform0Uniform, tex_mat0);
    bgfx::setUniform(MeshTexTransform1Uniform, tex_mat1);
}

std::uint32_t BgfxRenderer::Convert_Packed_Color(std::uint32_t argb_color)
{
    return Convert_ARGB_To_ABGR(argb_color);
}

// ============================================================================
// Draw submission (merged from bgfxfixedfunction.cpp)
// ============================================================================

namespace
{
bgfx::TextureHandle Resolve_Texture_Handle(TextureClass *texture)
{
    if (texture == nullptr) {
        return bgfx::isValid(BlackTexture) ? BlackTexture : BgfxRenderer::Get_White_Texture();
    }

    bgfx::TextureHandle handle = texture->Get_Bgfx_Texture();
    if (!bgfx::isValid(handle) && !texture->Is_Initialized()) {
        TextureLoader::Request_Foreground_Loading(texture);
        handle = texture->Get_Bgfx_Texture();
    }
    if (bgfx::isValid(handle)) {
        return handle;
    }
    return BgfxRenderer::Get_White_Texture();
}

uint32_t Resolve_Sampler_Flags(TextureClass *texture, unsigned stage)
{
    return texture != nullptr ? texture->Get_Bgfx_Sampler_Flags(stage) : 0u;
}

enum class FillMode { Solid, Wireframe, Points };

FillMode Resolve_Fill_Mode()
{
    switch (DX8Wrapper::Get_DX8_Render_State(D3DRS_FILLMODE)) {
    case D3DFILL_POINT:     return FillMode::Points;
    case D3DFILL_WIREFRAME: return FillMode::Wireframe;
    default:                return FillMode::Solid;
    }
}

uint64_t Resolve_Primitive_State(FillMode fill_mode)
{
    switch (fill_mode) {
    case FillMode::Wireframe: return BGFX_STATE_PT_LINES;
    case FillMode::Points:    return BGFX_STATE_PT_POINTS | BGFX_STATE_POINT_SIZE(1);
    default:                  return 0u;
    }
}

uint32_t Resolve_Submitted_Index_Count(FillMode fill_mode, bool strip, unsigned short polygon_count)
{
    switch (fill_mode) {
    case FillMode::Wireframe:
        return static_cast<uint32_t>(polygon_count) * 6u;
    case FillMode::Points:
        return strip ? static_cast<uint32_t>(polygon_count) + 2u : static_cast<uint32_t>(polygon_count) * 3u;
    default:
        return static_cast<uint32_t>(polygon_count) * 3u;
    }
}

void Write_Wireframe_Triangle(uint16_t *destination, unsigned short a, unsigned short b, unsigned short c, unsigned short min_vertex_index)
{
    destination[0] = static_cast<uint16_t>(a - min_vertex_index);
    destination[1] = static_cast<uint16_t>(b - min_vertex_index);
    destination[2] = static_cast<uint16_t>(b - min_vertex_index);
    destination[3] = static_cast<uint16_t>(c - min_vertex_index);
    destination[4] = static_cast<uint16_t>(c - min_vertex_index);
    destination[5] = static_cast<uint16_t>(a - min_vertex_index);
}

// Convert source indices into submission indices, handling wireframe/points/strip
// expansion and min_vertex_index subtraction.
void Build_Submission_Indices(
    uint16_t *dest,
    const unsigned short *source,
    unsigned short polygon_count,
    uint32_t submitted_index_count,
    unsigned short min_vertex_index,
    FillMode fill_mode,
    bool strip)
{
    if (fill_mode == FillMode::Wireframe) {
        for (unsigned short tri = 0; tri < polygon_count; ++tri) {
            unsigned short a, b, c;
            if (strip) {
                const bool odd = (tri & 1u) != 0u;
                a = source[tri + (odd ? 1 : 0)];
                b = source[tri + (odd ? 0 : 1)];
                c = source[tri + 2];
            } else {
                a = source[tri * 3 + 0];
                b = source[tri * 3 + 1];
                c = source[tri * 3 + 2];
            }
            Write_Wireframe_Triangle(dest + tri * 6u, a, b, c, min_vertex_index);
        }
    } else if (strip && fill_mode == FillMode::Solid) {
        for (unsigned short tri = 0; tri < polygon_count; ++tri) {
            const bool odd = (tri & 1u) != 0u;
            dest[tri * 3 + 0] = static_cast<uint16_t>(source[tri + (odd ? 1 : 0)] - min_vertex_index);
            dest[tri * 3 + 1] = static_cast<uint16_t>(source[tri + (odd ? 0 : 1)] - min_vertex_index);
            dest[tri * 3 + 2] = static_cast<uint16_t>(source[tri + 2] - min_vertex_index);
        }
    } else {
        for (uint32_t i = 0; i < submitted_index_count; ++i) {
            dest[i] = static_cast<uint16_t>(source[i] - min_vertex_index);
        }
    }
}

void Build_Transient_Vertices(
    bgfx::TransientVertexBuffer &tvb,
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    unsigned index_base_offset,
    unsigned short min_vertex_index,
    unsigned short vertex_count)
{
    const unsigned fvf = vertex_buffer.Vertex_Format_Info().Get_Vertex_Format();
    const unsigned fvf_size = vertex_buffer.Vertex_Format_Info().Get_Vertex_Size();
    const bool has_diffuse = (fvf & VERTEX_FORMAT_FLAG_DIFFUSE) != 0u;
    const bool has_specular = (fvf & VERTEX_FORMAT_FLAG_SPECULAR) != 0u;

    VertexBufferClass::AppendLockClass vertex_lock(
        const_cast<VertexBufferClass *>(&vertex_buffer),
        vertex_buffer_offset + index_base_offset + min_vertex_index,
        vertex_count);
    const unsigned char *source_vertices = reinterpret_cast<const unsigned char *>(vertex_lock.Get_Vertex_Array());

    std::memcpy(tvb.data, source_vertices, static_cast<size_t>(vertex_count) * fvf_size);

    if (has_diffuse) {
        const unsigned diffuse_offset = vertex_buffer.Vertex_Format_Info().Get_Diffuse_Offset();
        for (unsigned short v = 0; v < vertex_count; ++v) {
            uint32_t *color_ptr = reinterpret_cast<uint32_t *>(
                tvb.data + static_cast<size_t>(v) * fvf_size + diffuse_offset);
            *color_ptr = BgfxRenderer::Convert_Packed_Color(*color_ptr);
        }
    }

    if (has_specular) {
        const unsigned specular_offset = vertex_buffer.Vertex_Format_Info().Get_Specular_Offset();
        for (unsigned short v = 0; v < vertex_count; ++v) {
            uint32_t *color_ptr = reinterpret_cast<uint32_t *>(
                tvb.data + static_cast<size_t>(v) * fvf_size + specular_offset);
            *color_ptr = BgfxRenderer::Convert_Packed_Color(*color_ptr);
        }
    }
}

const bgfx::VertexLayout &Get_Vertex_Layout_For_Buffer(const VertexBufferClass &vertex_buffer)
{
    if (vertex_buffer.Type() == BUFFER_TYPE_RENDER) {
        return static_cast<const RenderVertexBufferClass &>(vertex_buffer).Get_Bgfx_Vertex_Layout();
    }
    return BgfxRenderer::Get_Fixed_Function_Layout();
}

float Decode_Dword_As_Float(unsigned value)
{
    float decoded = 0.0f;
    std::memcpy(&decoded, &value, sizeof(decoded));
    return decoded;
}

void Apply_Fog_Uniforms()
{
    bool fog_enabled = DX8Wrapper::Get_Fog_Enable();
    bool range_fog = false;
    if (fog_enabled) {
        unsigned fog_state = DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGTABLEMODE);
        if (fog_state == D3DFOG_NONE) fog_state = DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGVERTEXMODE);
        if (fog_state == D3DFOG_NONE || fog_state > D3DFOG_LINEAR) fog_enabled = false;
        range_fog = fog_enabled && DX8Wrapper::Get_DX8_Render_State(D3DRS_RANGEFOGENABLE) != FALSE;
    }

    float fog_config[4] = {
        fog_enabled ? 1.0f : 0.0f,
        Decode_Dword_As_Float(DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGSTART)),
        Decode_Dword_As_Float(DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGEND)),
        range_fog ? 1.0f : -1.0f};
    bgfx::setUniform(BgfxRenderer::Get_Fog_Config_Uniform(), fog_config);

    uint32_t fog_color_packed = DX8Wrapper::Get_Fog_Color();
    float fog_color[4] = {
        static_cast<float>((fog_color_packed >> 16) & 0xffu) / 255.0f,
        static_cast<float>((fog_color_packed >> 8) & 0xffu) / 255.0f,
        static_cast<float>(fog_color_packed & 0xffu) / 255.0f,
        0.0f};
    bgfx::setUniform(BgfxRenderer::Get_Fog_Color_Uniform(), fog_color);
}

bool Submit_Classified_Draw_Internal(
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    const IndexBufferClass &index_buffer,
    unsigned index_buffer_offset,
    unsigned index_base_offset,
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    TextureClass *const *textures,
    const VertexMaterialClass *material,
    const MaterialClassification &classification,
    bool receive_shadows,
    bool cast_shadows,
    const Matrix4 &world,
    const Matrix4 &view,
    const Matrix4 &projection,
    bool strip)
{
    if (!BgfxRenderer::Is_Initted() || vertex_count == 0 || polygon_count == 0) {
        return false;
    }

    const auto vertex_buffer_type = vertex_buffer.Type();
    const auto index_buffer_type = index_buffer.Type();
    const bool supported_vertex_buffer =
        vertex_buffer_type == BUFFER_TYPE_RENDER ||
        vertex_buffer_type == BUFFER_TYPE_SORTING ||
        vertex_buffer_type == BUFFER_TYPE_DYNAMIC_RENDER ||
        vertex_buffer_type == BUFFER_TYPE_DYNAMIC_SORTING;
    const bool supported_index_buffer =
        index_buffer_type == BUFFER_TYPE_RENDER ||
        index_buffer_type == BUFFER_TYPE_SORTING ||
        index_buffer_type == BUFFER_TYPE_DYNAMIC_RENDER ||
        index_buffer_type == BUFFER_TYPE_DYNAMIC_SORTING;
    if (!supported_vertex_buffer || !supported_index_buffer) {
        return false;
    }

    const FillMode fill_mode = Resolve_Fill_Mode();
    const uint32_t submitted_index_count = Resolve_Submitted_Index_Count(fill_mode, strip, polygon_count);
    const unsigned short source_index_count = strip
        ? static_cast<unsigned short>(polygon_count + 2)
        : static_cast<unsigned short>(polygon_count * 3u);

    const bgfx::VertexLayout &layout = Get_Vertex_Layout_For_Buffer(vertex_buffer);

    bool use_direct_vertex_buffer = false;
    if (vertex_buffer_type == BUFFER_TYPE_RENDER) {
        if (!static_cast<const RenderVertexBufferClass &>(vertex_buffer).Ensure_Bgfx_Buffer()) {
            return false;
        }
        use_direct_vertex_buffer = true;
    }

    bool use_direct_index_buffer = false;
    if (use_direct_vertex_buffer &&
        index_buffer_type == BUFFER_TYPE_RENDER &&
        fill_mode != FillMode::Wireframe &&
        (!strip || fill_mode == FillMode::Points)) {
        if (!static_cast<const RenderIndexBufferClass &>(index_buffer).Ensure_Bgfx_Buffer()) {
            return false;
        }
        use_direct_index_buffer = true;
    }

    if ((!use_direct_vertex_buffer && bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count) ||
        (!use_direct_index_buffer && bgfx::getAvailTransientIndexBuffer(submitted_index_count) < submitted_index_count)) {
        return false;
    }

    bgfx::TransientVertexBuffer transient_vertex_buffer;
    bgfx::TransientIndexBuffer transient_index_buffer;
    if (!use_direct_vertex_buffer) {
        bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, layout);
        Build_Transient_Vertices(
            transient_vertex_buffer, vertex_buffer,
            vertex_buffer_offset, index_base_offset,
            min_vertex_index, vertex_count);
    }

    if (!use_direct_index_buffer) {
        bgfx::allocTransientIndexBuffer(&transient_index_buffer, submitted_index_count);
        uint16_t *dest = reinterpret_cast<uint16_t *>(transient_index_buffer.data);

        const unsigned short *source_indices = nullptr;
        if (index_buffer_type == BUFFER_TYPE_RENDER) {
            source_indices =
                static_cast<const RenderIndexBufferClass &>(index_buffer).Get_Source_Index_Data()
                + index_buffer_offset + start_index;
        } else {
            IndexBufferClass::AppendLockClass index_lock(
                const_cast<IndexBufferClass *>(&index_buffer),
                index_buffer_offset + start_index,
                source_index_count);
            source_indices = index_lock.Get_Index_Array();
            Build_Submission_Indices(dest, source_indices, polygon_count,
                submitted_index_count, min_vertex_index, fill_mode, strip);
            source_indices = nullptr;
        }

        if (source_indices != nullptr) {
            Build_Submission_Indices(dest, source_indices, polygon_count,
                submitted_index_count, min_vertex_index, fill_mode, strip);
        }
    }

    // Bind vertex buffer
    const Matrix4 world_transform = world.Transpose();
    bgfx::setTransform(&world_transform[0][0]);
    if (use_direct_index_buffer) {
        bgfx::setVertexBuffer(
            0, static_cast<const RenderVertexBufferClass &>(vertex_buffer).Get_Bgfx_Vertex_Buffer());
    } else if (use_direct_vertex_buffer) {
        bgfx::setVertexBuffer(
            0, static_cast<const RenderVertexBufferClass &>(vertex_buffer).Get_Bgfx_Vertex_Buffer(),
            vertex_buffer_offset + index_base_offset + min_vertex_index, vertex_count);
    } else {
        bgfx::setVertexBuffer(0, &transient_vertex_buffer);
    }

    // Bind index buffer
    if (use_direct_index_buffer) {
        bgfx::setIndexBuffer(
            static_cast<const RenderIndexBufferClass &>(index_buffer).Get_Bgfx_Index_Buffer(),
            index_buffer_offset + start_index,
            fill_mode == FillMode::Points && strip ? source_index_count : submitted_index_count);
    } else {
        bgfx::setIndexBuffer(&transient_index_buffer);
    }

    // Textures
    TextureClass *stage0_texture = textures != nullptr ? textures[0] : nullptr;
    TextureClass *stage1_texture = textures != nullptr ? textures[1] : nullptr;
    bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(),
        Resolve_Texture_Handle(stage0_texture), Resolve_Sampler_Flags(stage0_texture, 0));
    bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(),
        Resolve_Texture_Handle(stage1_texture), Resolve_Sampler_Flags(stage1_texture, 1));

    // Material classification uniforms
    bgfx::setUniform(BgfxRenderer::Get_Frag_Config_Uniform(), classification.frag_config);

    float frag_config2_copy[4];
    std::memcpy(frag_config2_copy, classification.frag_config2, sizeof(frag_config2_copy));
    if (!DX8Wrapper::Get_Fog_Enable()) {
        frag_config2_copy[1] = 0.0f;
    }
    bgfx::setUniform(BgfxRenderer::Get_Frag_Config2_Uniform(), frag_config2_copy);

    Apply_Fog_Uniforms();

    // Per-pixel lighting uniforms (from pre-computed classification + per-mesh scene state)
    BgfxRenderer::Apply_Lighting_Uniforms(classification);

    // Bump env map matrix (dynamic, read from DX8Wrapper stage state)
    BgfxRenderer::Apply_Bump_Env_Uniforms(classification);

    if (classification.program == MeshShaderProgram::MeshTexgen) {
        BgfxRenderer::Apply_Texgen_Uniforms();
    }

    ShadowMapManager::Bind_Shadow_Uniforms(receive_shadows);

    const unsigned cull_mode = DX8Wrapper::Get_DX8_Render_State(D3DRS_CULLMODE);
    RenderStateStruct rs;
    DX8Wrapper::Get_Render_State(rs);
    uint16_t view_id = BgfxRenderer::Get_View_Id(view, projection);

    BgfxRenderer::Apply_Render_State(
        rs.shader,
        cull_mode != 0x12345678u ? cull_mode : D3DCULL_CW,
        Resolve_Primitive_State(fill_mode));
    bgfx::submit(view_id, BgfxRenderer::Get_Mesh_Program(classification.program));

    // Shadow cast pass
    const bool alpha_test_enabled = classification.frag_config[2] >= 0.0f;
    const bool blend_blocks_shadow_cast =
        rs.shader.Get_Dst_Blend_Func() != ShaderClass::DSTBLEND_ZERO && !alpha_test_enabled;
    if (cast_shadows && !blend_blocks_shadow_cast) {
        const uint32_t shadow_ib_count = fill_mode == FillMode::Points && strip ? source_index_count : submitted_index_count;
        ShadowMapManager::Submit_Shadow_Draws(
            vertex_buffer, vertex_buffer_offset, index_base_offset,
            min_vertex_index, vertex_count,
            index_buffer, index_buffer_offset, start_index,
            shadow_ib_count,
            use_direct_vertex_buffer, use_direct_index_buffer,
            &transient_vertex_buffer, &transient_index_buffer,
            world,
            Resolve_Texture_Handle(stage0_texture),
            Resolve_Sampler_Flags(stage0_texture, 0),
            Resolve_Texture_Handle(stage1_texture),
            Resolve_Sampler_Flags(stage1_texture, 1),
            alpha_test_enabled,
            cull_mode != 0x12345678u ? cull_mode : D3DCULL_CW);
    }

    return true;
}

bool Submit_Current_Draw(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool strip,
    bool use_explicit_shadow_flags,
    bool receive_shadows,
    bool cast_shadows)
{
    if (!BgfxRenderer::Is_Initted() || !DX8Wrapper::_Is_Triangle_Draw_Enabled()) {
        return false;
    }

    DX8Wrapper::Apply_Render_State_Changes();

    RenderStateStruct render_state;
    DX8Wrapper::Get_Render_State(render_state);
    if (render_state.vertex_buffer == nullptr || render_state.index_buffer == nullptr) {
        return false;
    }

    Matrix4 projection;
    DX8Wrapper::Get_Transform(D3DTS_PROJECTION, projection);

    const bool resolved_receive_shadows =
        use_explicit_shadow_flags
            ? receive_shadows
            : render_state.shader.Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_ZERO;
    const bool resolved_cast_shadows = use_explicit_shadow_flags ? cast_shadows : false;

    TextureClass *textures[2] = {render_state.Textures[0], render_state.Textures[1]};
    const unsigned fvf = render_state.vertex_buffer->Vertex_Format_Info().Get_Vertex_Format();
    const bool has_normals = (fvf & VERTEX_FORMAT_FLAG_NORMAL) != 0u;
    MaterialClassification classification = BgfxRenderer::Classify_Material(render_state.shader, render_state.material, has_normals);

    return Submit_Classified_Draw_Internal(
        *render_state.vertex_buffer,
        render_state.vba_offset,
        *render_state.index_buffer,
        render_state.iba_offset,
        render_state.index_base_offset,
        start_index, polygon_count,
        min_vertex_index, vertex_count,
        textures,
        render_state.material,
        classification,
        resolved_receive_shadows,
        resolved_cast_shadows,
        render_state.world, render_state.view, projection,
        strip);
}
} // anonymous namespace

bool BgfxRenderer::Submit_Current_Fixed_Function_Triangles(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count)
{
    return Submit_Current_Draw(start_index, polygon_count, min_vertex_index, vertex_count,
        false, false, false, false);
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Triangles(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool receive_shadows,
    bool cast_shadows)
{
    return Submit_Current_Draw(start_index, polygon_count, min_vertex_index, vertex_count,
        false, true, receive_shadows, cast_shadows);
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Strip(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count)
{
    return Submit_Current_Draw(start_index, polygon_count, min_vertex_index, vertex_count,
        true, false, false, false);
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Strip(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool receive_shadows,
    bool cast_shadows)
{
    return Submit_Current_Draw(start_index, polygon_count, min_vertex_index, vertex_count,
        true, true, receive_shadows, cast_shadows);
}

bool BgfxRenderer::Submit_Classified_Draw(
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    const IndexBufferClass &index_buffer,
    unsigned index_buffer_offset,
    unsigned index_base_offset,
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    TextureClass *const *textures,
    const VertexMaterialClass *material,
    const MaterialClassification &classification,
    bool receive_shadows,
    bool cast_shadows,
    const Matrix4 &world,
    const Matrix4 &view,
    const Matrix4 &projection,
    bool strip)
{
    return Submit_Classified_Draw_Internal(
        vertex_buffer, vertex_buffer_offset,
        index_buffer, index_buffer_offset, index_base_offset,
        start_index, polygon_count,
        min_vertex_index, vertex_count,
        textures, material, classification,
        receive_shadows, cast_shadows,
        world, view, projection, strip);
}

bool BgfxRenderer::Init_Render_Resources()
{
    // This layout matches the dynamic_vertex_format (XYZNDUV2) used by
    // sorting and dynamic buffers for the transient buffer fallback path.
    FixedFunctionLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
        .end();

    OverlayVertexLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    // Shared sampler uniforms
    if (!bgfx::isValid(Texture0Uniform))
        Texture0Uniform = bgfx::createUniform("s_texColor0", bgfx::UniformType::Sampler);
    if (!bgfx::isValid(Texture1Uniform))
        Texture1Uniform = bgfx::createUniform("s_texColor1", bgfx::UniformType::Sampler);

    // Overlay uniforms
    if (!bgfx::isValid(OverlayConfigUniform))
        OverlayConfigUniform = bgfx::createUniform("u_overlayConfig", bgfx::UniformType::Vec4);

    // Mesh uniforms — shared by all mesh programs
    if (!bgfx::isValid(MeshFogConfigUniform))
        MeshFogConfigUniform = bgfx::createUniform("u_meshFogConfig", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshFogColorUniform))
        MeshFogColorUniform = bgfx::createUniform("u_meshFogColor", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshFragConfigUniform))
        MeshFragConfigUniform = bgfx::createUniform("u_meshFragConfig", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshFragConfig2Uniform))
        MeshFragConfig2Uniform = bgfx::createUniform("u_meshFragConfig2", bgfx::UniformType::Vec4);

    // Lit-only uniforms
    if (!bgfx::isValid(MeshLitConfigUniform))
        MeshLitConfigUniform = bgfx::createUniform("u_meshLitConfig", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshMaterialAmbientUniform))
        MeshMaterialAmbientUniform = bgfx::createUniform("u_meshMaterialAmbient", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshMaterialDiffuseUniform))
        MeshMaterialDiffuseUniform = bgfx::createUniform("u_meshMaterialDiffuse", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshMaterialEmissiveUniform))
        MeshMaterialEmissiveUniform = bgfx::createUniform("u_meshMaterialEmissive", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshSceneAmbientUniform))
        MeshSceneAmbientUniform = bgfx::createUniform("u_meshSceneAmbient", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshLightDirUniform))
        MeshLightDirUniform = bgfx::createUniform("u_meshLightDir", bgfx::UniformType::Vec4, 4);
    if (!bgfx::isValid(MeshLightColorUniform))
        MeshLightColorUniform = bgfx::createUniform("u_meshLightColor", bgfx::UniformType::Vec4, 4);

    // Bump env map uniforms
    if (!bgfx::isValid(MeshBumpEnvMatUniform))
        MeshBumpEnvMatUniform = bgfx::createUniform("u_meshBumpEnvMat", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshBumpEnvLumUniform))
        MeshBumpEnvLumUniform = bgfx::createUniform("u_meshBumpEnvLum", bgfx::UniformType::Vec4);

    // Texgen-only uniforms
    if (!bgfx::isValid(MeshTexgenModeUniform))
        MeshTexgenModeUniform = bgfx::createUniform("u_meshTexgenMode", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshTexTransformFlagsUniform))
        MeshTexTransformFlagsUniform = bgfx::createUniform("u_meshTexTransformFlags", bgfx::UniformType::Vec4);
    if (!bgfx::isValid(MeshTexTransform0Uniform))
        MeshTexTransform0Uniform = bgfx::createUniform("u_meshTexTransform0", bgfx::UniformType::Mat4);
    if (!bgfx::isValid(MeshTexTransform1Uniform))
        MeshTexTransform1Uniform = bgfx::createUniform("u_meshTexTransform1", bgfx::UniformType::Mat4);

    if (!bgfx::isValid(WhiteTexture)) {
        constexpr uint32_t white_pixel = 0xffffffffu;
        const bgfx::Memory *texture_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
        WhiteTexture = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::BGRA8,
            BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
            texture_memory);
    }

    if (!bgfx::isValid(BlackTexture)) {
        constexpr uint32_t black_pixel = 0xff000000u;
        const bgfx::Memory *texture_memory = bgfx::copy(&black_pixel, sizeof(black_pixel));
        BlackTexture = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::BGRA8,
            BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT,
            texture_memory);
    }

    if (!bgfx::isValid(WhiteTexture) || !bgfx::isValid(BlackTexture))
        return false;

    if (!bgfx::isValid(OverlayProgram))
        OverlayProgram = Load_Program("vs_overlay", "fs_overlay");

    // Mesh programs (2 variants: mesh, mesh_texgen — both share fs_mesh)
    if (!bgfx::isValid(MeshProgram))
        MeshProgram = Load_Program("vs_mesh", "fs_mesh");
    if (!bgfx::isValid(MeshTexgenProgram))
        MeshTexgenProgram = Load_Program("vs_mesh_texgen", "fs_mesh");

    // Initialize shadow map system
    if (!ShadowMapManager::Is_Initted()) {
        ShadowMapManager::Init();
    }

    return bgfx::isValid(OverlayProgram)
        && bgfx::isValid(MeshProgram) && bgfx::isValid(MeshTexgenProgram);
}

void BgfxRenderer::Shutdown_Render_Resources()
{
    // Shut down shadow map system before destroying other resources
    ShadowMapManager::Shutdown();

    Destroy_Program(MeshTexgenProgram);
    Destroy_Program(MeshProgram);
    Destroy_Program(OverlayProgram);

    auto destroy_uniform = [](bgfx::UniformHandle &h) {
        if (bgfx::isValid(h)) { bgfx::destroy(h); h = BGFX_INVALID_HANDLE; }
    };

    destroy_uniform(MeshTexTransform1Uniform);
    destroy_uniform(MeshTexTransform0Uniform);
    destroy_uniform(MeshTexTransformFlagsUniform);
    destroy_uniform(MeshTexgenModeUniform);
    destroy_uniform(MeshBumpEnvLumUniform);
    destroy_uniform(MeshBumpEnvMatUniform);
    destroy_uniform(MeshLightColorUniform);
    destroy_uniform(MeshLightDirUniform);
    destroy_uniform(MeshSceneAmbientUniform);
    destroy_uniform(MeshMaterialEmissiveUniform);
    destroy_uniform(MeshMaterialDiffuseUniform);
    destroy_uniform(MeshMaterialAmbientUniform);
    destroy_uniform(MeshLitConfigUniform);
    destroy_uniform(MeshFragConfig2Uniform);
    destroy_uniform(MeshFragConfigUniform);
    destroy_uniform(MeshFogColorUniform);
    destroy_uniform(MeshFogConfigUniform);
    destroy_uniform(OverlayConfigUniform);

    if (bgfx::isValid(WhiteTexture)) {
        bgfx::destroy(WhiteTexture);
        WhiteTexture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(BlackTexture)) {
        bgfx::destroy(BlackTexture);
        BlackTexture = BGFX_INVALID_HANDLE;
    }

    destroy_uniform(Texture1Uniform);
    destroy_uniform(Texture0Uniform);
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
    // A clear starts a new logical bgfx pass. Keep view IDs monotonic for the frame,
    // but stop reusing cached camera views across pass boundaries.
    Invalidate_Configured_Views();

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

    const uint16_t clear_view_id = Acquire_View();
    bgfx::setViewClear(clear_view_id, clear_flags, clear_rgba, 1.0f, 0);
    bgfx::setViewFrameBuffer(clear_view_id, CurrentFrameBuffer);
    bgfx::setViewRect(
        clear_view_id,
        static_cast<uint16_t>(PendingViewportX),
        static_cast<uint16_t>(PendingViewportY),
        static_cast<uint16_t>(PendingViewportWidth),
        static_cast<uint16_t>(PendingViewportHeight));
    bgfx::touch(clear_view_id);
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

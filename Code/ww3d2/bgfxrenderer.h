#pragma once

#include "always.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "renderer_types.h"
#include "shader.h"
#include "ww3d.h"
#include "vector3.h"
#include "ww3dformat.h"

#include <bgfx/bgfx.h>
#include <cstdint>

class SurfaceClass;
class IndexBufferClass;
class MeshClass;
class TextureClass;
class VertexBufferClass;
class VertexMaterialClass;
struct RenderStateStruct;

// Stage color/alpha operation enum — replaces float-encoded D3DTOP opcodes.
// Values must match the constants in mesh_common.sh.
enum StageColorOp : std::uint8_t
{
    STAGE_DISABLE = 0,
    STAGE_MODULATE = 1,
    STAGE_SELECT_TEXTURE = 2,
    STAGE_SELECT_CURRENT = 3,
    STAGE_ADD = 4,
    STAGE_ADDSMOOTH = 5,
    STAGE_SUBTRACT = 6,
    STAGE_BLEND_TEX_ALPHA = 7,
    STAGE_BLEND_CUR_ALPHA = 8,
    STAGE_BUMPENVMAP = 9,
    STAGE_BUMPENVMAP_LUM = 10,
    STAGE_DOTPRODUCT3 = 11,
};

// Which mesh shader program to use for a draw call.
enum class MeshShaderProgram : std::uint8_t
{
    Mesh,
    MeshTexgen,
    Count,
};

// Pre-computed material properties for direct draw submission.
// All material-level state is resolved at classification time; only per-mesh
// scene/light data is read from DX8Wrapper at submit time.
struct MaterialClassification {
    MeshShaderProgram program;
    float frag_config[4];       // stage0ColorOp, stage1ColorOp, alphaTestRef, stage0AlphaOp
    float frag_config2[4];      // stage1AlphaOp, fogMode, 0, 0
    float lit_config[4];        // lightingMode(0/1/2), diffuseSource, ambientSource, emissiveSource
    float material_ambient[4];
    float material_diffuse[4];  // .a = opacity
    float material_emissive[4];
};

struct OverlayYUVSubmitDesc
{
    const WW3D::OverlaySubmitVertex *Vertices = nullptr;
    std::uint32_t VertexCount = 0;
    const std::uint16_t *Indices = nullptr;
    std::uint32_t IndexCount = 0;
    bgfx::TextureHandle LumaTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle ChromaTexture = BGFX_INVALID_HANDLE;
    std::uint32_t SamplerFlags = 0;
    bool FullRangeVideo = false;
};

class BgfxRenderer
{
public:
    static void Reset_Requested_Renderer();
    static bool Set_Requested_Renderer(const char *renderer_name);
    static bgfx::RendererType::Enum Get_Requested_Renderer();
    static const char *Get_Requested_Renderer_Name();
    static bool Init(void *window_handle, bool lite);
    static bool Configure_Window(void *window_handle, int width, int height, int bits, bool windowed, bool resize_window);
    static void Shutdown();
    static bool Reset();
    static bool Begin_Frame(bool clear_color, bool clear_depth, float red, float green, float blue);
    static void Clear_View(bool clear_color, bool clear_depth, const Vector3 &color);
    static void End_Frame();
    static void Set_Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void Set_Camera(const Matrix3D &view, const Matrix4 &projection);
    static uint16_t Get_View_Id(const Matrix4 &view, const Matrix4 &projection);
    static TextureClass *Create_Render_Target_Texture(uint32_t width, uint32_t height, WW3DFormat format);
    static bool Set_Render_Target(TextureClass &texture);
    static void Reset_Render_Target();
    static bool Has_Render_Target();
    static void Prepare_Overlay_View();
    static void Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed);
    static void Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed);
    static const bgfx::VertexLayout &Get_Fixed_Function_Layout();
    static const bgfx::VertexLayout &Get_Overlay_Layout();
    static uint16_t Get_Main_View_Id();
    static uint16_t Get_Overlay_View_Id();
    static const Matrix4 &Get_Current_View_Matrix();
    static const Matrix4 &Get_Current_Projection_Matrix();
    static bgfx::TextureHandle Get_White_Texture();
    static bgfx::UniformHandle Get_Texture0_Uniform();
    static bgfx::UniformHandle Get_Texture1_Uniform();
    static bgfx::ProgramHandle Get_Overlay_Program();
    static bgfx::ProgramHandle Get_Movie_YUV_Program();
    static bgfx::UniformHandle Get_Movie_YUV_Config_Uniform();
    static bgfx::ProgramHandle Get_Mesh_Program(MeshShaderProgram program, bool skinned = false);
    static bgfx::UniformHandle Get_Fog_Config_Uniform();
    static bgfx::UniformHandle Get_Fog_Color_Uniform();
    static bgfx::UniformHandle Get_Frag_Config_Uniform();
    static bgfx::UniformHandle Get_Frag_Config2_Uniform();
    static bool Is_Skinned_Vertex_Format(unsigned fvf);
    static void Reset_Skinning_Frame();
    static bool Bind_Skinning_Palette(const MeshClass &mesh);
    static bool Apply_Current_Skinning_Binding();
    static bool Supports_Texture_Format(WW3DFormat format);
    static bool Supports_Render_Target_Format(WW3DFormat format);
    static bool Submit_Current_Fixed_Function_Triangles(
        unsigned short start_index,
        unsigned short polygon_count,
        unsigned short min_vertex_index,
        unsigned short vertex_count);
    static bool Submit_Current_Fixed_Function_Triangles(
        unsigned short start_index,
        unsigned short polygon_count,
        unsigned short min_vertex_index,
        unsigned short vertex_count,
        bool receive_shadows,
        bool cast_shadows);
    static bool Submit_Current_Fixed_Function_Strip(
        unsigned short start_index,
        unsigned short polygon_count,
        unsigned short min_vertex_index,
        unsigned short vertex_count);
    static bool Submit_Current_Fixed_Function_Strip(
        unsigned short start_index,
        unsigned short polygon_count,
        unsigned short min_vertex_index,
        unsigned short vertex_count,
        bool receive_shadows,
        bool cast_shadows);
    static bgfx::TextureHandle Create_Texture_From_Surface(SurfaceClass &surface);
    static bgfx::TextureHandle Create_Texture(TextureClass &texture);
    static bgfx::ProgramHandle Load_Program(const char *vertex_shader_name, const char *fragment_shader_name);
    static void Destroy_Program(bgfx::ProgramHandle &program);
    static uint64_t Build_Render_State(const ShaderClass &shader, unsigned cull_mode = D3DCULL_CW);
    static void Apply_Render_State(const ShaderClass &shader, unsigned cull_mode = D3DCULL_CW, uint64_t extra_state = 0u);
    static void Apply_Overlay_Config(bool has_texture, bool alpha_mask_texture);
    static bool Submit_Overlay(const WW3D::OverlaySubmitDesc &submission);
    static bool Submit_YUV_Overlay(const OverlayYUVSubmitDesc &submission);
    static void Destroy_Texture_Handle(bgfx::TextureHandle &handle);

    // Material classification and direct draw submission
    static MaterialClassification Classify_Material(const ShaderClass &shader, const VertexMaterialClass *material, bool has_normals);
    static bool Submit_Classified_Draw(
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
        const ShaderClass &shader,
        const VertexMaterialClass *material,
        const MaterialClassification &classification,
        bool receive_shadows,
        bool cast_shadows,
        const Matrix4 &world,
        const Matrix4 &view,
        const Matrix4 &projection,
        bool strip,
        const WW3D::LightingSubmitDesc *lighting = nullptr,
        const WW3D::FixedFunctionStateDesc *render_state = nullptr);

    static std::uint32_t Convert_Packed_Color(std::uint32_t argb_color);
    static void Request_Screen_Shot(const char *file_path);
    static bool Start_Movie_Capture(const char *file_path_base, float frame_rate);
    static void Stop_Movie_Capture();
    static bool Write_Latest_Movie_Frame();
    static bool Is_Movie_Capture_Active();
    static float Get_Movie_Capture_Frame_Rate();

    static bool Is_Initted() { return IsInitted; }
    static uint32_t Get_Width() { return Width; }
    static uint32_t Get_Height() { return Height; }

    // Per-program uniform apply helper (public for classified draw path)
    static void Apply_Texgen_Uniforms(const WW3D::FixedFunctionStateDesc *render_state = nullptr);
    static void Apply_Lighting_Uniforms(
        const MaterialClassification &classification,
        const WW3D::LightingSubmitDesc *lighting = nullptr,
        const WW3D::FixedFunctionStateDesc *render_state = nullptr);
    static void Apply_Bump_Env_Uniforms(
        const MaterialClassification &classification,
        const WW3D::FixedFunctionStateDesc *render_state = nullptr);

private:
    static bool Init_Render_Resources();
    static void Shutdown_Render_Resources();
    static bool Update_Platform_Window(void *window_handle);
    static bool Query_Drawable_Size(void *window_handle, uint32_t &width, uint32_t &height);
    static void Apply_Clear(bool clear_color, bool clear_depth, float red, float green, float blue);
    static void Apply_Reset_State();
    static bgfx::ShaderHandle Load_Shader(const char *shader_name);

    static bool IsInitted;
    static uint32_t Width;
    static uint32_t Height;
    static uint32_t ActiveWidth;
    static uint32_t ActiveHeight;
    static uint32_t BitDepth;
    static bool Windowed;
    static void *WindowHandle;
    static bgfx::PlatformData PlatformData;
    static bgfx::VertexLayout FixedFunctionLayout;
    static bgfx::VertexLayout OverlayVertexLayout;
    static bgfx::TextureHandle WhiteTexture;
    static bgfx::UniformHandle Texture0Uniform;
    static bgfx::UniformHandle Texture1Uniform;

    // Overlay shader uniforms
    static bgfx::UniformHandle OverlayConfigUniform;
    static bgfx::UniformHandle MovieYUVConfigUniform;

    // Shared mesh uniforms (all programs use these)
    static bgfx::UniformHandle MeshFogConfigUniform;
    static bgfx::UniformHandle MeshFogColorUniform;
    static bgfx::UniformHandle MeshFragConfigUniform;
    static bgfx::UniformHandle MeshFragConfig2Uniform;

    // Per-pixel lighting uniforms (used by fragment shader)
    static bgfx::UniformHandle MeshLitConfigUniform;
    static bgfx::UniformHandle MeshMaterialAmbientUniform;
    static bgfx::UniformHandle MeshMaterialDiffuseUniform;
    static bgfx::UniformHandle MeshMaterialEmissiveUniform;
    static bgfx::UniformHandle MeshSceneAmbientUniform;
    static bgfx::UniformHandle MeshLightPosTypeUniform;
    static bgfx::UniformHandle MeshLightDirSpotUniform;
    static bgfx::UniformHandle MeshLightDiffuseRangeUniform;
    static bgfx::UniformHandle MeshLightAmbientAttenUniform;

    // Bump env map uniforms
    static bgfx::UniformHandle MeshBumpEnvMatUniform;
    static bgfx::UniformHandle MeshBumpEnvLumUniform;

    // Texgen-only uniforms
    static bgfx::UniformHandle MeshTexgenModeUniform;
    static bgfx::UniformHandle MeshTexTransformFlagsUniform;
    static bgfx::UniformHandle MeshTexTransform0Uniform;
    static bgfx::UniformHandle MeshTexTransform1Uniform;
    static bgfx::UniformHandle MeshSkinPaletteUniform;
    static bgfx::UniformHandle MeshSkinPaletteInfoUniform;

    // Shader programs
    static bgfx::ProgramHandle OverlayProgram;
    static bgfx::ProgramHandle MovieYUVProgram;
    static bgfx::ProgramHandle MeshProgram;
    static bgfx::ProgramHandle MeshTexgenProgram;
    static bgfx::ProgramHandle MeshSkinProgram;
    static bgfx::ProgramHandle MeshSkinTexgenProgram;

    static Matrix4 CurrentViewMatrix;
    static Matrix4 CurrentProjectionMatrix;
};

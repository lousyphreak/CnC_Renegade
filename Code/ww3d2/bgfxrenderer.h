#pragma once

#include "always.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "renderer_types.h"
#include "shader.h"
#include "vector3.h"
#include "ww3dformat.h"

#include <bgfx/bgfx.h>
#include <cstdint>

class SurfaceClass;
class IndexBufferClass;
class TextureClass;
class VertexBufferClass;
class VertexMaterialClass;
struct RenderStateStruct;

class BgfxRenderer
{
public:
    struct FixedFunctionShaderInputs
    {
        bool FogEnabled = false;
        std::uint32_t FogColor = 0;
        std::uint32_t TextureFactor = 0xffffffffu;
        float BumpEnvMatrix[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        float BumpEnvLuminanceScale = 1.0f;
        float BumpEnvLuminanceOffset = 0.0f;
        float Stage0Color[4] = {4.0f, 0.0f, 2.0f, 1.0f};
        float Stage0Alpha[4] = {2.0f, 0.0f, 2.0f, 1.0f};
        float Stage1Color[4] = {1.0f, 0.0f, 2.0f, 1.0f};
        float Stage1Alpha[4] = {1.0f, 0.0f, 2.0f, 1.0f};
        float MaterialAmbient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float MaterialDiffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float MaterialSpecular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float MaterialEmissive[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float MaterialParams[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        float SceneAmbient[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float LightingConfig[4] = {0.0f, 0.0f, 1.0f, 0.0f};
        float MaterialSourceConfig[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float LightPositions[16] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f};
        float LightDirections[16] = {
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f};
        float LightAmbient[16] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f};
        float LightDiffuse[16] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f};
        float LightSpecular[16] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f};
        float LightAttenuation[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f};
        float LightSpotParams[16] = {
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f};
    };

    static bool Init(void *window_handle, bool lite);
    static void Shutdown();
    static bool Reset();
    static bool Begin_Frame(bool clear_color, bool clear_depth, float red, float green, float blue);
    static void Clear_View(bool clear_color, bool clear_depth, const Vector3 &color);
    static void End_Frame();
    static void Set_Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void Set_Camera(const Matrix3D &view, const Matrix4 &projection);
    static uint16_t Get_View_Id(const Matrix4 &view, const Matrix4 &projection);
    static bool Set_Render_Target(TextureClass &texture);
    static void Reset_Render_Target();
    static void Prepare_Overlay_View();
    static void Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed);
    static void Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed);
    static const bgfx::VertexLayout &Get_Fixed_Function_Layout();
    static uint16_t Get_Main_View_Id();
    static uint16_t Get_Overlay_View_Id();
    static const Matrix4 &Get_Current_View_Matrix();
    static const Matrix4 &Get_Current_Projection_Matrix();
    static bgfx::TextureHandle Get_White_Texture();
    static bgfx::UniformHandle Get_Texture0_Uniform();
    static bgfx::UniformHandle Get_Texture1_Uniform();
    static bgfx::ProgramHandle Get_Fixed_Function_Program();
    static bool Supports_Texture_Format(WW3DFormat format);
    static bool Supports_Render_Target_Format(WW3DFormat format);
    static bool Submit_Cached_Fixed_Function_Triangles(
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
        const ShaderClass &shader,
        const Matrix4 &world,
        const Matrix4 &view,
        const Matrix4 &projection);
    static bool Submit_Current_Fixed_Function_Triangles(
        unsigned short start_index,
        unsigned short polygon_count,
        unsigned short min_vertex_index,
        unsigned short vertex_count);
    static bool Submit_Cached_Fixed_Function_Strip(
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
        const ShaderClass &shader,
        const Matrix4 &world,
        const Matrix4 &view,
        const Matrix4 &projection);
    static bool Submit_Current_Fixed_Function_Strip(
        unsigned short start_index,
        unsigned short polygon_count,
        unsigned short min_vertex_index,
        unsigned short vertex_count);
    static bgfx::TextureHandle Create_Texture_From_Surface(SurfaceClass &surface);
    static bgfx::TextureHandle Create_Texture(TextureClass &texture);
    static bgfx::ProgramHandle Load_Program(const char *vertex_shader_name, const char *fragment_shader_name);
    static void Destroy_Program(bgfx::ProgramHandle &program);
    static uint64_t Build_Render_State(const ShaderClass &shader, unsigned cull_mode = D3DCULL_CW);
    static void Apply_Fixed_Function_Shader_Inputs(
        const ShaderClass &shader,
        const FixedFunctionShaderInputs &inputs,
        const Matrix4 &view_matrix);
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
    static bgfx::TextureHandle WhiteTexture;
    static bgfx::UniformHandle Texture0Uniform;
    static bgfx::UniformHandle Texture1Uniform;
    static bgfx::UniformHandle FixedFunctionConfig1Uniform;
    static bgfx::UniformHandle FixedFunctionFogColorUniform;
    static bgfx::UniformHandle FixedFunctionFogParamsUniform;
    static bgfx::UniformHandle FixedFunctionTextureFactorUniform;
    static bgfx::UniformHandle FixedFunctionStage0ColorUniform;
    static bgfx::UniformHandle FixedFunctionStage0AlphaUniform;
    static bgfx::UniformHandle FixedFunctionStage1ColorUniform;
    static bgfx::UniformHandle FixedFunctionStage1AlphaUniform;
    static bgfx::UniformHandle FixedFunctionBumpEnvMatrixUniform;
    static bgfx::UniformHandle FixedFunctionBumpEnvParamsUniform;
    static bgfx::UniformHandle FixedFunctionMaterialAmbientUniform;
    static bgfx::UniformHandle FixedFunctionMaterialDiffuseUniform;
    static bgfx::UniformHandle FixedFunctionMaterialSpecularUniform;
    static bgfx::UniformHandle FixedFunctionMaterialEmissiveUniform;
    static bgfx::UniformHandle FixedFunctionMaterialParamsUniform;
    static bgfx::UniformHandle FixedFunctionViewerUniform;
    static bgfx::UniformHandle FixedFunctionSceneAmbientUniform;
    static bgfx::UniformHandle FixedFunctionLightingConfigUniform;
    static bgfx::UniformHandle FixedFunctionMaterialSourceConfigUniform;
    static bgfx::UniformHandle FixedFunctionLightPositionsUniform;
    static bgfx::UniformHandle FixedFunctionLightDirectionsUniform;
    static bgfx::UniformHandle FixedFunctionLightAmbientUniform;
    static bgfx::UniformHandle FixedFunctionLightDiffuseUniform;
    static bgfx::UniformHandle FixedFunctionLightSpecularUniform;
    static bgfx::UniformHandle FixedFunctionLightAttenuationUniform;
    static bgfx::UniformHandle FixedFunctionLightSpotParamsUniform;
    static bgfx::ProgramHandle FixedFunctionProgram;
    static Matrix4 CurrentViewMatrix;
    static Matrix4 CurrentProjectionMatrix;
};

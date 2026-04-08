#pragma once

#include "always.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "shader.h"
#include "vector3.h"

#include <bgfx/bgfx.h>

class SurfaceClass;
class TextureClass;

class BgfxRenderer
{
public:
    static bool Init(void *window_handle, bool lite);
    static void Shutdown();
    static bool Reset();
    static bool Begin_Frame(bool clear_color, bool clear_depth, float red, float green, float blue);
    static void Clear_View(bool clear_color, bool clear_depth, const Vector3 &color);
    static void End_Frame();
    static void Set_Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void Set_Camera(const Matrix3D &view, const Matrix4 &projection);
    static bool Set_Render_Target(TextureClass &texture);
    static void Reset_Render_Target();
    static void Prepare_Overlay_View();
    static void Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed);
    static void Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed);
    static const bgfx::VertexLayout &Get_Pos_Color_Texcoord_Layout();
    static uint16_t Get_Overlay_View_Id();
    static bgfx::TextureHandle Get_White_Texture();
    static bgfx::UniformHandle Get_Color_Texture_Uniform();
    static bgfx::ProgramHandle Get_Color_Texture_Program();
    static bgfx::TextureHandle Create_Texture_From_Surface(SurfaceClass &surface);
    static bgfx::TextureHandle Create_Texture(TextureClass &texture);
    static bgfx::ProgramHandle Load_Program(const char *vertex_shader_name, const char *fragment_shader_name);
    static void Destroy_Program(bgfx::ProgramHandle &program);
    static uint64_t Build_Render_State(const ShaderClass &shader);

    static bool Is_Initted() { return IsInitted; }
    static uint32_t Get_Width() { return Width; }
    static uint32_t Get_Height() { return Height; }

private:
    static bool Init_Render_Resources();
    static void Shutdown_Render_Resources();
    static bool Update_Platform_Window(void *window_handle);
    static bool Query_Drawable_Size(void *window_handle, uint32_t &width, uint32_t &height);
    static void Apply_Clear(bool clear_color, bool clear_depth, float red, float green, float blue);
    static bgfx::ShaderHandle Load_Shader(const char *shader_name);

    static bool IsInitted;
    static uint32_t Width;
    static uint32_t Height;
    static uint32_t ActiveWidth;
    static uint32_t ActiveHeight;
    static uint32_t BitDepth;
    static bool Windowed;
    static void *WindowHandle;
    static bgfx::VertexLayout PosColorTexcoordLayout;
    static bgfx::TextureHandle WhiteTexture;
    static bgfx::UniformHandle ColorTextureUniform;
    static bgfx::ProgramHandle ColorTextureProgram;
};

#pragma once

#include "always.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "vector3.h"

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
    static void Get_Render_Target_Resolution(int &width, int &height, int &bits, bool &windowed);
    static void Get_Device_Resolution(int &width, int &height, int &bits, bool &windowed);

    static bool Is_Initted() { return IsInitted; }
    static uint32_t Get_Width() { return Width; }
    static uint32_t Get_Height() { return Height; }

private:
    static bool Update_Platform_Window(void *window_handle);
    static bool Query_Drawable_Size(void *window_handle, uint32_t &width, uint32_t &height);
    static void Apply_Clear(bool clear_color, bool clear_depth, float red, float green, float blue);

    static bool IsInitted;
    static uint32_t Width;
    static uint32_t Height;
    static uint32_t BitDepth;
    static bool Windowed;
    static void *WindowHandle;
};

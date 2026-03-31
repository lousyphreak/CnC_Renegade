#include "dx8texman.h"

#if !RENEGADE_WITH_DX8_RENDERER && RENEGADE_WITH_BGFX_RENDERER

void DX8TextureManagerClass::Shutdown()
{
}

void DX8TextureManagerClass::Add(DX8TextureTrackerClass *)
{
}

void DX8TextureManagerClass::Remove(TextureClass *)
{
}

void DX8TextureManagerClass::Release_Textures()
{
}

void DX8TextureManagerClass::Recreate_Textures()
{
}

#endif

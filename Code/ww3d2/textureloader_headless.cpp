#include "textureloader.h"

#include "ww3d.h"

bool TextureLoader::TextureLoadSuspended = false;

void TextureLoader::Init(void) {}
void TextureLoader::Deinit(void) {}

void TextureLoader::Validate_Texture_Size(unsigned & width, unsigned & height)
{
	if (width == 0) {
		width = 1;
	}
	if (height == 0) {
		height = 1;
	}
}

IDirect3DTexture8 * TextureLoader::Load_Thumbnail(const StringClass &)
{
	return NULL;
}

IDirect3DSurface8 * TextureLoader::Load_Surface_Immediate(const StringClass &, WW3DFormat, bool)
{
	return NULL;
}

void TextureLoader::Request_Thumbnail(TextureClass *) {}
void TextureLoader::Request_Background_Loading(TextureClass *) {}
void TextureLoader::Request_Foreground_Loading(TextureClass *) {}
void TextureLoader::Flush_Pending_Load_Tasks(void) {}
void TextureLoader::Update(void(*)(void)) {}
bool TextureLoader::Is_DX8_Thread(void) { return true; }
void TextureLoader::Suspend_Texture_Load() { TextureLoadSuspended = true; }
void TextureLoader::Continue_Texture_Load() { TextureLoadSuspended = false; }

void TextureLoader::Process_Foreground_Load(TextureLoadTaskClass *) {}
void TextureLoader::Process_Foreground_Thumbnail(TextureLoadTaskClass *) {}
void TextureLoader::Begin_Load_And_Queue(TextureLoadTaskClass *) {}
void TextureLoader::Load_Thumbnail(TextureClass *) {}
#include "dx8texman.h"
#include "bgfx_compat_resources.h"

DX8TextureTrackerList DX8TextureManagerClass::Managed_Textures;

void DX8TextureManagerClass::Shutdown()
{
	while (!Managed_Textures.Is_Empty()) {
		DX8TextureTrackerClass *track = Managed_Textures.Remove_Head();
		delete track;
	}
}

void DX8TextureManagerClass::Add(DX8TextureTrackerClass *track)
{
	Managed_Textures.Add(track);
}

void DX8TextureManagerClass::Remove(TextureClass *tex)
{
	DX8TextureTrackerListIterator it(&Managed_Textures);
	while (!it.Is_Done()) {
		DX8TextureTrackerClass *track = it.Peek_Obj();
		if (track->Texture == tex) {
			it.Remove_Current_Object();
			delete track;
			break;
		}
		it.Next();
	}
}

void DX8TextureManagerClass::Release_Textures()
{
	DX8TextureTrackerListIterator it(&Managed_Textures);
	while (!it.Is_Done()) {
		DX8TextureTrackerClass *track = it.Peek_Obj();
		BgfxCompat_Release_Texture_Resources(track->Texture);
		track->Texture->Dirty = true;
		it.Next();
	}
}

void DX8TextureManagerClass::Recreate_Textures()
{
	DX8TextureTrackerListIterator it(&Managed_Textures);
	while (!it.Is_Done()) {
		DX8TextureTrackerClass *track = it.Peek_Obj();
		BgfxCompat_Recreate_Texture_Resources(track->Texture);
		track->Texture->Dirty = true;
		it.Next();
	}
}

#include "assetmgr.h"

#include "render2dsentence.h"

namespace {

class DummyAssetIterator final : public AssetIterator
{
public:
	bool Is_Done(void) override { return true; }
	const char * Current_Item_Name(void) override { return ""; }
};

class DummyRenderObjIterator final : public RenderObjIterator
{
public:
	bool Is_Done(void) override { return true; }
	const char * Current_Item_Name(void) override { return ""; }
	int Current_Item_Class_ID(void) override { return 0; }
};

} // namespace

WW3DAssetManager * WW3DAssetManager::TheInstance = NULL;

WW3DAssetManager::WW3DAssetManager(void)
	: TextureCache(NULL), WW3D_Load_On_Demand(false), Activate_Fog_On_Load(false), MetalManager(NULL), PrototypeHashTable(NULL)
{
	TheInstance = this;
}

WW3DAssetManager::~WW3DAssetManager(void)
{
	if (TheInstance == this) {
		TheInstance = NULL;
	}
}

bool WW3DAssetManager::Load_3D_Assets(const char *) { return true; }
bool WW3DAssetManager::Load_3D_Assets(FileClass &) { return true; }
void WW3DAssetManager::Free_Assets(void) {}
void WW3DAssetManager::Release_Unused_Assets(void) {}
RenderObjClass * WW3DAssetManager::Create_Render_Obj(const char *) { return NULL; }
bool WW3DAssetManager::Render_Obj_Exists(const char *) { return false; }
RenderObjIterator * WW3DAssetManager::Create_Render_Obj_Iterator(void) { return new DummyRenderObjIterator(); }
void WW3DAssetManager::Release_Render_Obj_Iterator(RenderObjIterator * iterator) { delete iterator; }
AssetIterator * WW3DAssetManager::Create_HAnim_Iterator(void) { return new DummyAssetIterator(); }
HAnimClass * WW3DAssetManager::Get_HAnim(const char *) { return NULL; }
void WW3DAssetManager::Log_Texture_Statistics() {}
TextureClass * WW3DAssetManager::Get_Texture(const char * filename, TextureClass::MipCountType mip_level_count, WW3DFormat texture_format, bool allow_compression)
{
	return NEW_REF(TextureClass, (filename, filename, mip_level_count, texture_format, allow_compression));
}
void WW3DAssetManager::Release_All_Textures(void) {}
void WW3DAssetManager::Release_Unused_Textures(void) {}
void WW3DAssetManager::Release_Texture(TextureClass * texture) { REF_PTR_RELEASE(texture); }
void WW3DAssetManager::Load_Procedural_Textures() {}
Font3DInstanceClass * WW3DAssetManager::Get_Font3DInstance(const char *) { return NULL; }
FontCharsClass * WW3DAssetManager::Get_FontChars(const char * name, int point_size, bool is_bold)
{
	FontCharsClass * font = NEW_REF(FontCharsClass, ());
	font->Initialize_GDI_Font(name != NULL ? name : "Arial", point_size > 0 ? point_size : 12, is_bold);
	return font;
}
AssetIterator * WW3DAssetManager::Create_HTree_Iterator(void) { return new DummyAssetIterator(); }
HTreeClass * WW3DAssetManager::Get_HTree(const char *) { return NULL; }
void WW3DAssetManager::Register_Prototype_Loader(PrototypeLoaderClass *) {}
void WW3DAssetManager::Add_Prototype(PrototypeClass *) {}
void WW3DAssetManager::Remove_Prototype(PrototypeClass *) {}
void WW3DAssetManager::Remove_Prototype(const char *) {}
PrototypeClass * WW3DAssetManager::Find_Prototype(const char *) { return NULL; }
AssetIterator * WW3DAssetManager::Create_Font3DData_Iterator(void) { return new DummyAssetIterator(); }
void WW3DAssetManager::Add_Font3DData(Font3DDataClass *) {}
void WW3DAssetManager::Remove_Font3DData(Font3DDataClass *) {}
Font3DDataClass * WW3DAssetManager::Get_Font3DData(const char *) { return NULL; }
void WW3DAssetManager::Release_All_Font3DDatas(void) {}
void WW3DAssetManager::Release_Unused_Font3DDatas(void) {}
void WW3DAssetManager::Release_All_FontChars(void) {}
void WW3DAssetManager::Free(void) {}
PrototypeLoaderClass * WW3DAssetManager::Find_Prototype_Loader(int) { return NULL; }
bool WW3DAssetManager::Load_Prototype(ChunkLoadClass &) { return false; }
void WW3DAssetManager::Log_All_Textures() {}

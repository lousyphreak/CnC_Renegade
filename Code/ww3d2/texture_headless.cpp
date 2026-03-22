#include "texture.h"

#include "ww3d.h"

TextureClass::TextureClass(unsigned width, unsigned height, WW3DFormat format, MipCountType mip_level_count, PoolType pool, bool)
	: TextureMinFilter(FILTER_TYPE_DEFAULT), TextureMagFilter(FILTER_TYPE_DEFAULT), MipMapFilter(mip_level_count != MIP_LEVELS_1 ? FILTER_TYPE_DEFAULT : FILTER_TYPE_NONE), UAddressMode(TEXTURE_ADDRESS_REPEAT), VAddressMode(TEXTURE_ADDRESS_REPEAT), D3DTexture(NULL), Initialized(true), texture_id(0), IsLightmap(false), IsProcedural(true), IsCompressionAllowed(false), InactivationTime(0), ExtendedInactivationTime(0), LastInactivationSyncTime(0), LastAccessed(WW3D::Get_Sync_Time()), TextureFormat(format), Width(static_cast<int>(width)), Height(static_cast<int>(height)), Pool(pool), Dirty(pool == POOL_DEFAULT), MipLevelCount(mip_level_count), TextureLoadTask(NULL), ThumbnailLoadTask(NULL)
{
}

TextureClass::TextureClass(const char * name, const char * full_path, MipCountType mip_level_count, WW3DFormat texture_format, bool allow_compression)
	: TextureClass(1, 1, texture_format == WW3D_FORMAT_UNKNOWN ? WW3D_FORMAT_A8R8G8B8 : texture_format, mip_level_count, POOL_MANAGED, false)
{
	Name = name != NULL ? name : "";
	if (full_path != NULL) {
		FullPath = full_path;
	} else {
		FullPath = Name;
	}
	IsProcedural = false;
	IsCompressionAllowed = allow_compression;
}

TextureClass::TextureClass(SurfaceClass * surface, MipCountType mip_level_count)
	: TextureClass(1, 1, WW3D_FORMAT_A8R8G8B8, mip_level_count, POOL_MANAGED, false)
{
	if (surface != NULL) {
		SurfaceClass::SurfaceDescription desc;
		surface->Get_Description(desc);
		Width = static_cast<int>(desc.Width);
		Height = static_cast<int>(desc.Height);
		TextureFormat = desc.Format;
	}
}

TextureClass::TextureClass(IDirect3DTexture8 * d3d_texture)
	: TextureClass(1, 1, WW3D_FORMAT_A8R8G8B8, MIP_LEVELS_ALL, POOL_MANAGED, false)
{
	D3DTexture = d3d_texture;
}

TextureClass::~TextureClass(void) {}

void TextureClass::Set_Texture_Name(const char * name) { Name = name != NULL ? name : ""; }
unsigned int TextureClass::Get_Mip_Level_Count(void) { return MipLevelCount == MIP_LEVELS_ALL ? 1U : static_cast<unsigned int>(MipLevelCount); }
void TextureClass::Init() { Initialized = true; }
SurfaceClass * TextureClass::Get_Surface_Level(unsigned int) { return NEW_REF(SurfaceClass, (Width > 0 ? Width : 1, Height > 0 ? Height : 1, TextureFormat)); }
IDirect3DSurface8 * TextureClass::Get_D3D_Surface_Level(unsigned int) { return NULL; }
unsigned int TextureClass::Get_Priority(void) { return 0; }
unsigned int TextureClass::Set_Priority(unsigned int) { return 0; }
void TextureClass::Set_Mip_Mapping(FilterType mipmap) { MipMapFilter = mipmap; }
unsigned TextureClass::Get_Texture_Memory_Usage() const { return static_cast<unsigned>(Width > 0 && Height > 0 ? Width * Height * 4 : 0); }
int TextureClass::_Get_Total_Locked_Surface_Size() { return 0; }
int TextureClass::_Get_Total_Texture_Size() { return 0; }
int TextureClass::_Get_Total_Lightmap_Texture_Size() { return 0; }
int TextureClass::_Get_Total_Procedural_Texture_Size() { return 0; }
int TextureClass::_Get_Total_Locked_Surface_Count() { return 0; }
int TextureClass::_Get_Total_Texture_Count() { return 0; }
int TextureClass::_Get_Total_Lightmap_Texture_Count() { return 0; }
int TextureClass::_Get_Total_Procedural_Texture_Count() { return 0; }
void TextureClass::_Init_Filters(TextureFilterMode) {}
void TextureClass::_Set_Default_Min_Filter(FilterType) {}
void TextureClass::_Set_Default_Mag_Filter(FilterType) {}
void TextureClass::_Set_Default_Mip_Filter(FilterType) {}
void TextureClass::Invalidate() { Dirty = true; }
bool TextureClass::Is_Missing_Texture() { return false; }
unsigned TextureClass::Get_Reduction() const { return static_cast<unsigned>(WW3D::Get_Texture_Reduction()); }
void TextureClass::Invalidate_Old_Unused_Textures(unsigned) {}
void TextureClass::Apply_New_Surface(IDirect3DTexture8 * tex, bool initialized) { D3DTexture = tex; Initialized = initialized; }

TextureClass * Load_Texture(ChunkLoadClass &) { return NEW_REF(TextureClass, (1U, 1U, WW3D_FORMAT_A8R8G8B8, TextureClass::MIP_LEVELS_1, TextureClass::POOL_MANAGED, false)); }
void Save_Texture(TextureClass *, ChunkSaveClass &) {}

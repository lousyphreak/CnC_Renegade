#include "texture.h"

#include "bgfx_compat_resources.h"

#include "assetmgr.h"
#include "w3d_file.h"
#include "ww3d.h"
#include "wwdebug.h"

#include <algorithm>
#include <cstring>

namespace {

bgfx::TextureHandle g_white_texture = BGFX_INVALID_HANDLE;
unsigned g_next_texture_id = 1;

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

void Decode_Pixel(const uint8_t *pixel, WW3DFormat format, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
{
	r = g = b = 255;
	a = 255;
	if (pixel == NULL) {
		return;
	}

	switch (format) {
		case WW3D_FORMAT_A8R8G8B8:
			r = pixel[2];
			g = pixel[1];
			b = pixel[0];
			a = pixel[3];
			break;
		case WW3D_FORMAT_X8R8G8B8:
			r = pixel[2];
			g = pixel[1];
			b = pixel[0];
			break;
		case WW3D_FORMAT_R8G8B8:
			r = pixel[2];
			g = pixel[1];
			b = pixel[0];
			break;
		case WW3D_FORMAT_R5G6B5:
		{
			const uint16_t value = *reinterpret_cast<const uint16_t *>(pixel);
			r = Expand_5_To_8(static_cast<uint8_t>((value >> 11) & 0x1F));
			g = Expand_6_To_8(static_cast<uint8_t>((value >> 5) & 0x3F));
			b = Expand_5_To_8(static_cast<uint8_t>(value & 0x1F));
			break;
		}
		case WW3D_FORMAT_A1R5G5B5:
		{
			const uint16_t value = *reinterpret_cast<const uint16_t *>(pixel);
			a = (value & 0x8000U) ? 255 : 0;
			r = Expand_5_To_8(static_cast<uint8_t>((value >> 10) & 0x1F));
			g = Expand_5_To_8(static_cast<uint8_t>((value >> 5) & 0x1F));
			b = Expand_5_To_8(static_cast<uint8_t>(value & 0x1F));
			break;
		}
		case WW3D_FORMAT_A4R4G4B4:
		{
			const uint16_t value = *reinterpret_cast<const uint16_t *>(pixel);
			a = Expand_4_To_8(static_cast<uint8_t>((value >> 12) & 0x0F));
			r = Expand_4_To_8(static_cast<uint8_t>((value >> 8) & 0x0F));
			g = Expand_4_To_8(static_cast<uint8_t>((value >> 4) & 0x0F));
			b = Expand_4_To_8(static_cast<uint8_t>(value & 0x0F));
			break;
		}
		case WW3D_FORMAT_A8:
			a = pixel[0];
			break;
		case WW3D_FORMAT_L8:
			r = g = b = pixel[0];
			break;
		case WW3D_FORMAT_A8L8:
			a = pixel[1];
			r = g = b = pixel[0];
			break;
		case WW3D_FORMAT_A4L4:
			a = Expand_4_To_8(static_cast<uint8_t>((pixel[0] >> 4) & 0x0F));
			r = g = b = Expand_4_To_8(static_cast<uint8_t>(pixel[0] & 0x0F));
			break;
		default:
			break;
	}
}

BgfxCompatTexture *Create_Texture_Backend(int width, int height, WW3DFormat format)
{
	BgfxCompatTexture *texture = new BgfxCompatTexture();
	texture->width = std::max(width, 1);
	texture->height = std::max(height, 1);
	texture->format = format == WW3D_FORMAT_UNKNOWN ? WW3D_FORMAT_A8R8G8B8 : format;
	texture->bytes.resize(static_cast<size_t>(texture->width) * static_cast<size_t>(texture->height) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(texture->format)), 0);
	texture->dirty = true;
	return texture;
}

void Destroy_Texture_Backend(IDirect3DTexture8 *texture)
{
	BgfxCompatTexture *backend = BgfxCompat_To_Texture(texture);
	if (backend != NULL) {
		if (bgfx::isValid(backend->handle)) {
			bgfx::destroy(backend->handle);
			backend->handle = BGFX_INVALID_HANDLE;
		}
		delete backend;
	}
}

void Load_Texture_From_Surface(BgfxCompatTexture *texture, SurfaceClass *surface)
{
	if (texture == NULL || surface == NULL) {
		return;
	}

	SurfaceClass::SurfaceDescription desc;
	surface->Get_Description(desc);
	texture->width = static_cast<int>(desc.Width);
	texture->height = static_cast<int>(desc.Height);
	texture->format = desc.Format;
	texture->bytes.resize(static_cast<size_t>(desc.Width) * static_cast<size_t>(desc.Height) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(desc.Format)), 0);
	int width = 0;
	int height = 0;
	int pixel_size = 0;
	uint8_t *copy = surface->CreateCopy(&width, &height, &pixel_size, false);
	if (copy != NULL) {
		std::memcpy(texture->bytes.data(), copy, texture->bytes.size());
		delete [] copy;
	}
	texture->dirty = true;
}

void Ensure_White_Texture()
{
	if (bgfx::isValid(g_white_texture)) {
		return;
	}

	const uint32_t white = 0xFFFFFFFFu;
	g_white_texture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(&white, sizeof(white)));
}

} // namespace

unsigned BgfxCompat_Get_Pixel_Size(WW3DFormat format)
{
	switch (format) {
		case WW3D_FORMAT_R8G8B8:
			return 3;
		case WW3D_FORMAT_R5G6B5:
		case WW3D_FORMAT_X1R5G5B5:
		case WW3D_FORMAT_A1R5G5B5:
		case WW3D_FORMAT_A4R4G4B4:
		case WW3D_FORMAT_A8R3G3B2:
		case WW3D_FORMAT_X4R4G4B4:
		case WW3D_FORMAT_A8P8:
		case WW3D_FORMAT_A8L8:
			return 2;
		case WW3D_FORMAT_R3G3B2:
		case WW3D_FORMAT_A8:
		case WW3D_FORMAT_P8:
		case WW3D_FORMAT_L8:
		case WW3D_FORMAT_A4L4:
			return 1;
		case WW3D_FORMAT_A8R8G8B8:
		case WW3D_FORMAT_X8R8G8B8:
		default:
			return 4;
	}
}

std::vector<uint8_t> BgfxCompat_Convert_Surface_To_RGBA8(const BgfxCompatSurface &surface)
{
	std::vector<uint8_t> rgba;
	rgba.resize(static_cast<size_t>(surface.width) * static_cast<size_t>(surface.height) * 4U, 0);
	const unsigned pixel_size = BgfxCompat_Get_Pixel_Size(surface.format);
	for (unsigned y = 0; y < surface.height; ++y) {
		for (unsigned x = 0; x < surface.width; ++x) {
			const size_t src_offset = (static_cast<size_t>(y) * static_cast<size_t>(surface.width) + static_cast<size_t>(x)) * pixel_size;
			const size_t dst_offset = (static_cast<size_t>(y) * static_cast<size_t>(surface.width) + static_cast<size_t>(x)) * 4U;
			uint8_t r, g, b, a;
			Decode_Pixel(surface.bytes.data() + src_offset, surface.format, r, g, b, a);
			rgba[dst_offset + 0] = r;
			rgba[dst_offset + 1] = g;
			rgba[dst_offset + 2] = b;
			rgba[dst_offset + 3] = a;
		}
	}
	return rgba;
}

bgfx::TextureHandle BgfxCompat_Get_Texture_Handle(TextureClass *texture)
{
	Ensure_White_Texture();
	if (texture == NULL) {
		return g_white_texture;
	}

	BgfxCompatTexture *backend = BgfxCompat_To_Texture(texture->Peek_DX8_Texture());
	if (backend == NULL) {
		return g_white_texture;
	}

	if (!backend->dirty && bgfx::isValid(backend->handle)) {
		return backend->handle;
	}

	const BgfxCompatSurface temp_surface{static_cast<unsigned>(std::max(backend->width, 1)), static_cast<unsigned>(std::max(backend->height, 1)), backend->format, backend->bytes};
	std::vector<uint8_t> rgba = BgfxCompat_Convert_Surface_To_RGBA8(temp_surface);
	bgfx::Memory const *memory = bgfx::copy(rgba.data(), static_cast<uint32_t>(rgba.size()));
	if (bgfx::isValid(backend->handle)) {
		bgfx::destroy(backend->handle);
		backend->handle = BGFX_INVALID_HANDLE;
	}
	backend->handle = bgfx::createTexture2D(
		static_cast<uint16_t>(std::max(backend->width, 1)),
		static_cast<uint16_t>(std::max(backend->height, 1)),
		false,
		1,
		bgfx::TextureFormat::RGBA8,
		0,
		memory);
	backend->dirty = false;
	return bgfx::isValid(backend->handle) ? backend->handle : g_white_texture;
}

uint64_t BgfxCompat_Get_Sampler_Flags(const TextureClass *texture)
{
	uint64_t flags = UINT64_MAX;
	if (texture == NULL) {
		return flags;
	}

	flags = 0;
	if (texture->Get_U_Addr_Mode() == TextureClass::TEXTURE_ADDRESS_CLAMP) {
		flags |= BGFX_SAMPLER_U_CLAMP;
	}
	if (texture->Get_V_Addr_Mode() == TextureClass::TEXTURE_ADDRESS_CLAMP) {
		flags |= BGFX_SAMPLER_V_CLAMP;
	}

	if (texture->Get_Min_Filter() == TextureClass::FILTER_TYPE_NONE) {
		flags |= BGFX_SAMPLER_MIN_POINT;
	} else {
		flags |= BGFX_SAMPLER_MIN_ANISOTROPIC;
	}
	if (texture->Get_Mag_Filter() == TextureClass::FILTER_TYPE_NONE) {
		flags |= BGFX_SAMPLER_MAG_POINT;
	} else {
		flags |= BGFX_SAMPLER_MAG_ANISOTROPIC;
	}
	if (texture->Get_Mip_Mapping() == TextureClass::FILTER_TYPE_NONE) {
		flags |= BGFX_SAMPLER_MIP_POINT;
	} else {
		flags |= BGFX_SAMPLER_MIP_POINT;
	}

	return flags;
}

bgfx::TextureHandle BgfxCompat_Get_White_Texture()
{
	Ensure_White_Texture();
	return g_white_texture;
}

void BgfxCompat_Shutdown_Texture_System()
{
	if (bgfx::isValid(g_white_texture)) {
		bgfx::destroy(g_white_texture);
		g_white_texture = BGFX_INVALID_HANDLE;
	}
}

TextureClass::TextureClass(unsigned width, unsigned height, WW3DFormat format, MipCountType mip_level_count, PoolType pool, bool)
	: TextureMinFilter(FILTER_TYPE_DEFAULT),
	  TextureMagFilter(FILTER_TYPE_DEFAULT),
	  MipMapFilter(mip_level_count != MIP_LEVELS_1 ? FILTER_TYPE_DEFAULT : FILTER_TYPE_NONE),
	  UAddressMode(TEXTURE_ADDRESS_REPEAT),
	  VAddressMode(TEXTURE_ADDRESS_REPEAT),
	  D3DTexture(reinterpret_cast<IDirect3DTexture8 *>(Create_Texture_Backend(static_cast<int>(width), static_cast<int>(height), format))),
	  Initialized(true),
	  texture_id(g_next_texture_id++),
	  IsLightmap(false),
	  IsProcedural(true),
	  IsCompressionAllowed(false),
	  InactivationTime(0),
	  ExtendedInactivationTime(0),
	  LastInactivationSyncTime(0),
	  LastAccessed(WW3D::Get_Sync_Time()),
	  TextureFormat(format == WW3D_FORMAT_UNKNOWN ? WW3D_FORMAT_A8R8G8B8 : format),
	  Width(static_cast<int>(width)),
	  Height(static_cast<int>(height)),
	  Pool(pool),
	  Dirty(pool == POOL_DEFAULT),
	  MipLevelCount(mip_level_count),
	  TextureLoadTask(NULL),
	  ThumbnailLoadTask(NULL)
{
}

TextureClass::TextureClass(const char *name, const char *full_path, MipCountType mip_level_count, WW3DFormat texture_format, bool allow_compression)
	: TextureClass(1U, 1U, texture_format == WW3D_FORMAT_UNKNOWN ? WW3D_FORMAT_A8R8G8B8 : texture_format, mip_level_count, POOL_MANAGED, false)
{
	Name = name != NULL ? name : "";
	if (full_path != NULL) {
		FullPath = full_path;
	} else {
		FullPath = Name;
	}
	IsProcedural = false;
	IsCompressionAllowed = allow_compression;

	SurfaceClass *surface = NEW_REF(SurfaceClass, (FullPath));
	Load_Texture_From_Surface(BgfxCompat_To_Texture(D3DTexture), surface);
	Width = surface->Get_Surface_Format() == WW3D_FORMAT_UNKNOWN ? 1 : BgfxCompat_To_Texture(D3DTexture)->width;
	Height = BgfxCompat_To_Texture(D3DTexture)->height;
	TextureFormat = BgfxCompat_To_Texture(D3DTexture)->format;
	REF_PTR_RELEASE(surface);
}

TextureClass::TextureClass(SurfaceClass *surface, MipCountType mip_level_count)
	: TextureClass(1U, 1U, WW3D_FORMAT_A8R8G8B8, mip_level_count, POOL_MANAGED, false)
{
	if (surface != NULL) {
		Load_Texture_From_Surface(BgfxCompat_To_Texture(D3DTexture), surface);
		Width = BgfxCompat_To_Texture(D3DTexture)->width;
		Height = BgfxCompat_To_Texture(D3DTexture)->height;
		TextureFormat = BgfxCompat_To_Texture(D3DTexture)->format;
	}
}

TextureClass::TextureClass(IDirect3DTexture8 *d3d_texture)
	: TextureMinFilter(FILTER_TYPE_DEFAULT),
	  TextureMagFilter(FILTER_TYPE_DEFAULT),
	  MipMapFilter(FILTER_TYPE_DEFAULT),
	  UAddressMode(TEXTURE_ADDRESS_REPEAT),
	  VAddressMode(TEXTURE_ADDRESS_REPEAT),
	  D3DTexture(d3d_texture),
	  Initialized(true),
	  texture_id(g_next_texture_id++),
	  IsLightmap(false),
	  IsProcedural(true),
	  IsCompressionAllowed(false),
	  InactivationTime(0),
	  ExtendedInactivationTime(0),
	  LastInactivationSyncTime(0),
	  LastAccessed(WW3D::Get_Sync_Time()),
	  TextureFormat(WW3D_FORMAT_A8R8G8B8),
	  Width(1),
	  Height(1),
	  Pool(POOL_MANAGED),
	  Dirty(false),
	  MipLevelCount(MIP_LEVELS_1),
	  TextureLoadTask(NULL),
	  ThumbnailLoadTask(NULL)
{
	BgfxCompatTexture *backend = BgfxCompat_To_Texture(D3DTexture);
	if (backend != NULL) {
		Width = backend->width;
		Height = backend->height;
		TextureFormat = backend->format;
	}
}

TextureClass::~TextureClass(void)
{
	Destroy_Texture_Backend(D3DTexture);
	D3DTexture = NULL;
	delete TextureLoadTask;
	TextureLoadTask = NULL;
	delete ThumbnailLoadTask;
	ThumbnailLoadTask = NULL;
}

void TextureClass::Set_Texture_Name(const char *name)
{
	Name = name != NULL ? name : "";
}

uint32_t TextureClass::Get_Mip_Level_Count(void)
{
	return 1;
}

void TextureClass::Init()
{
	Initialized = true;
	LastAccessed = WW3D::Get_Sync_Time();
}

SurfaceClass *TextureClass::Get_Surface_Level(uint32_t)
{
	BgfxCompatTexture *backend = BgfxCompat_To_Texture(D3DTexture);
	if (backend == NULL) {
		return NULL;
	}
	SurfaceClass *surface = NEW_REF(SurfaceClass, (static_cast<unsigned>(backend->width), static_cast<unsigned>(backend->height), backend->format));
	surface->Copy(backend->bytes.data());
	return surface;
}

IDirect3DSurface8 *TextureClass::Get_D3D_Surface_Level(uint32_t)
{
	return NULL;
}

uint32_t TextureClass::Get_Priority(void)
{
	return 0;
}

uint32_t TextureClass::Set_Priority(uint32_t)
{
	return 0;
}

void TextureClass::Set_Mip_Mapping(FilterType mipmap)
{
	MipMapFilter = mipmap;
}

unsigned TextureClass::Get_Texture_Memory_Usage() const
{
	const BgfxCompatTexture *backend = BgfxCompat_To_Texture(D3DTexture);
	return backend != NULL ? static_cast<unsigned>(backend->bytes.size()) : 0U;
}

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

void TextureClass::Invalidate()
{
	BgfxCompatTexture *backend = BgfxCompat_To_Texture(D3DTexture);
	if (backend != NULL) {
		backend->dirty = true;
	}
	Dirty = true;
}

bool TextureClass::Is_Missing_Texture()
{
	return false;
}

unsigned TextureClass::Get_Reduction() const
{
	return static_cast<unsigned>(WW3D::Get_Texture_Reduction());
}

void TextureClass::Invalidate_Old_Unused_Textures(unsigned)
{
}

void TextureClass::Apply_New_Surface(IDirect3DTexture8 *tex, bool initialized)
{
	if (D3DTexture != tex) {
		Destroy_Texture_Backend(D3DTexture);
		D3DTexture = tex;
	}
	Initialized = initialized;
}

void TextureClass::Apply(uint32_t)
{
}

void TextureClass::Load_Locked_Surface()
{
	Initialized = false;
}

void TextureClass::Apply_Null(uint32_t)
{
}

TextureClass *Load_Texture(ChunkLoadClass &cload)
{
	TextureClass *newtex = NULL;

	char name[256] = {0};
	if (cload.Open_Chunk() && (cload.Cur_Chunk_ID() == W3D_CHUNK_TEXTURE)) {

		W3dTextureInfoStruct texinfo = {};
		bool hastexinfo = false;

		while (cload.Open_Chunk()) {
			switch (cload.Cur_Chunk_ID()) {
				case W3D_CHUNK_TEXTURE_NAME:
				{
					const unsigned length = std::min<unsigned>(cload.Cur_Chunk_Length(), sizeof(name) - 1U);
					cload.Read(&name, length);
					name[length] = '\0';
					break;
				}

				case W3D_CHUNK_TEXTURE_INFO:
					cload.Read(&texinfo, sizeof(W3dTextureInfoStruct));
					hastexinfo = true;
					break;

				default:
					break;
			}

			cload.Close_Chunk();
		}
		cload.Close_Chunk();

		if (name[0] == '\0') {
			return NULL;
		}

		if (hastexinfo) {
			TextureClass::MipCountType mipcount = TextureClass::MIP_LEVELS_ALL;
			const bool no_lod = ((texinfo.Attributes & W3DTEXTURE_NO_LOD) == W3DTEXTURE_NO_LOD);

			if (no_lod) {
				mipcount = TextureClass::MIP_LEVELS_1;
			} else {
				switch (texinfo.Attributes & W3DTEXTURE_MIP_LEVELS_MASK) {
					case W3DTEXTURE_MIP_LEVELS_ALL:
						mipcount = TextureClass::MIP_LEVELS_ALL;
						break;
					case W3DTEXTURE_MIP_LEVELS_2:
						mipcount = TextureClass::MIP_LEVELS_2;
						break;
					case W3DTEXTURE_MIP_LEVELS_3:
						mipcount = TextureClass::MIP_LEVELS_3;
						break;
					case W3DTEXTURE_MIP_LEVELS_4:
						mipcount = TextureClass::MIP_LEVELS_4;
						break;
					default:
						WWASSERT(false);
						mipcount = TextureClass::MIP_LEVELS_ALL;
						break;
				}
			}

			newtex = WW3DAssetManager::Get_Instance()->Get_Texture(name, mipcount, WW3D_FORMAT_UNKNOWN);

			if (no_lod) {
				newtex->Set_Mip_Mapping(TextureClass::FILTER_TYPE_NONE);
			}

			const bool u_clamp = ((texinfo.Attributes & W3DTEXTURE_CLAMP_U) != 0);
			newtex->Set_U_Addr_Mode(u_clamp ? TextureClass::TEXTURE_ADDRESS_CLAMP : TextureClass::TEXTURE_ADDRESS_REPEAT);

			const bool v_clamp = ((texinfo.Attributes & W3DTEXTURE_CLAMP_V) != 0);
			newtex->Set_V_Addr_Mode(v_clamp ? TextureClass::TEXTURE_ADDRESS_CLAMP : TextureClass::TEXTURE_ADDRESS_REPEAT);
		} else {
			newtex = WW3DAssetManager::Get_Instance()->Get_Texture(name);
		}

		WWASSERT(newtex != NULL);
	}

	return newtex;
}

void Save_Texture(TextureClass *, ChunkSaveClass &)
{
}

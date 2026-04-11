/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/texture.cpp                            $*
 *                                                                                             *
 *                      $Author:: Steve_t                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/09/02 2:57p                                               $*
 *                                                                                             *
 *                    $Revision:: 83                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   FileListTextureClass::Load_Frame_Surface -- Load source texture                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "texture.h"

#include <cstdint>
#include <stdio.h>
#include "renderer_types.h"
#include "bgfxrenderer.h"
#include "dx8wrapper.h"
#include <nstrdup.h>
#include "w3d_file.h"
#include "assetmgr.h"
#include "formconv.h"
#include "textureloader.h"
#include "missingtexture.h"
#include "ffactory.h"
#include "dx8caps.h"
#include "dx8texman.h"
#include "meshmatdesc.h"
#include "texturethumbnail.h"
#include "wwdebug.h"

const unsigned DEFAULT_INACTIVATION_TIME=20000;

/*
** Definitions of static members:
*/

static unsigned unused_texture_id;

unsigned _MinTextureFilters[MAX_TEXTURE_STAGES][TextureClass::FILTER_TYPE_COUNT];
unsigned _MagTextureFilters[MAX_TEXTURE_STAGES][TextureClass::FILTER_TYPE_COUNT];
unsigned _MipMapFilters[MAX_TEXTURE_STAGES][TextureClass::FILTER_TYPE_COUNT];

// ----------------------------------------------------------------------------

static int Calculate_Texture_Memory_Usage(const TextureClass* texture,int red_factor=0)
{
	// Set performance statistics

	int size=0;
	for (unsigned i = red_factor; i < texture->Get_Mip_Level_Count(); ++i) {
		SurfaceClass *surface = const_cast<TextureClass *>(texture)->Get_Surface_Level(i);
		if (!surface) {
			break;
		}

		SurfaceClass::SurfaceDescription desc;
		surface->Get_Description(desc);
		switch (desc.Format) {
		case WW3D_FORMAT_DXT1:
			size += ((desc.Width + 3) / 4) * ((desc.Height + 3) / 4) * 8;
			break;
		case WW3D_FORMAT_DXT2:
		case WW3D_FORMAT_DXT3:
		case WW3D_FORMAT_DXT4:
		case WW3D_FORMAT_DXT5:
			size += ((desc.Width + 3) / 4) * ((desc.Height + 3) / 4) * 16;
			break;
		default:
			size += static_cast<int>(desc.Width * desc.Height * Get_Bytes_Per_Pixel(desc.Format));
			break;
		}

		surface->Release_Ref();
	}
	return size;
}

static unsigned Clamp_Mip_Dimension(unsigned value, bool compressed)
{
	unsigned min_value = compressed ? 4U : 1U;
	if (value < min_value) {
		return min_value;
	}
	return value;
}

static bool Is_Compressed_Format(WW3DFormat format)
{
	switch (format) {
	case WW3D_FORMAT_DXT1:
	case WW3D_FORMAT_DXT2:
	case WW3D_FORMAT_DXT3:
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5:
		return true;
	default:
		return false;
	}
}

static unsigned Get_Full_Mip_Count(unsigned width, unsigned height, bool compressed)
{
	unsigned count = 1;
	const unsigned min_dimension = compressed ? 4U : 1U;
	while (width > min_dimension || height > min_dimension) {
		width >>= 1;
		height >>= 1;
		if (width < min_dimension) {
			width = min_dimension;
		}
		if (height < min_dimension) {
			height = min_dimension;
		}
		++count;
	}
	return count;
}

static unsigned Get_Requested_Mip_Count(unsigned width, unsigned height, WW3DFormat format, TextureClass::MipCountType mip_level_count)
{
	if (mip_level_count != TextureClass::MIP_LEVELS_ALL) {
		return std::min<unsigned>(mip_level_count, TextureClass::MIP_LEVELS_MAX);
	}

	return std::min<unsigned>(Get_Full_Mip_Count(width, height, Is_Compressed_Format(format)), TextureClass::MIP_LEVELS_MAX);
}

/*************************************************************************
**                             TextureClass
*************************************************************************/

TextureClass::TextureClass(unsigned width, unsigned height, WW3DFormat format, MipCountType mip_level_count, PoolType pool,bool rendertarget)
	:
	BgfxTexture(BGFX_INVALID_HANDLE),
	BgfxFrameBuffer(BGFX_INVALID_HANDLE),
	texture_id(unused_texture_id++),
	Initialized(true),
	TextureMinFilter(FILTER_TYPE_DEFAULT),
	TextureMagFilter(FILTER_TYPE_DEFAULT),
	MipMapFilter((mip_level_count!=MIP_LEVELS_1) ? FILTER_TYPE_DEFAULT : FILTER_TYPE_NONE),
	UAddressMode(TEXTURE_ADDRESS_REPEAT),
	VAddressMode(TEXTURE_ADDRESS_REPEAT),
	MipLevelCount(mip_level_count),
	Pool(pool),
	Dirty(false),
	IsRenderTargetTexture(rendertarget),
	IsLightmap(false),
	IsProcedural(true),
	Name(""),
	TextureFormat(format),
	IsCompressionAllowed(false),
	TextureLoadTask(NULL),
	ThumbnailLoadTask(NULL),
	Width(width),
	Height(height),
	InactivationTime(0),		// Don't inactivate!
	ExtendedInactivationTime(0),
	LastInactivationSyncTime(0)
{
	switch (format) {
	case WW3D_FORMAT_DXT1:
	case WW3D_FORMAT_DXT2:
	case WW3D_FORMAT_DXT3:
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5:
		IsCompressionAllowed=true;
		break;
	default:
		break;
	}
	if (!rendertarget) {
		const bool compressed = Is_Compressed_Format(format);
		const unsigned level_count = Get_Requested_Mip_Count(width, height, format, mip_level_count);
		unsigned mip_width = Clamp_Mip_Dimension(width, compressed);
		unsigned mip_height = Clamp_Mip_Dimension(height, compressed);
		SurfaceLevels.reserve(level_count);
		for (unsigned i = 0; i < level_count; ++i) {
			SurfaceLevels.push_back(new SurfaceClass(mip_width, mip_height, format));
			mip_width = Clamp_Mip_Dimension(MAX(mip_width >> 1, 1U), compressed);
			mip_height = Clamp_Mip_Dimension(MAX(mip_height >> 1, 1U), compressed);
		}
	}
	if (pool==POOL_DEFAULT)
	{
		Dirty=true;
		DX8TextureTrackerClass *track=new
		DX8TextureTrackerClass(width, height, format, mip_level_count,rendertarget,
		this);
		DX8TextureManagerClass::Add(track);
	}
	LastAccessed=WW3D::Get_Sync_Time();
}

// ----------------------------------------------------------------------------

TextureClass::TextureClass(
	const char *name,
	const char *full_path,
	MipCountType mip_level_count,
	WW3DFormat texture_format,
	bool allow_compression)
	:
	BgfxTexture(BGFX_INVALID_HANDLE),
	BgfxFrameBuffer(BGFX_INVALID_HANDLE),
	texture_id(unused_texture_id++),
	Initialized(false),
	TextureMinFilter(FILTER_TYPE_DEFAULT),
	TextureMagFilter(FILTER_TYPE_DEFAULT),
	MipMapFilter((mip_level_count!=MIP_LEVELS_1) ? FILTER_TYPE_DEFAULT : FILTER_TYPE_NONE),
	UAddressMode(TEXTURE_ADDRESS_REPEAT),
	VAddressMode(TEXTURE_ADDRESS_REPEAT),
	MipLevelCount(mip_level_count),
	Pool(POOL_MANAGED),
	Dirty(false),
	IsRenderTargetTexture(false),
	IsLightmap(false),
	IsProcedural(false),
	TextureFormat(texture_format),
	IsCompressionAllowed(allow_compression),
	TextureLoadTask(NULL),
	ThumbnailLoadTask(NULL),
	Width(0),
	Height(0),
	InactivationTime(DEFAULT_INACTIVATION_TIME),		// Default inactivation time 30 seconds
	ExtendedInactivationTime(0),
	LastInactivationSyncTime(0)
{
	switch (TextureFormat) {
	case WW3D_FORMAT_DXT1:
	case WW3D_FORMAT_DXT2:
	case WW3D_FORMAT_DXT3:
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5:
		IsCompressionAllowed=true;
		break;
	case WW3D_FORMAT_U8V8:		// Bumpmap
	case WW3D_FORMAT_L6V5U5:	// Bumpmap
	case WW3D_FORMAT_X8L8V8U8:	// Bumpmap
		// If requesting bumpmap format that isn't available we'll just return the surface in whatever color
		// format the texture file is in. (This is illegal case, the format support should always be queried
		// before creating a bump texture!)
		if (!DX8Wrapper::Is_Initted() || !DX8Wrapper::Get_Current_Caps()->Support_Texture_Format(TextureFormat)) {
			TextureFormat=WW3D_FORMAT_UNKNOWN;
		}
		// If bump format is valid, make sure compression is not allowed so that we don't even attempt to load
		// from a compressed file (quality isn't good enough for bump map). Also disable mipmapping.
		else {
			IsCompressionAllowed=false;
			MipLevelCount=MIP_LEVELS_1;
			MipMapFilter=FILTER_TYPE_NONE;
		}
		break;

	default:
		break;
	}

	WWASSERT_PRINT(name && name[0], "TextureClass CTor: NULL or empty texture name\n");
	int len=strlen(name);
	for (int i=0;i<len;++i) {
		if (name[i]=='+') {
			IsLightmap=true;

			// Set bilinear filtering for lightmaps (they are very stretched and
			// low detail so we don't care for anisotropic or trilinear filtering...)
			TextureMinFilter=FILTER_TYPE_FAST;
			TextureMagFilter=FILTER_TYPE_FAST;
			if (mip_level_count!=MIP_LEVELS_1) MipMapFilter=FILTER_TYPE_FAST;
			break;
		}
	}
	Set_Texture_Name(name);
	Set_Full_Path(full_path);
	WWASSERT(name[0]!='\0');
	if (!WW3D::Is_Texturing_Enabled()) {
		Initialized=true;
	}

	// Find original size from the thumbnail (but don't create thumbnail texture yet!)
	ThumbnailClass* thumb=NULL;
	ThumbnailManagerClass* thumb_man=ThumbnailManagerClass::Peek_List().Head();
	while (thumb_man) {
		thumb=thumb_man->Peek_Thumbnail_Instance(Get_Full_Path());
		if (thumb) {
			Width=thumb->Get_Original_Texture_Width();
			Height=thumb->Get_Original_Texture_Height();
			break;
		}
		thumb_man=thumb_man->Succ();
	}

	LastAccessed=WW3D::Get_Sync_Time();

	// If the thumbnails are not enabled, init the texture at this point to avoid stalling when the
	// mesh is rendered.
	if (!WW3D::Get_Thumbnail_Enabled()) {
		if (TextureLoader::Is_DX8_Thread()) {
			Init();
		}
	}
}

// ----------------------------------------------------------------------------

TextureClass::TextureClass(SurfaceClass *surface, MipCountType mip_level_count)
	:
	BgfxTexture(BGFX_INVALID_HANDLE),
	BgfxFrameBuffer(BGFX_INVALID_HANDLE),
	texture_id(unused_texture_id++),
	Initialized(true),
	TextureMinFilter(FILTER_TYPE_DEFAULT),
	TextureMagFilter(FILTER_TYPE_DEFAULT),
	MipMapFilter((mip_level_count!=MIP_LEVELS_1) ? FILTER_TYPE_DEFAULT : FILTER_TYPE_NONE),
	UAddressMode(TEXTURE_ADDRESS_REPEAT),
	VAddressMode(TEXTURE_ADDRESS_REPEAT),
	MipLevelCount(mip_level_count),
	Pool(POOL_MANAGED),
	Dirty(false),
	IsRenderTargetTexture(false),
	IsLightmap(false),
	Name(""),
	IsProcedural(true),
	TextureFormat(surface->Get_Surface_Format()),
	IsCompressionAllowed(false),
	TextureLoadTask(NULL),
	ThumbnailLoadTask(NULL),
	Width(0),
	Height(0),
	InactivationTime(0),		// Don't inactivate
	ExtendedInactivationTime(0),
	LastInactivationSyncTime(0)
{
	SurfaceClass::SurfaceDescription sd;
	surface->Get_Description(sd);
	Width=sd.Width;
	Height=sd.Height;
	switch (sd.Format) {
	case WW3D_FORMAT_DXT1:
	case WW3D_FORMAT_DXT2:
	case WW3D_FORMAT_DXT3:
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5:
		IsCompressionAllowed=true;
		break;
	default:
		break;
	}
	surface->Add_Ref();
	SurfaceLevels.push_back(surface);
	if (BgfxRenderer::Is_Initted()) {
		BgfxTexture = BgfxRenderer::Create_Texture(*this);
	}
	LastAccessed=WW3D::Get_Sync_Time();
}

// ----------------------------------------------------------------------------

TextureClass::~TextureClass(void)
{
	delete TextureLoadTask;
	TextureLoadTask=NULL;
	delete ThumbnailLoadTask;
	ThumbnailLoadTask=NULL;

	Release_Bgfx_Texture();
	Release_Surface_Levels();
	DX8TextureManagerClass::Remove(this);
}

void TextureClass::Release_Surface_Levels()
{
	for (size_t i = 0; i < SurfaceLevels.size(); ++i) {
		if (SurfaceLevels[i] != NULL) {
			SurfaceLevels[i]->Release_Ref();
		}
	}
	SurfaceLevels.clear();
}

void TextureClass::Invalidate_Old_Unused_Textures(unsigned invalidation_time_override)
{
	unsigned synctime=WW3D::Get_Sync_Time();
	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager

	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		TextureClass* tex=ite.Peek_Value();

		// Consider invalidating if texture has been initialized and defines inactivation time
		if (tex->Initialized && tex->InactivationTime) {
			unsigned age=synctime-tex->LastAccessed;

			if (invalidation_time_override) {
				if (age>invalidation_time_override) {
					tex->Invalidate();
					tex->LastInactivationSyncTime=synctime;
				}
			}
			else {
				// Not used in the last n milliseconds?
				if (age>(tex->InactivationTime+tex->ExtendedInactivationTime)) {
					tex->Invalidate();
					tex->LastInactivationSyncTime=synctime;
				}
			}
		}
	}
}


// ----------------------------------------------------------------------------

void TextureClass::Init()
{
	// If the texture has already been initialised we should exit now
	if (Initialized) return;

	// If the texture has recently been inactivated, increase the inactivation time (this texture obviously
	// should not have been inactivated yet).

	if (InactivationTime && LastInactivationSyncTime) {
		if ((WW3D::Get_Sync_Time()-LastInactivationSyncTime)<InactivationTime) {
			ExtendedInactivationTime=3*InactivationTime;
		}
		LastInactivationSyncTime=0;
	}


	if (SurfaceLevels.empty()) {
		if (!WW3D::Get_Thumbnail_Enabled() || MipLevelCount==MIP_LEVELS_1) {
//		if (MipLevelCount==MIP_LEVELS_1) {
			TextureLoader::Request_Foreground_Loading(this);
		}
		else {
			WW3DFormat format=TextureFormat;
			Load_Locked_Surface();
			TextureFormat=format;
		}
	}

	if (!Initialized) {
		TextureLoader::Request_Background_Loading(this);
	}

	LastAccessed=WW3D::Get_Sync_Time();
}

void TextureClass::Invalidate()
{
	if (TextureLoadTask) {
		return;
	}
	if (ThumbnailLoadTask) {
		return;
	}

	// Don't invalidate procedural textures
	if (IsProcedural) {
		return;
	}

	Release_Bgfx_Texture();
	Release_Surface_Levels();

	Initialized=false;

	LastAccessed=WW3D::Get_Sync_Time();
}

// ----------------------------------------------------------------------------

void TextureClass::Load_Locked_Surface()
{
	Release_Bgfx_Texture();
	Release_Surface_Levels();
	TextureLoader::Request_Thumbnail(this);
	Initialized=false;
}

// ----------------------------------------------------------------------------

bool TextureClass::Is_Missing_Texture()
{
	return this == MissingTexture::_Peek_Missing_Texture_Instance();
}

// ----------------------------------------------------------------------------

bgfx::TextureHandle TextureClass::Get_Bgfx_Texture()
{
	if (bgfx::isValid(BgfxTexture)) {
		return BgfxTexture;
	}

	if (!Initialized) {
		Init();
	}

	if (!Initialized || !BgfxRenderer::Is_Initted()) {
		return BGFX_INVALID_HANDLE;
	}

	LastAccessed=WW3D::Get_Sync_Time();
	BgfxTexture = BgfxRenderer::Create_Texture(*this);
	return BgfxTexture;
}

bgfx::FrameBufferHandle TextureClass::Get_Bgfx_Frame_Buffer()
{
	if (!bgfx::isValid(BgfxFrameBuffer)) {
		Get_Bgfx_Texture();
	}
	return BgfxFrameBuffer;
}

uint32_t TextureClass::Get_Bgfx_Sampler_Flags() const
{
	uint32_t flags = 0;

	switch (TextureMinFilter) {
	case FILTER_TYPE_NONE:
		flags |= BGFX_SAMPLER_MIN_POINT;
		break;
	case FILTER_TYPE_FAST:
	case FILTER_TYPE_BEST:
	case FILTER_TYPE_DEFAULT:
	default:
		break;
	}

	switch (TextureMagFilter) {
	case FILTER_TYPE_NONE:
		flags |= BGFX_SAMPLER_MAG_POINT;
		break;
	case FILTER_TYPE_FAST:
	case FILTER_TYPE_BEST:
	case FILTER_TYPE_DEFAULT:
	default:
		break;
	}

	switch (MipMapFilter) {
	case FILTER_TYPE_NONE:
	case FILTER_TYPE_FAST:
		flags |= BGFX_SAMPLER_MIP_POINT;
		break;
	case FILTER_TYPE_BEST:
	case FILTER_TYPE_DEFAULT:
	default:
		break;
	}

	if (Get_U_Addr_Mode() == TEXTURE_ADDRESS_CLAMP) {
		flags |= BGFX_SAMPLER_U_CLAMP;
	}
	if (Get_V_Addr_Mode() == TEXTURE_ADDRESS_CLAMP) {
		flags |= BGFX_SAMPLER_V_CLAMP;
	}

	return flags;
}

void TextureClass::Release_Bgfx_Texture()
{
	if (bgfx::isValid(BgfxTexture)) {
		if (bgfx::isValid(BgfxFrameBuffer) && BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxFrameBuffer);
		}
		BgfxFrameBuffer = BGFX_INVALID_HANDLE;
		if (BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxTexture);
		}
		BgfxTexture = BGFX_INVALID_HANDLE;
	}
	if (bgfx::isValid(BgfxFrameBuffer)) {
		if (BgfxRenderer::Is_Initted()) {
			bgfx::destroy(BgfxFrameBuffer);
		}
		BgfxFrameBuffer = BGFX_INVALID_HANDLE;
	}
}

// ----------------------------------------------------------------------------

void TextureClass::Set_Texture_Name(const char * name)
{
	Name=name;
}

// ----------------------------------------------------------------------------

unsigned int TextureClass::Get_Mip_Level_Count(void)
{
	if (IsRenderTargetTexture) {
		return 1;
	}
	return SurfaceLevels.size();
}

// ----------------------------------------------------------------------------

SurfaceClass *TextureClass::Get_Surface_Level(unsigned int level)
{
	if (IsRenderTargetTexture) {
		WWDEBUG_SAY(("TextureClass::Get_Surface_Level(%u) is unavailable for render target '%s'\n", level, Name.Peek_Buffer()));
		return 0;
	}

	if (SurfaceLevels.empty()) {
		if (!Initialized) {
			Init();
		}
	}

	if (!SurfaceLevels.empty()) {
		if (level >= SurfaceLevels.size()) {
			WWASSERT_PRINT(0, "Get_Surface_Level: level out of range!\n");
			return 0;
		}

		SurfaceLevels[level]->Add_Ref();
		return SurfaceLevels[level];
	}

	WWDEBUG_SAY(("TextureClass::Get_Surface_Level(%u) could not provide surface data for '%s'\n", level, Name.Peek_Buffer()));
	return 0;
}

// ----------------------------------------------------------------------------

void TextureClass::Set_Mip_Mapping(FilterType mipmap)
{
	if (mipmap != FILTER_TYPE_NONE && Get_Mip_Level_Count() <= 1) {
		WWASSERT_PRINT(0, "Trying to enable MipMapping on texture w/o Mip levels!\n");
		return;
	}
	MipMapFilter=mipmap;
}

unsigned TextureClass::Get_Reduction() const
{
	if (MipLevelCount==MIP_LEVELS_1) return 0;

	int reduction=WW3D::Get_Texture_Reduction();
	if (MipLevelCount && reduction>MipLevelCount) {
		reduction=MipLevelCount;
	}
	return reduction;
}

// ----------------------------------------------------------------------------

void TextureClass::Apply(unsigned int stage)
{
	if (!Initialized) {
		Init();
	}
	LastAccessed=WW3D::Get_Sync_Time();

	DX8_RECORD_TEXTURE(this);

	// Bind the engine texture directly; bgfx submission resolves native ownership at draw time.
	if (WW3D::Is_Texturing_Enabled()) {
		DX8Wrapper::Set_Texture(stage, this);
	} else {
		DX8Wrapper::Set_Texture(stage, NULL);
	}

	DX8Wrapper::Set_DX8_Texture_Stage_State(stage,D3DTSS_MINFILTER,_MinTextureFilters[stage][TextureMinFilter]);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage,D3DTSS_MAGFILTER,_MagTextureFilters[stage][TextureMagFilter]);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage,D3DTSS_MIPFILTER,_MipMapFilters[stage][MipMapFilter]);

	switch (Get_U_Addr_Mode()) {

		case TEXTURE_ADDRESS_REPEAT:
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
			break;

		case TEXTURE_ADDRESS_CLAMP:
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
			break;

	}

	switch (Get_V_Addr_Mode()) {

		case TEXTURE_ADDRESS_REPEAT:
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
			break;

		case TEXTURE_ADDRESS_CLAMP:
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
			break;

	}
}

// ----------------------------------------------------------------------------

void TextureClass::Apply_Null(unsigned int stage)
{
	DX8Wrapper::Set_Texture(stage, NULL);
}

// ----------------------------------------------------------------------------

void TextureClass::Apply_New_Surface(SurfaceClass *surface, bool initialized)
{
	SurfaceClass *surfaces[1] = { surface };
	Apply_New_Surface(surfaces, 1, initialized);
}

void TextureClass::Apply_New_Surface(SurfaceClass *const *surfaces, unsigned level_count, bool initialized)
{
	WWASSERT(surfaces != NULL);
	WWASSERT(level_count > 0);

	Release_Bgfx_Texture();
	Release_Surface_Levels();

	SurfaceLevels.reserve(level_count);
	for (unsigned i = 0; i < level_count; ++i) {
		WWASSERT(surfaces[i] != NULL);
		SurfaceLevels.push_back(surfaces[i]);
	}

	SurfaceClass::SurfaceDescription desc;
	SurfaceLevels[0]->Get_Description(desc);
	TextureFormat = desc.Format;
	Width = desc.Width;
	Height = desc.Height;
	MipLevelCount = (MipCountType)level_count;

	if (initialized) {
		Initialized = true;
	}
}

// ----------------------------------------------------------------------------

unsigned TextureClass::Get_Texture_Memory_Usage() const
{
	if (!Initialized) return Calculate_Texture_Memory_Usage(this,0);
	return Calculate_Texture_Memory_Usage(this,0);
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Locked_Surface_Size()
{
	int total_locked_surface_size=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {

		// Get the current texture
		TextureClass* tex=ite.Peek_Value();
		if (!tex->Initialized) {
			total_locked_surface_size+=tex->Get_Texture_Memory_Usage();
		}
	}
	return total_locked_surface_size;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Texture_Size()
{
	int total_texture_size=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		// Get the current texture
		TextureClass* tex=ite.Peek_Value();
		total_texture_size+=tex->Get_Texture_Memory_Usage();
	}
	return total_texture_size;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Lightmap_Texture_Size()
{
	int total_texture_size=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		// Get the current texture
		TextureClass* tex=ite.Peek_Value();
		if (tex->Is_Lightmap()) {
			total_texture_size+=tex->Get_Texture_Memory_Usage();
		}
	}
	return total_texture_size;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Procedural_Texture_Size()
{
	int total_texture_size=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		// Get the current texture
		TextureClass* tex=ite.Peek_Value();
		if (tex->Is_Procedural()) {
			total_texture_size+=tex->Get_Texture_Memory_Usage();
		}
	}
	return total_texture_size;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Texture_Count()
{
	int texture_count=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		texture_count++;
	}

	return texture_count;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Lightmap_Texture_Count()
{
	int texture_count=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		if (ite.Peek_Value()->Is_Lightmap()) {
			texture_count++;
		}
	}

	return texture_count;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Procedural_Texture_Count()
{
	int texture_count=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		if (ite.Peek_Value()->Is_Procedural()) {
			texture_count++;
		}
	}

	return texture_count;
}

// ----------------------------------------------------------------------------

int TextureClass::_Get_Total_Locked_Surface_Count()
{
	int texture_count=0;

	HashTemplateIterator<StringClass,TextureClass*> ite(WW3DAssetManager::Get_Instance()->Texture_Hash());
	// Loop through all the textures in the manager
	for (ite.First ();!ite.Is_Done();ite.Next ()) {
		// Get the current texture
		TextureClass* tex=ite.Peek_Value();
		if (!tex->Initialized) {
			texture_count++;
		}
	}

	return texture_count;
}

void TextureClass::_Init_Filters(TextureClass::TextureFilterMode filter_type)
{
	const D3DCAPS8& dx8caps=DX8Wrapper::Get_Current_Caps()->Get_DX8_Caps();

	_MinTextureFilters[0][FILTER_TYPE_NONE]=D3DTEXF_POINT;
	_MagTextureFilters[0][FILTER_TYPE_NONE]=D3DTEXF_POINT;
	_MipMapFilters[0][FILTER_TYPE_NONE]=D3DTEXF_NONE;

	_MinTextureFilters[0][FILTER_TYPE_FAST]=D3DTEXF_LINEAR;
	_MagTextureFilters[0][FILTER_TYPE_FAST]=D3DTEXF_LINEAR;
	_MipMapFilters[0][FILTER_TYPE_FAST]=D3DTEXF_POINT;

	_MagTextureFilters[0][FILTER_TYPE_BEST]=D3DTEXF_POINT;
	_MinTextureFilters[0][FILTER_TYPE_BEST]=D3DTEXF_POINT;
	_MipMapFilters[0][FILTER_TYPE_BEST]=D3DTEXF_POINT;

	if (dx8caps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFLINEAR) _MagTextureFilters[0][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;
	if (dx8caps.TextureFilterCaps&D3DPTFILTERCAPS_MINFLINEAR) _MinTextureFilters[0][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;

	// Set anisotropic filtering only if requested and available
	if (filter_type==TextureClass::TEXTURE_FILTER_ANISOTROPIC) {
		if (dx8caps.TextureFilterCaps&D3DPTFILTERCAPS_MAGFANISOTROPIC) _MagTextureFilters[0][FILTER_TYPE_BEST]=D3DTEXF_ANISOTROPIC;
		if (dx8caps.TextureFilterCaps&D3DPTFILTERCAPS_MINFANISOTROPIC) _MinTextureFilters[0][FILTER_TYPE_BEST]=D3DTEXF_ANISOTROPIC;
	}

	// Set linear mip filter only if requested trilinear or anisotropic, and linear available
	if (filter_type==TextureClass::TEXTURE_FILTER_ANISOTROPIC || filter_type==TextureClass::TEXTURE_FILTER_TRILINEAR) {
		if (dx8caps.TextureFilterCaps&D3DPTFILTERCAPS_MIPFLINEAR) _MipMapFilters[0][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;
	}

	// For stages above zero, set best filter to the same as the stage zero, except if anisotropic
	for (int i=1;i<MAX_TEXTURE_STAGES;++i) {
/*		_MinTextureFilters[i][FILTER_TYPE_NONE]=D3DTEXF_POINT;
		_MagTextureFilters[i][FILTER_TYPE_NONE]=D3DTEXF_POINT;
		_MipMapFilters[i][FILTER_TYPE_NONE]=D3DTEXF_NONE;

		_MinTextureFilters[i][FILTER_TYPE_FAST]=D3DTEXF_LINEAR;
		_MagTextureFilters[i][FILTER_TYPE_FAST]=D3DTEXF_LINEAR;
		_MipMapFilters[i][FILTER_TYPE_FAST]=D3DTEXF_POINT;

		_MagTextureFilters[i][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;
		_MinTextureFilters[i][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;
		_MipMapFilters[i][FILTER_TYPE_BEST]=D3DTEXF_POINT;
*/
		_MinTextureFilters[i][FILTER_TYPE_NONE]=_MinTextureFilters[i-1][FILTER_TYPE_NONE];
		_MagTextureFilters[i][FILTER_TYPE_NONE]=_MagTextureFilters[i-1][FILTER_TYPE_NONE];
		_MipMapFilters[i][FILTER_TYPE_NONE]=_MipMapFilters[i-1][FILTER_TYPE_NONE];

		_MinTextureFilters[i][FILTER_TYPE_FAST]=_MinTextureFilters[i-1][FILTER_TYPE_FAST];
		_MagTextureFilters[i][FILTER_TYPE_FAST]=_MagTextureFilters[i-1][FILTER_TYPE_FAST];
		_MipMapFilters[i][FILTER_TYPE_FAST]=_MipMapFilters[i-1][FILTER_TYPE_FAST];

		if (_MagTextureFilters[i-1][FILTER_TYPE_BEST]==D3DTEXF_ANISOTROPIC) {
			_MagTextureFilters[i][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;
		}
		else {
			_MagTextureFilters[i][FILTER_TYPE_BEST]=_MagTextureFilters[i-1][FILTER_TYPE_BEST];
		}

		if (_MinTextureFilters[i-1][FILTER_TYPE_BEST]==D3DTEXF_ANISOTROPIC) {
			_MinTextureFilters[i][FILTER_TYPE_BEST]=D3DTEXF_LINEAR;
		}
		else {
			_MinTextureFilters[i][FILTER_TYPE_BEST]=_MinTextureFilters[i-1][FILTER_TYPE_BEST];
		}
		_MipMapFilters[i][FILTER_TYPE_BEST]=_MipMapFilters[i-1][FILTER_TYPE_BEST];


	}

	// Set default to best. The level of best filter mode is controlled by the input parameter.
	for (int i = 0; i < MAX_TEXTURE_STAGES; ++i) {
		_MinTextureFilters[i][FILTER_TYPE_DEFAULT]=_MinTextureFilters[i][FILTER_TYPE_BEST];
		_MagTextureFilters[i][FILTER_TYPE_DEFAULT]=_MagTextureFilters[i][FILTER_TYPE_BEST];
		_MipMapFilters[i][FILTER_TYPE_DEFAULT]=_MipMapFilters[i][FILTER_TYPE_BEST];

		DX8Wrapper::Set_DX8_Texture_Stage_State(i,D3DTSS_MAXANISOTROPY,2);
	}

}

void TextureClass::_Set_Default_Min_Filter(FilterType filter)
{
	for (int i=0;i<MAX_TEXTURE_STAGES;++i) {
		_MinTextureFilters[i][FILTER_TYPE_DEFAULT]=_MinTextureFilters[i][filter];
	}
}

void TextureClass::_Set_Default_Mag_Filter(FilterType filter)
{
	for (int i=0;i<MAX_TEXTURE_STAGES;++i) {
		_MagTextureFilters[i][FILTER_TYPE_DEFAULT]=_MagTextureFilters[i][filter];
	}
}

void TextureClass::_Set_Default_Mip_Filter(FilterType filter)
{
	for (int i=0;i<MAX_TEXTURE_STAGES;++i) {
		_MipMapFilters[i][FILTER_TYPE_DEFAULT]=_MipMapFilters[i][filter];
	}
}

// Utility functions
TextureClass *Load_Texture(ChunkLoadClass & cload)
{
	// Assume failure
	TextureClass *newtex = NULL;

	char name[256];
	if (cload.Open_Chunk () && (cload.Cur_Chunk_ID () == W3D_CHUNK_TEXTURE)) {

		W3dTextureInfoStruct texinfo;
		bool hastexinfo = false;

		/*
		** Read in the texture filename, and a possible texture info structure.
		*/
		while (cload.Open_Chunk()) {
			switch (cload.Cur_Chunk_ID()) {
				case W3D_CHUNK_TEXTURE_NAME:
					cload.Read(&name,cload.Cur_Chunk_Length());
					break;

				case W3D_CHUNK_TEXTURE_INFO:
					cload.Read(&texinfo,sizeof(W3dTextureInfoStruct));
					hastexinfo = true;
					break;
			};
			cload.Close_Chunk();
		}
		cload.Close_Chunk();

		/*
		** Get the texture from the asset manager
		*/
		if (hastexinfo) {

			TextureClass::MipCountType mipcount;

			bool no_lod = ((texinfo.Attributes & W3DTEXTURE_NO_LOD) == W3DTEXTURE_NO_LOD);

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
						WWASSERT (false);
						mipcount = TextureClass::MIP_LEVELS_ALL;
						break;
				}
			}

			WW3DFormat format=WW3D_FORMAT_UNKNOWN;

			switch (texinfo.Attributes & W3DTEXTURE_TYPE_MASK) {

				case W3DTEXTURE_TYPE_COLORMAP:
					// Do nothing.
					break;

				case W3DTEXTURE_TYPE_BUMPMAP:
				{
					if (DX8Wrapper::Is_Initted() && DX8Wrapper::Get_Current_Caps()->Support_Bump_Envmap()) {
						// No mipmaps to bumpmap for now
						mipcount=TextureClass::MIP_LEVELS_1;

						if (DX8Wrapper::Get_Current_Caps()->Support_Texture_Format(WW3D_FORMAT_U8V8)) format=WW3D_FORMAT_U8V8;
						else if (DX8Wrapper::Get_Current_Caps()->Support_Texture_Format(WW3D_FORMAT_X8L8V8U8)) format=WW3D_FORMAT_X8L8V8U8;
						else if (DX8Wrapper::Get_Current_Caps()->Support_Texture_Format(WW3D_FORMAT_L6V5U5)) format=WW3D_FORMAT_L6V5U5;
					}
					break;
				}

				default:
					WWASSERT (false);
					break;
			}

			newtex = WW3DAssetManager::Get_Instance()->Get_Texture (name, mipcount, format);

			if (no_lod) {
				newtex->Set_Mip_Mapping(TextureClass::FILTER_TYPE_NONE);
			}
			bool u_clamp = ((texinfo.Attributes & W3DTEXTURE_CLAMP_U) != 0);
			newtex->Set_U_Addr_Mode(u_clamp ? TextureClass::TEXTURE_ADDRESS_CLAMP : TextureClass::TEXTURE_ADDRESS_REPEAT);
			bool v_clamp = ((texinfo.Attributes & W3DTEXTURE_CLAMP_V) != 0);
			newtex->Set_V_Addr_Mode(v_clamp ? TextureClass::TEXTURE_ADDRESS_CLAMP : TextureClass::TEXTURE_ADDRESS_REPEAT);

		} else {
			newtex = WW3DAssetManager::Get_Instance()->Get_Texture(name);
		}

		WWASSERT(newtex);
	}

	// Return a pointer to the new texture
	return newtex;
}

// Utility function used by Save_Texture
void setup_texture_attributes(TextureClass * tex, W3dTextureInfoStruct * texinfo)
{
	texinfo->Attributes = 0;

	if (tex->Get_Mip_Mapping() == TextureClass::FILTER_TYPE_NONE) texinfo->Attributes |= W3DTEXTURE_NO_LOD;
	if (tex->Get_U_Addr_Mode() == TextureClass::TEXTURE_ADDRESS_CLAMP) texinfo->Attributes |= W3DTEXTURE_CLAMP_U;
	if (tex->Get_V_Addr_Mode() == TextureClass::TEXTURE_ADDRESS_CLAMP) texinfo->Attributes |= W3DTEXTURE_CLAMP_V;
}


void Save_Texture(TextureClass * texture,ChunkSaveClass & csave)
{
	const char * filename;
	W3dTextureInfoStruct texinfo;
	memset(&texinfo,0,sizeof(texinfo));

	filename = texture->Get_Full_Path();

	setup_texture_attributes(texture, &texinfo);

	csave.Begin_Chunk(W3D_CHUNK_TEXTURE_NAME);
	csave.Write(filename,strlen(filename)+1);
	csave.End_Chunk();

	if ((texinfo.Attributes != 0) || (texinfo.AnimType != 0) || (texinfo.FrameCount != 0)) {
		csave.Begin_Chunk(W3D_CHUNK_TEXTURE_INFO);
		csave.Write(&texinfo, sizeof(texinfo));
		csave.End_Chunk();
	}
}

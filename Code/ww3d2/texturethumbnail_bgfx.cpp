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

#include "texturethumbnail.h"
#include "hashtemplate.h"
#include "missingtexture.h"
#include "TARGA.H"
#include "bgfx_compat_resources.h"
#include "ww3dformat.h"
#include "textureloader.h"
#include "bitmaphandler.h"
#include "ffactory.h"
#include "rawfile.h"
#include "mixfile.h"
#include "wwdebug.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

DLListClass<ThumbnailManagerClass> ThumbnailManagerClass::ThumbnailManagerList;
static bool g_thumbnail_update_notice_logged = false;

const char* ThumbFileHeader="THU4";

static bool Is_Mix_File(const std::filesystem::path &path)
{
	if (!path.has_extension()) {
		return false;
	}

	std::string extension = path.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char value) {
		return static_cast<char>(std::tolower(value));
	});
	return extension == ".mix";
}

constexpr uint32_t Make_FourCC(char a, char b, char c, char d)
{
	return static_cast<uint32_t>(static_cast<uint8_t>(a)) |
		(static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
		(static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
		(static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
}

struct PackedDDColorKey
{
	uint32_t low_value;
	uint32_t high_value;
};

struct PackedDDCaps2
{
	uint32_t caps;
	uint32_t caps2;
	uint32_t caps3;
	uint32_t caps4;
};

struct PackedDDPixelFormat
{
	uint32_t size;
	uint32_t flags;
	uint32_t fourcc;
	uint32_t rgb_bit_count;
	uint32_t r_bit_mask;
	uint32_t g_bit_mask;
	uint32_t b_bit_mask;
	uint32_t alpha_bit_mask;
};

struct PackedDDSurfaceDesc2
{
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
	uint32_t pitch_or_linear_size;
	uint32_t back_buffer_count;
	uint32_t mipmap_count_or_refresh_rate;
	uint32_t alpha_bit_depth;
	uint32_t reserved;
	uint32_t surface;
	PackedDDColorKey ck_dest_overlay;
	PackedDDColorKey ck_dest_blt;
	PackedDDColorKey ck_src_overlay;
	PackedDDColorKey ck_src_blt;
	PackedDDPixelFormat pixel_format;
	PackedDDCaps2 caps;
	uint32_t texture_stage;
};

enum class DDSKind { DXT1, DXT3, DXT5 };

bool Parse_DDS_Header(FileClass *file, PackedDDSurfaceDesc2 &desc, DDSKind &kind)
{
	char magic[4] = {};
	if (file->Read(magic, sizeof(magic)) != static_cast<int>(sizeof(magic)) || std::memcmp(magic, "DDS ", sizeof(magic)) != 0) {
		return false;
	}

	if (file->Read(&desc, sizeof(desc)) != static_cast<int>(sizeof(desc)) || desc.size != sizeof(desc)) {
		return false;
	}

	const uint32_t fourcc = desc.pixel_format.fourcc;
	if (fourcc == Make_FourCC('D', 'X', 'T', '1')) {
		kind = DDSKind::DXT1;
		return true;
	}

	if (fourcc == Make_FourCC('D', 'X', 'T', '2') || fourcc == Make_FourCC('D', 'X', 'T', '3')) {
		kind = DDSKind::DXT3;
		return true;
	}

	if (fourcc == Make_FourCC('D', 'X', 'T', '4') || fourcc == Make_FourCC('D', 'X', 'T', '5')) {
		kind = DDSKind::DXT5;
		return true;
	}

	return false;
}

bool Get_DDS_File_Info(const char *filename, unsigned &width, unsigned &height, WW3DFormat &format, unsigned &mip_count, uint32_t &date_time)
{
	if (filename == NULL) {
		return false;
	}

	char dds_name[256] = {};
	std::snprintf(dds_name, sizeof(dds_name), "%s", filename);
	const size_t len = std::strlen(dds_name);
	if (len < 4) {
		return false;
	}

	dds_name[len - 3] = 'd';
	dds_name[len - 2] = 'd';
	dds_name[len - 1] = 's';

	file_auto_ptr file(_TheFileFactory, dds_name);
	if (!file->Is_Available() || !file->Open(FileClass::READ)) {
		return false;
	}

	date_time = file->Get_Date_Time();
	PackedDDSurfaceDesc2 desc = {};
	DDSKind kind = DDSKind::DXT1;
	if (!Parse_DDS_Header(file, desc, kind)) {
		file->Close();
		return false;
	}

	file->Close();
	width = std::max(desc.width, 1U);
	height = std::max(desc.height, 1U);
	mip_count = std::max(desc.mipmap_count_or_refresh_rate, 1U);

	switch (kind) {
	case DDSKind::DXT1:
		format = WW3D_FORMAT_DXT1;
		break;
	case DDSKind::DXT3:
		format = WW3D_FORMAT_DXT3;
		break;
	case DDSKind::DXT5:
	default:
		format = WW3D_FORMAT_DXT5;
		break;
	}

	return true;
}

static void Create_Hash_Name(StringClass& name, const StringClass& thumb_name)
{
	name=thumb_name;
	int len=name.Get_Length();
	WWASSERT(!stricmp(&name[len-4],".tga") || !stricmp(&name[len-4],".dds"));
	name[len-4]='\0';
}

	/*	file_auto_ptr my_tga_file(_TheFileFactory,filename);
	if (my_tga_file->Is_Available()) {
		my_tga_file->Open();
		unsigned size=my_tga_file->Size();
		char* tga_memory=new char[size];
		my_tga_file->Read(tga_memory,size);
		my_tga_file->Close();

		StringClass pth("data\\");
		pth+=filename;
		RawFileClass tmp_tga_file(pth);
		tmp_tga_file.Create();
		tmp_tga_file.Write(tga_memory,size);
		tmp_tga_file.Close();
		delete[] tga_memory;

	}
*/


ThumbnailClass::ThumbnailClass(
	ThumbnailManagerClass* manager,
	const char* name,
	uint8_t* bitmap,
	unsigned w,
	unsigned h,
	unsigned original_w,
	unsigned original_h,
	unsigned original_mip_level_count,
	WW3DFormat original_format,
	bool allocated,
	uint32_t date_time)
	:
	Manager(manager),
	Name(name),
	Bitmap(bitmap),
	Allocated(allocated),
	Width(w),
	Height(h),
	OriginalTextureWidth(original_w),
	OriginalTextureHeight(original_h),
	OriginalTextureMipLevelCount(original_mip_level_count),
	OriginalTextureFormat(original_format),
	DateTime(date_time)
{
	Manager->Insert_To_Hash(this);
}

// ----------------------------------------------------------------------------
//
// Load texture and generate mipmap levels if requested. The function tries
// to create texture that matches targa format. If suitable format is not
// available, it selects closest matching format and performs color space
// conversion.
//
// ----------------------------------------------------------------------------

ThumbnailClass::ThumbnailClass(ThumbnailManagerClass* manager, const StringClass& filename)
	:
	Manager(manager),
	Bitmap(0),
	Name(filename),
	Allocated(false),
	Width(0),
	Height(0),
	OriginalTextureWidth(0),
	OriginalTextureHeight(0),
	OriginalTextureMipLevelCount(0),
	OriginalTextureFormat(WW3D_FORMAT_UNKNOWN),
	DateTime(0)
{
	unsigned reduction_factor=3;

	// First, try loading image from a DDS file
	if (Get_DDS_File_Info(filename, OriginalTextureWidth, OriginalTextureHeight, OriginalTextureFormat, OriginalTextureMipLevelCount, DateTime)) {

		int len=Name.Get_Length();
		WWASSERT(len>4);
		Name[len-3]='d';
		Name[len-2]='d';
		Name[len-1]='s';

		Width = OriginalTextureWidth;
		Height = OriginalTextureHeight;
		for (unsigned level = 0; level + 1 < OriginalTextureMipLevelCount && (Width > 32 || Height > 32); ++level) {
			Width = std::max(Width >> 1, 1U);
			Height = std::max(Height >> 1, 1U);
		}

		IDirect3DSurface8 *loaded_surface = TextureLoader::Load_Surface_Immediate(filename, WW3D_FORMAT_UNKNOWN, true);
		const BgfxCompatSurface *dds_surface = BgfxCompat_To_Surface(loaded_surface);
		if (dds_surface == NULL) {
			return;
		}

		Bitmap=new uint8_t[Width*Height*2];
		Allocated=true;
		BitmapHandlerClass::Copy_Image(
			Bitmap,
			Width,
			Height,
			Width * 2,
			WW3D_FORMAT_A4R4G4B4,
			dds_surface->bytes.data(),
			dds_surface->width,
			dds_surface->height,
			dds_surface->width * BgfxCompat_Get_Pixel_Size(dds_surface->format),
			dds_surface->format,
			NULL,
			0,
			false);
		delete dds_surface;
	}
	// If DDS file can't be used try loading from TGA
	else {
		// Make sure the file can be opened. If not, return missing texture.
		Targa targa;
		if (TARGA_ERROR_HANDLER(targa.Open(filename,TGA_READMODE),filename)) return;

		// DX8 uses image upside down compared to TGA
		targa.Header.ImageDescriptor ^= TGAIDF_YORIGIN;

		WW3DFormat src_format,dest_format;
		unsigned src_bpp=0;
		Get_WW3D_Format(src_format,src_bpp,targa);
		if (src_format==WW3D_FORMAT_UNKNOWN) {
			WWDEBUG_SAY(("Unknown texture format for %s\n",filename));
			return;
		}

		// Destination size will be the next power of two square from the larger width and height...
		OriginalTextureWidth=targa.Header.Width;
		OriginalTextureHeight=targa.Header.Height;
		OriginalTextureFormat=src_format;
		Width=targa.Header.Width>>reduction_factor;
		Height=targa.Header.Height>>reduction_factor;
		OriginalTextureMipLevelCount=1;
		unsigned iw=1;
		unsigned ih=1;
		while (iw<OriginalTextureWidth && ih<OriginalTextureHeight) {
			iw+=iw;
			ih+=ih;
			OriginalTextureMipLevelCount++;
		}

		while (Width>32 || Height>32) {
			reduction_factor++;
			Width>>=2;
			Height>>=2;
		}

		unsigned poweroftwowidth = 1;
		while (poweroftwowidth < Width) {
			poweroftwowidth <<= 1;
		}

		unsigned poweroftwoheight = 1;
		while (poweroftwoheight < Height) {
			poweroftwoheight <<= 1;
		}

		Width=poweroftwowidth;
		Height=poweroftwoheight;

		unsigned src_width=targa.Header.Width;
		unsigned src_height=targa.Header.Height;

		// NOTE: We load the palette but we do not yet support paletted textures!
		uint8_t palette[256*4];
		targa.SetPalette(palette);
		if (TARGA_ERROR_HANDLER(targa.Load(filename, TGAF_IMAGE, false),filename)) return;

		// Get time stamp from the tga file
		{
			file_auto_ptr my_tga_file(_TheFileFactory,filename);
			WWASSERT(my_tga_file->Is_Available());
			my_tga_file->Open();
			DateTime=my_tga_file->Get_Date_Time();
			my_tga_file->Close();
		}

		uint8_t* src_surface=(uint8_t*)targa.GetImage();

		int len=Name.Get_Length();
		WWASSERT(len>4);
		Name[len-3]='t';
		Name[len-2]='g';
		Name[len-1]='a';

		Bitmap=new uint8_t[Width*Height*2];
		Allocated=true;

		dest_format=WW3D_FORMAT_A8R8G8B8;
		BitmapHandlerClass::Copy_Image(
			Bitmap,
			Width,
			Height,
			Width*2,
			WW3D_FORMAT_A4R4G4B4,
			src_surface,
			src_width,
			src_height,
			src_width*src_bpp,
			src_format,
			(uint8_t*)targa.GetPalette(),
			targa.Header.CMapDepth>>3,
			false);
	}

	Manager->Insert_To_Hash(this);
}

ThumbnailClass::~ThumbnailClass()
{
	if (Allocated) delete[] Bitmap;
	Manager->Remove_From_Hash(this);
}

void ThumbnailManagerClass::Create_Thumbnails()
{
	SimpleFileFactoryClass ff;
	ff.Set_Sub_Directory("Data\\");

	MixFileFactoryClass mix(MixFileName, &ff);
	FileFactoryClass* old_file_factory=_TheFileFactory;
	_TheFileFactory=&mix;
	if (mix.Is_Valid()) {
		DynamicVectorClass<StringClass> list;
		list.Set_Growth_Step (1000);
		mix.Build_Filename_List(list);
		for (int i=0;i<list.Count();++i) {
			int len=list[i].Get_Length();
			if (!stricmp(&list[i][len-4],".tga") || !stricmp(&list[i][len-4],".dds")) {
				if (!Peek_Thumbnail_Instance(list[i])) {
					new ThumbnailClass(this,list[i]);
				}
			}
		}
	}
	_TheFileFactory=old_file_factory;
}

void ThumbnailManagerClass::Load()
{
	WWASSERT(!ThumbnailMemory);

	// If the thumbnail hash table file is available, init hash table
	DateTime=0;
	file_auto_ptr thumb_file(_TheFileFactory, ThumbnailFileName);

	if (thumb_file->Is_Available()) {
		thumb_file->Open(FileClass::READ);

		DateTime=thumb_file->Get_Date_Time();

		char tmp[4];
		thumb_file->Read(tmp,4);
		if (tmp[0]==ThumbFileHeader[0] &&
			tmp[1]==ThumbFileHeader[1] &&
			tmp[2]==ThumbFileHeader[2] &&
			tmp[3]==ThumbFileHeader[3]) {

			int total_thumb_count;
			int total_header_length;
			int total_data_length;
			thumb_file->Read(&total_thumb_count,sizeof(int));
			thumb_file->Read(&total_header_length,sizeof(int));
			thumb_file->Read(&total_data_length,sizeof(int));
			if (total_thumb_count) {
				WWASSERT(total_data_length && total_header_length);
				ThumbnailMemory=new uint8_t[total_data_length];
				// Load thumbs
				for (int i=0;i<total_thumb_count;++i) {
					char name[256];
					int offset;
					int width;
					int height;
					int original_width;
					int original_height;
					int original_mip_level_count;
					WW3DFormat original_format;
					int name_len;
					uint32_t date_time;
					thumb_file->Read(&date_time,sizeof(uint32_t));
					thumb_file->Read(&offset,sizeof(int));
					thumb_file->Read(&width,sizeof(int));
					thumb_file->Read(&height,sizeof(int));
					thumb_file->Read(&original_width,sizeof(int));
					thumb_file->Read(&original_height,sizeof(int));
					thumb_file->Read(&original_mip_level_count,sizeof(int));
					thumb_file->Read(&original_format,sizeof(int));
					thumb_file->Read(&name_len,sizeof(int));
					WWASSERT(name_len<255);
					thumb_file->Read(name,name_len);
					name[name_len]='\0';

					// If per-texture time stamp test is enabled, thumbnail is only used if its time stamp
					// matches the texture's time stamp.
					bool valid=true;
					if (Is_Per_Texture_Time_Stamp_Used()) {
						file_auto_ptr texture_file(_TheFileFactory, name);
						if (texture_file->Is_Available()) {
							texture_file->Open();
							if (texture_file->Get_Date_Time()!=date_time) {
								valid=false;
							}
							texture_file->Close();
						}
						else {
							valid=false;
						}
					}

					if (valid) {
						new ThumbnailClass(
							this,
							name,
							ThumbnailMemory+offset-total_header_length,
							width,
							height,
							original_width,
							original_height,
							original_mip_level_count,
							original_format,
							false,
							date_time);
					}
				}
				thumb_file->Read(ThumbnailMemory,total_data_length);
			}
		}
		thumb_file->Close();
	}
	Changed=false;
}

void ThumbnailManagerClass::Save(bool force)
{
	// Save only if changed
	if (!Changed && !force) return;
	Changed=false;

	// If the thumbnail hash table was modified, save it to disk
	HashTemplateIterator<StringClass,ThumbnailClass*> ite(ThumbnailHash);
	int total_header_length=0;
	int total_data_length=0;
	int total_thumb_count=0;
	total_header_length+=4;	// header
	total_header_length+=4;	// thumb count
	total_header_length+=4;	// header size
	total_header_length+=4;	// data length

	for (ite.First();!ite.Is_Done();ite.Next()) {
		ThumbnailClass* thumb=ite.Peek_Value();
		total_header_length+=4;	// per-thumb date-time
		total_header_length+=4;	// int bitmap offset
		total_header_length+=4;	// int bitmap width
		total_header_length+=4;	// int bitmap height
		total_header_length+=4;	// int original bitmap width
		total_header_length+=4;	// int original bitmap height
		total_header_length+=4;	// int original mip level count
		total_header_length+=4;	// int original format
		total_header_length+=4;	// int name string length

		total_header_length+=strlen(thumb->Get_Name());
		total_data_length+=thumb->Get_Width()*thumb->Get_Height()*2;
		total_thumb_count++;
	}
	int offset=total_header_length;

	file_auto_ptr thumb_file(_TheWritingFileFactory, ThumbnailFileName);
	if (thumb_file->Is_Available()) {
		thumb_file->Delete();
	}
	thumb_file->Create();
	thumb_file->Open(FileClass::WRITE);

	thumb_file->Write(ThumbFileHeader,4);
	thumb_file->Write(&total_thumb_count,sizeof(int));
	thumb_file->Write(&total_header_length,sizeof(int));
	thumb_file->Write(&total_data_length,sizeof(int));

	// Save names and offsets
	for (ite.First();!ite.Is_Done();ite.Next()) {
		ThumbnailClass* thumb=ite.Peek_Value();
		const char* name=thumb->Get_Name();
		int name_len=strlen(name);
		int width=thumb->Get_Width();
		int height=thumb->Get_Height();
		int original_width=thumb->Get_Original_Texture_Width();
		int original_height=thumb->Get_Original_Texture_Height();
		int original_mip_level_count=thumb->Get_Original_Texture_Mip_Level_Count();
		WW3DFormat original_format=thumb->Get_Original_Texture_Format();
		uint32_t date_time=thumb->Get_Date_Time();

		thumb_file->Write(&date_time,sizeof(uint32_t));
		thumb_file->Write(&offset,sizeof(int));
		thumb_file->Write(&width,sizeof(int));
		thumb_file->Write(&height,sizeof(int));
		thumb_file->Write(&original_width,sizeof(int));
		thumb_file->Write(&original_height,sizeof(int));
		thumb_file->Write(&original_mip_level_count,sizeof(int));
		thumb_file->Write(&original_format,sizeof(int));
		thumb_file->Write(&name_len,sizeof(int));
		thumb_file->Write(name,name_len);
		offset+=width*height*2;
	}

	// Save bitmaps
	offset=total_header_length;
	for (ite.First();!ite.Is_Done();ite.Next()) {
		ThumbnailClass* thumb=ite.Peek_Value();
		int width=thumb->Get_Width();
		int height=thumb->Get_Height();
		thumb_file->Write(thumb->Peek_Bitmap(),width*height*2);
	}
	if (DateTime) {
#ifdef _UNIX
		WWDEBUG_SAY(("ThumbnailManagerClass::Save skipped thumbnail timestamp preservation for %s because RawFileClass::Set_Date_Time is not implemented on Unix yet.\n", ThumbnailFileName.Peek_Buffer()));
#else
		thumb_file->Set_Date_Time(DateTime);
#endif
	}
	thumb_file->Close();
}


// ----------------------------------------------------------------------------
ThumbnailManagerClass::ThumbnailManagerClass(const char* thumbnail_filename, const char* mix_filename)
	:
	ThumbnailMemory(NULL),
	ThumbnailFileName(thumbnail_filename),
	MixFileName(mix_filename),
	PerTextureTimeStampUsed(false),
	CreateThumbnailIfNotFound(false),
	Changed(false),
	DateTime(0)
{
	Load();
}

// ----------------------------------------------------------------------------
ThumbnailManagerClass::~ThumbnailManagerClass()
{
	Save();
	HashTemplateIterator<StringClass,ThumbnailClass*> ite(ThumbnailHash);
	ite.First();
	while (!ite.Is_Done()) {
		ThumbnailClass* thumb=ite.Peek_Value();
		delete thumb;
		ite.First();
	}

	if (ThumbnailMemory) delete[] ThumbnailMemory;
	ThumbnailMemory=NULL;
}

// ----------------------------------------------------------------------------
void ThumbnailManagerClass::Add_Thumbnail_Manager(const char* thumbnail_filename, const char* mix_filename)
{
	// First loop over all thumbnail managers to see if we already have this one created. This isn't
	// supposed to be called often at all and there are usually just couple managers alive,
	// so we'll do pure string compares here...
	ThumbnailManagerClass* man=ThumbnailManagerList.Head();
	while (man) {
		if (man->ThumbnailFileName==thumbnail_filename) return;
		man=man->Succ();
	}

	// Always update thumbnail files when they're out of date
	Update_Thumbnail_File(mix_filename,false);

	// Not found, create and add to the list.
	man=new ThumbnailManagerClass(thumbnail_filename,mix_filename);
	ThumbnailManagerList.Add_Tail(man);
}
// ----------------------------------------------------------------------------
void ThumbnailManagerClass::Remove_Thumbnail_Manager(const char* thumbnail_filename)
{
	ThumbnailManagerClass* man=ThumbnailManagerList.Head();
	while (man) {
		if (man->ThumbnailFileName==thumbnail_filename) {
			delete man;
			return;
		}
		man=man->Succ();
	}
}
// ----------------------------------------------------------------------------
ThumbnailClass* ThumbnailManagerClass::Peek_Thumbnail_Instance(const StringClass& name)
{
	ThumbnailClass* thumb=Get_From_Hash(name);
	if (!thumb) {
		if (Is_Thumbnail_Created_If_Not_Found()) {
			thumb=new ThumbnailClass(this,name);
			if (!thumb->Peek_Bitmap()) {
				delete thumb;
				thumb=NULL;
			}
		}
	}
	return thumb;
}

void ThumbnailManagerClass::Insert_To_Hash(ThumbnailClass* thumb)
{
	Changed=true;
	StringClass hash_name(0,true);
	Create_Hash_Name(hash_name,thumb->Get_Name());
	ThumbnailHash.Insert(hash_name,thumb);
}

ThumbnailClass* ThumbnailManagerClass::Get_From_Hash(const StringClass& name)
{
	StringClass hash_name(0,true);
	Create_Hash_Name(hash_name,name);
	return ThumbnailHash.Get(hash_name);
}

void ThumbnailManagerClass::Remove_From_Hash(ThumbnailClass* thumb)
{
	Changed=true;
	StringClass hash_name(0,true);
	Create_Hash_Name(hash_name,thumb->Get_Name());
	ThumbnailHash.Remove(hash_name);
}

void ThumbnailManagerClass::Update_Thumbnail_File(const char* mix_file_name,bool display_message_box)
{
	SimpleFileFactoryClass ff;
	ff.Set_Sub_Directory("Data\\");

	StringClass thumb_file_name(mix_file_name,true);
	int len=thumb_file_name.Get_Length();
	WWASSERT(len>4);
	thumb_file_name[len-3]='t';
	thumb_file_name[len-2]='h';
	thumb_file_name[len-1]='u';
	FileClass* mix_file=ff.Get_File(mix_file_name);
	FileClass* thumb_file=ff.Get_File(thumb_file_name);

	WWASSERT(mix_file && thumb_file);

	// If mix file isn't found but thumb file is, delete the obsolete thumb file
	mix_file->Open(FileClass::READ);
	thumb_file->Open(FileClass::READ);		//|FileClass::WRITE);	Changed this to READ only since we never use the handle for writing and it may cause contention amongst slave servers ST - 12/14/2001 8:26PM
	if (!mix_file->Is_Available()) {
		if (thumb_file->Is_Available()) {
			thumb_file->Delete();
		}
		mix_file->Close();
		thumb_file->Close();
		ff.Return_File(mix_file);
		ff.Return_File(thumb_file);
		return;
	}

	uint32_t mix_date_time=mix_file->Get_Date_Time();
	if (thumb_file->Is_Available()) {
		uint32_t thumb_date_time=thumb_file->Get_Date_Time();
		if (mix_date_time!=thumb_date_time) {
			thumb_file->Delete();
		}
		// Read header to make sure the thumb file is of correct version.
		char tmp[4];
		thumb_file->Read(tmp,4);
		if (tmp[0]!=ThumbFileHeader[0] ||
			tmp[1]!=ThumbFileHeader[1] ||
			tmp[2]!=ThumbFileHeader[2] ||
			tmp[3]!=ThumbFileHeader[3]) {
			thumb_file->Delete();
		}
	}

	if (thumb_file->Is_Available()) {
		mix_file->Close();
		thumb_file->Close();
		ff.Return_File(mix_file);
		ff.Return_File(thumb_file);
		return;
	}

	if (display_message_box && !g_thumbnail_update_notice_logged) {
		g_thumbnail_update_notice_logged = true;
		WWRELEASE_SAY((
			"Updating texture thumbnails because '%s' is missing a current thumbnail database; this may take a while but only needs to happen after a mix file changes.\n",
			mix_file_name));
	}

	// we don't currently have a thumbnail file (either we just deleted it or it never existed, we don't care)
	// so we must create one now.
	ThumbnailManagerClass* manager=new ThumbnailManagerClass(thumb_file_name, mix_file_name);
	manager->DateTime=mix_date_time;	// Set the datetime to mixfile's datetime.
	manager->Create_Thumbnails();
	manager->Save(true);
	delete manager;

	// close files and return pointers
	mix_file->Close();
	thumb_file->Close();
	ff.Return_File(mix_file);
	ff.Return_File(thumb_file);

}

// Verify that up-to-date thumbnails exist for all textures
void ThumbnailManagerClass::Pre_Init(bool display_message_box)
{
	WWASSERT(!ThumbnailManagerList.Head());

	// Collect all mix file names
	DynamicVectorClass<StringClass> mix_names;
	std::error_code ec;
	const std::filesystem::path data_dir = std::filesystem::current_path(ec) / "Data";
	if (ec) {
		WWDEBUG_SAY(("ThumbnailManagerClass::Pre_Init failed to resolve the current working directory: %s\n", ec.message().c_str()));
	} else if (!std::filesystem::exists(data_dir, ec)) {
		if (ec) {
			WWDEBUG_SAY(("ThumbnailManagerClass::Pre_Init failed to probe '%s': %s\n", data_dir.string().c_str(), ec.message().c_str()));
		} else {
			WWDEBUG_SAY(("ThumbnailManagerClass::Pre_Init could not find '%s'; skipping thumbnail pre-generation scan.\n", data_dir.string().c_str()));
		}
	} else {
		for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(data_dir, ec)) {
			if (ec) {
				WWDEBUG_SAY(("ThumbnailManagerClass::Pre_Init stopped scanning '%s': %s\n", data_dir.string().c_str(), ec.message().c_str()));
				break;
			}

			if (!entry.is_regular_file()) {
				continue;
			}

			if (!Is_Mix_File(entry.path())) {
				continue;
			}

			const std::string mix_name = entry.path().filename().string();
			mix_names.Add(mix_name.c_str());
		}
	}

	// First generate thumbnails for always.dat
	Update_Thumbnail_File("always.dat",display_message_box);

	// Then loop over all .mix files
	for (int i=0;i<mix_names.Count();++i) {
		Update_Thumbnail_File(mix_names[i],display_message_box);
	}
}

void ThumbnailManagerClass::Init()
{
	// Renegade uses always.dat... Should all our games use, or should this be moved to somewhere else?
	Add_Thumbnail_Manager("always.thu","always.dat");
	Add_Thumbnail_Manager("always2.thu","always2.dat");
}

void ThumbnailManagerClass::Deinit()
{
	while (ThumbnailManagerClass* man=ThumbnailManagerList.Head()) {
		delete man;
	}
}

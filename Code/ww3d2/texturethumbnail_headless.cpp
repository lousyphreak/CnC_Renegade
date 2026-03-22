#include "texturethumbnail.h"

DLListClass<ThumbnailManagerClass> ThumbnailManagerClass::ThumbnailManagerList;

ThumbnailClass::ThumbnailClass(
    ThumbnailManagerClass *manager,
    const char *name,
    unsigned char *bitmap,
    unsigned w,
    unsigned h,
    unsigned original_w,
    unsigned original_h,
    unsigned original_mip_level_count,
    WW3DFormat original_format,
    bool allocated,
    unsigned long date_time)
    : Name(name, true),
      Bitmap(bitmap),
      Width(w),
      Height(h),
      OriginalTextureWidth(original_w),
      OriginalTextureHeight(original_h),
      OriginalTextureMipLevelCount(original_mip_level_count),
      OriginalTextureFormat(original_format),
      DateTime(date_time),
      Allocated(allocated),
      Manager(manager)
{
}

ThumbnailClass::ThumbnailClass(ThumbnailManagerClass *manager, const StringClass &filename)
    : Name(filename),
      Bitmap(nullptr),
      Width(0),
      Height(0),
      OriginalTextureWidth(0),
      OriginalTextureHeight(0),
      OriginalTextureMipLevelCount(0),
      OriginalTextureFormat(WW3D_FORMAT_UNKNOWN),
      DateTime(0),
      Allocated(false),
      Manager(manager)
{
}

ThumbnailClass::~ThumbnailClass()
{
    if (Allocated) {
        delete[] Bitmap;
    }
}

ThumbnailManagerClass::ThumbnailManagerClass(const char *thumbnail_filename, const char *mix_file_name)
    : CreateThumbnailIfNotFound(false),
      PerTextureTimeStampUsed(false),
      ThumbnailFileName(thumbnail_filename, true),
      MixFileName(mix_file_name, true),
    ThumbnailHash(),
      ThumbnailMemory(nullptr),
      Changed(false),
      DateTime(0)
{
}

ThumbnailManagerClass::~ThumbnailManagerClass()
{
    delete[] ThumbnailMemory;
}

void ThumbnailManagerClass::Remove_From_Hash(ThumbnailClass *) {}
void ThumbnailManagerClass::Insert_To_Hash(ThumbnailClass *) {}
ThumbnailClass *ThumbnailManagerClass::Get_From_Hash(const StringClass &) { return nullptr; }
void ThumbnailManagerClass::Create_Thumbnails() {}
void ThumbnailManagerClass::Update_Thumbnail_File(const char *, bool) {}
void ThumbnailManagerClass::Load() {}
void ThumbnailManagerClass::Save(bool) {}
void ThumbnailManagerClass::Add_Thumbnail_Manager(const char *, const char *) {}
void ThumbnailManagerClass::Remove_Thumbnail_Manager(const char *) {}
ThumbnailClass *ThumbnailManagerClass::Peek_Thumbnail_Instance(const StringClass &) { return nullptr; }
void ThumbnailManagerClass::Pre_Init(bool) {}
void ThumbnailManagerClass::Init() {}
void ThumbnailManagerClass::Deinit() {}

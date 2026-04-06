#include "textureloader.h"

#include "ffactory.h"
#include "ww3d.h"
#include "wwdebug.h"
#include "texture.h"

#if RENEGADE_WITH_BGFX_RENDERER

#include "bgfx_compat_resources.h"
#include "TARGA.H"
#include "bitmaphandler.h"
#include "ww3dformat.h"
#include "texturethumbnail.h"

#include <algorithm>
#include <cstring>

namespace {

////////////////////////////////////////////////////////////////////////////////
//
// DDS format structures
//
////////////////////////////////////////////////////////////////////////////////

constexpr uint32_t Make_FourCC(char a, char b, char c, char d)
{
return static_cast<uint32_t>(static_cast<uint8_t>(a)) |
(static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
(static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
(static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
}

struct DDSBGRAColor
{
uint8_t b = 0;
uint8_t g = 0;
uint8_t r = 0;
uint8_t a = 255;
};

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

////////////////////////////////////////////////////////////////////////////////
//
// DDS block decoders
//
////////////////////////////////////////////////////////////////////////////////

DDSBGRAColor Decode_RGB565(uint16_t value)
{
DDSBGRAColor color;
color.r = static_cast<uint8_t>((((value >> 11) & 0x1F) * 255U + 15U) / 31U);
color.g = static_cast<uint8_t>((((value >> 5) & 0x3F) * 255U + 31U) / 63U);
color.b = static_cast<uint8_t>(((value & 0x1F) * 255U + 15U) / 31U);
color.a = 255;
return color;
}

DDSBGRAColor Interpolate_Color(const DDSBGRAColor &first, const DDSBGRAColor &second, unsigned first_weight, unsigned second_weight, unsigned divisor)
{
DDSBGRAColor color;
color.b = static_cast<uint8_t>((first.b * first_weight + second.b * second_weight) / divisor);
color.g = static_cast<uint8_t>((first.g * first_weight + second.g * second_weight) / divisor);
color.r = static_cast<uint8_t>((first.r * first_weight + second.r * second_weight) / divisor);
color.a = static_cast<uint8_t>((first.a * first_weight + second.a * second_weight) / divisor);
return color;
}

void Write_Pixel(BgfxCompatSurface *surface, unsigned x, unsigned y, const DDSBGRAColor &color)
{
if (surface == NULL || x >= surface->width || y >= surface->height) {
return;
}

const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(surface->width) + static_cast<size_t>(x)) * 4U;
surface->bytes[offset + 0] = color.b;
surface->bytes[offset + 1] = color.g;
surface->bytes[offset + 2] = color.r;
surface->bytes[offset + 3] = color.a;
}

void Decode_DXT1_Block(const uint8_t *block, BgfxCompatSurface *surface, unsigned block_x, unsigned block_y)
{
const uint16_t color0 = static_cast<uint16_t>(block[0] | (block[1] << 8));
const uint16_t color1 = static_cast<uint16_t>(block[2] | (block[3] << 8));

DDSBGRAColor colors[4];
colors[0] = Decode_RGB565(color0);
colors[1] = Decode_RGB565(color1);
if (color0 > color1) {
colors[2] = Interpolate_Color(colors[0], colors[1], 2U, 1U, 3U);
colors[3] = Interpolate_Color(colors[0], colors[1], 1U, 2U, 3U);
} else {
colors[2] = Interpolate_Color(colors[0], colors[1], 1U, 1U, 2U);
colors[3] = DDSBGRAColor();
colors[3].a = 0;
}

const uint32_t indices = static_cast<uint32_t>(block[4]) |
(static_cast<uint32_t>(block[5]) << 8) |
(static_cast<uint32_t>(block[6]) << 16) |
(static_cast<uint32_t>(block[7]) << 24);

for (unsigned py = 0; py < 4; ++py) {
for (unsigned px = 0; px < 4; ++px) {
const unsigned shift = 2U * (4U * py + px);
const unsigned index = (indices >> shift) & 0x3U;
Write_Pixel(surface, block_x + px, block_y + py, colors[index]);
}
}
}

void Decode_DXT3_Block(const uint8_t *block, BgfxCompatSurface *surface, unsigned block_x, unsigned block_y)
{
const uint8_t *alpha_block = block;
const uint8_t *color_block = block + 8;
const uint16_t color0 = static_cast<uint16_t>(color_block[0] | (color_block[1] << 8));
const uint16_t color1 = static_cast<uint16_t>(color_block[2] | (color_block[3] << 8));

DDSBGRAColor colors[4];
colors[0] = Decode_RGB565(color0);
colors[1] = Decode_RGB565(color1);
colors[2] = Interpolate_Color(colors[0], colors[1], 2U, 1U, 3U);
colors[3] = Interpolate_Color(colors[0], colors[1], 1U, 2U, 3U);

const uint32_t indices = static_cast<uint32_t>(color_block[4]) |
(static_cast<uint32_t>(color_block[5]) << 8) |
(static_cast<uint32_t>(color_block[6]) << 16) |
(static_cast<uint32_t>(color_block[7]) << 24);

for (unsigned py = 0; py < 4; ++py) {
const uint16_t alpha_row = static_cast<uint16_t>(alpha_block[py * 2] | (alpha_block[py * 2 + 1] << 8));
for (unsigned px = 0; px < 4; ++px) {
const unsigned shift = 2U * (4U * py + px);
const unsigned color_index = (indices >> shift) & 0x3U;
DDSBGRAColor color = colors[color_index];
const uint8_t alpha = static_cast<uint8_t>(((alpha_row >> (px * 4U)) & 0xFU) * 17U);
color.a = alpha;
Write_Pixel(surface, block_x + px, block_y + py, color);
}
}
}

void Decode_DXT5_Block(const uint8_t *block, BgfxCompatSurface *surface, unsigned block_x, unsigned block_y)
{
uint8_t alpha_values[8] = {};
alpha_values[0] = block[0];
alpha_values[1] = block[1];
if (alpha_values[0] > alpha_values[1]) {
alpha_values[2] = static_cast<uint8_t>((6U * alpha_values[0] + 1U * alpha_values[1] + 3U) / 7U);
alpha_values[3] = static_cast<uint8_t>((5U * alpha_values[0] + 2U * alpha_values[1] + 3U) / 7U);
alpha_values[4] = static_cast<uint8_t>((4U * alpha_values[0] + 3U * alpha_values[1] + 3U) / 7U);
alpha_values[5] = static_cast<uint8_t>((3U * alpha_values[0] + 4U * alpha_values[1] + 3U) / 7U);
alpha_values[6] = static_cast<uint8_t>((2U * alpha_values[0] + 5U * alpha_values[1] + 3U) / 7U);
alpha_values[7] = static_cast<uint8_t>((1U * alpha_values[0] + 6U * alpha_values[1] + 3U) / 7U);
} else {
alpha_values[2] = static_cast<uint8_t>((4U * alpha_values[0] + 1U * alpha_values[1] + 2U) / 5U);
alpha_values[3] = static_cast<uint8_t>((3U * alpha_values[0] + 2U * alpha_values[1] + 2U) / 5U);
alpha_values[4] = static_cast<uint8_t>((2U * alpha_values[0] + 3U * alpha_values[1] + 2U) / 5U);
alpha_values[5] = static_cast<uint8_t>((1U * alpha_values[0] + 4U * alpha_values[1] + 2U) / 5U);
alpha_values[6] = 0;
alpha_values[7] = 255;
}

uint64_t alpha_indices = 0;
for (unsigned i = 0; i < 6; ++i) {
alpha_indices |= static_cast<uint64_t>(block[2 + i]) << (8U * i);
}

const uint8_t *color_block = block + 8;
const uint16_t color0 = static_cast<uint16_t>(color_block[0] | (color_block[1] << 8));
const uint16_t color1 = static_cast<uint16_t>(color_block[2] | (color_block[3] << 8));

DDSBGRAColor colors[4];
colors[0] = Decode_RGB565(color0);
colors[1] = Decode_RGB565(color1);
colors[2] = Interpolate_Color(colors[0], colors[1], 2U, 1U, 3U);
colors[3] = Interpolate_Color(colors[0], colors[1], 1U, 2U, 3U);

const uint32_t color_indices = static_cast<uint32_t>(color_block[4]) |
(static_cast<uint32_t>(color_block[5]) << 8) |
(static_cast<uint32_t>(color_block[6]) << 16) |
(static_cast<uint32_t>(color_block[7]) << 24);

for (unsigned py = 0; py < 4; ++py) {
for (unsigned px = 0; px < 4; ++px) {
const unsigned pixel_index = 4U * py + px;
const unsigned alpha_index = static_cast<unsigned>((alpha_indices >> (3U * pixel_index)) & 0x7U);
const unsigned color_shift = 2U * pixel_index;
const unsigned color_index = (color_indices >> color_shift) & 0x3U;
DDSBGRAColor color = colors[color_index];
color.a = alpha_values[alpha_index];
Write_Pixel(surface, block_x + px, block_y + py, color);
}
}
}

////////////////////////////////////////////////////////////////////////////////
//
// Surface / texture creation helpers
//
////////////////////////////////////////////////////////////////////////////////

BgfxCompatSurface *Create_Surface(unsigned width, unsigned height, WW3DFormat format)
{
BgfxCompatSurface *surface = new BgfxCompatSurface();
surface->width = std::max(width, 1U);
surface->height = std::max(height, 1U);
surface->format = format;
surface->bytes.resize(static_cast<size_t>(surface->width) * static_cast<size_t>(surface->height) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(format)), 0);
return surface;
}

BgfxCompatTexture *Create_Bgfx_Texture(unsigned width, unsigned height, WW3DFormat format)
{
BgfxCompatTexture *texture = new BgfxCompatTexture();
texture->width = static_cast<int>(std::max(width, 1U));
texture->height = static_cast<int>(std::max(height, 1U));
texture->format = (format == WW3D_FORMAT_UNKNOWN) ? WW3D_FORMAT_A8R8G8B8 : format;
texture->bytes.resize(
static_cast<size_t>(texture->width) *
static_cast<size_t>(texture->height) *
static_cast<size_t>(BgfxCompat_Get_Pixel_Size(texture->format)), 0);
texture->dirty = true;
return texture;
}

void Destroy_Bgfx_Texture_Local(IDirect3DTexture8 *&tex)
{
BgfxCompatTexture *backend = BgfxCompat_To_Texture(tex);
if (backend != NULL) {
if (bgfx::isValid(backend->handle)) {
bgfx::destroy(backend->handle);
backend->handle = BGFX_INVALID_HANDLE;
}
delete backend;
}
tex = NULL;
}

IDirect3DTexture8 *Create_Missing_Bgfx_Texture()
{
BgfxCompatTexture *tex = Create_Bgfx_Texture(4, 4, WW3D_FORMAT_A8R8G8B8);
for (int y = 0; y < 4; ++y) {
for (int x = 0; x < 4; ++x) {
size_t offset = static_cast<size_t>(y * 4 + x) * 4;
bool checker = ((x ^ y) & 1) != 0;
tex->bytes[offset + 0] = 0;                    // B
tex->bytes[offset + 1] = 0;                    // G
tex->bytes[offset + 2] = checker ? 255 : 0;   // R
tex->bytes[offset + 3] = 255;                   // A
}
}
return reinterpret_cast<IDirect3DTexture8 *>(tex);
}

////////////////////////////////////////////////////////////////////////////////
//
// DDS / TGA file loading
//
////////////////////////////////////////////////////////////////////////////////

enum class DDSKind { DXT1, DXT3, DXT5 };

bool Parse_DDS_Header(FileClass *file, PackedDDSurfaceDesc2 &desc, DDSKind &kind, unsigned &block_size)
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
block_size = 8;
kind = DDSKind::DXT1;
} else if (fourcc == Make_FourCC('D', 'X', 'T', '2') || fourcc == Make_FourCC('D', 'X', 'T', '3')) {
block_size = 16;
kind = DDSKind::DXT3;
} else if (fourcc == Make_FourCC('D', 'X', 'T', '4') || fourcc == Make_FourCC('D', 'X', 'T', '5')) {
block_size = 16;
kind = DDSKind::DXT5;
} else {
return false;
}

return true;
}

bool Load_DDS_Surface(const char *filename, BgfxCompatSurface *surface)
{
if (filename == NULL || surface == NULL) {
return false;
}

char dds_name[256] = {};
std::snprintf(dds_name, sizeof(dds_name), "%s", filename);
const size_t name_length = std::strlen(dds_name);
if (name_length < 4) {
return false;
}
dds_name[name_length - 3] = 'd';
dds_name[name_length - 2] = 'd';
dds_name[name_length - 1] = 's';

file_auto_ptr file(_TheFileFactory, dds_name);
if (!file->Is_Available() || !file->Open(FileClass::READ)) {
return false;
}

PackedDDSurfaceDesc2 desc = {};
DDSKind kind;
unsigned block_size = 0;
if (!Parse_DDS_Header(file, desc, kind, block_size)) {
file->Close();
return false;
}

surface->width = std::max(desc.width, 1U);
surface->height = std::max(desc.height, 1U);
surface->format = WW3D_FORMAT_A8R8G8B8;
surface->bytes.assign(static_cast<size_t>(surface->width) * static_cast<size_t>(surface->height) * 4U, 0);

const unsigned blocks_x = std::max((surface->width + 3U) / 4U, 1U);
const unsigned blocks_y = std::max((surface->height + 3U) / 4U, 1U);
uint8_t block[16] = {};
for (unsigned by = 0; by < blocks_y; ++by) {
for (unsigned bx = 0; bx < blocks_x; ++bx) {
if (file->Read(block, static_cast<int>(block_size)) != static_cast<int>(block_size)) {
file->Close();
return false;
}
switch (kind) {
case DDSKind::DXT1:
Decode_DXT1_Block(block, surface, bx * 4U, by * 4U);
break;
case DDSKind::DXT3:
Decode_DXT3_Block(block, surface, bx * 4U, by * 4U);
break;
case DDSKind::DXT5:
Decode_DXT5_Block(block, surface, bx * 4U, by * 4U);
break;
}
}
}

file->Close();
return true;
}

bool Load_TGA_Surface(const char *filename, BgfxCompatSurface *surface)
{
if (filename == NULL || surface == NULL) {
return false;
}

Targa targa;
if (TARGA_ERROR_HANDLER(targa.Open(filename, TGA_READMODE), filename) != 0) {
return false;
}

// Match the legacy loader: flip the Y-origin before loading so the image
// ends up in the expected orientation.
targa.Header.ImageDescriptor ^= TGAIDF_YORIGIN;

WW3DFormat src_format;
WW3DFormat dest_format;
unsigned src_bpp = 0;
Get_WW3D_Format(dest_format, src_format, src_bpp, targa);
if (src_format == WW3D_FORMAT_UNKNOWN) {
	return false;
}

unsigned src_width = std::max(static_cast<unsigned>(targa.Header.Width), 1U);
unsigned src_height = std::max(static_cast<unsigned>(targa.Header.Height), 1U);
unsigned width = src_width;
unsigned height = src_height;

uint8_t palette[256 * 4];
targa.SetPalette(palette);

if (TARGA_ERROR_HANDLER(targa.Load(filename, TGAF_IMAGE, false), filename) != 0) {
	return false;
}

uint8_t *src_surface = reinterpret_cast<uint8_t *>(targa.GetImage());
if (src_surface == NULL) {
	return false;
}

std::unique_ptr<uint8_t[]> converted_surface;
if (src_format == WW3D_FORMAT_A1R5G5B5 ||
	src_format == WW3D_FORMAT_R5G6B5 ||
	src_format == WW3D_FORMAT_A4R4G4B4 ||
	src_format == WW3D_FORMAT_P8 ||
	src_format == WW3D_FORMAT_L8 ||
	src_width != width ||
	src_height != height) {

	converted_surface.reset(new uint8_t[static_cast<size_t>(width) * static_cast<size_t>(height) * 4U]);
	dest_format = Get_Valid_Texture_Format(WW3D_FORMAT_A8R8G8B8, false);
	BitmapHandlerClass::Copy_Image(
		converted_surface.get(),
		width,
		height,
		width * 4U,
		WW3D_FORMAT_A8R8G8B8,
		src_surface,
		src_width,
		src_height,
		src_width * src_bpp,
		src_format,
		reinterpret_cast<uint8_t *>(targa.GetPalette()),
		targa.Header.CMapDepth >> 3,
		false);
	src_surface = converted_surface.get();
	src_format = WW3D_FORMAT_A8R8G8B8;
	src_width = width;
	src_height = height;
	src_bpp = Get_Bytes_Per_Pixel(src_format);
}

surface->width = width;
surface->height = height;
surface->format = dest_format;
surface->bytes.resize(
	static_cast<size_t>(width) *
	static_cast<size_t>(height) *
	static_cast<size_t>(BgfxCompat_Get_Pixel_Size(dest_format)),
	0);

BitmapHandlerClass::Copy_Image(
	surface->bytes.data(),
	width,
	height,
	width * BgfxCompat_Get_Pixel_Size(dest_format),
	dest_format,
	src_surface,
	src_width,
	src_height,
	src_width * src_bpp,
	src_format,
	reinterpret_cast<uint8_t *>(targa.GetPalette()),
	targa.Header.CMapDepth >> 3,
	false);

return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// File info extraction (header only, no pixel data)
//
////////////////////////////////////////////////////////////////////////////////

bool Get_DDS_File_Info(const char *filename, unsigned &width, unsigned &height,
WW3DFormat &format, unsigned &mip_count)
{
if (filename == NULL) return false;

char dds_name[256] = {};
std::snprintf(dds_name, sizeof(dds_name), "%s", filename);
const size_t len = std::strlen(dds_name);
if (len < 4) return false;
dds_name[len - 3] = 'd';
dds_name[len - 2] = 'd';
dds_name[len - 1] = 's';

file_auto_ptr file(_TheFileFactory, dds_name);
if (!file->Is_Available() || !file->Open(FileClass::READ)) return false;

PackedDDSurfaceDesc2 desc = {};
DDSKind kind;
unsigned block_size = 0;
if (!Parse_DDS_Header(file, desc, kind, block_size)) {
file->Close();
return false;
}
file->Close();

width = std::max(desc.width, 1U);
height = std::max(desc.height, 1U);
mip_count = std::max(desc.mipmap_count_or_refresh_rate, 1U);

switch (kind) {
case DDSKind::DXT1: format = WW3D_FORMAT_DXT1; break;
case DDSKind::DXT3: format = WW3D_FORMAT_DXT3; break;
case DDSKind::DXT5: format = WW3D_FORMAT_DXT5; break;
}

return true;
}

bool Get_Texture_Information(
const char *filename,
unsigned reduction,
unsigned &w,
unsigned &h,
WW3DFormat &format,
unsigned &mip_count,
bool compressed)
{
ThumbnailClass *thumb = NULL;
ThumbnailManagerClass *thumb_man = ThumbnailManagerClass::Peek_List().Head();
while (thumb_man) {
thumb = thumb_man->Peek_Thumbnail_Instance(filename);
if (thumb) break;
thumb_man = thumb_man->Succ();
}

if (!thumb) {
if (compressed) {
unsigned orig_w, orig_h;
if (!Get_DDS_File_Info(filename, orig_w, orig_h, format, mip_count)) return false;
w = std::max(orig_w >> reduction, 1U);
h = std::max(orig_h >> reduction, 1U);
if (mip_count > reduction) mip_count -= reduction;
else mip_count = 1;
return true;
}

Targa targa;
if (TARGA_ERROR_HANDLER(targa.Open(filename, TGA_READMODE), filename)) {
return false;
}

unsigned src_bpp;
WW3DFormat dest_format;
Get_WW3D_Format(dest_format, format, src_bpp, targa);

w = static_cast<unsigned>(targa.Header.Width >> reduction);
h = static_cast<unsigned>(targa.Header.Height >> reduction);
if (w < 1) w = 1;
if (h < 1) h = 1;


mip_count = 0;
return true;
}

if (compressed &&
thumb->Get_Original_Texture_Format() != WW3D_FORMAT_DXT1 &&
thumb->Get_Original_Texture_Format() != WW3D_FORMAT_DXT2 &&
thumb->Get_Original_Texture_Format() != WW3D_FORMAT_DXT3 &&
thumb->Get_Original_Texture_Format() != WW3D_FORMAT_DXT4 &&
thumb->Get_Original_Texture_Format() != WW3D_FORMAT_DXT5) {
return false;
}

w = std::max(thumb->Get_Original_Texture_Width() >> reduction, 1U);
h = std::max(thumb->Get_Original_Texture_Height() >> reduction, 1U);
mip_count = thumb->Get_Original_Texture_Mip_Level_Count();
format = thumb->Get_Original_Texture_Format();
return true;
}

} // anonymous namespace

#endif // RENEGADE_WITH_BGFX_RENDERER


////////////////////////////////////////////////////////////////////////////////
//
// TextureLoadTaskListClass implementation
//
////////////////////////////////////////////////////////////////////////////////

TextureLoadTaskListClass::TextureLoadTaskListClass(void)
: Root()
{
Root.Next = Root.Prev = &Root;
}

void TextureLoadTaskListClass::Push_Front(TextureLoadTaskClass *task)
{
WWASSERT(task != NULL && task->Next == NULL && task->Prev == NULL);
task->Next = Root.Next;
task->Prev = &Root;
task->List = this;
Root.Next->Prev = task;
Root.Next = task;
}

void TextureLoadTaskListClass::Push_Back(TextureLoadTaskClass *task)
{
WWASSERT(task != NULL && task->Next == NULL && task->Prev == NULL);
task->Next = &Root;
task->Prev = Root.Prev;
task->List = this;
Root.Prev->Next = task;
Root.Prev = task;
}

TextureLoadTaskClass *TextureLoadTaskListClass::Pop_Front(void)
{
if (Is_Empty()) {
return 0;
}
TextureLoadTaskClass *task = (TextureLoadTaskClass *)Root.Next;
Remove(task);
return task;
}

TextureLoadTaskClass *TextureLoadTaskListClass::Pop_Back(void)
{
if (Is_Empty()) {
return 0;
}
TextureLoadTaskClass *task = (TextureLoadTaskClass *)Root.Prev;
Remove(task);
return task;
}

void TextureLoadTaskListClass::Remove(TextureLoadTaskClass *task)
{
if (task->List != this) {
return;
}
task->Prev->Next = task->Next;
task->Next->Prev = task->Prev;
task->Prev = 0;
task->Next = 0;
task->List = 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// SynchronizedTextureLoadTaskListClass implementation
//
////////////////////////////////////////////////////////////////////////////////

SynchronizedTextureLoadTaskListClass::SynchronizedTextureLoadTaskListClass(void)
: TextureLoadTaskListClass(),
  CriticalSection()
{
}

void SynchronizedTextureLoadTaskListClass::Push_Front(TextureLoadTaskClass *task)
{
FastCriticalSectionClass::LockClass lock(CriticalSection);
TextureLoadTaskListClass::Push_Front(task);
}

void SynchronizedTextureLoadTaskListClass::Push_Back(TextureLoadTaskClass *task)
{
FastCriticalSectionClass::LockClass lock(CriticalSection);
TextureLoadTaskListClass::Push_Back(task);
}

TextureLoadTaskClass *SynchronizedTextureLoadTaskListClass::Pop_Front(void)
{
if (Is_Empty()) {
return 0;
}
FastCriticalSectionClass::LockClass lock(CriticalSection);
return TextureLoadTaskListClass::Pop_Front();
}

TextureLoadTaskClass *SynchronizedTextureLoadTaskListClass::Pop_Back(void)
{
if (Is_Empty()) {
return 0;
}
FastCriticalSectionClass::LockClass lock(CriticalSection);
return TextureLoadTaskListClass::Pop_Back();
}

void SynchronizedTextureLoadTaskListClass::Remove(TextureLoadTaskClass *task)
{
FastCriticalSectionClass::LockClass lock(CriticalSection);
TextureLoadTaskListClass::Remove(task);
}


////////////////////////////////////////////////////////////////////////////////
//
// Static data
//
////////////////////////////////////////////////////////////////////////////////

bool TextureLoader::TextureLoadSuspended = false;

static TextureLoadTaskListClass _FreeList;


////////////////////////////////////////////////////////////////////////////////
//
// TextureLoader implementation
//
////////////////////////////////////////////////////////////////////////////////

void TextureLoader::Init(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
ThumbnailManagerClass::Init();
#endif
}

void TextureLoader::Deinit(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
TextureLoadTaskClass::Delete_Free_Pool();
ThumbnailManagerClass::Deinit();
#endif
}

bool TextureLoader::Is_DX8_Thread(void)
{
return true;
}

void TextureLoader::Validate_Texture_Size(unsigned &width, unsigned &height)
{
if (width == 0) width = 1;
if (height == 0) height = 1;

unsigned pow2_w = 1;
while (pow2_w < width) pow2_w <<= 1;

unsigned pow2_h = 1;
while (pow2_h < height) pow2_h <<= 1;

const unsigned max_size = 4096;
if (pow2_w > max_size) pow2_w = max_size;
if (pow2_h > max_size) pow2_h = max_size;

if (pow2_w > pow2_h) {
while (pow2_w / pow2_h > 8) pow2_h *= 2;
} else {
while (pow2_h / pow2_w > 8) pow2_w *= 2;
}

width = pow2_w;
height = pow2_h;
}

IDirect3DTexture8 *TextureLoader::Load_Thumbnail(const StringClass &filename)
{
#if RENEGADE_WITH_BGFX_RENDERER
ThumbnailClass *thumb = NULL;
ThumbnailManagerClass *thumb_man = ThumbnailManagerClass::Peek_List().Head();
while (thumb_man) {
thumb = thumb_man->Peek_Thumbnail_Instance(filename);
if (thumb) break;
thumb_man = thumb_man->Succ();
}

if (!thumb) {
return NULL;
}

WWASSERT(thumb->Get_Format() == WW3D_FORMAT_A4R4G4B4);
unsigned width = thumb->Get_Width();
unsigned height = thumb->Get_Height();

BgfxCompatTexture *tex = Create_Bgfx_Texture(width, height, WW3D_FORMAT_A8R8G8B8);

BitmapHandlerClass::Copy_Image(
tex->bytes.data(),
width,
height,
width * 4,
WW3D_FORMAT_A8R8G8B8,
thumb->Peek_Bitmap(),
width,
height,
width * 2,
WW3D_FORMAT_A4R4G4B4,
NULL,
0,
false);

return reinterpret_cast<IDirect3DTexture8 *>(tex);
#else
return NULL;
#endif
}

IDirect3DSurface8 *TextureLoader::Load_Surface_Immediate(const StringClass &filename, WW3DFormat, bool)
{
#if RENEGADE_WITH_BGFX_RENDERER
BgfxCompatSurface *surface = Create_Surface(1U, 1U, WW3D_FORMAT_A8R8G8B8);
if (Load_DDS_Surface(filename, surface) || Load_TGA_Surface(filename, surface)) {
return reinterpret_cast<IDirect3DSurface8 *>(surface);
}
delete surface;
#endif
return NULL;
}

void TextureLoader::Request_Thumbnail(TextureClass *tc)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (tc->Peek_DX8_Texture()) {
return;
}

Load_Thumbnail(tc);
#endif
}

void TextureLoader::Request_Background_Loading(TextureClass *tc)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (tc->Is_Initialized()) {
return;
}

if (tc->TextureLoadTask) {
return;
}

TextureLoadTaskClass *task = TextureLoadTaskClass::Create(
tc, TextureLoadTaskClass::TASK_LOAD, TextureLoadTaskClass::PRIORITY_LOW);
task->Finish_Load();
task->Destroy();
#endif
}

void TextureLoader::Request_Foreground_Loading(TextureClass *tc)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (tc->Is_Initialized()) {
return;
}

TextureLoadTaskClass *task_thumb = tc->ThumbnailLoadTask;
TextureLoadTaskClass *task = tc->TextureLoadTask;

if (task_thumb) {
task_thumb->Destroy();
}

if (!task) {
task = TextureLoadTaskClass::Create(
tc, TextureLoadTaskClass::TASK_LOAD, TextureLoadTaskClass::PRIORITY_HIGH);
}

task->Finish_Load();
task->Destroy();
#endif
}

void TextureLoader::Flush_Pending_Load_Tasks(void)
{
// All loading is synchronous in bgfx; nothing to flush.
}

void TextureLoader::Update(void (*)(void))
{
#if RENEGADE_WITH_BGFX_RENDERER
if (TextureLoadSuspended) {
return;
}
TextureClass::Invalidate_Old_Unused_Textures(0);
#endif
}

void TextureLoader::Suspend_Texture_Load()
{
TextureLoadSuspended = true;
}

void TextureLoader::Continue_Texture_Load()
{
TextureLoadSuspended = false;
}

void TextureLoader::Process_Foreground_Load(TextureLoadTaskClass *task)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (task) {
task->Finish_Load();
task->Destroy();
}
#endif
}

void TextureLoader::Process_Foreground_Thumbnail(TextureLoadTaskClass *task)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (task) {
switch (task->Get_State()) {
case TextureLoadTaskClass::STATE_NONE:
Load_Thumbnail(task->Peek_Texture());
// fall through
case TextureLoadTaskClass::STATE_COMPLETE:
task->Destroy();
break;
default:
break;
}
}
#endif
}

void TextureLoader::Begin_Load_And_Queue(TextureLoadTaskClass *task)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (task) {
task->Finish_Load();
task->Destroy();
}
#endif
}

void TextureLoader::Load_Thumbnail(TextureClass *tc)
{
#if RENEGADE_WITH_BGFX_RENDERER
IDirect3DTexture8 *d3d_texture = Load_Thumbnail(tc->Get_Full_Path());
if (d3d_texture) {
tc->Apply_New_Surface(d3d_texture, false);
// Ownership transferred to TextureClass; no Release needed in bgfx.
}
#endif
}


////////////////////////////////////////////////////////////////////////////////
//
// TextureLoadTaskClass implementation
//
////////////////////////////////////////////////////////////////////////////////

TextureLoadTaskClass::TextureLoadTaskClass()
: Texture(0),
  D3DTexture(0),
  Format(WW3D_FORMAT_UNKNOWN),
  Width(0),
  Height(0),
  MipLevelCount(0),
  Reduction(0),
  Type(TASK_NONE),
  Priority(PRIORITY_LOW),
  State(STATE_NONE)
{
for (int i = 0; i < TextureClass::MIP_LEVELS_MAX; ++i) {
LockedSurfacePtr[i] = NULL;
LockedSurfacePitch[i] = 0;
}
}

TextureLoadTaskClass::~TextureLoadTaskClass(void)
{
Deinit();
}

TextureLoadTaskClass *TextureLoadTaskClass::Create(TextureClass *tc, TaskType type, PriorityType priority)
{
TextureLoadTaskClass *task = _FreeList.Pop_Front();
if (!task) {
task = new TextureLoadTaskClass;
}
task->Init(tc, type, priority);
return task;
}

void TextureLoadTaskClass::Destroy(void)
{
Deinit();
_FreeList.Push_Front(this);
}

void TextureLoadTaskClass::Delete_Free_Pool(void)
{
while (TextureLoadTaskClass *task = _FreeList.Pop_Front()) {
delete task;
}
}

void TextureLoadTaskClass::Init(TextureClass *tc, TaskType type, PriorityType priority)
{
WWASSERT(tc);
REF_PTR_SET(Texture, tc);

WWASSERT(Texture->Get_Full_Path() != "");

Type = type;
Priority = priority;
State = STATE_NONE;
D3DTexture = 0;

Format = Texture->Get_Texture_Format();
Width = 0;
Height = 0;
MipLevelCount = Texture->MipLevelCount;
Reduction = Texture->Get_Reduction();

for (int i = 0; i < TextureClass::MIP_LEVELS_MAX; ++i) {
LockedSurfacePtr[i] = NULL;
LockedSurfacePitch[i] = 0;
}

switch (Type) {
case TASK_THUMBNAIL:
WWASSERT(Texture->ThumbnailLoadTask == NULL);
Texture->ThumbnailLoadTask = this;
break;
case TASK_LOAD:
WWASSERT(Texture->TextureLoadTask == NULL);
Texture->TextureLoadTask = this;
break;
default:
break;
}
}

void TextureLoadTaskClass::Deinit(void)
{
WWASSERT(Next == NULL);
WWASSERT(Prev == NULL);
WWASSERT(D3DTexture == NULL);

for (int i = 0; i < TextureClass::MIP_LEVELS_MAX; ++i) {
WWASSERT(LockedSurfacePtr[i] == NULL);
}

if (Texture) {
switch (Type) {
case TASK_THUMBNAIL:
WWASSERT(Texture->ThumbnailLoadTask == this);
Texture->ThumbnailLoadTask = NULL;
break;
case TASK_LOAD:
WWASSERT(Texture->TextureLoadTask == this);
Texture->TextureLoadTask = NULL;
break;
default:
break;
}

REF_PTR_RELEASE(Texture);
}
}

bool TextureLoadTaskClass::Begin_Load(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
bool loaded = false;

if (Texture->Is_Compression_Allowed()) {
loaded = Begin_Compressed_Load();
}

if (!loaded) {
loaded = Begin_Uncompressed_Load();
}

if (!loaded) {
return false;
}

Lock_Surfaces();
State = STATE_LOAD_BEGUN;
return true;
#else
return false;
#endif
}

bool TextureLoadTaskClass::Load(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
WWASSERT(Peek_D3D_Texture());

bool loaded = false;

if (Texture->Is_Compression_Allowed()) {
loaded = Load_Compressed_Mipmap();
}

if (!loaded) {
loaded = Load_Uncompressed_Mipmap();
}

State = STATE_LOAD_MIPMAP;
return loaded;
#else
return false;
#endif
}

void TextureLoadTaskClass::End_Load(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
Unlock_Surfaces();
Apply(true);
State = STATE_LOAD_COMPLETE;
#endif
}

void TextureLoadTaskClass::Finish_Load(void)
{
switch (State) {
// Fall-through below is intentional.
case STATE_NONE:
if (!Begin_Load()) {
Apply_Missing_Texture();
break;
}
// fall through

case STATE_LOAD_BEGUN:
Load();
// fall through

case STATE_LOAD_MIPMAP:
End_Load();
// fall through

default:
break;
}
}

void TextureLoadTaskClass::Apply_Missing_Texture(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
WWASSERT(!D3DTexture);
D3DTexture = Create_Missing_Bgfx_Texture();
Apply(true);
#endif
}

void TextureLoadTaskClass::Apply(bool initialize)
{
#if RENEGADE_WITH_BGFX_RENDERER
WWASSERT(D3DTexture);

for (unsigned i = 0; i < TextureClass::MIP_LEVELS_MAX; ++i) {
WWASSERT(LockedSurfacePtr[i] == NULL);
}

Texture->Apply_New_Surface(D3DTexture, initialize);

// Apply_New_Surface takes ownership of the texture.
// Do not free; just clear our reference.
D3DTexture = NULL;
#endif
}


bool TextureLoadTaskClass::Begin_Compressed_Load(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
unsigned orig_w, orig_h, orig_mip_count;
WW3DFormat orig_format;
if (!Get_Texture_Information(
Texture->Get_Full_Path(), Get_Reduction(),
orig_w, orig_h, orig_format, orig_mip_count, true)) {
return false;
}

unsigned width = orig_w;
unsigned height = orig_h;
TextureLoader::Validate_Texture_Size(width, height);

// If validation changed dimensions, try reducing to find a valid size
if (width != orig_w || height != orig_h) {
for (unsigned i = 1; i < orig_mip_count; ++i) {
unsigned w = orig_w >> i;
if (w < 4) w = 4;
unsigned h = orig_h >> i;
if (h < 4) h = 4;
unsigned tmp_w = w;
unsigned tmp_h = h;
TextureLoader::Validate_Texture_Size(w, h);
if (w == tmp_w && h == tmp_h) {
Reduction += i;
width = w;
height = h;
break;
}
}
}

Width = width;
Height = height;
// Decompress DXT to A8R8G8B8 since bgfx doesn't store compressed blocks
Format = WW3D_FORMAT_A8R8G8B8;
MipLevelCount = 1;

BgfxCompatTexture *tex = Create_Bgfx_Texture(Width, Height, Format);
D3DTexture = reinterpret_cast<IDirect3DTexture8 *>(tex);

return true;
#else
return false;
#endif
}

bool TextureLoadTaskClass::Begin_Uncompressed_Load(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
unsigned width, height, orig_mip_count;
WW3DFormat orig_format;
if (!Get_Texture_Information(
Texture->Get_Full_Path(), Get_Reduction(),
width, height, orig_format, orig_mip_count, false)) {
return false;
}

unsigned ow = width;
unsigned oh = height;
TextureLoader::Validate_Texture_Size(width, height);

Width = width;
Height = height;

// Determine destination format; always use uncompressed
WW3DFormat dest_format = WW3D_FORMAT_A8R8G8B8;
if (Format != WW3D_FORMAT_UNKNOWN) {
// If a specific uncompressed format was requested, use it
switch (Format) {
case WW3D_FORMAT_DXT1:
case WW3D_FORMAT_DXT2:
case WW3D_FORMAT_DXT3:
case WW3D_FORMAT_DXT4:
case WW3D_FORMAT_DXT5:
case WW3D_FORMAT_UNKNOWN:
dest_format = WW3D_FORMAT_A8R8G8B8;
break;
default:
dest_format = Format;
break;
}
}

Format = dest_format;
MipLevelCount = 1;

BgfxCompatTexture *tex = Create_Bgfx_Texture(Width, Height, Format);
D3DTexture = reinterpret_cast<IDirect3DTexture8 *>(tex);

return true;
#else
return false;
#endif
}

bool TextureLoadTaskClass::Load_Compressed_Mipmap(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
// Load DDS level 0 to a temporary surface, then copy to locked texture buffer
BgfxCompatSurface temp_surface;
if (!Load_DDS_Surface(Texture->Get_Full_Path(), &temp_surface)) {
return false;
}

unsigned width = Get_Width();
unsigned height = Get_Height();

BitmapHandlerClass::Copy_Image(
Get_Locked_Surface_Ptr(0),
width,
height,
Get_Locked_Surface_Pitch(0),
WW3D_FORMAT_A8R8G8B8,
temp_surface.bytes.data(),
temp_surface.width,
temp_surface.height,
temp_surface.width * 4,
WW3D_FORMAT_A8R8G8B8,
NULL,
0,
false);

return true;
#else
return false;
#endif
}

bool TextureLoadTaskClass::Load_Uncompressed_Mipmap(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
if (!Get_Mip_Level_Count()) {
return false;
}

Targa targa;
if (TARGA_ERROR_HANDLER(targa.Open(Texture->Get_Full_Path(), TGA_READMODE), Texture->Get_Full_Path())) {
return false;
}

// Flip Y-origin to match expected orientation
targa.Header.ImageDescriptor ^= TGAIDF_YORIGIN;

WW3DFormat src_format;
WW3DFormat dest_format;
unsigned src_bpp = 0;
Get_WW3D_Format(dest_format, src_format, src_bpp, targa);
if (src_format == WW3D_FORMAT_UNKNOWN) return false;

dest_format = Get_Format();

uint8_t palette[256 * 4];
targa.SetPalette(palette);

unsigned src_width = targa.Header.Width;
unsigned src_height = targa.Header.Height;
unsigned width = Get_Width();
unsigned height = Get_Height();

if (TARGA_ERROR_HANDLER(targa.Load(Texture->Get_Full_Path(), TGAF_IMAGE, false), Texture->Get_Full_Path())) {
return false;
}

uint8_t *src_surface = (uint8_t *)targa.GetImage();
uint8_t *converted_surface = NULL;

// Convert paletted/16-bit formats to A8R8G8B8, or if scaling is needed
if (src_format == WW3D_FORMAT_A1R5G5B5 ||
src_format == WW3D_FORMAT_R5G6B5 ||
src_format == WW3D_FORMAT_A4R4G4B4 ||
src_format == WW3D_FORMAT_P8 ||
src_format == WW3D_FORMAT_L8 ||
src_width != width ||
src_height != height) {

converted_surface = new uint8_t[width * height * 4];
BitmapHandlerClass::Copy_Image(
converted_surface,
width,
height,
width * 4,
WW3D_FORMAT_A8R8G8B8,
src_surface,
src_width,
src_height,
src_width * src_bpp,
src_format,
(uint8_t *)targa.GetPalette(),
targa.Header.CMapDepth >> 3,
false);

src_surface = converted_surface;
src_format = WW3D_FORMAT_A8R8G8B8;
src_width = width;
src_height = height;
src_bpp = 4;
}

unsigned src_pitch = src_width * src_bpp;

// Copy to level 0
WWASSERT(Get_Locked_Surface_Ptr(0));
BitmapHandlerClass::Copy_Image(
Get_Locked_Surface_Ptr(0),
width,
height,
Get_Locked_Surface_Pitch(0),
Get_Format(),
src_surface,
src_width,
src_height,
src_pitch,
src_format,
NULL,
0,
false);

if (converted_surface) {
delete[] converted_surface;
}

return true;
#else
return false;
#endif
}

void TextureLoadTaskClass::Lock_Surfaces(void)
{
#if RENEGADE_WITH_BGFX_RENDERER
BgfxCompatTexture *tex = BgfxCompat_To_Texture(D3DTexture);
WWASSERT(tex);
MipLevelCount = 1;
LockedSurfacePtr[0] = tex->bytes.data();
LockedSurfacePitch[0] = static_cast<unsigned>(tex->width) * BgfxCompat_Get_Pixel_Size(tex->format);
#endif
}

void TextureLoadTaskClass::Unlock_Surfaces(void)
{
for (uint32_t i = 0; i < TextureClass::MIP_LEVELS_MAX; ++i) {
LockedSurfacePtr[i] = NULL;
LockedSurfacePitch[i] = 0;
}
}

uint8_t *TextureLoadTaskClass::Get_Locked_Surface_Ptr(uint32_t level)
{
WWASSERT(level < MipLevelCount);
WWASSERT(LockedSurfacePtr[level]);
return LockedSurfacePtr[level];
}

uint32_t TextureLoadTaskClass::Get_Locked_Surface_Pitch(uint32_t level) const
{
WWASSERT(level < MipLevelCount);
WWASSERT(LockedSurfacePtr[level]);
return LockedSurfacePitch[level];
}

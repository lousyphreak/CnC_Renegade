#include "textureloader.h"

#include "ffactory.h"
#include "ww3d.h"

#if RENEGADE_WITH_BGFX_RENDERER

#include "bgfx_compat_resources.h"
#include "TARGA.H"
#include "wwdebug.h"

#include <algorithm>
#include <cstring>

namespace {

constexpr uint32_t Make_FourCC(char a, char b, char c, char d)
{
	return static_cast<uint32_t>(static_cast<unsigned char>(a)) |
		(static_cast<uint32_t>(static_cast<unsigned char>(b)) << 8) |
		(static_cast<uint32_t>(static_cast<unsigned char>(c)) << 16) |
		(static_cast<uint32_t>(static_cast<unsigned char>(d)) << 24);
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

BgfxCompatSurface *Create_Surface(unsigned width, unsigned height, WW3DFormat format)
{
	BgfxCompatSurface *surface = new BgfxCompatSurface();
	surface->width = std::max(width, 1U);
	surface->height = std::max(height, 1U);
	surface->format = format;
	surface->bytes.resize(static_cast<size_t>(surface->width) * static_cast<size_t>(surface->height) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(format)), 0);
	return surface;
}

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

	char magic[4] = {};
	if (file->Read(magic, sizeof(magic)) != static_cast<int>(sizeof(magic)) || std::memcmp(magic, "DDS ", sizeof(magic)) != 0) {
		file->Close();
		return false;
	}

	PackedDDSurfaceDesc2 desc = {};
	if (file->Read(&desc, sizeof(desc)) != static_cast<int>(sizeof(desc)) || desc.size != sizeof(desc)) {
		file->Close();
		return false;
	}

	const uint32_t fourcc = desc.pixel_format.fourcc;
	unsigned block_size = 0;
	enum class DDSKind { DXT1, DXT3, DXT5 } kind;
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
	if (TARGA_ERROR_HANDLER(targa.Load(filename, TGAF_IMAGE, false), filename) != 0) {
		return false;
	}

	surface->width = std::max(static_cast<unsigned>(targa.Header.Width), 1U);
	surface->height = std::max(static_cast<unsigned>(targa.Header.Height), 1U);
	switch (targa.Header.PixelDepth) {
		case 8:
			surface->format = WW3D_FORMAT_A8;
			break;
		case 24:
			surface->format = WW3D_FORMAT_R8G8B8;
			break;
		case 32:
		default:
			surface->format = WW3D_FORMAT_A8R8G8B8;
			break;
	}

	const size_t byte_count = static_cast<size_t>(surface->width) * static_cast<size_t>(surface->height) * static_cast<size_t>(BgfxCompat_Get_Pixel_Size(surface->format));
	surface->bytes.resize(byte_count, 0);
	if (targa.GetImage() != NULL) {
		std::memcpy(surface->bytes.data(), targa.GetImage(), byte_count);
	}
	return true;
}

} // namespace

#endif

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

IDirect3DSurface8 * TextureLoader::Load_Surface_Immediate(const StringClass &filename, WW3DFormat, bool)
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
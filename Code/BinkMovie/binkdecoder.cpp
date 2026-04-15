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
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**	General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "binkdecoder.h"
#include "ffactory.h"
#include "wwfile.h"

#include <SDL3/SDL.h>

#ifndef restrict
#define restrict __restrict
#endif

extern "C" {
#include "../../external/ffmpeg-8.1/libavcodec/binkdata.h"
#include "../../external/ffmpeg-8.1/libavcodec/binkdsp.h"
#include "../../external/ffmpeg-8.1/libavcodec/wma_freqs.h"
#include "../../external/ffmpeg-8.1/libavcodec/binkdsp.c"
#include "../../external/ffmpeg-8.1/libavcodec/wma_freqs.c"
}

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr uint32_t BINK_FLAG_ALPHA = 0x00100000;
constexpr uint32_t BINK_FLAG_GRAY = 0x00020000;

constexpr uint16_t BINK_AUD_STEREO = 0x2000;
constexpr uint16_t BINK_AUD_USEDCT = 0x1000;

constexpr int MAX_DCT_CHANNELS = 6;
constexpr int MAX_PACKET_CHANNELS = 2;
constexpr double kPi = 3.14159265358979323846;

template <typename T>
constexpr T FFMIN(T lhs, T rhs)
{
	return lhs < rhs ? lhs : rhs;
}

template <typename T>
constexpr T FFMAX(T lhs, T rhs)
{
	return lhs > rhs ? lhs : rhs;
}

template <typename T>
constexpr T FFALIGN(T value, T align)
{
	return (value + align - 1) & ~(align - 1);
}

uint16_t Read_LE16(const uint8_t *data)
{
	return static_cast<uint16_t>(data[0]) |
		(static_cast<uint16_t>(data[1]) << 8);
}

uint32_t Read_LE32(const uint8_t *data)
{
	return static_cast<uint32_t>(data[0]) |
		(static_cast<uint32_t>(data[1]) << 8) |
		(static_cast<uint32_t>(data[2]) << 16) |
		(static_cast<uint32_t>(data[3]) << 24);
}

uint32_t Make_Tag(char a, char b, char c, char d)
{
	return static_cast<uint32_t>(static_cast<uint8_t>(a)) |
		(static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
		(static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
		(static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
}

void Report_Error(const char *format, ...)
{
	std::va_list args;
	va_start(args, format);
	std::fputs("BinkMovie: ", stderr);
	std::vfprintf(stderr, format, args);
	std::fputc('\n', stderr);
	va_end(args);
}

float Int_As_Float(uint32_t value)
{
	float result = 0.0f;
	std::memcpy(&result, &value, sizeof(result));
	return result;
}

uint8_t Clamp_Byte(int value)
{
	return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

uint16_t Pack_RGB565(uint8_t red, uint8_t green, uint8_t blue)
{
	return static_cast<uint16_t>(((red >> 3) << 11) |
		((green >> 2) << 5) |
		(blue >> 3));
}

int Floor_Log2(uint32_t value)
{
	int result = -1;
	while (value != 0U) {
		value >>= 1U;
		++result;
	}
	return result < 0 ? 0 : result;
}

struct Complex
{
	float re = 0.0f;
	float im = 0.0f;
};

class BitReader
{
public:
	BitReader() = default;

	BitReader(const uint8_t *data, size_t size, bool little_endian = false)
		: Data(data), SizeBits(size * 8U), BitPosition(0U), LittleEndian(little_endian)
	{
	}

	void Reset(const uint8_t *data, size_t size, bool little_endian = false)
	{
		Data = data;
		SizeBits = size * 8U;
		BitPosition = 0U;
		LittleEndian = little_endian;
	}

	size_t Get_Bits_Left() const
	{
		return (BitPosition <= SizeBits) ? (SizeBits - BitPosition) : 0U;
	}

	bool Read_Bit(uint32_t &value)
	{
		return Read_Bits(1, value);
	}

	bool Read_Bits(int bits, uint32_t &value)
	{
		if (bits < 0 || bits > 32 || Get_Bits_Left() < static_cast<size_t>(bits)) {
			value = 0;
			return false;
		}

		if (bits == 0) {
			value = 0;
			return true;
		}

		uint32_t result = 0U;
		for (int index = 0; index < bits; ++index) {
			const size_t byte_index = BitPosition >> 3;
			if (LittleEndian) {
				const int bit_index = static_cast<int>(BitPosition & 7U);
				result |= ((Data[byte_index] >> bit_index) & 1U) << index;
			} else {
				const int bit_index = 7 - static_cast<int>(BitPosition & 7U);
				result = (result << 1U) | ((Data[byte_index] >> bit_index) & 1U);
			}
			++BitPosition;
		}
		value = result;
		return true;
	}

	uint32_t Peek_Bits(int bits) const
	{
		if (bits <= 0) {
			return 0;
		}

		uint32_t result = 0U;
		size_t position = BitPosition;
		for (int index = 0; index < bits; ++index) {
			if (position >= SizeBits) {
				if (!LittleEndian) {
					result <<= static_cast<uint32_t>(bits - index);
				}
				break;
			}
			const size_t byte_index = position >> 3;
			if (LittleEndian) {
				const int bit_index = static_cast<int>(position & 7U);
				result |= ((Data[byte_index] >> bit_index) & 1U) << index;
			} else {
				const int bit_index = 7 - static_cast<int>(position & 7U);
				result = (result << 1U) | ((Data[byte_index] >> bit_index) & 1U);
			}
			++position;
		}
		return result;
	}

	bool Skip_Bits(size_t bits)
	{
		if (Get_Bits_Left() < bits) {
			return false;
		}

		BitPosition += bits;
		return true;
	}

	void Align_To_32()
	{
		const size_t remainder = BitPosition & 31U;
		if (remainder != 0U) {
			BitPosition += 32U - remainder;
			if (BitPosition > SizeBits) {
				BitPosition = SizeBits;
			}
		}
	}

private:
	const uint8_t *Data = nullptr;
	size_t SizeBits = 0U;
	size_t BitPosition = 0U;
	bool LittleEndian = false;
};

enum OldSources
{
	BINKB_SRC_BLOCK_TYPES = 0,
	BINKB_SRC_COLORS,
	BINKB_SRC_PATTERN,
	BINKB_SRC_X_OFF,
	BINKB_SRC_Y_OFF,
	BINKB_SRC_INTRA_DC,
	BINKB_SRC_INTER_DC,
	BINKB_SRC_INTRA_Q,
	BINKB_SRC_INTER_Q,
	BINKB_SRC_INTER_COEFS,
	BINKB_NB_SRC,
};

enum Sources
{
	BINK_SRC_BLOCK_TYPES = 0,
	BINK_SRC_SUB_BLOCK_TYPES,
	BINK_SRC_COLORS,
	BINK_SRC_PATTERN,
	BINK_SRC_X_OFF,
	BINK_SRC_Y_OFF,
	BINK_SRC_INTRA_DC,
	BINK_SRC_INTER_DC,
	BINK_SRC_RUN,
	BINK_NB_SRC,
};

enum BlockTypes
{
	SKIP_BLOCK = 0,
	SCALED_BLOCK,
	MOTION_BLOCK,
	RUN_BLOCK,
	RESIDUE_BLOCK,
	INTRA_BLOCK,
	FILL_BLOCK,
	INTER_BLOCK,
	PATTERN_BLOCK,
	RAW_BLOCK,
};

struct Tree
{
	int vlc_num = 0;
	std::array<uint8_t, 16> syms{};
};

struct Bundle
{
	int len = 0;
	Tree tree{};
	uint8_t *data = nullptr;
	uint8_t *data_end = nullptr;
	uint8_t *cur_dec = nullptr;
	uint8_t *cur_ptr = nullptr;
};

struct HuffEntry
{
	uint8_t symbol = 0;
	uint8_t bits = 0;
};

std::array<std::array<HuffEntry, 128>, 16> g_static_huff_tables{};
std::once_flag g_static_huff_once;
std::array<int, 256> g_limited_luma_table{};
std::once_flag g_limited_luma_once;

void Initialize_Limited_Luma_Table()
{
	for (int value = 0; value < 256; ++value) {
		g_limited_luma_table[value] = 298 * std::max(0, value - 16);
	}
}

void Build_Static_Huff_Tables()
{
	for (size_t tree_index = 0; tree_index < g_static_huff_tables.size(); ++tree_index) {
		auto &table = g_static_huff_tables[tree_index];
		table.fill(HuffEntry{});

		for (size_t symbol = 0; symbol < 16; ++symbol) {
			const uint8_t bit_count = bink_tree_lens[tree_index][symbol];
			const uint8_t code = bink_tree_bits[tree_index][symbol];
			const int fill_count = 1 << (7 - bit_count);
			for (int fill = 0; fill < fill_count; ++fill) {
				const int index = code | (fill << bit_count);
				table[index].symbol = static_cast<uint8_t>(symbol);
				table[index].bits = bit_count;
			}
		}
	}
}

bool Decode_Static_Huff(BitReader &bit_reader, int tree_num, uint8_t &symbol)
{
	std::call_once(g_static_huff_once, Build_Static_Huff_Tables);
	if (tree_num < 0 || tree_num >= static_cast<int>(g_static_huff_tables.size()) || bit_reader.Get_Bits_Left() == 0U) {
		return false;
	}

	const uint32_t key = bit_reader.Peek_Bits(7);
	const HuffEntry entry = g_static_huff_tables[tree_num][key & 0x7F];
	if (entry.bits == 0 || bit_reader.Get_Bits_Left() < entry.bits) {
		return false;
	}

	if (!bit_reader.Skip_Bits(entry.bits)) {
		return false;
	}

	symbol = entry.symbol;
	return true;
}

bool Get_Huff(BitReader &bit_reader, const Tree &tree, uint8_t &value)
{
	uint8_t symbol = 0;
	if (!Decode_Static_Huff(bit_reader, tree.vlc_num, symbol)) {
		return false;
	}

	value = tree.syms[symbol];
	return true;
}

void Merge_Tree_Symbols(BitReader &bit_reader, uint8_t *dst, uint8_t *src, int size)
{
	uint8_t *src2 = src + size;
	int size2 = size;

	while (size > 0 && size2 > 0) {
		uint32_t take_second = 0;
		bit_reader.Read_Bit(take_second);
		if (take_second == 0U) {
			*dst++ = *src++;
			--size;
		} else {
			*dst++ = *src2++;
			--size2;
		}
	}

	while (size-- > 0) {
		*dst++ = *src++;
	}

	while (size2-- > 0) {
		*dst++ = *src2++;
	}
}

bool Read_Tree(BitReader &bit_reader, Tree &tree)
{
	uint32_t tree_index = 0;
	if (!bit_reader.Read_Bits(4, tree_index)) {
		return false;
	}

	tree.vlc_num = static_cast<int>(tree_index);
	if (tree.vlc_num == 0) {
		for (int index = 0; index < 16; ++index) {
			tree.syms[index] = static_cast<uint8_t>(index);
		}
		return true;
	}

	uint32_t explicit_symbols = 0;
	if (!bit_reader.Read_Bit(explicit_symbols)) {
		return false;
	}

	if (explicit_symbols != 0U) {
		uint32_t len = 0;
		if (!bit_reader.Read_Bits(3, len)) {
			return false;
		}

		std::bitset<16> seen;
		for (uint32_t index = 0; index <= len; ++index) {
			uint32_t symbol = 0;
			if (!bit_reader.Read_Bits(4, symbol)) {
				return false;
			}

			tree.syms[index] = static_cast<uint8_t>(symbol);
			seen.set(symbol);
		}

		for (uint32_t index = 0; index < 16 && len < 15U; ++index) {
			if (!seen.test(index)) {
				tree.syms[++len] = static_cast<uint8_t>(index);
			}
		}
		return true;
	}

	uint32_t depth = 0;
	if (!bit_reader.Read_Bits(2, depth)) {
		return false;
	}

	std::array<uint8_t, 16> tmp1{};
	std::array<uint8_t, 16> tmp2{};
	for (int index = 0; index < 16; ++index) {
		tmp1[index] = static_cast<uint8_t>(index);
	}

	uint8_t *in = tmp1.data();
	uint8_t *out = tmp2.data();
	for (uint32_t level = 0; level <= depth; ++level) {
		const int size = 1 << level;
		for (int start = 0; start < 16; start += size * 2) {
			Merge_Tree_Symbols(bit_reader, out + start, in + start, size);
		}
		std::swap(in, out);
	}

	std::memcpy(tree.syms.data(), in, tree.syms.size());
	return true;
}

struct PlaneBuffer
{
	int width = 0;
	int height = 0;
	int stride = 0;
	std::vector<uint8_t> pixels;

	void Allocate(int plane_width, int plane_height)
	{
		width = plane_width;
		height = plane_height;
		stride = FFALIGN(plane_width, 16);
		const int padded_height = FFALIGN(plane_height, 16);
		pixels.assign(static_cast<size_t>(stride) * static_cast<size_t>(padded_height), 0U);
	}

	void Clear()
	{
		std::fill(pixels.begin(), pixels.end(), 0U);
	}
};

struct VideoFrame
{
	std::array<PlaneBuffer, 4> planes;
};

std::array<std::array<int32_t, 64>, 16> g_binkb_intra_quant{};
std::array<std::array<int32_t, 64>, 16> g_binkb_inter_quant{};
std::once_flag g_binkb_quant_once;

void Calculate_Binkb_Quant()
{
	std::array<uint8_t, 64> inverse_scan{};
	static const int scale_table[64] = {
		1073741824,1489322693,1402911301,1262586814,1073741824, 843633538, 581104888, 296244703,
		1489322693,2065749918,1945893874,1751258219,1489322693,1170153332, 806015634, 410903207,
		1402911301,1945893874,1832991949,1649649171,1402911301,1102260336, 759250125, 387062357,
		1262586814,1751258219,1649649171,1484645031,1262586814, 992008094, 683307060, 348346918,
		1073741824,1489322693,1402911301,1262586814,1073741824, 843633538, 581104888, 296244703,
		 843633538,1170153332,1102260336, 992008094, 843633538, 662838617, 456571181, 232757969,
		 581104888, 806015634, 759250125, 683307060, 581104888, 456571181, 314491699, 160326478,
		 296244703, 410903207, 387062357, 348346918, 296244703, 232757969, 160326478,  81733730,
	};

	for (int index = 0; index < 64; ++index) {
		inverse_scan[bink_scan[index]] = static_cast<uint8_t>(index);
	}

	const int64_t constant = (1LL << 30);
	for (int q = 0; q < 16; ++q) {
		for (int index = 0; index < 64; ++index) {
			const int scan_index = inverse_scan[index];
			g_binkb_intra_quant[q][scan_index] =
				static_cast<int32_t>((static_cast<int64_t>(binkb_intra_seed[index]) * scale_table[index] * binkb_num[q]) /
				(static_cast<int64_t>(binkb_den[q]) * (constant >> 12)));
			g_binkb_inter_quant[q][scan_index] =
				static_cast<int32_t>((static_cast<int64_t>(binkb_inter_seed[index]) * scale_table[index] * binkb_num[q]) /
				(static_cast<int64_t>(binkb_den[q]) * (constant >> 12)));
		}
	}
}

class VideoDecoder
{
public:
	bool Initialize(uint32_t width, uint32_t height, uint32_t codec_tag, uint32_t flags)
	{
		Version = static_cast<int>(codec_tag >> 24);
		Width = static_cast<int>(width);
		Height = static_cast<int>(height);
		Flags = flags;
		HasAlpha = (flags & BINK_FLAG_ALPHA) != 0U;
		SwapPlanes = Version >= 'h';
		FullRange = Version == 'k';
		FrameNumber = 0U;

		if (Width <= 0 || Height <= 0) {
			Report_Error("invalid Bink video size %ux%u", width, height);
			return false;
		}

		std::call_once(g_binkb_quant_once, Calculate_Binkb_Quant);
		ff_binkdsp_init(&DSP);
		if (!Allocate_Frame(CurrentFrame) || !Allocate_Frame(LastFrame)) {
			return false;
		}

		Initialize_Bundles();
		return true;
	}

	bool Decode(const uint8_t *packet_data, size_t packet_size)
	{
		BitReader bit_reader(packet_data, packet_size, true);
		Clear_Frame(CurrentFrame);

		if (HasAlpha) {
			if (Version >= 'i' && !bit_reader.Skip_Bits(32U)) {
				return false;
			}
			if (!Decode_Plane(bit_reader, 3, false)) {
				return false;
			}
		}

		if (Version >= 'i' && !bit_reader.Skip_Bits(32U)) {
			return false;
		}

		++FrameNumber;
		for (int plane = 0; plane < 3; ++plane) {
			const int plane_index = (!plane || !SwapPlanes) ? plane : (plane ^ 3);
			const bool is_chroma = plane != 0;
			if (Version > 'b') {
				if (!Decode_Plane(bit_reader, plane_index, is_chroma)) {
					return false;
				}
			} else {
				if (!Decode_Plane_B(bit_reader, plane_index, FrameNumber == 1U, is_chroma)) {
					return false;
				}
			}
		}

		LastFrame = CurrentFrame;
		return true;
	}

	void Copy_To_RGB565(void *dest, int32_t dest_pitch, uint32_t dest_height, uint32_t dest_x, uint32_t dest_y) const
	{
		if (dest == nullptr || dest_pitch <= 0) {
			return;
		}

		std::call_once(g_limited_luma_once, Initialize_Limited_Luma_Table);

		const PlaneBuffer &luma = CurrentFrame.planes[0];
		const PlaneBuffer &u_plane = CurrentFrame.planes[1];
		const PlaneBuffer &v_plane = CurrentFrame.planes[2];

		auto *dest_bytes = static_cast<uint8_t *>(dest);
		const int max_copy_height = std::min<int>(luma.height, static_cast<int>(dest_height) - static_cast<int>(dest_y));
		const int max_copy_width = luma.width;
		for (int y = 0; y < max_copy_height; y += 2) {
			auto *row0 = reinterpret_cast<uint16_t *>(dest_bytes + static_cast<size_t>(y + static_cast<int>(dest_y)) * static_cast<size_t>(dest_pitch));
			auto *row1 = (y + 1 < max_copy_height) ?
				reinterpret_cast<uint16_t *>(dest_bytes + static_cast<size_t>(y + 1 + static_cast<int>(dest_y)) * static_cast<size_t>(dest_pitch)) :
				nullptr;

			const uint8_t *luma_row0 = luma.pixels.data() + static_cast<size_t>(y) * static_cast<size_t>(luma.stride);
			const uint8_t *luma_row1 = (y + 1 < max_copy_height) ?
				(luma.pixels.data() + static_cast<size_t>(y + 1) * static_cast<size_t>(luma.stride)) :
				luma_row0;
			const int chroma_y = y >> 1;
			const uint8_t *u_row = u_plane.pixels.data() + static_cast<size_t>(chroma_y) * static_cast<size_t>(u_plane.stride);
			const uint8_t *v_row = v_plane.pixels.data() + static_cast<size_t>(chroma_y) * static_cast<size_t>(v_plane.stride);

			for (int chroma_x = 0; chroma_x < u_plane.width; ++chroma_x) {
				const int u_value = u_row[chroma_x];
				const int v_value = v_row[chroma_x];
				const int pixel_x = chroma_x << 1;
				if (pixel_x >= max_copy_width) {
					break;
				}

				int red_offset = 0;
				int green_offset = 0;
				int blue_offset = 0;
				int green_u = 0;
				int green_v = 0;
				if (FullRange) {
					const int d = u_value - 128;
					const int e = v_value - 128;
					red_offset = (359 * e + 128) >> 8;
					green_u = 88 * d;
					green_v = 183 * e + 128;
					blue_offset = (454 * d + 128) >> 8;
				} else {
					const int d = u_value - 128;
					const int e = v_value - 128;
					red_offset = 409 * e + 128;
					green_offset = -100 * d - 208 * e + 128;
					blue_offset = 516 * d + 128;
				}

				auto convert_pixel = [&](uint8_t y_value) -> uint16_t {
					int red = 0;
					int green = 0;
					int blue = 0;
					if (FullRange) {
						red = static_cast<int>(y_value) + red_offset;
						green = static_cast<int>(y_value) - ((green_u + green_v) >> 8);
						blue = static_cast<int>(y_value) + blue_offset;
					} else {
						const int luma_value = g_limited_luma_table[y_value];
						red = (luma_value + red_offset) >> 8;
						green = (luma_value + green_offset) >> 8;
						blue = (luma_value + blue_offset) >> 8;
					}
					return Pack_RGB565(Clamp_Byte(red), Clamp_Byte(green), Clamp_Byte(blue));
				};

				row0[pixel_x + static_cast<int>(dest_x)] = convert_pixel(luma_row0[pixel_x]);
				if (pixel_x + 1 < max_copy_width) {
					row0[pixel_x + 1 + static_cast<int>(dest_x)] = convert_pixel(luma_row0[pixel_x + 1]);
				}
				if (row1 != nullptr) {
					row1[pixel_x + static_cast<int>(dest_x)] = convert_pixel(luma_row1[pixel_x]);
					if (pixel_x + 1 < max_copy_width) {
						row1[pixel_x + 1 + static_cast<int>(dest_x)] = convert_pixel(luma_row1[pixel_x + 1]);
					}
				}
			}
		}
	}

	bool Get_Frame_Planes(BINKFRAMEPLANES &planes) const
	{
		std::memset(&planes, 0, sizeof(planes));

		const PlaneBuffer &luma = CurrentFrame.planes[0];
		const PlaneBuffer &u_plane = CurrentFrame.planes[1];
		const PlaneBuffer &v_plane = CurrentFrame.planes[2];
		if (luma.pixels.empty() || u_plane.pixels.empty() || v_plane.pixels.empty()) {
			return false;
		}

		planes.YPlane = luma.pixels.data();
		planes.UPlane = u_plane.pixels.data();
		planes.VPlane = v_plane.pixels.data();
		planes.YStride = luma.stride;
		planes.UStride = u_plane.stride;
		planes.VStride = v_plane.stride;
		planes.LumaWidth = static_cast<uint32_t>(luma.width);
		planes.LumaHeight = static_cast<uint32_t>(luma.height);
		planes.ChromaWidth = static_cast<uint32_t>(u_plane.width);
		planes.ChromaHeight = static_cast<uint32_t>(u_plane.height);
		planes.Flags = FullRange ? BINKFRAMEPLANES_FULL_RANGE : 0U;

		if (HasAlpha) {
			const PlaneBuffer &alpha = CurrentFrame.planes[3];
			planes.APlane = alpha.pixels.data();
			planes.AStride = alpha.stride;
			planes.Flags |= BINKFRAMEPLANES_HAS_ALPHA;
		}

		return true;
	}

private:
	static constexpr uint8_t BinkbBundleSizes[BINKB_NB_SRC] = { 4, 8, 8, 5, 5, 11, 11, 4, 4, 7 };
	static constexpr uint8_t BinkbBundleSigned[BINKB_NB_SRC] = { 0, 0, 0, 1, 1, 0, 1, 0, 0, 0 };
	static constexpr uint8_t BinkRleLens[4] = { 4, 8, 12, 32 };
	static constexpr uint8_t RleLengthTable[16] = { 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64 };

	bool Allocate_Frame(VideoFrame &frame)
	{
		frame.planes[0].Allocate(Width, Height);
		frame.planes[1].Allocate((Width + 1) >> 1, (Height + 1) >> 1);
		frame.planes[2].Allocate((Width + 1) >> 1, (Height + 1) >> 1);
		if (HasAlpha) {
			frame.planes[3].Allocate(Width, Height);
		}
		return true;
	}

	void Clear_Frame(VideoFrame &frame)
	{
		frame.planes[0].Clear();
		frame.planes[1].pixels.assign(frame.planes[1].pixels.size(), 0x80U);
		frame.planes[2].pixels.assign(frame.planes[2].pixels.size(), 0x80U);
		if (HasAlpha) {
			frame.planes[3].Clear();
		}
	}

	void Initialize_Bundles()
	{
		const int bw = (Width + 7) >> 3;
		const int bh = (Height + 7) >> 3;
		const int blocks = bw * bh;

		BundleStorage.assign(static_cast<size_t>(blocks) * 64U * BINKB_NB_SRC, 0U);
		uint8_t *cursor = BundleStorage.data();
		for (int index = 0; index < BINKB_NB_SRC; ++index) {
			Bundles[index].data = cursor;
			cursor += blocks * 64;
			Bundles[index].data_end = cursor;
		}
	}

	void Init_Lengths(int width, int bw)
	{
		width = FFALIGN(width, 8);
		Bundles[BINK_SRC_BLOCK_TYPES].len = Floor_Log2(static_cast<uint32_t>((width >> 3) + 511)) + 1;
		Bundles[BINK_SRC_SUB_BLOCK_TYPES].len = Floor_Log2(static_cast<uint32_t>((width >> 4) + 511)) + 1;
		Bundles[BINK_SRC_COLORS].len = Floor_Log2(static_cast<uint32_t>(bw * 64 + 511)) + 1;
		Bundles[BINK_SRC_INTRA_DC].len =
		Bundles[BINK_SRC_INTER_DC].len =
		Bundles[BINK_SRC_X_OFF].len =
		Bundles[BINK_SRC_Y_OFF].len = Floor_Log2(static_cast<uint32_t>((width >> 3) + 511)) + 1;
		Bundles[BINK_SRC_PATTERN].len = Floor_Log2(static_cast<uint32_t>((bw << 3) + 511)) + 1;
		Bundles[BINK_SRC_RUN].len = Floor_Log2(static_cast<uint32_t>(bw * 48 + 511)) + 1;
	}

	bool Read_Bundle_Header(BitReader &bit_reader, int bundle_num)
	{
		if (bundle_num == BINK_SRC_COLORS) {
			for (Tree &tree : ColorHighTrees) {
				if (!Read_Tree(bit_reader, tree)) {
					return false;
				}
			}
			ColorLastValue = 0;
		}

		if (bundle_num != BINK_SRC_INTRA_DC && bundle_num != BINK_SRC_INTER_DC) {
			if (!Read_Tree(bit_reader, Bundles[bundle_num].tree)) {
				return false;
			}
		}

		Bundles[bundle_num].cur_dec = Bundles[bundle_num].data;
		Bundles[bundle_num].cur_ptr = Bundles[bundle_num].data;
		return true;
	}

	bool Read_Bundle_Value(BitReader &bit_reader, Bundle &bundle, uint32_t &count)
	{
		if (bundle.cur_dec == nullptr || bundle.cur_dec > bundle.cur_ptr) {
			count = 0;
			return true;
		}

		if (!bit_reader.Read_Bits(bundle.len, count)) {
			return false;
		}

		if (count == 0U) {
			bundle.cur_dec = nullptr;
		}
		return true;
	}

	bool Read_Runs(BitReader &bit_reader, Bundle &bundle)
	{
		uint32_t count = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, count)) {
			return false;
		}
		if (count == 0U) {
			return true;
		}

		uint8_t *decode_end = bundle.cur_dec + count;
		if (decode_end > bundle.data_end) {
			return false;
		}

		uint32_t repeat = 0;
		if (!bit_reader.Read_Bit(repeat)) {
			return false;
		}

		if (repeat != 0U) {
			uint32_t value = 0;
			if (!bit_reader.Read_Bits(4, value)) {
				return false;
			}
			std::memset(bundle.cur_dec, static_cast<int>(value), count);
			bundle.cur_dec += count;
			return true;
		}

		while (bundle.cur_dec < decode_end) {
			uint8_t value = 0;
			if (!Get_Huff(bit_reader, bundle.tree, value)) {
				return false;
			}
			*bundle.cur_dec++ = value;
		}
		return true;
	}

	bool Read_Motion_Values(BitReader &bit_reader, Bundle &bundle)
	{
		uint32_t count = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, count)) {
			return false;
		}
		if (count == 0U) {
			return true;
		}

		uint8_t *decode_end = bundle.cur_dec + count;
		if (decode_end > bundle.data_end) {
			return false;
		}

		uint32_t repeat = 0;
		if (!bit_reader.Read_Bit(repeat)) {
			return false;
		}

		if (repeat != 0U) {
			uint32_t value = 0;
			if (!bit_reader.Read_Bits(4, value)) {
				return false;
			}
			int signed_value = static_cast<int>(value);
			if (signed_value != 0) {
				uint32_t sign = 0;
				if (!bit_reader.Read_Bit(sign)) {
					return false;
				}
				const int sign_mask = -static_cast<int>(sign);
				signed_value = (signed_value ^ sign_mask) - sign_mask;
			}
			std::memset(bundle.cur_dec, signed_value & 0xFF, count);
			bundle.cur_dec += count;
			return true;
		}

		while (bundle.cur_dec < decode_end) {
			uint8_t value = 0;
			if (!Get_Huff(bit_reader, bundle.tree, value)) {
				return false;
			}
			int signed_value = static_cast<int>(value);
			if (signed_value != 0) {
				uint32_t sign = 0;
				if (!bit_reader.Read_Bit(sign)) {
					return false;
				}
				const int sign_mask = -static_cast<int>(sign);
				signed_value = (signed_value ^ sign_mask) - sign_mask;
			}
			*bundle.cur_dec++ = static_cast<uint8_t>(signed_value & 0xFF);
		}
		return true;
	}

	bool Read_Block_Types(BitReader &bit_reader, Bundle &bundle)
	{
		uint32_t count = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, count)) {
			return false;
		}

		if (count == 0U) {
			return true;
		}

		if (Version == 'k') {
			count ^= 0xBBU;
			if (count == 0U) {
				bundle.cur_dec = nullptr;
				return true;
			}
		}

		uint8_t *decode_end = bundle.cur_dec + count;
		if (decode_end > bundle.data_end) {
			return false;
		}

		uint32_t repeat = 0;
		if (!bit_reader.Read_Bit(repeat)) {
			return false;
		}

		int last = 0;
		if (repeat != 0U) {
			uint32_t value = 0;
			if (!bit_reader.Read_Bits(4, value)) {
				return false;
			}
			std::memset(bundle.cur_dec, static_cast<int>(value), count);
			bundle.cur_dec += count;
			return true;
		}

		while (bundle.cur_dec < decode_end) {
			uint8_t value = 0;
			if (!Get_Huff(bit_reader, bundle.tree, value)) {
				return false;
			}
			if (value < 12U) {
				last = value;
				*bundle.cur_dec++ = value;
			} else {
				const int run = BinkRleLens[value - 12U];
				if (decode_end - bundle.cur_dec < run) {
					return false;
				}
				std::memset(bundle.cur_dec, last, static_cast<size_t>(run));
				bundle.cur_dec += run;
			}
		}
		return true;
	}

	bool Read_Patterns(BitReader &bit_reader, Bundle &bundle)
	{
		uint32_t count = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, count)) {
			return false;
		}
		if (count == 0U) {
			return true;
		}

		uint8_t *decode_end = bundle.cur_dec + count;
		if (decode_end > bundle.data_end) {
			return false;
		}

		while (bundle.cur_dec < decode_end) {
			uint8_t low = 0;
			uint8_t high = 0;
			if (!Get_Huff(bit_reader, bundle.tree, low) || !Get_Huff(bit_reader, bundle.tree, high)) {
				return false;
			}
			*bundle.cur_dec++ = static_cast<uint8_t>(low | (high << 4));
		}

		return true;
	}

	bool Read_Colors(BitReader &bit_reader, Bundle &bundle)
	{
		uint32_t count = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, count)) {
			return false;
		}
		if (count == 0U) {
			return true;
		}

		uint8_t *decode_end = bundle.cur_dec + count;
		if (decode_end > bundle.data_end) {
			return false;
		}

		uint32_t repeat = 0;
		if (!bit_reader.Read_Bit(repeat)) {
			return false;
		}

		auto decode_color = [&]() -> std::optional<uint8_t> {
			uint8_t high = 0;
			uint8_t low = 0;
			if (!Get_Huff(bit_reader, ColorHighTrees[ColorLastValue], high) ||
				!Get_Huff(bit_reader, bundle.tree, low)) {
				return std::nullopt;
			}

			ColorLastValue = high;
			int value = (static_cast<int>(high) << 4) | low;
			if (Version < 'i') {
				const int sign = static_cast<int>(static_cast<int8_t>(value)) >> 7;
				value = ((value & 0x7F) ^ sign) - sign;
				value += 0x80;
			}
			return static_cast<uint8_t>(value);
		};

		if (repeat != 0U) {
			const auto color = decode_color();
			if (!color.has_value()) {
				return false;
			}
			std::memset(bundle.cur_dec, color.value(), count);
			bundle.cur_dec += count;
			return true;
		}

		while (bundle.cur_dec < decode_end) {
			const auto color = decode_color();
			if (!color.has_value()) {
				return false;
			}
			*bundle.cur_dec++ = color.value();
		}

		return true;
	}

	bool Read_DCs(BitReader &bit_reader, Bundle &bundle, int start_bits, bool has_sign)
	{
		uint32_t length = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, length)) {
			return false;
		}
		if (length == 0U) {
			return true;
		}

		uint32_t value = 0;
		if (!bit_reader.Read_Bits(start_bits - (has_sign ? 1 : 0), value)) {
			return false;
		}

		int decoded = static_cast<int>(value);
		if (decoded != 0 && has_sign) {
			uint32_t sign = 0;
			if (!bit_reader.Read_Bit(sign)) {
				return false;
			}
			const int sign_mask = -static_cast<int>(sign);
			decoded = (decoded ^ sign_mask) - sign_mask;
		}

		auto *dst = reinterpret_cast<int16_t *>(bundle.cur_dec);
		auto *dst_end = reinterpret_cast<int16_t *>(bundle.data_end);
		if (dst >= dst_end) {
			return false;
		}

		*dst++ = static_cast<int16_t>(decoded);
		--length;

		for (uint32_t index = 0; index < length; index += 8U) {
			const uint32_t length2 = FFMIN<uint32_t>(length - index, 8U);
			uint32_t bit_count = 0;
			if (!bit_reader.Read_Bits(4, bit_count)) {
				return false;
			}

			if ((dst_end - dst) < static_cast<std::ptrdiff_t>(length2)) {
				return false;
			}

			if (bit_count != 0U) {
				for (uint32_t inner = 0; inner < length2; ++inner) {
					uint32_t delta_bits = 0;
					if (!bit_reader.Read_Bits(static_cast<int>(bit_count), delta_bits)) {
						return false;
					}

					int delta = static_cast<int>(delta_bits);
					if (delta != 0) {
						uint32_t sign = 0;
						if (!bit_reader.Read_Bit(sign)) {
							return false;
						}
						const int sign_mask = -static_cast<int>(sign);
						delta = (delta ^ sign_mask) - sign_mask;
					}

					decoded += delta;
					if (decoded < -32768 || decoded > 32767) {
						return false;
					}

					*dst++ = static_cast<int16_t>(decoded);
				}
			} else {
				for (uint32_t inner = 0; inner < length2; ++inner) {
					*dst++ = static_cast<int16_t>(decoded);
				}
			}
		}

		bundle.cur_dec = reinterpret_cast<uint8_t *>(dst);
		return true;
	}

	int Get_Value(int bundle_num)
	{
		Bundle &bundle = Bundles[bundle_num];
		if (bundle_num < BINK_SRC_X_OFF || bundle_num == BINK_SRC_RUN) {
			return *bundle.cur_ptr++;
		}
		if (bundle_num == BINK_SRC_X_OFF || bundle_num == BINK_SRC_Y_OFF) {
			return static_cast<int8_t>(*bundle.cur_ptr++);
		}
		const int16_t value = *reinterpret_cast<int16_t *>(bundle.cur_ptr);
		bundle.cur_ptr += 2;
		return value;
	}

	void Init_Binkb_Bundles()
	{
		for (int index = 0; index < BINKB_NB_SRC; ++index) {
			Bundles[index].cur_dec = Bundles[index].data;
			Bundles[index].cur_ptr = Bundles[index].data;
			Bundles[index].len = 13;
		}
	}

	bool Read_Binkb_Bundle(BitReader &bit_reader, int bundle_num)
	{
		const int bits = BinkbBundleSizes[bundle_num];
		const int mask = 1 << (bits - 1);
		const bool is_signed = BinkbBundleSigned[bundle_num] != 0U;
		Bundle &bundle = Bundles[bundle_num];

		uint32_t length = 0;
		if (!Read_Bundle_Value(bit_reader, bundle, length)) {
			return false;
		}
		if (length == 0U) {
			return true;
		}

		if ((bundle.data_end - bundle.cur_dec) < static_cast<std::ptrdiff_t>(length * (1 + (bits > 8)))) {
			return false;
		}

		if (bits <= 8) {
			for (uint32_t index = 0; index < length; ++index) {
				uint32_t value = 0;
				if (!bit_reader.Read_Bits(bits, value)) {
					return false;
				}
				const int decoded = is_signed ? (static_cast<int>(value) - mask) : static_cast<int>(value);
				*bundle.cur_dec++ = static_cast<uint8_t>(decoded & 0xFF);
			}
			return true;
		}

		auto *dst = reinterpret_cast<int16_t *>(bundle.cur_dec);
		for (uint32_t index = 0; index < length; ++index) {
			uint32_t value = 0;
			if (!bit_reader.Read_Bits(bits, value)) {
				return false;
			}
			const int decoded = is_signed ? (static_cast<int>(value) - mask) : static_cast<int>(value);
			*dst++ = static_cast<int16_t>(decoded);
		}
		bundle.cur_dec = reinterpret_cast<uint8_t *>(dst);
		return true;
	}

	int Get_Binkb_Value(int bundle_num)
	{
		const int bits = BinkbBundleSizes[bundle_num];
		Bundle &bundle = Bundles[bundle_num];
		if (bits <= 8) {
			const uint8_t value = *bundle.cur_ptr++;
			return BinkbBundleSigned[bundle_num] ? static_cast<int8_t>(value) : value;
		}

		const int16_t value = *reinterpret_cast<int16_t *>(bundle.cur_ptr);
		bundle.cur_ptr += 2;
		return value;
	}

	bool Read_DCT_Coefficients(BitReader &bit_reader, int32_t block[64], const uint8_t *scan,
		int &coefficient_count, int coefficient_indices[64], int q)
	{
		int coefficient_list[128];
		int mode_list[128];
		int list_start = 64;
		int list_end = 64;
		int count = 0;

		uint32_t bits_value = 0;
		if (!bit_reader.Read_Bits(4, bits_value)) {
			return false;
		}

		coefficient_list[list_end] = 4;  mode_list[list_end++] = 0;
		coefficient_list[list_end] = 24; mode_list[list_end++] = 0;
		coefficient_list[list_end] = 44; mode_list[list_end++] = 0;
		coefficient_list[list_end] = 1;  mode_list[list_end++] = 3;
		coefficient_list[list_end] = 2;  mode_list[list_end++] = 3;
		coefficient_list[list_end] = 3;  mode_list[list_end++] = 3;

		for (int bits = static_cast<int>(bits_value) - 1; bits >= 0; --bits) {
			int list_pos = list_start;
			while (list_pos < list_end) {
				uint32_t take = 0;
				if ((mode_list[list_pos] | coefficient_list[list_pos]) == 0) {
					++list_pos;
					continue;
				}
				if (!bit_reader.Read_Bit(take)) {
					return false;
				}
				if (take == 0U) {
					++list_pos;
					continue;
				}

				int coefficient = coefficient_list[list_pos];
				const int mode = mode_list[list_pos];
				switch (mode) {
				case 0:
					coefficient_list[list_pos] = coefficient + 4;
					mode_list[list_pos] = 1;
					[[fallthrough]];
				case 2:
					if (mode == 2) {
						coefficient_list[list_pos] = 0;
						mode_list[list_pos++] = 0;
					}
					for (int index = 0; index < 4; ++index, ++coefficient) {
						if (!bit_reader.Read_Bit(take)) {
							return false;
						}
						if (take != 0U) {
							coefficient_list[--list_start] = coefficient;
							mode_list[list_start] = 3;
						} else {
							int value = 0;
							if (bits == 0) {
								if (!bit_reader.Read_Bit(take)) {
									return false;
								}
								value = 1 - (static_cast<int>(take) << 1);
							} else {
								uint32_t magnitude = 0;
								if (!bit_reader.Read_Bits(bits, magnitude) || !bit_reader.Read_Bit(take)) {
									return false;
								}
								value = static_cast<int>(magnitude) | (1 << bits);
								const int sign_mask = -static_cast<int>(take);
								value = (value ^ sign_mask) - sign_mask;
							}
							block[scan[coefficient]] = value;
							coefficient_indices[count++] = coefficient;
						}
					}
					break;
				case 1:
					mode_list[list_pos] = 2;
					for (int index = 0; index < 3; ++index) {
						coefficient += 4;
						coefficient_list[list_end] = coefficient;
						mode_list[list_end++] = 2;
					}
					break;
				case 3:
					{
						int value = 0;
						if (bits == 0) {
							if (!bit_reader.Read_Bit(take)) {
								return false;
							}
							value = 1 - (static_cast<int>(take) << 1);
						} else {
							uint32_t magnitude = 0;
							if (!bit_reader.Read_Bits(bits, magnitude) || !bit_reader.Read_Bit(take)) {
								return false;
							}
							value = static_cast<int>(magnitude) | (1 << bits);
							const int sign_mask = -static_cast<int>(take);
							value = (value ^ sign_mask) - sign_mask;
						}
						block[scan[coefficient]] = value;
						coefficient_indices[count++] = coefficient;
						coefficient_list[list_pos] = 0;
						mode_list[list_pos++] = 0;
					}
					break;
				}
			}
		}

		int quant_index = q;
		if (quant_index == -1) {
			uint32_t value = 0;
			if (!bit_reader.Read_Bits(4, value)) {
				return false;
			}
			quant_index = static_cast<int>(value);
		}

		if (quant_index < 0 || quant_index > 15) {
			return false;
		}

		coefficient_count = count;
		LastQuantIndex = quant_index;
		return true;
	}

	void Unquantize_DCT_Coefficients(int32_t block[64], const int32_t quant[64],
		int coefficient_count, const int coefficient_indices[64], const uint8_t *scan)
	{
		block[0] = static_cast<int32_t>((static_cast<int64_t>(block[0]) * quant[0]) >> 11);
		for (int index = 0; index < coefficient_count; ++index) {
			const int scan_index = coefficient_indices[index];
			block[scan[scan_index]] = static_cast<int32_t>(
				(static_cast<int64_t>(block[scan[scan_index]]) * quant[scan_index]) >> 11);
		}
	}

	bool Read_Residue(BitReader &bit_reader, int16_t block[64], int masks_count)
	{
		int coefficient_list[128];
		int mode_list[128];
		int nonzero_coefficients[64];
		int nonzero_count = 0;
		int list_start = 64;
		int list_end = 64;

		coefficient_list[list_end] = 4; mode_list[list_end++] = 0;
		coefficient_list[list_end] = 24; mode_list[list_end++] = 0;
		coefficient_list[list_end] = 44; mode_list[list_end++] = 0;
		coefficient_list[list_end] = 0; mode_list[list_end++] = 2;

		uint32_t bits = 0;
		if (!bit_reader.Read_Bits(3, bits)) {
			return false;
		}

		for (int mask = 1 << bits; mask != 0; mask >>= 1) {
			for (int index = 0; index < nonzero_count; ++index) {
				uint32_t take = 0;
				if (!bit_reader.Read_Bit(take)) {
					return false;
				}
				if (take == 0U) {
					continue;
				}

				if (block[nonzero_coefficients[index]] < 0) {
					block[nonzero_coefficients[index]] -= static_cast<int16_t>(mask);
				} else {
					block[nonzero_coefficients[index]] += static_cast<int16_t>(mask);
				}

				if (--masks_count < 0) {
					return true;
				}
			}

			int list_pos = list_start;
			while (list_pos < list_end) {
				uint32_t take = 0;
				if ((coefficient_list[list_pos] | mode_list[list_pos]) == 0) {
					++list_pos;
					continue;
				}
				if (!bit_reader.Read_Bit(take)) {
					return false;
				}
				if (take == 0U) {
					++list_pos;
					continue;
				}

				int coefficient = coefficient_list[list_pos];
				const int mode = mode_list[list_pos];
				switch (mode) {
				case 0:
					coefficient_list[list_pos] = coefficient + 4;
					mode_list[list_pos] = 1;
					[[fallthrough]];
				case 2:
					if (mode == 2) {
						coefficient_list[list_pos] = 0;
						mode_list[list_pos++] = 0;
					}
					for (int inner = 0; inner < 4; ++inner, ++coefficient) {
						if (!bit_reader.Read_Bit(take)) {
							return false;
						}
						if (take != 0U) {
							coefficient_list[--list_start] = coefficient;
							mode_list[list_start] = 3;
						} else {
							if (!bit_reader.Read_Bit(take)) {
								return false;
							}
							nonzero_coefficients[nonzero_count++] = bink_scan[coefficient];
							const int sign_mask = -static_cast<int>(take);
							block[bink_scan[coefficient]] = static_cast<int16_t>((mask ^ sign_mask) - sign_mask);
							if (--masks_count < 0) {
								return true;
							}
						}
					}
					break;
				case 1:
					mode_list[list_pos] = 2;
					for (int inner = 0; inner < 3; ++inner) {
						coefficient += 4;
						coefficient_list[list_end] = coefficient;
						mode_list[list_end++] = 2;
					}
					break;
				case 3:
					if (!bit_reader.Read_Bit(take)) {
						return false;
					}
					nonzero_coefficients[nonzero_count++] = bink_scan[coefficient];
					{
						const int sign_mask = -static_cast<int>(take);
						block[bink_scan[coefficient]] = static_cast<int16_t>((mask ^ sign_mask) - sign_mask);
					}
					coefficient_list[list_pos] = 0;
					mode_list[list_pos++] = 0;
					if (--masks_count < 0) {
						return true;
					}
					break;
				}
			}
		}

		return true;
	}

	void Copy_8x8(uint8_t *dst, const uint8_t *src, int stride)
	{
		for (int row = 0; row < 8; ++row) {
			std::memcpy(dst + static_cast<size_t>(row) * stride,
				src + static_cast<size_t>(row) * stride, 8U);
		}
	}

	void Copy_8x8_Overlapped(uint8_t *dst, const uint8_t *src, int stride)
	{
		uint8_t temporary[64];
		for (int row = 0; row < 8; ++row) {
			std::memcpy(temporary + row * 8, src + static_cast<size_t>(row) * stride, 8U);
		}
		for (int row = 0; row < 8; ++row) {
			std::memcpy(dst + static_cast<size_t>(row) * stride, temporary + row * 8, 8U);
		}
	}

	void Fill_Block(uint8_t *dst, int stride, int width, int height, uint8_t value)
	{
		for (int row = 0; row < height; ++row) {
			std::memset(dst + static_cast<size_t>(row) * stride, value, static_cast<size_t>(width));
		}
	}

	void Clear_Block(int16_t block[64])
	{
		std::memset(block, 0, sizeof(int16_t) * 64U);
	}

	bool Bink_Put_Pixels(int plane_index, uint8_t *dst, uint8_t *prev, int stride, const uint8_t *ref_start, const uint8_t *ref_end)
	{
		(void)plane_index;
		const int xoff = Get_Value(BINK_SRC_X_OFF);
		const int yoff = Get_Value(BINK_SRC_Y_OFF);
		const uint8_t *ref = prev + xoff + yoff * stride;
		if (ref < ref_start || ref > ref_end) {
			return false;
		}
		Copy_8x8(dst, ref, stride);
		return true;
	}

	bool Decode_Plane_B(BitReader &bit_reader, int plane_index, bool is_key, bool is_chroma)
	{
		PlaneBuffer &plane = CurrentFrame.planes[plane_index];
		const int stride = plane.stride;
		const int bw = is_chroma ? ((Width + 15) >> 4) : ((Width + 7) >> 3);
		const int bh = is_chroma ? ((Height + 15) >> 4) : ((Height + 7) >> 3);
		const int y_bias = is_key ? -15 : 0;

		Init_Binkb_Bundles();
		const uint8_t *ref_start = plane.pixels.data();
		const uint8_t *ref_end = ref_start + ((bh - 1) * stride + bw - 1) * 8;

		std::array<int, 64> coord_map{};
		for (int index = 0; index < 64; ++index) {
			coord_map[index] = (index & 7) + (index >> 3) * stride;
		}

		for (int by = 0; by < bh; ++by) {
			for (int index = 0; index < BINKB_NB_SRC; ++index) {
				if (!Read_Binkb_Bundle(bit_reader, index)) {
					return false;
				}
			}

			uint8_t *dst = plane.pixels.data() + static_cast<size_t>(8 * by) * stride;
			for (int bx = 0; bx < bw; ++bx, dst += 8) {
				const int block_type = Get_Binkb_Value(BINKB_SRC_BLOCK_TYPES);
				std::array<int16_t, 64> block{};
				std::array<int32_t, 64> dct_block{};
				int quant_index = 0;
				int coefficient_count = 0;
				int coefficient_indices[64]{};

				switch (block_type) {
				case 0:
					break;
				case 1:
					{
						uint32_t pattern_index = 0;
						if (!bit_reader.Read_Bits(4, pattern_index)) {
							return false;
						}
						const uint8_t *scan = bink_patterns[pattern_index];
						int position = 0;
						do {
							uint32_t mode = 0;
							uint32_t run_bits = 0;
							if (!bit_reader.Read_Bit(mode) ||
								!bit_reader.Read_Bits(binkb_runbits[position], run_bits)) {
								return false;
							}
							const int run = static_cast<int>(run_bits) + 1;
							position += run;
							if (position > 64) {
								return false;
							}
							if (mode != 0U) {
								const int value = Get_Binkb_Value(BINKB_SRC_COLORS);
								for (int index = 0; index < run; ++index) {
									dst[coord_map[*scan++]] = static_cast<uint8_t>(value);
								}
							} else {
								for (int index = 0; index < run; ++index) {
									dst[coord_map[*scan++]] = static_cast<uint8_t>(Get_Binkb_Value(BINKB_SRC_COLORS));
								}
							}
						} while (position < 63);

						if (position == 63) {
							dst[coord_map[*scan]] = static_cast<uint8_t>(Get_Binkb_Value(BINKB_SRC_COLORS));
						}
					}
					break;
				case 2:
					dct_block[0] = Get_Binkb_Value(BINKB_SRC_INTRA_DC);
					if (!Read_DCT_Coefficients(bit_reader, dct_block.data(), bink_scan, coefficient_count, coefficient_indices, Get_Binkb_Value(BINKB_SRC_INTRA_Q))) {
						return false;
					}
					quant_index = LastQuantIndex;
					Unquantize_DCT_Coefficients(dct_block.data(), g_binkb_intra_quant[quant_index].data(), coefficient_count, coefficient_indices, bink_scan);
					DSP.idct_put(dst, stride, dct_block.data());
					break;
				case 3:
				case 4:
				case 7:
					{
						const int xoff = Get_Binkb_Value(BINKB_SRC_X_OFF);
						const int yoff = Get_Binkb_Value(BINKB_SRC_Y_OFF) + y_bias;
						const uint8_t *ref = dst + xoff + yoff * stride;
						if (ref < ref_start || ref > ref_end) {
							return false;
						}
						if (ref + 8 * stride < dst || ref >= dst + 8 * stride) {
							Copy_8x8(dst, ref, stride);
						} else {
							Copy_8x8_Overlapped(dst, ref, stride);
						}

						if (block_type == 3) {
							Clear_Block(block.data());
							const int residue_masks = Get_Binkb_Value(BINKB_SRC_INTER_COEFS);
							if (!Read_Residue(bit_reader, block.data(), residue_masks)) {
								return false;
							}
							DSP.add_pixels8(dst, block.data(), stride);
						} else if (block_type == 4) {
							dct_block[0] = Get_Binkb_Value(BINKB_SRC_INTER_DC);
							if (!Read_DCT_Coefficients(bit_reader, dct_block.data(), bink_scan, coefficient_count, coefficient_indices, Get_Binkb_Value(BINKB_SRC_INTER_Q))) {
								return false;
							}
							quant_index = LastQuantIndex;
							Unquantize_DCT_Coefficients(dct_block.data(), g_binkb_inter_quant[quant_index].data(), coefficient_count, coefficient_indices, bink_scan);
							DSP.idct_add(dst, stride, dct_block.data());
						}
					}
					break;
				case 5:
					Fill_Block(dst, stride, 8, 8, static_cast<uint8_t>(Get_Binkb_Value(BINKB_SRC_COLORS)));
					break;
				case 6:
					{
						const int color0 = Get_Binkb_Value(BINKB_SRC_COLORS);
						const int color1 = Get_Binkb_Value(BINKB_SRC_COLORS);
						for (int row = 0; row < 8; ++row) {
							int pattern = Get_Binkb_Value(BINKB_SRC_PATTERN);
							for (int column = 0; column < 8; ++column, pattern >>= 1) {
								dst[row * stride + column] = static_cast<uint8_t>((pattern & 1) != 0 ? color1 : color0);
							}
						}
					}
					break;
				case 8:
					for (int row = 0; row < 8; ++row) {
						std::memcpy(dst + row * stride, Bundles[BINKB_SRC_COLORS].cur_ptr + row * 8, 8U);
					}
					Bundles[BINKB_SRC_COLORS].cur_ptr += 64;
					break;
				default:
					return false;
				}
			}
		}

		bit_reader.Align_To_32();
		return true;
	}

	bool Decode_Plane(BitReader &bit_reader, int plane_index, bool is_chroma)
	{
		PlaneBuffer &plane = CurrentFrame.planes[plane_index];
		const PlaneBuffer &reference_plane = (FrameNumber > 1U) ? LastFrame.planes[plane_index] : CurrentFrame.planes[plane_index];
		const int stride = plane.stride;
		const int bw = is_chroma ? ((Width + 15) >> 4) : ((Width + 7) >> 3);
		const int bh = is_chroma ? ((Height + 15) >> 4) : ((Height + 7) >> 3);
		const int plane_width = plane.width;
		const int plane_height = plane.height;

		if (Version == 'k') {
			uint32_t fill_plane = 0;
			if (!bit_reader.Read_Bit(fill_plane)) {
				return false;
			}
			if (fill_plane != 0U) {
				uint32_t fill_value = 0;
				if (!bit_reader.Read_Bits(8, fill_value)) {
					return false;
				}
				for (int row = 0; row < plane_height; ++row) {
					std::memset(plane.pixels.data() + static_cast<size_t>(row) * stride, static_cast<int>(fill_value), static_cast<size_t>(plane_width));
				}
				bit_reader.Align_To_32();
				return true;
			}
		}

		Init_Lengths(FFMAX(plane_width, 8), bw);
		for (int index = 0; index < BINK_NB_SRC; ++index) {
			if (!Read_Bundle_Header(bit_reader, index)) {
				return false;
			}
		}

		const uint8_t *ref_start = reference_plane.pixels.data();
		const uint8_t *ref_end = ref_start + (bw - 1 + reference_plane.stride * (bh - 1)) * 8;

		std::array<int, 64> coord_map{};
		for (int index = 0; index < 64; ++index) {
			coord_map[index] = (index & 7) + (index >> 3) * stride;
		}

		for (int by = 0; by < bh; ++by) {
			if (!Read_Block_Types(bit_reader, Bundles[BINK_SRC_BLOCK_TYPES])) {
				return false;
			}
			if (!Read_Block_Types(bit_reader, Bundles[BINK_SRC_SUB_BLOCK_TYPES])) {
				return false;
			}
			if (!Read_Colors(bit_reader, Bundles[BINK_SRC_COLORS])) {
				return false;
			}
			if (!Read_Patterns(bit_reader, Bundles[BINK_SRC_PATTERN])) {
				return false;
			}
			if (!Read_Motion_Values(bit_reader, Bundles[BINK_SRC_X_OFF])) {
				return false;
			}
			if (!Read_Motion_Values(bit_reader, Bundles[BINK_SRC_Y_OFF])) {
				return false;
			}
			if (!Read_DCs(bit_reader, Bundles[BINK_SRC_INTRA_DC], 11, false)) {
				return false;
			}
			if (!Read_DCs(bit_reader, Bundles[BINK_SRC_INTER_DC], 11, true)) {
				return false;
			}
			if (!Read_Runs(bit_reader, Bundles[BINK_SRC_RUN])) {
				return false;
			}

			uint8_t *dst = plane.pixels.data() + static_cast<size_t>(8 * by) * stride;
			uint8_t *prev = const_cast<uint8_t *>(reference_plane.pixels.data()) + static_cast<size_t>(8 * by) * reference_plane.stride;
			for (int bx = 0; bx < bw; ++bx, dst += 8, prev += 8) {
				int block_type = Get_Value(BINK_SRC_BLOCK_TYPES);
				std::array<int16_t, 64> block{};
				std::array<uint8_t, 64> scaled_block{};
				std::array<int32_t, 64> dct_block{};
				int coefficient_count = 0;
				int coefficient_indices[64]{};

				if (((by & 1) != 0 || (bx & 1) != 0) && block_type == SCALED_BLOCK) {
					++bx;
					dst += 8;
					prev += 8;
					continue;
				}

				switch (block_type) {
				case SKIP_BLOCK:
					Copy_8x8(dst, prev, stride);
					break;
				case SCALED_BLOCK:
					block_type = Get_Value(BINK_SRC_SUB_BLOCK_TYPES);
					switch (block_type) {
					case RUN_BLOCK:
						{
							uint32_t pattern_index = 0;
							if (!bit_reader.Read_Bits(4, pattern_index)) {
								return false;
							}
							const uint8_t *scan = bink_patterns[pattern_index];
							int position = 0;
							do {
								const int run = Get_Value(BINK_SRC_RUN) + 1;
								uint32_t repeat = 0;
								if (!bit_reader.Read_Bit(repeat)) {
									return false;
								}
								position += run;
								if (position > 64) {
									return false;
								}
								if (repeat != 0U) {
									const int value = Get_Value(BINK_SRC_COLORS);
									for (int index = 0; index < run; ++index) {
										scaled_block[*scan++] = static_cast<uint8_t>(value);
									}
								} else {
									for (int index = 0; index < run; ++index) {
										scaled_block[*scan++] = static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS));
									}
								}
							} while (position < 63);

							if (position == 63) {
								scaled_block[*scan] = static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS));
							}
						}
						break;
					case INTRA_BLOCK:
						dct_block[0] = Get_Value(BINK_SRC_INTRA_DC);
						if (!Read_DCT_Coefficients(bit_reader, dct_block.data(), bink_scan, coefficient_count, coefficient_indices, -1)) {
							return false;
						}
						Unquantize_DCT_Coefficients(dct_block.data(), bink_intra_quant[LastQuantIndex], coefficient_count, coefficient_indices, bink_scan);
						DSP.idct_put(scaled_block.data(), 8, dct_block.data());
						break;
					case FILL_BLOCK:
						Fill_Block(dst, stride, 16, 16, static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS)));
						break;
					case PATTERN_BLOCK:
						{
							const int color0 = Get_Value(BINK_SRC_COLORS);
							const int color1 = Get_Value(BINK_SRC_COLORS);
							for (int row = 0; row < 8; ++row) {
								int pattern = Get_Value(BINK_SRC_PATTERN);
								for (int column = 0; column < 8; ++column, pattern >>= 1) {
									scaled_block[column + row * 8] = static_cast<uint8_t>((pattern & 1) != 0 ? color1 : color0);
								}
							}
						}
						break;
					case RAW_BLOCK:
						for (int row = 0; row < 8; ++row) {
							for (int column = 0; column < 8; ++column) {
								scaled_block[column + row * 8] = static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS));
							}
						}
						break;
					default:
						return false;
					}

					if (block_type != FILL_BLOCK) {
						DSP.scale_block(scaled_block.data(), dst, stride);
					}
					++bx;
					dst += 8;
					prev += 8;
					break;
				case MOTION_BLOCK:
					if (!Bink_Put_Pixels(plane_index, dst, prev, stride, ref_start, ref_end)) {
						return false;
					}
					break;
				case RUN_BLOCK:
					{
						uint32_t pattern_index = 0;
						if (!bit_reader.Read_Bits(4, pattern_index)) {
							return false;
						}
						const uint8_t *scan = bink_patterns[pattern_index];
						int position = 0;
						do {
							const int run = Get_Value(BINK_SRC_RUN) + 1;
							uint32_t repeat = 0;
							if (!bit_reader.Read_Bit(repeat)) {
								return false;
							}
							position += run;
							if (position > 64) {
								return false;
							}
							if (repeat != 0U) {
								const int value = Get_Value(BINK_SRC_COLORS);
								for (int index = 0; index < run; ++index) {
									dst[coord_map[*scan++]] = static_cast<uint8_t>(value);
								}
							} else {
								for (int index = 0; index < run; ++index) {
									dst[coord_map[*scan++]] = static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS));
								}
							}
						} while (position < 63);

						if (position == 63) {
							dst[coord_map[*scan]] = static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS));
						}
					}
					break;
				case RESIDUE_BLOCK:
					if (!Bink_Put_Pixels(plane_index, dst, prev, stride, ref_start, ref_end)) {
						return false;
					}
					Clear_Block(block.data());
					{
						uint32_t residue_masks = 0;
						if (!bit_reader.Read_Bits(7, residue_masks) || !Read_Residue(bit_reader, block.data(), static_cast<int>(residue_masks))) {
							return false;
						}
					}
					DSP.add_pixels8(dst, block.data(), stride);
					break;
				case INTRA_BLOCK:
					dct_block[0] = Get_Value(BINK_SRC_INTRA_DC);
					if (!Read_DCT_Coefficients(bit_reader, dct_block.data(), bink_scan, coefficient_count, coefficient_indices, -1)) {
						return false;
					}
					Unquantize_DCT_Coefficients(dct_block.data(), bink_intra_quant[LastQuantIndex], coefficient_count, coefficient_indices, bink_scan);
					DSP.idct_put(dst, stride, dct_block.data());
					break;
				case FILL_BLOCK:
					Fill_Block(dst, stride, 8, 8, static_cast<uint8_t>(Get_Value(BINK_SRC_COLORS)));
					break;
				case INTER_BLOCK:
					if (!Bink_Put_Pixels(plane_index, dst, prev, stride, ref_start, ref_end)) {
						return false;
					}
					dct_block[0] = Get_Value(BINK_SRC_INTER_DC);
					if (!Read_DCT_Coefficients(bit_reader, dct_block.data(), bink_scan, coefficient_count, coefficient_indices, -1)) {
						return false;
					}
					Unquantize_DCT_Coefficients(dct_block.data(), bink_inter_quant[LastQuantIndex], coefficient_count, coefficient_indices, bink_scan);
					DSP.idct_add(dst, stride, dct_block.data());
					break;
				case PATTERN_BLOCK:
					{
						const int color0 = Get_Value(BINK_SRC_COLORS);
						const int color1 = Get_Value(BINK_SRC_COLORS);
						for (int row = 0; row < 8; ++row) {
							int pattern = Get_Value(BINK_SRC_PATTERN);
							for (int column = 0; column < 8; ++column, pattern >>= 1) {
								dst[row * stride + column] = static_cast<uint8_t>((pattern & 1) != 0 ? color1 : color0);
							}
						}
					}
					break;
				case RAW_BLOCK:
					for (int row = 0; row < 8; ++row) {
						std::memcpy(dst + row * stride, Bundles[BINK_SRC_COLORS].cur_ptr + row * 8, 8U);
					}
					Bundles[BINK_SRC_COLORS].cur_ptr += 64;
					break;
				default:
					return false;
				}
			}
		}

		bit_reader.Align_To_32();
		return true;
	}

	BinkDSPContext DSP{};
	int Width = 0;
	int Height = 0;
	int Version = 0;
	uint32_t Flags = 0;
	bool HasAlpha = false;
	bool SwapPlanes = false;
	bool FullRange = false;
	uint32_t FrameNumber = 0;
	int ColorLastValue = 0;
	int LastQuantIndex = 0;
	std::array<Tree, 16> ColorHighTrees{};
	std::array<Bundle, BINKB_NB_SRC> Bundles{};
	std::vector<uint8_t> BundleStorage;
	VideoFrame CurrentFrame;
	VideoFrame LastFrame;
};

class AudioTrackDecoder
{
public:
	static constexpr uint8_t RleLengthTable[16] = { 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64 };

	bool Initialize(uint32_t codec_tag, int sample_rate, int channels, bool use_dct)
	{
		if (sample_rate <= 0 || channels <= 0 || channels > MAX_DCT_CHANNELS) {
			return false;
		}

		UseDCT = use_dct;
		VersionB = ((codec_tag >> 24) == 'b');
		PhysicalChannels = channels;
		SampleRate = sample_rate;

		int effective_sample_rate = sample_rate;
		int frame_len_bits = 0;
		if (sample_rate < 22050) {
			frame_len_bits = 9;
		} else if (sample_rate < 44100) {
			frame_len_bits = 10;
		} else {
			frame_len_bits = 11;
		}

		if (UseDCT) {
			CodedChannels = channels;
		} else {
			CodedChannels = 1;
			effective_sample_rate *= channels;
			if (!VersionB) {
				int tmp = channels;
				while (tmp > 1) {
					frame_len_bits++;
					tmp >>= 1;
				}
			}
		}

		FrameLength = 1 << frame_len_bits;
		OverlapLength = FrameLength / 16;
		ValidFloatSamplesPerBlock = FrameLength - OverlapLength;
		BlockSize = ValidFloatSamplesPerBlock * FFMIN(MAX_PACKET_CHANNELS, CodedChannels);

		const int sample_rate_half = (effective_sample_rate + 1) / 2;
		Root = UseDCT ?
			static_cast<float>(FrameLength / (std::sqrt(static_cast<float>(FrameLength)) * 32768.0f)) :
			static_cast<float>(2.0 / (std::sqrt(static_cast<float>(FrameLength)) * 32768.0));

		for (int index = 0; index < 96; ++index) {
			QuantTable[index] = std::exp(index * 0.15289164787221953823f) * Root;
		}

		NumBands = 1;
		for (; NumBands < 25; ++NumBands) {
			if (sample_rate_half <= ff_wma_critical_freqs[NumBands - 1]) {
				break;
			}
		}

		Bands[0] = 2;
		for (int index = 1; index < NumBands; ++index) {
			Bands[index] = static_cast<unsigned>((ff_wma_critical_freqs[index - 1] * FrameLength / sample_rate_half) & ~1);
		}
		Bands[NumBands] = static_cast<unsigned>(FrameLength);
		First = true;
		for (auto &buffer : Previous) {
			buffer.assign(static_cast<size_t>(OverlapLength), 0.0f);
		}
		for (auto &buffer : ChannelBuffers) {
			buffer.assign(static_cast<size_t>(FrameLength), 0.0f);
		}
		TransformFloatWork.assign(static_cast<size_t>(FrameLength + 2), 0.0f);
		TransformComplexWork.assign(static_cast<size_t>((FrameLength >> 1) + 1), Complex{});

		Build_Transform_Tables();
		return true;
	}

	bool Decode_Packet(const std::vector<uint8_t> &packet, std::vector<float> &pcm_output)
	{
		pcm_output.clear();
		if (packet.size() < 4U) {
			return false;
		}

		BitReader bit_reader(packet.data(), packet.size(), true);
		uint32_t reported_size = 0;
		if (!bit_reader.Read_Bits(32, reported_size)) {
			return false;
		}
		(void)reported_size;

		for (auto &buffer : ChannelBuffers) {
			std::fill(buffer.begin(), buffer.end(), 0.0f);
		}

		int channel_offset = 0;
		while (bit_reader.Get_Bits_Left() > 0U) {
			const int chunk_channels = FFMIN(MAX_PACKET_CHANNELS, CodedChannels - channel_offset);
			if (chunk_channels <= 0) {
				channel_offset = 0;
				continue;
			}

			if (!Decode_Block(bit_reader, ChannelBuffers, chunk_channels, channel_offset)) {
				return false;
			}

			channel_offset += MAX_PACKET_CHANNELS;
			bit_reader.Align_To_32();

			if (channel_offset >= CodedChannels) {
				channel_offset = 0;
				Append_Output(ChannelBuffers, pcm_output);
			}
		}

		return channel_offset == 0;
	}

	int Get_Sample_Rate() const
	{
		return SampleRate;
	}

	int Get_Channel_Count() const
	{
		return PhysicalChannels;
	}

private:
	struct RdftConfig
	{
		std::array<float, 8> fact{};
		std::vector<float> tcos;
		std::vector<float> tsin;
		float fft_scale = 1.0f;
	};

	bool Decode_Block(BitReader &bit_reader, std::array<std::vector<float>, MAX_DCT_CHANNELS> &output,
		int channels, int channel_offset)
	{
		std::array<float, 25> quant{};
		std::array<float, 4098> coefficients{};

		if (UseDCT) {
			if (!bit_reader.Skip_Bits(2U)) {
				return false;
			}
		}

		for (int channel = 0; channel < channels; ++channel) {
			if (VersionB) {
				uint32_t first = 0;
				uint32_t second = 0;
				if (!bit_reader.Read_Bits(32, first) || !bit_reader.Read_Bits(32, second)) {
					return false;
				}
				coefficients[0] = Int_As_Float(first) * Root;
				coefficients[1] = Int_As_Float(second) * Root;
			} else {
				float first = 0.0f;
				float second = 0.0f;
				if (!Read_Float29(bit_reader, first) || !Read_Float29(bit_reader, second)) {
					return false;
				}
				coefficients[0] = first * Root;
				coefficients[1] = second * Root;
			}

			for (int index = 0; index < NumBands; ++index) {
				uint32_t value = 0;
				if (!bit_reader.Read_Bits(8, value)) {
					return false;
				}
				quant[index] = QuantTable[FFMIN<uint32_t>(value, 95U)];
			}

			int band_index = 0;
			float current_quant = quant[0];
			int coefficient_index = 2;
			while (coefficient_index < FrameLength) {
				int block_end = 0;
				if (VersionB) {
					block_end = coefficient_index + 16;
				} else {
					uint32_t flag = 0;
					if (!bit_reader.Read_Bit(flag)) {
						return false;
					}
					if (flag != 0U) {
						uint32_t length = 0;
						if (!bit_reader.Read_Bits(4, length)) {
							return false;
						}
						block_end = coefficient_index + RleLengthTable[length] * 8;
					} else {
						block_end = coefficient_index + 8;
					}
				}

				block_end = FFMIN(block_end, FrameLength);

				uint32_t width = 0;
				if (!bit_reader.Read_Bits(4, width)) {
					return false;
				}

				if (width == 0U) {
					std::fill(coefficients.begin() + coefficient_index, coefficients.begin() + block_end, 0.0f);
					coefficient_index = block_end;
					while (Bands[band_index] < static_cast<unsigned>(coefficient_index)) {
						current_quant = quant[band_index++];
					}
				} else {
					while (coefficient_index < block_end) {
						if (Bands[band_index] == static_cast<unsigned>(coefficient_index)) {
							current_quant = quant[band_index++];
						}

						uint32_t coefficient_value = 0;
						if (!bit_reader.Read_Bits(static_cast<int>(width), coefficient_value)) {
							return false;
						}

						if (coefficient_value != 0U) {
							uint32_t sign = 0;
							if (!bit_reader.Read_Bit(sign)) {
								return false;
							}
							coefficients[coefficient_index] = (sign != 0U ? -1.0f : 1.0f) * current_quant * coefficient_value;
						} else {
							coefficients[coefficient_index] = 0.0f;
						}

						++coefficient_index;
					}
				}
			}

			if (UseDCT) {
				Inverse_DCT_III(coefficients.data(), output[channel + channel_offset].data());
			} else {
				Inverse_RDFT(coefficients.data(), output[channel + channel_offset].data());
			}
		}

		for (int channel = 0; channel < channels; ++channel) {
			const int count = OverlapLength * channels;
			std::vector<float> &buffer = output[channel + channel_offset];
			if (!First) {
				for (int sample = 0, weight = channel; sample < OverlapLength; ++sample, weight += channels) {
					buffer[sample] = (Previous[channel + channel_offset][sample] * (count - weight) +
						buffer[sample] * weight) / count;
				}
			}

			std::copy(buffer.begin() + (FrameLength - OverlapLength), buffer.begin() + FrameLength,
				Previous[channel + channel_offset].begin());
		}

		First = false;
		return true;
	}

	void Append_Output(const std::array<std::vector<float>, MAX_DCT_CHANNELS> &channel_buffers, std::vector<float> &pcm_output) const
	{
		if (UseDCT) {
			const size_t start = pcm_output.size();
			pcm_output.resize(start + static_cast<size_t>(ValidFloatSamplesPerBlock * PhysicalChannels));
			for (int sample = 0; sample < ValidFloatSamplesPerBlock; ++sample) {
				for (int channel = 0; channel < PhysicalChannels; ++channel) {
					pcm_output[start + static_cast<size_t>(sample * PhysicalChannels + channel)] = channel_buffers[channel][sample];
				}
			}
		} else {
			pcm_output.insert(pcm_output.end(),
				channel_buffers[0].begin(),
				channel_buffers[0].begin() + ValidFloatSamplesPerBlock);
		}
	}

	bool Read_Float29(BitReader &bit_reader, float &value)
	{
		uint32_t power = 0;
		uint32_t mantissa = 0;
		uint32_t sign = 0;
		if (!bit_reader.Read_Bits(5, power) ||
			!bit_reader.Read_Bits(23, mantissa) ||
			!bit_reader.Read_Bit(sign)) {
			return false;
		}

		value = std::ldexp(static_cast<float>(mantissa), static_cast<int>(power) - 23);
		if (sign != 0U) {
			value = -value;
		}
		return true;
	}

	void Build_Transform_Tables()
	{
		if (UseDCT) {
			Initialize_RDFT_Config(DctRdft, FrameLength, 1.0f / static_cast<float>(FrameLength));
			DctExp.resize(static_cast<size_t>(FrameLength) + static_cast<size_t>(FrameLength / 2));
			for (int index = 0; index < FrameLength; ++index) {
				DctExp[index] = static_cast<float>(std::cos((index * kPi) / (FrameLength * 2.0)));
			}
			for (int index = 0; index < (FrameLength / 2); ++index) {
				DctExp[FrameLength + index] = static_cast<float>(0.5 / std::sin(((2 * index + 1) * kPi) / (FrameLength * 2.0)));
			}
		} else {
			Initialize_RDFT_Config(AudioRdft, FrameLength, 0.5f);
		}
	}

	void Initialize_RDFT_Config(RdftConfig &config, int length, float scale)
	{
		const int length4 = FFALIGN(length, 4) / 4;
		const float multiplier = 2.0f * scale;
		const float frequency = static_cast<float>((2.0 * kPi) / static_cast<double>(length));

		config.tcos.resize(length4);
		config.tsin.resize(length4);
		config.fft_scale = scale;

		config.fact[0] = 0.5f * multiplier;
		config.fact[1] = 0.5f * multiplier;
		config.fact[2] = multiplier;
		config.fact[3] = -multiplier;
		config.fact[4] = 0.5f * multiplier;
		config.fact[5] = -0.5f * multiplier;
		config.fact[6] = (0.5f - 1.0f) * multiplier;
		config.fact[7] = -(0.5f - 1.0f) * multiplier;

		for (int index = 0; index < length4; ++index) {
			config.tcos[index] = std::cos(index * frequency);
			config.tsin[index] = std::cos(((length - index * 4) / 4.0f) * frequency);
		}
	}

	void Inverse_Complex_DFT(Complex *data, int length, float scale) const
	{
		if (length <= 0) {
			return;
		}

		int reversed_index = 0;
		for (int index = 1; index < length; ++index) {
			int bit = length >> 1;
			while ((reversed_index & bit) != 0) {
				reversed_index ^= bit;
				bit >>= 1;
			}
			reversed_index ^= bit;
			if (index < reversed_index) {
				std::swap(data[index], data[reversed_index]);
			}
		}

		for (int span = 2; span <= length; span <<= 1) {
			const int half_span = span >> 1;
			const float angle = static_cast<float>((2.0 * kPi) / static_cast<double>(span));
			const float wlen_re = std::cos(angle);
			const float wlen_im = std::sin(angle);

			for (int base = 0; base < length; base += span) {
				float w_re = 1.0f;
				float w_im = 0.0f;
				for (int offset = 0; offset < half_span; ++offset) {
					Complex &even = data[base + offset];
					Complex &odd = data[base + offset + half_span];

					const float odd_re = odd.re * w_re - odd.im * w_im;
					const float odd_im = odd.re * w_im + odd.im * w_re;
					const float even_re = even.re;
					const float even_im = even.im;

					even.re = even_re + odd_re;
					even.im = even_im + odd_im;
					odd.re = even_re - odd_re;
					odd.im = even_im - odd_im;

					const float next_w_re = w_re * wlen_re - w_im * wlen_im;
					w_im = w_re * wlen_im + w_im * wlen_re;
					w_re = next_w_re;
				}
			}
		}

		for (int index = 0; index < length; ++index) {
			data[index].re *= scale;
			data[index].im *= scale;
		}
	}

	void Inverse_RDFT_Wrapped(const float *packed_input, float *output, const RdftConfig &config) const
	{
		const int len = FrameLength;
		const int len2 = len >> 1;
		const int len4 = len >> 2;
		Complex *data = TransformComplexWork.data();

		data[0].re = packed_input[0];
		data[0].im = packed_input[len];
		for (int index = 1; index < len2; ++index) {
			data[index].re = packed_input[index * 2];
			data[index].im = packed_input[index * 2 + 1];
		}
		data[len2].re = packed_input[len];
		data[len2].im = 0.0f;

		const Complex dc = data[0];
		data[0].re = config.fact[0] * (dc.re + dc.im);
		data[0].im = config.fact[1] * (dc.re - dc.im);
		data[len4].re = config.fact[2] * data[len4].re;
		data[len4].im = config.fact[3] * data[len4].im;

		for (int index = 1; index < len4; ++index) {
			Complex even{};
			even.re = config.fact[4] * (data[index].re + data[len2 - index].re);
			even.im = config.fact[5] * (data[index].im - data[len2 - index].im);

			Complex odd{};
			odd.re = config.fact[6] * (data[index].im + data[len2 - index].im);
			odd.im = config.fact[7] * (data[index].re - data[len2 - index].re);

			Complex twiddled{};
			twiddled.re = odd.re * config.tcos[index] - odd.im * config.tsin[index];
			twiddled.im = odd.re * config.tsin[index] + odd.im * config.tcos[index];

			data[index].re = even.re + twiddled.re;
			data[index].im = twiddled.im - even.im;
			data[len2 - index].re = even.re - twiddled.re;
			data[len2 - index].im = twiddled.im + even.im;
		}

		Inverse_Complex_DFT(data, len2, config.fft_scale);
		for (int index = 0; index < len2; ++index) {
			output[index * 2] = data[index].re;
			output[index * 2 + 1] = data[index].im;
		}
	}

	void Inverse_DCT_III(const float *coefficients, float *output) const
	{
		float *work = TransformFloatWork.data();
		std::fill(work, work + FrameLength + 1, 0.0f);
		std::copy(coefficients, coefficients + FrameLength, work);
		work[FrameLength] = 2.0f * work[FrameLength - 1];

		for (int index = FrameLength - 2; index >= 2; index -= 2) {
			const float value1 = work[index];
			const float value2 = work[index - 1] - work[index + 1];
			work[index + 1] = DctExp[FrameLength - index] * value2 + DctExp[index] * value1;
			work[index] = DctExp[FrameLength - index] * value1 - DctExp[index] * value2;
		}

		Inverse_RDFT_Wrapped(work, output, DctRdft);
		for (int index = 0; index < (FrameLength >> 1); ++index) {
			const float in1 = output[index];
			const float in2 = output[FrameLength - index - 1];
			const float cosine = DctExp[FrameLength + index];
			const float sum = in1 + in2;
			const float diff = (in1 - in2) * cosine;
			output[index] = sum + diff;
			output[FrameLength - index - 1] = sum - diff;
		}
	}

	void Inverse_RDFT(const float *coefficients, float *output) const
	{
		float *work = TransformFloatWork.data();
		std::fill(work, work + FrameLength + 2, 0.0f);
		std::copy(coefficients, coefficients + FrameLength, work);
		for (int index = 2; index < FrameLength; index += 2) {
			work[index + 1] *= -1.0f;
		}
		work[FrameLength] = work[1];
		work[FrameLength + 1] = 0.0f;
		work[1] = 0.0f;
		Inverse_RDFT_Wrapped(work, output, AudioRdft);
	}

	bool UseDCT = false;
	bool VersionB = false;
	bool First = true;
	int SampleRate = 0;
	int PhysicalChannels = 0;
	int CodedChannels = 0;
	int FrameLength = 0;
	int OverlapLength = 0;
	int ValidFloatSamplesPerBlock = 0;
	int BlockSize = 0;
	int NumBands = 0;
	float Root = 0.0f;
	std::array<unsigned, 26> Bands{};
	std::array<std::vector<float>, MAX_DCT_CHANNELS> Previous{};
	std::array<std::vector<float>, MAX_DCT_CHANNELS> ChannelBuffers{};
	std::array<float, 96> QuantTable{};
	RdftConfig AudioRdft;
	RdftConfig DctRdft;
	std::vector<float> DctExp;
	mutable std::vector<float> TransformFloatWork;
	mutable std::vector<Complex> TransformComplexWork;
};

struct FrameIndexEntry
{
	uint32_t position = 0;
	uint32_t size = 0;
	bool keyframe = false;
};

struct AudioTrack
{
	uint32_t id = 0;
	int sample_rate = 0;
	int channels = 0;
	bool use_dct = false;
	AudioTrackDecoder decoder;
	SDL_AudioStream *stream = nullptr;

	~AudioTrack()
	{
		if (stream != nullptr) {
			SDL_DestroyAudioStream(stream);
			stream = nullptr;
		}
	}
};

class PlayerState
{
public:
	PlayerState(BINK *owner, std::vector<uint8_t> file_data)
		: Owner(owner), FileData(std::move(file_data))
	{
	}

	bool Initialize(const char *filename)
	{
		Filename = filename != nullptr ? filename : "";
		return Parse_Header() && Open_Audio_Streams();
	}

	uint32_t Wait()
	{
		if (!Started) {
			Started = true;
			StartTicks = SDL_GetTicksNS();
			return 0;
		}

		if (Owner->FrameRate == 0U) {
			return 0;
		}

		const uint64_t target = StartTicks +
			(static_cast<uint64_t>(CurrentFrame) * static_cast<uint64_t>(Owner->FrameRateDiv) * 1000000000ULL) /
			static_cast<uint64_t>(Owner->FrameRate);
		return SDL_GetTicksNS() < target ? 1U : 0U;
	}

	bool Decode_Frame()
	{
		if (FrameDecoded) {
			return true;
		}

		if (!Load_Current_Frame()) {
			return false;
		}

		for (size_t index = 0; index < AudioTracks.size(); ++index) {
			if (!AudioPackets[index].empty()) {
				std::vector<float> pcm;
				if (!AudioTracks[index].decoder.Decode_Packet(AudioPackets[index], pcm)) {
					Report_Error("failed decoding Bink audio track %zu in %s", index, Filename.c_str());
					return false;
				}

				if (!pcm.empty() && !SDL_PutAudioStreamData(AudioTracks[index].stream, pcm.data(),
					static_cast<int>(pcm.size() * sizeof(float)))) {
					Report_Error("failed queuing Bink audio for %s: %s", Filename.c_str(), SDL_GetError());
					return false;
				}
			}
		}

		if (!Video.Decode(VideoPacket.data(), VideoPacket.size())) {
			Report_Error("failed decoding Bink video frame %u in %s", Owner->FrameNum, Filename.c_str());
			return false;
		}

		FrameDecoded = true;
		return true;
	}

	void Copy_To_Buffer(void *dest, int32_t dest_pitch, uint32_t dest_height, uint32_t dest_x, uint32_t dest_y, uint32_t flags)
	{
		if ((flags & BINKCOPYNOSCALING) == 0U || (flags & BINKSURFACE565) == 0U) {
			Report_Error("unsupported BinkCopyToBuffer flags 0x%08x for %s", flags, Filename.c_str());
			return;
		}

		Video.Copy_To_RGB565(dest, dest_pitch, dest_height, dest_x, dest_y);
	}

	void Next_Frame()
	{
		if (CurrentFrame + 1U < FrameTable.size()) {
			++CurrentFrame;
			Owner->FrameNum = CurrentFrame + 1U;
			FrameLoaded = false;
			FrameDecoded = false;
			for (auto &packet : AudioPackets) {
				packet.clear();
			}
			VideoPacket.clear();
		}
	}

	bool Get_Frame_Planes(BINKFRAMEPLANES *planes) const
	{
		return planes != nullptr && FrameDecoded && Video.Get_Frame_Planes(*planes);
	}

private:
	bool Parse_Header()
	{
		if (FileData.size() < 44U) {
			Report_Error("Bink file is too small: %s", Filename.c_str());
			return false;
		}

		size_t header_offset = 0U;
		uint32_t tag = Read_LE32(FileData.data());
		if (tag == Make_Tag('S', 'M', 'U', 'S')) {
			while (header_offset + 4U <= FileData.size()) {
				tag = Read_LE32(FileData.data() + header_offset);
				if ((tag & 0x00FFFFFFU) == Make_Tag('B', 'I', 'K', '\0')) {
					break;
				}
				header_offset += 512U;
			}
		}

		if (header_offset + 44U > FileData.size()) {
			Report_Error("unable to locate Bink header in %s", Filename.c_str());
			return false;
		}

		CodecTag = Read_LE32(FileData.data() + header_offset);
		const uint32_t signature = CodecTag & 0x00FFFFFFU;
		const int revision = static_cast<int>(CodecTag >> 24);
		if (signature != Make_Tag('B', 'I', 'K', '\0') ||
			(revision != 'b' && revision != 'f' && revision != 'g' && revision != 'h' && revision != 'i' && revision != 'k')) {
			Report_Error("unsupported Bink revision 0x%08x in %s", CodecTag, Filename.c_str());
			return false;
		}

		const uint8_t *header = FileData.data() + header_offset;
		const uint32_t file_size = Read_LE32(header + 4) + 8U;
		const uint32_t frame_count = Read_LE32(header + 8);
		Owner->Width = Read_LE32(header + 20);
		Owner->Height = Read_LE32(header + 24);
		Owner->FrameRate = Read_LE32(header + 28);
		Owner->FrameRateDiv = Read_LE32(header + 32);
		VideoFlags = Read_LE32(header + 36);
		const uint32_t audio_track_count = Read_LE32(header + 40);

		if (Owner->Width == 0U || Owner->Height == 0U || Owner->FrameRate == 0U || Owner->FrameRateDiv == 0U || frame_count == 0U) {
			Report_Error("invalid Bink header in %s", Filename.c_str());
			return false;
		}

		size_t offset = header_offset + 44U;
		if (revision == 'k') {
			offset += 4U;
		}

		std::vector<int> sample_rates(audio_track_count);
		std::vector<uint16_t> audio_flags(audio_track_count);
		if (audio_track_count > 0U) {
			offset += static_cast<size_t>(audio_track_count) * 4U;
			if (offset > FileData.size()) {
				return false;
			}

			for (uint32_t track = 0; track < audio_track_count; ++track) {
				if (offset + 4U > FileData.size()) {
					return false;
				}
				sample_rates[track] = Read_LE16(FileData.data() + offset);
				audio_flags[track] = Read_LE16(FileData.data() + offset + 2U);
				offset += 4U;
			}

			AudioTracks.resize(audio_track_count);
			for (uint32_t track = 0; track < audio_track_count; ++track) {
				if (offset + 4U > FileData.size()) {
					return false;
				}
				AudioTracks[track].id = Read_LE32(FileData.data() + offset);
				AudioTracks[track].sample_rate = sample_rates[track];
				AudioTracks[track].channels = (audio_flags[track] & BINK_AUD_STEREO) != 0U ? 2 : 1;
				AudioTracks[track].use_dct = (audio_flags[track] & BINK_AUD_USEDCT) != 0U;
				offset += 4U;
			}
		}

		if (offset + 4U > FileData.size()) {
			return false;
		}

		FrameTable.resize(frame_count);
		uint32_t next_position = Read_LE32(FileData.data() + offset);
		offset += 4U;
		bool next_keyframe = true;
		for (uint32_t frame = 0; frame < frame_count; ++frame) {
			uint32_t position = next_position;
			const bool keyframe = next_keyframe;
			if (frame == frame_count - 1U) {
				next_position = file_size;
				next_keyframe = false;
			} else {
				if (offset + 4U > FileData.size()) {
					return false;
				}
				next_position = Read_LE32(FileData.data() + offset);
				offset += 4U;
				next_keyframe = (next_position & 1U) != 0U;
			}

			position &= ~1U;
			next_position &= ~1U;
			if (next_position <= position || next_position > FileData.size()) {
				return false;
			}

			FrameTable[frame].position = position;
			FrameTable[frame].size = next_position - position;
			FrameTable[frame].keyframe = keyframe;
		}

		Owner->Frames = frame_count;
		Owner->FrameNum = 1U;
		CurrentFrame = 0U;
		AudioPackets.resize(AudioTracks.size());

		for (AudioTrack &track : AudioTracks) {
			if (!track.decoder.Initialize(CodecTag, track.sample_rate, track.channels, track.use_dct)) {
				Report_Error("unsupported Bink audio configuration in %s", Filename.c_str());
				return false;
			}
		}

		if (!Video.Initialize(Owner->Width, Owner->Height, CodecTag, VideoFlags)) {
			return false;
		}

		return true;
	}

	bool Open_Audio_Streams()
	{
		if (AudioTracks.empty()) {
			return true;
		}

		if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0U && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
			Report_Error("failed to initialize SDL audio for %s: %s", Filename.c_str(), SDL_GetError());
			return false;
		}

		for (AudioTrack &track : AudioTracks) {
			SDL_AudioSpec spec{};
			spec.channels = track.channels;
			spec.freq = track.sample_rate;
			spec.format = SDL_AUDIO_F32;

			track.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
			if (track.stream == nullptr) {
				Report_Error("failed to open SDL audio stream for %s: %s", Filename.c_str(), SDL_GetError());
				return false;
			}
			if (!SDL_ResumeAudioStreamDevice(track.stream)) {
				Report_Error("failed to resume SDL audio stream for %s: %s", Filename.c_str(), SDL_GetError());
				return false;
			}
		}

		return true;
	}

	bool Load_Current_Frame()
	{
		if (FrameLoaded) {
			return true;
		}

		if (CurrentFrame >= FrameTable.size()) {
			return false;
		}

		const FrameIndexEntry &entry = FrameTable[CurrentFrame];
		if (static_cast<size_t>(entry.position + entry.size) > FileData.size()) {
			return false;
		}

		const uint8_t *cursor = FileData.data() + entry.position;
		uint32_t remaining = entry.size;
		for (size_t track_index = 0; track_index < AudioTracks.size(); ++track_index) {
			if (remaining < 4U) {
				return false;
			}

			const uint32_t audio_size = Read_LE32(cursor);
			cursor += 4;
			remaining -= 4U;

			if (audio_size > remaining) {
				return false;
			}

			AudioPackets[track_index].assign(cursor, cursor + audio_size);
			cursor += audio_size;
			remaining -= audio_size;
		}

		VideoPacket.assign(cursor, cursor + remaining);
		FrameLoaded = true;
		return true;
	}

	BINK *Owner = nullptr;
	std::string Filename;
	std::vector<uint8_t> FileData;
	uint32_t CodecTag = 0U;
	uint32_t VideoFlags = 0U;
	uint32_t CurrentFrame = 0U;
	uint64_t StartTicks = 0U;
	bool Started = false;
	bool FrameLoaded = false;
	bool FrameDecoded = false;
	std::vector<FrameIndexEntry> FrameTable;
	std::vector<AudioTrack> AudioTracks;
	std::vector<std::vector<uint8_t>> AudioPackets;
	std::vector<uint8_t> VideoPacket;
	VideoDecoder Video;
};

PlayerState *Get_Player_State(HBINK bink)
{
	return (bink != nullptr) ? static_cast<PlayerState *>(bink->Internal) : nullptr;
}

std::vector<uint8_t> Load_File(const char *filename)
{
	if (filename == nullptr || filename[0] == '\0' || _TheFileFactory == nullptr) {
		return {};
	}

	FileClass *file = _TheFileFactory->Get_File(filename);
	if (file == nullptr) {
		return {};
	}

	std::vector<uint8_t> data;
	if (!file->Is_Available()) {
		_TheFileFactory->Return_File(file);
		return data;
	}

	if (!file->Is_Open() && file->Open(FileClass::READ) == 0) {
		_TheFileFactory->Return_File(file);
		return data;
	}

	const int size = file->Size();
	if (size <= 0) {
		file->Close();
		_TheFileFactory->Return_File(file);
		return data;
	}

	data.resize(static_cast<size_t>(size));
	const int bytes_read = file->Read(data.data(), size);
	file->Close();
	_TheFileFactory->Return_File(file);
	if (bytes_read != size) {
		data.clear();
	}

	return data;
}

} // namespace

namespace FFBink
{

HBINK Open_Bink_Handle(const char *filename, uint32_t flags)
{
	(void)flags;

	const std::vector<uint8_t> file_data = Load_File(filename);
	if (file_data.empty()) {
		Report_Error("unable to open Bink file %s", filename != nullptr ? filename : "(null)");
		return nullptr;
	}

	auto *bink = new BINK{};
	auto player = std::make_unique<PlayerState>(bink, file_data);
	if (!player->Initialize(filename)) {
		delete bink;
		return nullptr;
	}

	bink->Internal = player.release();
	return bink;
}

void Close_Bink_Handle(HBINK bink)
{
	if (bink == nullptr) {
		return;
	}

	delete static_cast<PlayerState *>(bink->Internal);
	bink->Internal = nullptr;
	delete bink;
}

uint32_t Wait_For_Frame(HBINK bink)
{
	PlayerState *player = Get_Player_State(bink);
	return player != nullptr ? player->Wait() : 0U;
}

void Decode_Frame(HBINK bink)
{
	PlayerState *player = Get_Player_State(bink);
	if (player != nullptr) {
		player->Decode_Frame();
	}
}

void Advance_Frame(HBINK bink)
{
	PlayerState *player = Get_Player_State(bink);
	if (player != nullptr) {
		player->Next_Frame();
	}
}

void Copy_Frame_To_Buffer(HBINK bink, void *dest, int32_t dest_pitch, uint32_t dest_height,
	uint32_t dest_x, uint32_t dest_y, uint32_t flags)
{
	PlayerState *player = Get_Player_State(bink);
	if (player != nullptr) {
		player->Copy_To_Buffer(dest, dest_pitch, dest_height, dest_x, dest_y, flags);
	}
}

bool Get_Frame_Planes(HBINK bink, BINKFRAMEPLANES *planes)
{
	PlayerState *player = Get_Player_State(bink);
	return player != nullptr && player->Get_Frame_Planes(planes);
}

} // namespace FFBink

extern "C"
{

void BinkSoundUseDirectSound(uintptr_t direct_sound)
{
	(void)direct_sound;
}

HBINK BinkOpen(const char *filename, uint32_t flags)
{
	return FFBink::Open_Bink_Handle(filename, flags);
}

void BinkClose(HBINK bink)
{
	FFBink::Close_Bink_Handle(bink);
}

uint32_t BinkWait(HBINK bink)
{
	return FFBink::Wait_For_Frame(bink);
}

void BinkDoFrame(HBINK bink)
{
	FFBink::Decode_Frame(bink);
}

void BinkNextFrame(HBINK bink)
{
	FFBink::Advance_Frame(bink);
}

void BinkCopyToBuffer(HBINK bink, void *dest, int32_t dest_pitch, uint32_t dest_height,
	uint32_t dest_x, uint32_t dest_y, uint32_t flags)
{
	FFBink::Copy_Frame_To_Buffer(bink, dest, dest_pitch, dest_height, dest_x, dest_y, flags);
}

int32_t BinkGetFramePlanes(HBINK bink, BINKFRAMEPLANES *planes)
{
	return FFBink::Get_Frame_Planes(bink, planes) ? 1 : 0;
}

}

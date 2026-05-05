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

#include "BINKMovie.h"
#include "ww3d.h"
#include "ww3dformat.h"
#include "texture.h"
#include "surfaceclass.h"
#include "render2d.h"
#include "bgfxrenderer.h"
#include "Bink.h"
#include "rect.h"
#include "subtitlemanager.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
constexpr float kMovieReferenceAspect = 4.0f / 3.0f;

RectClass Get_Movie_Display_Rect()
{
	const RectClass &screen_rect = Render2DClass::Get_Screen_Resolution();
	const float screen_width = (screen_rect.Width() > 0.0f) ? screen_rect.Width() : 1.0f;
	const float screen_height = (screen_rect.Height() > 0.0f) ? screen_rect.Height() : 1.0f;

	float movie_width = screen_width;
	float movie_height = screen_height;
	if ((screen_width / screen_height) > kMovieReferenceAspect) {
		movie_width = screen_height * kMovieReferenceAspect;
	} else {
		movie_height = screen_width / kMovieReferenceAspect;
	}

	const float left = screen_rect.Left + ((screen_width - movie_width) * 0.5f);
	const float top = screen_rect.Top + ((screen_height - movie_height) * 0.5f);
	return RectClass(left, top, left + movie_width, top + movie_height);
}
}

class BINKMovieClass
{
	private:
		StringClass Filename;
		HBINK Bink;
		bool FrameChanged;
		bool FullRangeVideo;
		uint32_t TicksPerFrame;

#if !RENEGADE_WITH_BGFX_RENDERER
		unsigned TextureCount;
		struct TextureInfoStruct {
			TextureClass* Texture;
			int TextureWidth;
			int TextureHeight;
			int TextureLocX;
			int TextureLocY;
			RectClass UV;
			RectClass Rect;
		};

		TextureInfoStruct* TextureInfos;
		uint8_t* TempBuffer;
		Render2DClass Renderer;
#else
		struct MovieTextureState
		{
			bgfx::TextureHandle LumaTexture = BGFX_INVALID_HANDLE;
			bgfx::TextureHandle ChromaTexture = BGFX_INVALID_HANDLE;
			bgfx::TextureFormat::Enum LumaFormat = bgfx::TextureFormat::Count;
			bgfx::TextureFormat::Enum ChromaFormat = bgfx::TextureFormat::Count;
			std::vector<uint8_t> LumaUploadBuffer;
			std::vector<uint8_t> ChromaUploadBuffer;
		};

		MovieTextureState MovieTextures;
#endif
		SubTitleManagerClass* SubTitleManager;

	public:
		BINKMovieClass(const char* filename,const char* subtitlename,FontCharsClass* font);
		~BINKMovieClass();

		void Update();
		void Render();
		bool Is_Complete();
};

namespace
{
#if RENEGADE_WITH_BGFX_RENDERER
	constexpr uint64_t kMovieSamplerFlags =
		BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT;

	bool Is_Movie_Texture_Format_Supported(bgfx::TextureFormat::Enum format)
	{
		if (format == bgfx::TextureFormat::Count) {
			return false;
		}

		const bgfx::Caps *caps = bgfx::getCaps();
		if (caps == nullptr) {
			return bgfx::isTextureValid(0, false, 1, format, kMovieSamplerFlags);
		}

		return (caps->formats[format] & BGFX_CAPS_FORMAT_TEXTURE_2D) != 0
			&& bgfx::isTextureValid(0, false, 1, format, kMovieSamplerFlags);
	}

	void Pack_Luma_Upload(std::vector<uint8_t> &dest, const BINKFRAMEPLANES &planes, bgfx::TextureFormat::Enum format)
	{
		const size_t width = static_cast<size_t>(planes.LumaWidth);
		const size_t height = static_cast<size_t>(planes.LumaHeight);
		if (format == bgfx::TextureFormat::R8) {
			dest.resize(width * height);
			for (size_t y = 0; y < height; ++y) {
				const uint8_t *src_row = planes.YPlane + y * static_cast<size_t>(planes.YStride);
				std::memcpy(dest.data() + y * width, src_row, width);
			}
			return;
		}

		dest.resize(width * height * 4U);
		for (size_t y = 0; y < height; ++y) {
			const uint8_t *src_row = planes.YPlane + y * static_cast<size_t>(planes.YStride);
			uint8_t *dst_row = dest.data() + y * width * 4U;
			for (size_t x = 0; x < width; ++x) {
				const uint8_t value = src_row[x];
				dst_row[x * 4U + 0U] = value;
				dst_row[x * 4U + 1U] = value;
				dst_row[x * 4U + 2U] = value;
				dst_row[x * 4U + 3U] = 255U;
			}
		}
	}

	void Pack_Chroma_Upload(std::vector<uint8_t> &dest, const BINKFRAMEPLANES &planes, bgfx::TextureFormat::Enum format)
	{
		const size_t width = static_cast<size_t>(planes.ChromaWidth);
		const size_t height = static_cast<size_t>(planes.ChromaHeight);
		if (format == bgfx::TextureFormat::RG8) {
			dest.resize(width * height * 2U);
			for (size_t y = 0; y < height; ++y) {
				const uint8_t *u_row = planes.UPlane + y * static_cast<size_t>(planes.UStride);
				const uint8_t *v_row = planes.VPlane + y * static_cast<size_t>(planes.VStride);
				uint8_t *dst_row = dest.data() + y * width * 2U;
				for (size_t x = 0; x < width; ++x) {
					dst_row[x * 2U + 0U] = u_row[x];
					dst_row[x * 2U + 1U] = v_row[x];
				}
			}
			return;
		}

		dest.resize(width * height * 4U);
		for (size_t y = 0; y < height; ++y) {
			const uint8_t *u_row = planes.UPlane + y * static_cast<size_t>(planes.UStride);
			const uint8_t *v_row = planes.VPlane + y * static_cast<size_t>(planes.VStride);
			uint8_t *dst_row = dest.data() + y * width * 4U;
			for (size_t x = 0; x < width; ++x) {
				dst_row[x * 4U + 0U] = u_row[x];
				dst_row[x * 4U + 1U] = v_row[x];
				dst_row[x * 4U + 2U] = 0U;
				dst_row[x * 4U + 3U] = 255U;
			}
		}
	}
#endif
}


static BINKMovieClass* CurrentMovie;


void BINKMovie::Play(const char* filename,const char* subtitlename, FontCharsClass* font)
{
	if (CurrentMovie) {
		delete CurrentMovie;
		CurrentMovie = NULL;
	}

	CurrentMovie = new BINKMovieClass(filename,subtitlename,font);
}


void BINKMovie::Stop()
{
	if (CurrentMovie) {
		delete CurrentMovie;
		CurrentMovie = NULL;
	}
}


void BINKMovie::Update()
{
	if (CurrentMovie) {
		CurrentMovie->Update();
	}
}


void BINKMovie::Render()
{
	if (CurrentMovie) {
		CurrentMovie->Render();
	}
}


void BINKMovie::Init()
{
	BinkSoundUseDirectSound(0);
}


void BINKMovie::Shutdown()
{
	Stop();
}


bool BINKMovie::Is_Complete()
{
	if (CurrentMovie) {
		return CurrentMovie->Is_Complete();
	}

	return true;
}


// ----------------------------------------------------------------------------

BINKMovieClass::BINKMovieClass(const char* filename, const char* subtitlename, FontCharsClass* font)
	:
	Filename(filename),
	Bink(0),
	FrameChanged(true),
	FullRangeVideo(false),
	TicksPerFrame(0),
#if !RENEGADE_WITH_BGFX_RENDERER
	TextureCount(0),
	TextureInfos(NULL),
	TempBuffer(NULL),
#endif
	SubTitleManager(NULL)
{
	Bink = BinkOpen(Filename, 0);

	if (Bink == NULL) {
		return;
	}

#if !RENEGADE_WITH_BGFX_RENDERER
	TempBuffer = new uint8_t[Bink->Width * Bink->Height*2];

	WW3D::RenderCapabilitiesStruct capabilities;
	WW3D::Get_Current_Render_Capabilities(capabilities);
	unsigned poweroftwowidth = 1;

	while (poweroftwowidth < Bink->Width) {
		poweroftwowidth <<= 1;
	}

	unsigned poweroftwoheight = 1;
	
	while (poweroftwoheight < Bink->Height) {
		poweroftwoheight <<= 1;
	}

	if (capabilities.MaxTextureWidth != 0 && poweroftwowidth > capabilities.MaxTextureWidth) {
		poweroftwowidth = capabilities.MaxTextureWidth;
	}
	
	if (capabilities.MaxTextureHeight != 0 && poweroftwoheight > capabilities.MaxTextureHeight) {
		poweroftwoheight = capabilities.MaxTextureHeight;
	}

	TextureCount = 0;
	unsigned max_width = poweroftwowidth;
	unsigned max_height = poweroftwoheight;
	unsigned x, y;

	for (y = 0; y < Bink->Height; y += max_height-2) {		// Two pixels are lost due to duplicated edges to prevent bilinear artifacts
		for (x = 0; x < Bink->Width; x += max_width-2) {
			++TextureCount;
		}
	}

	TextureInfos = new TextureInfoStruct[TextureCount];
	unsigned cnt = 0;
	
	for (y = 0; y < Bink->Height; y += max_height-1) {
		for (x = 0; x < Bink->Width; x += max_width-1) {
			TextureInfos[cnt].Texture = new TextureClass(
				max_width, max_height, WW3D_FORMAT_R5G6B5,
				TextureClass::MIP_LEVELS_1, TextureClass::POOL_MANAGED, false);

			TextureInfos[cnt].TextureLocX = x;
			TextureInfos[cnt].TextureLocY = y;
			TextureInfos[cnt].TextureWidth = max_width;
			TextureInfos[cnt].UV.Right = float(max_width) / float(max_width);

			if ((TextureInfos[cnt].TextureWidth + x) > Bink->Width) {
				TextureInfos[cnt].TextureWidth = Bink->Width - x;
				TextureInfos[cnt].UV.Right = float(TextureInfos[cnt].TextureWidth - 1) / float(max_width);
			}

			TextureInfos[cnt].TextureHeight = max_height;
			TextureInfos[cnt].UV.Bottom = float(max_height) / float(max_height);

			if ((TextureInfos[cnt].TextureHeight + y) > Bink->Height) {
				TextureInfos[cnt].TextureHeight = Bink->Height - y;
				TextureInfos[cnt].UV.Bottom = float(TextureInfos[cnt].TextureHeight + 1) / float(max_height);
			}

			TextureInfos[cnt].UV.Left = 1.0f / float(max_width);
			TextureInfos[cnt].UV.Top = 1.0f / float(max_height);

			TextureInfos[cnt].Rect.Left = float(TextureInfos[cnt].TextureLocX) / float(Bink->Width);
			TextureInfos[cnt].Rect.Top = float(TextureInfos[cnt].TextureLocY) / float(Bink->Height);
			TextureInfos[cnt].Rect.Right = float(TextureInfos[cnt].TextureLocX + TextureInfos[cnt].TextureWidth) / float(Bink->Width);
			TextureInfos[cnt].Rect.Bottom = float(TextureInfos[cnt].TextureLocY + TextureInfos[cnt].TextureHeight) / float(Bink->Height);

			++cnt;
		}
	}

	Renderer.Reset();
#else
	MovieTextures.LumaFormat = Is_Movie_Texture_Format_Supported(bgfx::TextureFormat::R8)
		? bgfx::TextureFormat::R8
		: bgfx::TextureFormat::RGBA8;
	MovieTextures.ChromaFormat = Is_Movie_Texture_Format_Supported(bgfx::TextureFormat::RG8)
		? bgfx::TextureFormat::RG8
		: bgfx::TextureFormat::RGBA8;

	MovieTextures.LumaTexture = bgfx::createTexture2D(
		static_cast<uint16_t>(Bink->Width),
		static_cast<uint16_t>(Bink->Height),
		false,
		1,
		MovieTextures.LumaFormat,
		kMovieSamplerFlags);

	const uint16_t chroma_width = static_cast<uint16_t>((Bink->Width + 1U) / 2U);
	const uint16_t chroma_height = static_cast<uint16_t>((Bink->Height + 1U) / 2U);
	MovieTextures.ChromaTexture = bgfx::createTexture2D(
		chroma_width,
		chroma_height,
		false,
		1,
		MovieTextures.ChromaFormat,
		kMovieSamplerFlags);
#endif

	// Calculate the time per frame of video
	uint32_t rate = (Bink->FrameRate / Bink->FrameRateDiv);
	TicksPerFrame = (60 / rate);

	if (subtitlename && font) {
		SubTitleManager = SubTitleManagerClass::Create(filename, subtitlename, font);
	}
}


BINKMovieClass::~BINKMovieClass()
{
	if (Bink == NULL) {
		return;
	}

	if (Bink) {
		BinkClose(Bink);
	}

#if !RENEGADE_WITH_BGFX_RENDERER
	delete[] TempBuffer;

	if (TextureInfos) {
		for (unsigned t = 0; t < TextureCount; ++t) {
			REF_PTR_RELEASE(TextureInfos[t].Texture);
		}

		delete[] TextureInfos;
	}
#else
	auto destroy_texture = [](bgfx::TextureHandle &handle) {
		BgfxRenderer::Destroy_Texture_Handle(handle);
	};
	destroy_texture(MovieTextures.LumaTexture);
	destroy_texture(MovieTextures.ChromaTexture);
#endif

	if (SubTitleManager) {
		delete SubTitleManager;
	}
}


void BINKMovieClass::Update()
{
	if (!Bink) {
		return;
	}

	FrameChanged |= !BinkWait(Bink);
}


static void Upload_Texture_From_Buffer(
	SurfaceClass *surface,
	int texture_loc_x,
	int texture_loc_y,
	const uint8_t *source_buffer,
	int source_width,
	int source_height)
{
	if (surface == NULL || source_buffer == NULL || source_width <= 0 || source_height <= 0) {
		return;
	}

	SurfaceClass::SurfaceDescription desc;
	surface->Get_Description(desc);

	int pitch = 0;
	uint8_t *dest = static_cast<uint8_t *>(surface->Lock(&pitch));
	if (dest == NULL) {
		return;
	}

	const uint16_t *source_pixels = reinterpret_cast<const uint16_t *>(source_buffer);
	for (unsigned y = 0; y < desc.Height; ++y) {
		const int source_y = std::clamp(texture_loc_y + static_cast<int>(y) - 1, 0, source_height - 1);
		const uint16_t *source_row = source_pixels + static_cast<size_t>(source_y) * static_cast<size_t>(source_width);
		uint16_t *dest_row = reinterpret_cast<uint16_t *>(dest + static_cast<size_t>(y) * static_cast<size_t>(pitch));

		const unsigned left_repeat = static_cast<unsigned>(std::max(0, 1 - texture_loc_x));
		unsigned write_x = 0;
		for (; write_x < left_repeat && write_x < desc.Width; ++write_x) {
			dest_row[write_x] = source_row[0];
		}

		const int source_start_x = std::max(texture_loc_x - 1, 0);
		const unsigned remaining_width = desc.Width - write_x;
		const unsigned copy_width = static_cast<unsigned>(std::max(0, std::min<int>(remaining_width, source_width - source_start_x)));
		if (copy_width > 0U) {
			std::memcpy(dest_row + write_x, source_row + source_start_x, static_cast<size_t>(copy_width) * sizeof(uint16_t));
			write_x += copy_width;
		}

		const uint16_t last_pixel = source_row[source_width - 1];
		for (; write_x < desc.Width; ++write_x) {
			dest_row[write_x] = last_pixel;
		}
	}

	surface->Unlock();
}


void BINKMovieClass::Render()
{
	if (!Bink) {
		return;
	}

	const RectClass screen_rect = Render2DClass::Get_Screen_Resolution();
	const RectClass movie_rect = Get_Movie_Display_Rect();

	// decompress a frame
	if (FrameChanged) {
		BinkDoFrame(Bink);
		FrameChanged = false;

#if RENEGADE_WITH_BGFX_RENDERER
		BINKFRAMEPLANES planes{};
		if (BinkGetFramePlanes(Bink, &planes) != 0
			&& bgfx::isValid(MovieTextures.LumaTexture)
			&& bgfx::isValid(MovieTextures.ChromaTexture)) {
			FullRangeVideo = (planes.Flags & BINKFRAMEPLANES_FULL_RANGE) != 0U;
			Pack_Luma_Upload(MovieTextures.LumaUploadBuffer, planes, MovieTextures.LumaFormat);
			Pack_Chroma_Upload(MovieTextures.ChromaUploadBuffer, planes, MovieTextures.ChromaFormat);

			bgfx::updateTexture2D(
				MovieTextures.LumaTexture,
				0,
				0,
				0,
				0,
				static_cast<uint16_t>(planes.LumaWidth),
				static_cast<uint16_t>(planes.LumaHeight),
				bgfx::copy(MovieTextures.LumaUploadBuffer.data(), static_cast<uint32_t>(MovieTextures.LumaUploadBuffer.size())));
			bgfx::updateTexture2D(
				MovieTextures.ChromaTexture,
				0,
				0,
				0,
				0,
				static_cast<uint16_t>(planes.ChromaWidth),
				static_cast<uint16_t>(planes.ChromaHeight),
				bgfx::copy(MovieTextures.ChromaUploadBuffer.data(), static_cast<uint32_t>(MovieTextures.ChromaUploadBuffer.size())));
		}
#else
		BinkCopyToBuffer(Bink, TempBuffer, Bink->Width * 2, Bink->Height, 0, 0, BINKSURFACE565|BINKCOPYNOSCALING);

		for (unsigned t = 0; t < TextureCount; ++t) {
			SurfaceClass *surface = TextureInfos[t].Texture->Get_Surface_Level(0);
			if (surface != NULL) {
				Upload_Texture_From_Buffer(
					surface,
					TextureInfos[t].TextureLocX,
					TextureInfos[t].TextureLocY,
					TempBuffer,
					Bink->Width,
					Bink->Height);
				surface->Release_Ref();
			}
		}
#endif

		if (Bink->FrameNum < Bink->Frames) // goto the next if not on the last
			BinkNextFrame(Bink);
	}

#if RENEGADE_WITH_BGFX_RENDERER
	if (bgfx::isValid(MovieTextures.LumaTexture) && bgfx::isValid(MovieTextures.ChromaTexture)) {
		const float safe_screen_width = (screen_rect.Width() > 0.0f) ? screen_rect.Width() : 1.0f;
		const float safe_screen_height = (screen_rect.Height() > 0.0f) ? screen_rect.Height() : 1.0f;
		const float left = (((movie_rect.Left - screen_rect.Left) / safe_screen_width) * 2.0f) - 1.0f;
		const float right = (((movie_rect.Right - screen_rect.Left) / safe_screen_width) * 2.0f) - 1.0f;
		const float top = 1.0f - (((movie_rect.Top - screen_rect.Top) / safe_screen_height) * 2.0f);
		const float bottom = 1.0f - (((movie_rect.Bottom - screen_rect.Top) / safe_screen_height) * 2.0f);
		const uint32_t diffuse = 0xffffffffu;
		const WW3D::OverlaySubmitVertex vertices[4] = {
			{left,  top,    0.0f, diffuse, 0.0f, 0.0f},
			{left,  bottom, 0.0f, diffuse, 0.0f, 1.0f},
			{right, top,    0.0f, diffuse, 1.0f, 0.0f},
			{right, bottom, 0.0f, diffuse, 1.0f, 1.0f},
		};
		static const uint16_t indices[6] = {0, 1, 2, 2, 1, 3};

		OverlayYUVSubmitDesc submission;
		submission.Vertices = vertices;
		submission.VertexCount = 4;
		submission.Indices = indices;
		submission.IndexCount = 6;
		submission.LumaTexture = MovieTextures.LumaTexture;
		submission.ChromaTexture = MovieTextures.ChromaTexture;
		submission.SamplerFlags = kMovieSamplerFlags;
		submission.FullRangeVideo = FullRangeVideo;
		WW3D::Submit_YUV_Overlay(submission);
	}
#else
	for (unsigned t = 0; t < TextureCount; ++t) {
		Renderer.Reset();
		Renderer.Set_Texture(TextureInfos[t].Texture);
		Renderer.Set_Coordinate_Range(screen_rect);

		RectClass rect = TextureInfos[t].Rect;
		rect.Left = movie_rect.Left + (rect.Left * movie_rect.Width());
		rect.Top = movie_rect.Top + (rect.Top * movie_rect.Height());
		rect.Right = movie_rect.Left + (rect.Right * movie_rect.Width());
		rect.Bottom = movie_rect.Top + (rect.Bottom * movie_rect.Height());
		Renderer.Add_Quad(rect, TextureInfos[t].UV, 0xffffffff);
		Renderer.Render();
	}
#endif

	if (SubTitleManager) {
		uint32_t movieTime = (Bink->FrameNum * TicksPerFrame);
		SubTitleManager->Process(movieTime);
		SubTitleManager->Render();
	}
}


bool BINKMovieClass::Is_Complete()
{
	if (!Bink) return true;
	return (Bink->FrameNum>=Bink->Frames);
}

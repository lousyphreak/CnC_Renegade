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

#include "render2dsentence.h"

#include "surfaceclass.h"
#include "texture.h"

#include <algorithm>

namespace {

DynamicVectorClass<Render2DSentenceClass *> &Get_Live_Sentence_Renderers()
{
	static DynamicVectorClass<Render2DSentenceClass *> renderers;
	return renderers;
}

int Headless_Char_Width(FontCharsClass * font, WCHAR ch)
{
	if (font == NULL) {
		return 0;
	}

	const int point_size = std::max(font->Get_Char_Height(), 1);
	if (ch == L' ') {
		return std::max(point_size / 3, 1);
	}

	if (ch == L'\t') {
		return std::max(point_size, 1);
	}

	return std::max((point_size * 3) / 5, 1);
}

Vector2 Compute_Extents(FontCharsClass * font, const WCHAR * text, float wrap_width, int * row_count)
{
	if (row_count != NULL) {
		*row_count = 0;
	}

	if (font == NULL || text == NULL) {
		return Vector2(0.0f, 0.0f);
	}

	const float line_height = static_cast<float>(std::max(font->Get_Char_Height(), 1));
	float current_width = 0.0f;
	float max_width = 0.0f;
	int rows = 1;

	for (const WCHAR * cursor = text; *cursor != 0; ++cursor) {
		const WCHAR ch = *cursor;
		if (ch == L'\n') {
			max_width = std::max(max_width, current_width);
			current_width = 0.0f;
			++rows;
			continue;
		}

		const float spacing = static_cast<float>(font->Get_Char_Spacing(ch));
		if (wrap_width > 0.0f && current_width > 0.0f && (current_width + spacing) >= wrap_width) {
			max_width = std::max(max_width, current_width);
			current_width = 0.0f;
			++rows;
		}

		current_width += spacing;
	}

	max_width = std::max(max_width, current_width);

	if (row_count != NULL) {
		*row_count = rows;
	}

	return Vector2(max_width, rows * line_height);
}

} // namespace

FontCharsClass::FontCharsClass(void) :
	CurrPixelOffset(0),
	CharHeight(0),
	PointSize(0),
	OldGDIFont(NULL),
	OldGDIBitmap(NULL),
	GDIBitmap(NULL),
	GDIFont(NULL),
	GDIBitmapBits(NULL),
	MemDC(NULL),
	UnicodeCharArray(NULL),
	FirstUnicodeChar(0xFFFF),
	LastUnicodeChar(0),
	IsBold(false),
	BufferList(sizeof(PreAllocatedBufferList) / sizeof(uint16_t *), PreAllocatedBufferList)
{
	::memset(ASCIICharArray, 0, sizeof(ASCIICharArray));
}

FontCharsClass::~FontCharsClass()
{
	for (int index = 0; index < BufferList.Count(); ++index) {
		delete [] BufferList[index];
	}
	BufferList.Reset_Active();
}

void FontCharsClass::Initialize_GDI_Font(const char * font_name, int point_size, bool is_bold)
{
	Name.Format("%s%d", font_name != NULL ? font_name : "", point_size);
	GDIFontName = (font_name != NULL) ? font_name : "";
	PointSize = std::max(point_size, 1);
	CharHeight = PointSize;
	IsBold = is_bold;
}

bool FontCharsClass::Is_Font(const char * font_name, int point_size, bool is_bold)
{
	return GDIFontName.Compare_No_Case(font_name != NULL ? font_name : "") == 0 && point_size == PointSize && is_bold == IsBold;
}

int FontCharsClass::Get_Char_Width(WCHAR ch)
{
	return Headless_Char_Width(this, ch);
}

int FontCharsClass::Get_Char_Spacing(WCHAR ch)
{
	const int width = Get_Char_Width(ch);
	return width > 0 ? width + 1 : 0;
}

void FontCharsClass::Blit_Char(WCHAR ch, uint16_t * dest_ptr, int dest_stride, int x, int y)
{
	if (dest_ptr == NULL || dest_stride <= 0) {
		return;
	}

	const int width = Get_Char_Width(ch);
	if (width <= 0 || CharHeight <= 0) {
		return;
	}

	const int stride = dest_stride >> 1;
	dest_ptr += (stride * y) + x;
	for (int row = 0; row < CharHeight; ++row) {
		for (int col = 0; col < width; ++col) {
			dest_ptr[col] = 0xFFFF;
		}
		dest_ptr += stride;
	}
}

Render2DSentenceClass::Render2DSentenceClass(void) :
	SentenceData(),
	PendingSurfaces(),
	Renderers(sizeof(PreAllocatedRenderers) / sizeof(RendererDataStruct), PreAllocatedRenderers),
	Font(NULL),
	BaseLocation(0.0f, 0.0f),
	Location(0.0f, 0.0f),
	Cursor(0.0f, 0.0f),
	TextureOffset(0, 0),
	TextureStartX(0),
	CurrTextureSize(0),
	TextureSizeHint(0),
	CurSurface(NULL),
	MonoSpaced(false),
	WrapWidth(0.0f),
	TabStop(5.0f),
	ClipRect(0, 0, 0, 0),
	DrawExtents(0, 0, 0, 0),
	IsClippedEnabled(false),
	LockedPtr(NULL),
	LockedStride(0),
	CurTexture(NULL),
	Shader(Render2DClass::Get_Default_Shader()),
	TrackedFontID(-1)
{
	Get_Live_Sentence_Renderers().Add(this);
}

Render2DSentenceClass::~Render2DSentenceClass(void)
{
	const int id = Get_Live_Sentence_Renderers().ID(this);
	if (id != -1) {
		Get_Live_Sentence_Renderers().Delete(id);
	}

	REF_PTR_RELEASE(Font);
	Reset();
}

void Render2DSentenceClass::Render(void)
{
	for (int index = 0; index < Renderers.Count(); ++index) {
		if (Renderers[index].Renderer != NULL) {
			Renderers[index].Renderer->Render();
		}
	}
}

void Render2DSentenceClass::Reset(void)
{
	for (int index = 0; index < SentenceData.Count(); ++index) {
		REF_PTR_RELEASE(SentenceData[index].Surface);
	}
	SentenceData.Reset_Active();

	for (int index = 0; index < PendingSurfaces.Count(); ++index) {
		REF_PTR_RELEASE(PendingSurfaces[index].Surface);
	}
	PendingSurfaces.Reset_Active();

	for (int index = 0; index < Renderers.Count(); ++index) {
		delete Renderers[index].Renderer;
	}
	Renderers.Reset_Active();

	REF_PTR_RELEASE(CurSurface);
	REF_PTR_RELEASE(CurTexture);
	LockedPtr = NULL;
	LockedStride = 0;
	Cursor.Set(0.0f, 0.0f);
	DrawExtents.Set(0, 0, 0, 0);
}

void Render2DSentenceClass::Reset_Polys(void)
{
	for (int index = 0; index < Renderers.Count(); ++index) {
		if (Renderers[index].Renderer != NULL) {
			Renderers[index].Renderer->Reset();
		}
	}
}

void Render2DSentenceClass::Set_Font(FontCharsClass * font)
{
	Reset();
	TrackedFontID = -1;
	REF_PTR_SET(Font, font);
}

void Render2DSentenceClass::Refresh_Tracked_Fonts(FontCharsClass *const * fonts, int font_count)
{
	if (fonts == NULL || font_count <= 0) {
		return;
	}

	DynamicVectorClass<Render2DSentenceClass *> &renderers = Get_Live_Sentence_Renderers();
	for (int index = 0; index < renderers.Count(); ++index) {
		Render2DSentenceClass *renderer = renderers[index];
		if (renderer == NULL) {
			continue;
		}

		const int tracked_font_id = renderer->TrackedFontID;
		if (tracked_font_id >= 0 && tracked_font_id < font_count) {
			renderer->Set_Font(fonts[tracked_font_id]);
			renderer->TrackedFontID = tracked_font_id;
		}
	}
}

void Render2DSentenceClass::Set_Location(const Vector2 & loc)
{
	Location = loc;
}

void Render2DSentenceClass::Set_Tabstop(float stop)
{
	TabStop = stop;
}

void Render2DSentenceClass::Set_Base_Location(const Vector2 & loc)
{
	const Vector2 delta = loc - BaseLocation;
	BaseLocation = loc;
	for (int index = 0; index < Renderers.Count(); ++index) {
		if (Renderers[index].Renderer != NULL) {
			Renderers[index].Renderer->Move(delta);
		}
	}
}

void Render2DSentenceClass::Make_Additive(void)
{
	Shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ONE);
	Shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_ONE);
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Secondary_Gradient(ShaderClass::SECONDARY_GRADIENT_DISABLE);
	Set_Shader(Shader);
}

void Render2DSentenceClass::Set_Shader(ShaderClass shader)
{
	Shader = shader;
	for (int index = 0; index < Renderers.Count(); ++index) {
		if (Renderers[index].Renderer != NULL) {
			*(Renderers[index].Renderer->Get_Shader()) = Shader;
		}
	}
}

Vector2 Render2DSentenceClass::Get_Text_Extents(const WCHAR * text)
{
	return Compute_Extents(Font, text, 0.0f, NULL);
}

Vector2 Render2DSentenceClass::Get_Formatted_Text_Extents(const WCHAR * text, int * row_count)
{
	return Compute_Extents(Font, text, WrapWidth, row_count);
}

const WCHAR * Render2DSentenceClass::Find_Row_Start(const WCHAR * text, int row_index)
{
	if (text == NULL || row_index <= 0 || Font == NULL) {
		return text;
	}

	int current_row = 0;
	float current_width = 0.0f;

	for (const WCHAR * cursor = text; *cursor != 0; ++cursor) {
		const WCHAR ch = *cursor;
		if (ch == L'\n') {
			++current_row;
			current_width = 0.0f;
			if (current_row == row_index) {
				return cursor + 1;
			}
			continue;
		}

		const float spacing = static_cast<float>(Font->Get_Char_Spacing(ch));
		if (WrapWidth > 0.0f && current_width > 0.0f && (current_width + spacing) >= WrapWidth) {
			++current_row;
			current_width = 0.0f;
			if (current_row == row_index) {
				return cursor;
			}
		}

		current_width += spacing;
	}

	return NULL;
}

void Render2DSentenceClass::Build_Sentence(const WCHAR * text)
{
	const Vector2 extents = Compute_Extents(Font, text, WrapWidth, NULL);
	DrawExtents.Set(0.0f, 0.0f, extents.X, extents.Y);
}

void Render2DSentenceClass::Draw_Sentence(uint32_t)
{
	const float width = DrawExtents.Width();
	const float height = DrawExtents.Height();
	DrawExtents.Set(Location.X, Location.Y, Location.X + width, Location.Y + height);
}

void Render2DSentenceClass::Force_Alpha(float alpha)
{
	for (int index = 0; index < Renderers.Count(); ++index) {
		if (Renderers[index].Renderer != NULL) {
			Renderers[index].Renderer->Force_Alpha(alpha);
		}
	}
}

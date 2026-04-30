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
 *                     $Archive:: /Commando/Code/ww3d2/render2dsentence.cpp                   $*
 *                                                                                             *
 *                       $Author:: Steve_t                  $*
 *                                                                                             *
 *								$Modtime:: 8/26/02 3:18p                                              $*
 *                                                                                             *
 *                    $Revision:: 27                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "render2dsentence.h"
#include "surfaceclass.h"
#include "texture.h"
#include "ww3d.h"
#include "wwprofile.h"
#include "wwmemlog.h"
#include "dx8wrapper.h"
#include "../wwlib/osdep.h"
#include "ffactory.h"
#include "wwfile.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


////////////////////////////////////////////////////////////////////////////////////
//	Local constants
////////////////////////////////////////////////////////////////////////////////////
const int CHAR_TEXTURE_SIZE	= 256;
const int CHAR_BUFFER_LEN		= 32768;


// Macros.
// NOTE 0: Word wrap logic does not apply to Han characters (Chinese, Japanese & Korean).
//			  Therefore treat each of these characters as a word which can be preceeded by a line break.
// NOTE 1: This is a simplification. Some Korean characters should not be line break characters.
#define IS_BREAK_CHAR(ch) ((ch == L' ') || ((ch >= 0x3000) && (ch <= 0xdfff)))

namespace
{
constexpr float LogicalDpi = 96.0f;
constexpr const char *FontSearchRoots[] = {
	"/usr/share/fonts",
	"/usr/local/share/fonts",
};
std::vector<std::string> RegisteredFontFiles;
struct FontCandidateInfo
{
	std::string Path;
	std::string Stem;
	std::vector<std::string> Aliases;
};
std::vector<FontCandidateInfo> CachedSystemFontCandidates;
bool CachedSystemFontCandidatesScanned = false;
std::unordered_map<std::string, std::vector<std::string> > CachedRegisteredFontAliases;
std::unordered_map<std::string, std::string> CachedResolvedFontPaths;

std::string Normalize_Font_Family(const std::string &text)
{
	std::string normalized;
	normalized.reserve(text.size());

	for (unsigned char ch : text) {
		if (std::isalnum(ch) != 0) {
			normalized.push_back(static_cast<char>(std::tolower(ch)));
		}
	}

	return normalized;
}

bool Font_File_Extension_Matches(const std::string &path)
{
	std::string extension = renegade_osdep::Get_Path_Extension(path);
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});

	return extension == ".ttf" || extension == ".otf" || extension == ".ttc";
}

bool Filename_Indicates_Bold(const std::string &text)
{
	return text.find("bold") != std::string::npos || text.find("demi") != std::string::npos || text.find("black") != std::string::npos;
}

bool Filename_Indicates_Italic(const std::string &text)
{
	return text.find("italic") != std::string::npos || text.find("oblique") != std::string::npos;
}

bool Stem_Looks_Script_Specific(const std::string &stem)
{
	static const char *tokens[] = {
		"arabic", "armenian", "bengali", "cherokee", "cjk", "devanagari", "ethiopic", "georgian",
		"gujarati", "gurmukhi", "hebrew", "jp", "japanese", "kannada", "khmer", "kr", "korean",
		"lao", "malayalam", "myanmar", "oriya", "sinhala", "tamil", "telugu", "thai", "tibetan"
	};

	for (const char *token : tokens) {
		if (stem.find(token) != std::string::npos) {
			return true;
		}
	}

	return false;
}

int Score_Font_Candidate(const std::string &stem, const std::vector<std::string> &families, bool is_bold)
{
	int best_score = -1;
	const bool candidate_is_bold = Filename_Indicates_Bold(stem);
	const bool candidate_is_italic = Filename_Indicates_Italic(stem);

	for (size_t index = 0; index < families.size(); ++index) {
		if (stem.find(families[index]) == std::string::npos) {
			continue;
		}

		int score = 100 - static_cast<int>(index * 10);
		if (candidate_is_bold == is_bold) {
			score += 25;
		}
		if (candidate_is_italic) {
			score -= is_bold ? 10 : 25;
		}
		if (Stem_Looks_Script_Specific(stem)) {
			score -= 60;
		}

		best_score = std::max(best_score, score);
	}

	return best_score;
}

int Score_Font_Aliases(const std::vector<std::string> &aliases, const std::vector<std::string> &families, bool is_bold, const std::string &stem)
{
	int best_score = -1;
	const bool candidate_is_bold = Filename_Indicates_Bold(stem);
	const bool candidate_is_italic = Filename_Indicates_Italic(stem);

	for (size_t family_index = 0; family_index < families.size(); ++family_index) {
		for (const std::string &alias : aliases) {
			if (alias != families[family_index]) {
				continue;
			}

			int score = 200 - static_cast<int>(family_index * 10);
			if (candidate_is_bold == is_bold) {
				score += 25;
			}
			if (candidate_is_italic) {
				score -= is_bold ? 10 : 25;
			}

			best_score = std::max(best_score, score);
		}
	}

	return best_score;
}

bool Read_Binary_File(const std::string &path, std::vector<unsigned char> &contents)
{
	if (!renegade_osdep::Read_Entire_File(path, contents)) {
		return false;
	}

	return !contents.empty();
}

bool Resolve_Font_Path(const char *font_name, bool is_bold, std::string &resolved_path);

bool Read_File_Data(FileClass &file, std::vector<unsigned char> &contents)
{
	if (!file.Is_Available()) {
		return false;
	}

	if (!file.Is_Open() && file.Open(FileClass::READ) == 0) {
		return false;
	}

	const int file_size = file.Size();
	if (file_size <= 0) {
		file.Close();
		return false;
	}

	contents.resize(static_cast<size_t>(file_size));
	const int bytes_read = file.Read(contents.data(), file_size);
	file.Close();
	return bytes_read == file_size;
}

uint16_t Read_Big_Endian_U16(const unsigned char *data)
{
	return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]));
}

uint32_t Read_Big_Endian_U32(const unsigned char *data)
{
	return
		(static_cast<uint32_t>(data[0]) << 24) |
		(static_cast<uint32_t>(data[1]) << 16) |
		(static_cast<uint32_t>(data[2]) << 8) |
		static_cast<uint32_t>(data[3]);
}

std::string Decode_Font_Name_String(const unsigned char *data, size_t length, uint16_t platform_id)
{
	std::string decoded;

	if ((platform_id == 0 || platform_id == 3) && (length % 2) == 0) {
		decoded.reserve(length / 2);
		for (size_t index = 0; index < length; index += 2) {
			const uint16_t code_unit = Read_Big_Endian_U16(data + index);
			if (code_unit == 0) {
				continue;
			}

			if (code_unit <= 0x7F) {
				decoded.push_back(static_cast<char>(code_unit));
			}
		}
	} else {
		decoded.assign(reinterpret_cast<const char *>(data), length);
	}

	return decoded;
}

void Extract_Font_Name_Aliases(const std::vector<unsigned char> &font_data, std::vector<std::string> &aliases)
{
	if (font_data.size() < 12) {
		return;
	}

	const uint16_t table_count = Read_Big_Endian_U16(font_data.data() + 4);
	size_t table_offset = 12;
	uint32_t name_offset = 0;
	uint32_t name_length = 0;

	for (uint16_t index = 0; index < table_count && (table_offset + 16) <= font_data.size(); ++index, table_offset += 16) {
		const unsigned char *record = font_data.data() + table_offset;
		if (std::memcmp(record, "name", 4) == 0) {
			name_offset = Read_Big_Endian_U32(record + 8);
			name_length = Read_Big_Endian_U32(record + 12);
			break;
		}
	}

	if (name_offset == 0 || name_length < 6 || (static_cast<size_t>(name_offset) + static_cast<size_t>(name_length)) > font_data.size()) {
		return;
	}

	const unsigned char *name_table = font_data.data() + name_offset;
	const uint16_t record_count = Read_Big_Endian_U16(name_table + 2);
	const uint16_t string_offset = Read_Big_Endian_U16(name_table + 4);
	size_t record_offset = 6;

	for (uint16_t index = 0; index < record_count && (record_offset + 12) <= name_length; ++index, record_offset += 12) {
		const unsigned char *record = name_table + record_offset;
		const uint16_t platform_id = Read_Big_Endian_U16(record + 0);
		const uint16_t name_id = Read_Big_Endian_U16(record + 6);
		const uint16_t value_length = Read_Big_Endian_U16(record + 8);
		const uint16_t value_offset = Read_Big_Endian_U16(record + 10);

		if (name_id != 1 && name_id != 4 && name_id != 6) {
			continue;
		}

		const size_t value_start = static_cast<size_t>(string_offset) + static_cast<size_t>(value_offset);
		if ((value_start + value_length) > name_length) {
			continue;
		}

		const std::string decoded = Decode_Font_Name_String(name_table + value_start, value_length, platform_id);
		const std::string normalized = Normalize_Font_Family(decoded);
		if (!normalized.empty()) {
			aliases.push_back(normalized);
		}
	}
}

bool Read_Resource_Font_File(const char *filename, std::vector<unsigned char> &contents)
{
	if (filename == nullptr || *filename == '\0' || _TheFileFactory == NULL) {
		return false;
	}

	file_auto_ptr file(_TheFileFactory, filename);
	if (file.get() == NULL) {
		return false;
	}

	return Read_File_Data(*file, contents);
}

std::vector<std::string> Build_Requested_Font_Families(const char *font_name)
{
	std::vector<std::string> requested_families;
	requested_families.emplace_back(Normalize_Font_Family(font_name));

	if (requested_families[0].find("arial") != std::string::npos) {
		requested_families.emplace_back("arial");
		requested_families.emplace_back("arialmt");
		requested_families.emplace_back("notosans");
		requested_families.emplace_back("liberationsans");
		requested_families.emplace_back("dejavusans");
	}

	if (requested_families[0].find("regatta") != std::string::npos) {
		requested_families.emplace_back("regatta");
		requested_families.emplace_back("regattacondensedlet");
		requested_families.emplace_back("notosans");
		requested_families.emplace_back("liberationsansnarrow");
		requested_families.emplace_back("liberationsans");
		requested_families.emplace_back("dejavusans");
	}

	requested_families.emplace_back("notosans");
	requested_families.emplace_back("liberationsans");
	requested_families.emplace_back("dejavusans");
	return requested_families;
}

std::string Build_Font_Request_Key(const char *font_name, bool is_bold)
{
	std::string key = Normalize_Font_Family(font_name != nullptr ? font_name : "");
	key += is_bold ? "|bold" : "|regular";
	return key;
}

const std::vector<std::string> &Get_Registered_Font_Aliases(const std::string &registered_file)
{
	std::unordered_map<std::string, std::vector<std::string> >::const_iterator cached_aliases =
		CachedRegisteredFontAliases.find(registered_file);
	if (cached_aliases != CachedRegisteredFontAliases.end()) {
		return cached_aliases->second;
	}

	std::vector<unsigned char> font_data;
	std::vector<std::string> aliases;
	if (Read_Resource_Font_File(registered_file.c_str(), font_data) && !font_data.empty()) {
		Extract_Font_Name_Aliases(font_data, aliases);
	}

	return CachedRegisteredFontAliases.insert(std::make_pair(registered_file, aliases)).first->second;
}

void Cache_System_Font_Candidates(void)
{
	if (CachedSystemFontCandidatesScanned) {
		return;
	}

	CachedSystemFontCandidatesScanned = true;

	std::vector<std::string> search_roots(std::begin(FontSearchRoots), std::end(FontSearchRoots));
	if (const char *home = std::getenv("HOME")) {
		search_roots.emplace_back(renegade_osdep::Join_Path(home, ".fonts"));
		search_roots.emplace_back(renegade_osdep::Join_Path(home, ".local/share/fonts"));
	}

	for (const std::string &root : search_roots) {
		if (!renegade_osdep::Path_Is_Directory(root)) {
			continue;
		}

		std::vector<std::string> files;
		if (!renegade_osdep::Collect_Regular_Files_Recursive(root, files)) {
			continue;
		}

		for (const std::string &path : files) {
			if (!Font_File_Extension_Matches(path)) {
				continue;
			}

			FontCandidateInfo candidate;
			candidate.Path = path;
			candidate.Stem = Normalize_Font_Family(renegade_osdep::Get_Path_Stem(path));

			std::vector<unsigned char> font_data;
			if (Read_Binary_File(path, font_data)) {
				Extract_Font_Name_Aliases(font_data, candidate.Aliases);
			}

			CachedSystemFontCandidates.push_back(candidate);
		}
	}
}

bool Resolve_Registered_Font_File(const char *font_name, bool is_bold, std::string &resolved_file)
{
	if (font_name == nullptr || *font_name == '\0') {
		return false;
	}

	const std::string requested_family = Normalize_Font_Family(font_name);

	for (const std::string &registered_file : RegisteredFontFiles) {
		const std::vector<std::string> &aliases = Get_Registered_Font_Aliases(registered_file);
		for (const std::string &alias : aliases) {
			if (alias == requested_family) {
				resolved_file = registered_file;
				return true;
			}
		}
	}

	std::vector<std::string> requested_families = Build_Requested_Font_Families(font_name);

	int best_score = -1;
	for (const std::string &registered_file : RegisteredFontFiles) {
		const int score = Score_Font_Candidate(Normalize_Font_Family(renegade_osdep::Get_Path_Stem(registered_file)), requested_families, is_bold);
		if (score > best_score) {
			best_score = score;
			resolved_file = registered_file;
		}
	}

	return best_score >= 0;
}

bool Read_Font_Data(const char *font_name, bool is_bold, std::vector<unsigned char> &contents, std::string &source_name)
{
	contents.clear();
	source_name.clear();

	if (Read_Resource_Font_File(font_name, contents)) {
		source_name = font_name;
		return true;
	}

	std::string registered_file;
	if (Resolve_Registered_Font_File(font_name, is_bold, registered_file) &&
		Read_Resource_Font_File(registered_file.c_str(), contents))
	{
		source_name = registered_file;
		return true;
	}

	std::string font_path;
	if (Resolve_Font_Path(font_name, is_bold, font_path) && Read_Binary_File(font_path, contents)) {
		source_name = font_path;
		return true;
	}

	return false;
}

bool Resolve_Font_Path(const char *font_name, bool is_bold, std::string &resolved_path)
{
	if (font_name == nullptr || *font_name == '\0') {
		return false;
	}

	std::string direct_path;
	if (renegade_osdep::Resolve_Existing_Path(font_name, direct_path) && renegade_osdep::Path_Is_Regular_File(direct_path)) {
		resolved_path = direct_path;
		return true;
	}

	const std::string request_key = Build_Font_Request_Key(font_name, is_bold);
	std::unordered_map<std::string, std::string>::const_iterator cached_path =
		CachedResolvedFontPaths.find(request_key);
	if (cached_path != CachedResolvedFontPaths.end()) {
		resolved_path = cached_path->second;
		return true;
	}

	std::vector<std::string> requested_families = Build_Requested_Font_Families(font_name);
	Cache_System_Font_Candidates();

	int best_score = -1;
	std::string first_font_path;
	for (std::vector<FontCandidateInfo>::const_iterator it = CachedSystemFontCandidates.begin();
		it != CachedSystemFontCandidates.end();
		++it)
	{
		if (first_font_path.empty()) {
			first_font_path = it->Path;
		}

		int score = -1;
		if (!it->Aliases.empty()) {
			score = Score_Font_Aliases(it->Aliases, requested_families, is_bold, it->Stem);
		}

		if (score < 0) {
			score = Score_Font_Candidate(it->Stem, requested_families, is_bold);
		}
		if (score > best_score) {
			best_score = score;
			resolved_path = it->Path;
		}
	}

	if (best_score >= 0) {
		CachedResolvedFontPaths.insert(std::make_pair(request_key, resolved_path));
		return true;
	}

	if (!first_font_path.empty()) {
		resolved_path = first_font_path;
		CachedResolvedFontPaths.insert(std::make_pair(request_key, resolved_path));
		return true;
	}

	return false;
}
}

void FontCharsClass::Register_Font_File(const char *filename)
{
	if (filename == nullptr || *filename == '\0') {
		return;
	}

	for (const std::string &registered_file : RegisteredFontFiles) {
		if (stricmp(registered_file.c_str(), filename) == 0) {
			return;
		}
	}

	RegisteredFontFiles.emplace_back(filename);
}

void FontCharsClass::Unregister_Font_File(const char *filename)
{
	if (filename == nullptr || *filename == '\0') {
		return;
	}

	const auto it = std::remove_if(
		RegisteredFontFiles.begin(),
		RegisteredFontFiles.end(),
		[filename](const std::string &registered_file) {
			return stricmp(registered_file.c_str(), filename) == 0;
		});
	RegisteredFontFiles.erase(it, RegisteredFontFiles.end());
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Render2DSentenceClass
//
////////////////////////////////////////////////////////////////////////////////////
Render2DSentenceClass::Render2DSentenceClass (void) :
	Font (NULL),
	Location (0.0F,0.0F),
	Cursor (0.0F,0.0F),
	TextureOffset (0, 0),
	TextureStartX (0),
	CurSurface (NULL),
	CurrTextureSize (0),
	MonoSpaced (false),
	IsClippedEnabled (false),
	ClipRect (0, 0, 0, 0),
	BaseLocation (0, 0),
	LockedPtr (NULL),
	LockedStride (0),
	TextureSizeHint (0),
	WrapWidth (0),
	TabStop (5.0),
	DrawExtents (0, 0, 0, 0),
	Renderers(sizeof(PreAllocatedRenderers)/sizeof(RendererDataStruct),PreAllocatedRenderers),
	CachedWrapWidth (0.0F),
	CachedTabStop (0.0F),
	CachedTextureSizeHint (0),
	CachedFont (NULL),
	HasCachedSentence (false)
{
	Shader = Render2DClass::Get_Default_Shader ();
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	~Render2DSentenceClass
//
////////////////////////////////////////////////////////////////////////////////////
Render2DSentenceClass::~Render2DSentenceClass (void)
{
	REF_PTR_RELEASE (Font);
	Reset ();
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Set_Font
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Set_Font (FontCharsClass *font)
{
	if (Font == font) {
		return;
	}

	Reset ();
	REF_PTR_SET (Font, font);
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Reset_Polys
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Reset_Polys (void)
{
	for (int index = 0; index < Renderers.Count (); index ++) {
		Renderers[index].Renderer->Reset ();
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Reset
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Reset (void)
{
	Clear_Built_Sentence ();
	MonoSpaced = false;
	CachedSentenceText = L"";
	CachedWrapWidth = 0.0F;
	CachedTabStop = 0.0F;
	CachedTextureSizeHint = 0;
	CachedFont = NULL;
	HasCachedSentence = false;
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Make_Additive
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Make_Additive (void)
{
	Shader.Set_Dst_Blend_Func (ShaderClass::DSTBLEND_ONE);
	Shader.Set_Src_Blend_Func (ShaderClass::SRCBLEND_SRC_ALPHA);
	Shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Secondary_Gradient (ShaderClass::SECONDARY_GRADIENT_DISABLE);

	Set_Shader (Shader);
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Make_Additive
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Set_Shader (ShaderClass shader)
{
	Shader = shader;
	const WW3D::OverlayStateDesc overlay_state = Render2DClass::Overlay_State_From_Shader(Shader);

	//
	//	Change each renderer's shader
	//
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Set_Overlay_State(overlay_state);
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Render
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Render (void)
{
	if (!WW3D::Is_Device_Ready()) return;
	//
	//	Build any textures that are pending
	//
	Build_Textures ();

	//
	//	Ask each renderer to draw its contents
	//
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Render ();
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Set_Base_Location
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Set_Base_Location (const Vector2 &loc)
{
	Vector2 dif		= loc - BaseLocation;
	BaseLocation	= loc;
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Move (dif);
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Set_Location
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Set_Location (const Vector2 &loc)
{
	Location	= loc;
	return ;
}


void
Render2DSentenceClass::Set_Tabstop(float stop)
{
	if (stop > 0.0) {
		TabStop = stop;
	} else {
		TabStop = 1.0;
	}
}

////////////////////////////////////////////////////////////////////////////////////
//
//	Get_Text_Extents
//
////////////////////////////////////////////////////////////////////////////////////
Vector2
Render2DSentenceClass::Get_Text_Extents (const WCHAR *text)
{
	if (!DX8Wrapper::Is_Initted()) {
		Vector2 temp(0,0);
		return(temp);
	}

	Vector2 extent (0, Font->Get_Char_Height());

	while (*text) {
		WCHAR ch = *text++;

		if ( ch != (WCHAR)'\n' ) {
			extent.X += Font->Get_Char_Spacing( ch );
		}
	}

	return extent;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Find_Row_Start
//
////////////////////////////////////////////////////////////////////////////////////
const WCHAR *
Render2DSentenceClass::Find_Row_Start( const WCHAR * text, int row_index )
{
	if (row_index == 0) {
		return text;
	}

	if (!DX8Wrapper::Is_Initted()) {
		return text;
	}

	const WCHAR *retval = NULL;

	float max_x_pos	= 0;
	float x_pos			= 0;
	float y_pos			= Font->Get_Char_Height ();

	int row_counter = 0;

	while (*text) {
		WCHAR ch = *text++;

		bool is_wrapped = false;

		//
		// Check to see if we need to wrap on this word-break
		//
		if (IS_BREAK_CHAR (ch) && WrapWidth > 0) {

			//
			//	Find the width of the next word
			//
			const WCHAR *word	= text;
			float word_width = Font->Get_Char_Spacing (ch);
			while ((*word != 0) && ((*word > L' ') && !IS_BREAK_CHAR (*word))) {
				word_width += Font->Get_Char_Spacing (*word++);
			}

			//
			//	Did the word extend past the wrap width?
			//
			if ((x_pos + word_width) >= WrapWidth) {
				is_wrapped = true;
			}

		} else if (ch == L'\n') {
			is_wrapped = true;
		}

		//
		//	Handle line wrapping
		//
		if (is_wrapped) {
			max_x_pos = max (max_x_pos, x_pos);
			x_pos = 0;
			y_pos += Font->Get_Char_Height ();

			//
			//	Is this the line we're looking for?
			//
			row_counter ++;
			if (row_counter == row_index) {
				retval = (ch == L' ' || ch == L'\n') ? text : text - 1;
				break;
			}
		}

		if (ch != (WCHAR)'\n') {
			x_pos += Font->Get_Char_Spacing (ch);
		}
	}

	return retval;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Get_Formatted_Text_Extents
//
////////////////////////////////////////////////////////////////////////////////////
Vector2
Render2DSentenceClass::Get_Formatted_Text_Extents (const WCHAR *text, int *row_count)
{
	if (!DX8Wrapper::Is_Initted()) {
		Vector2 temp(0,0);
		return(temp);
	}
	
	float max_x_pos	= 0;
	float x_pos			= 0;
	float y_pos			= Font->Get_Char_Height ();

	int row_counter = 0;

	while (*text) {
		WCHAR ch = *text++;

		bool is_wrapped = false;

		//
		// Check to see if we need to wrap on this word-break
		//
		if (IS_BREAK_CHAR (ch) && WrapWidth > 0) {

			//
			//	Find the width of the next word
			//
			const WCHAR *word	= text;
			float word_width = Font->Get_Char_Spacing (ch);
			while ((*word != 0) && ((*word > L' ') && !IS_BREAK_CHAR (*word))) {
				word_width += Font->Get_Char_Spacing (*word++);
			}

			//
			//	Did the word extend past the wrap width?
			//
			if ((x_pos + word_width) >= WrapWidth) {
				is_wrapped = true;
			}

		} else if (ch == L'\n') {
			is_wrapped = true;
		}

		//
		//	Handle line wrapping
		//
		if (is_wrapped) {
			max_x_pos = max (max_x_pos, x_pos);
			x_pos = 0;
			y_pos += Font->Get_Char_Height ();
			row_counter ++;
		}

		if (ch != (WCHAR)'\n') {
			x_pos += Font->Get_Char_Spacing (ch);
		}
	}

	//
	//	Build a Vector2 out of our extents
	//
	Vector2 extent;
	extent.X = max (max_x_pos, x_pos);
	extent.Y = y_pos;

	//
	//	Return the row count to the caller (if necessary)
	//
	if (row_count != NULL) {
		(*row_count) = row_counter + 1;
	}

	return extent;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Reset_Sentence_Data
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Reset_Sentence_Data (void)
{
	//
	//	Release our hold on each texture used in the sentence
	//
	for (int index = 0; index < SentenceData.Count (); index ++) {
		REF_PTR_RELEASE (SentenceData[index].Surface);
	}

	SentenceData.Reset_Active();
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Clear_Built_Sentence
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Clear_Built_Sentence (void)
{
	//
	//	Make sure we unlock the current surface (if necessary)
	//
	if (LockedPtr != NULL) {
		CurSurface->Unlock ();
		LockedPtr = NULL;
	}

	//
	//	Release our hold on the current surface
	//
	REF_PTR_RELEASE (CurSurface);

	//
	//	Free each renderer
	//
	for (int index = 0; index < Renderers.Count (); index ++) {
		delete Renderers[index].Renderer;
	}
	Renderers.Reset_Active ();

	Cursor.Set (0, 0);
	TextureOffset.Set (0, 0);
	TextureStartX = 0;
	CurrTextureSize = 0;

	Release_Pending_Surfaces ();
	Reset_Sentence_Data ();
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Release_Pending_Surfaces
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Release_Pending_Surfaces (void)
{
	//
	//	Release our hold on each pending surface
	//
	for (int index = 0; index < PendingSurfaces.Count (); index ++) {
		SurfaceClass *curr_surface = PendingSurfaces[index].Surface;
		REF_PTR_RELEASE (curr_surface);
	}

	PendingSurfaces.Reset_Active();
	return;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Build_Textures
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Build_Textures (void)
{
	WWMEMLOG(MEM_TEXTURE);

	//
	//	Make sure we unlock the current surface
	//
	if (LockedPtr != NULL) {
		CurSurface->Unlock ();
		LockedPtr = NULL;
	}

	//
	//	Release our hold on the current surface
	//
	REF_PTR_RELEASE (CurSurface);
	TextureOffset.Set (0, 0);
	TextureStartX = 0;

	//
	//	Convert all pending surfaces to textures
	//
	for (int index = 0; index < PendingSurfaces.Count (); index ++) {
		PendingSurfaceStruct &surface_info = PendingSurfaces[index];
		SurfaceClass *curr_surface = surface_info.Surface;

		//
		//	Get the dimensions of the surface
		//
		SurfaceClass::SurfaceDescription desc;
		curr_surface->Get_Description (desc);
		//
		//	Create the new texture
		//
		TextureClass *new_texture = new TextureClass (desc.Width, desc.Width, WW3D_FORMAT_A4R4G4B4, TextureClass::MIP_LEVELS_1);
		new_texture->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		new_texture->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		new_texture->Set_Min_Filter(TextureClass::FILTER_TYPE_NONE);
		new_texture->Set_Mag_Filter(TextureClass::FILTER_TYPE_NONE);
		SurfaceClass *texture_surface = new_texture->Get_Surface_Level ();

		//
		//	Copy the contents of the texture from the surface
		//
		texture_surface->Copy(0, 0, 0, 0, desc.Width, desc.Height, curr_surface);
		REF_PTR_RELEASE (texture_surface);

		//
		//	Assign this texture to any renderers that need it
		//
		for (int renderer_index = 0; renderer_index < surface_info.Renderers.Count (); renderer_index ++) {
			Render2DClass *renderer = surface_info.Renderers[renderer_index];
			renderer->Set_Texture (new_texture);
		}

		//
		//	Release our hold on the objects
		//
		REF_PTR_RELEASE (new_texture);
		REF_PTR_RELEASE (curr_surface);
	}

	//
	//	Reset the list
	//
	PendingSurfaces.Reset_Active();
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Draw_Sentence
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Draw_Sentence (uint32 color)
{
	Render2DClass *curr_renderer	= NULL;
	SurfaceClass *curr_surface		= NULL;

	DrawExtents.Set (0, 0, 0, 0);

	//
	//	Loop over all the parts of the sentence
	//
	for (int index = 0; index < SentenceData.Count (); index ++) {
		SentenceDataStruct &data = SentenceData[index];

		//
		//	Has the surface changed?
		//
		if (data.Surface != curr_surface) {
			curr_surface = data.Surface;

			//
			//	Try to find a renderer that uses the same "texture"
			//
			bool found = false;
			for (int renderer_index = 0; renderer_index < Renderers.Count (); renderer_index ++) {
				if (Renderers[renderer_index].Surface == curr_surface) {
					found = true;
					curr_renderer = Renderers[renderer_index].Renderer;
					break;
				}
			}

			//
			//	Create a new renderer if we couldn't find an appropriate one
			//
			if (found == false) {

				//
				//	Allocate a new renderer
				//
				curr_renderer = new Render2DClass;
				curr_renderer->Set_Coordinate_Range (Render2DClass::Get_Screen_Resolution ());
				curr_renderer->Set_Overlay_State(Render2DClass::Overlay_State_From_Shader(Shader));

				//
				//	Add it to our list
				//
				RendererDataStruct render_info;
				render_info.Renderer	= curr_renderer;
				render_info.Surface	= curr_surface;
				Renderers.Add (render_info);

				//
				//	Now, add this renderer to the surface pending list
				//
				for (int surface_index = 0; surface_index < PendingSurfaces.Count (); surface_index ++) {
					PendingSurfaceStruct &surface_info = PendingSurfaces[surface_index];
					if (surface_info.Surface == curr_surface) {
						surface_info.Renderers.Add (curr_renderer);
					}
				}
			}
		}

		//
		//	Get the dimensions of the surface
		//
		SurfaceClass::SurfaceDescription desc;
		curr_surface->Get_Description (desc);

		//
		//	Add a quad that contains this sentence chunk
		//
		RectClass screen_rect	= data.ScreenRect;
		screen_rect					+= Location;
		RectClass uv_rect			= data.UVRect;

		//
		//	Clip the quad (as necessary)
		//
		bool add_quad = true;
		if (IsClippedEnabled) {

			//
			//	Check for completely clipped
			//
			if (	screen_rect.Right <= ClipRect.Left ||
					screen_rect.Bottom <= ClipRect.Top)
			{
				add_quad = false;
			/*} else if (	screen_rect.Top < ClipRect.Top ||
							screen_rect.Bottom > ClipRect.Bottom)
			{
				add_quad = false;*/
			} else {

				//
				//	Clip the polygons to the specified area
				//
				RectClass clipped_rect;
				clipped_rect.Left		= max (screen_rect.Left, ClipRect.Left);
				clipped_rect.Right	= min (screen_rect.Right, ClipRect.Right);
				clipped_rect.Top		= max (screen_rect.Top, ClipRect.Top);
				clipped_rect.Bottom	= min (screen_rect.Bottom, ClipRect.Bottom);

				//
				//	Clip the texture to the specified area
				//
				RectClass clipped_uv_rect;
				float percent				= ((clipped_rect.Left - screen_rect.Left) / screen_rect.Width ());
				clipped_uv_rect.Left		= uv_rect.Left + (uv_rect.Width () * percent);

				percent						= ((clipped_rect.Right - screen_rect.Left) / screen_rect.Width ());
				clipped_uv_rect.Right	= uv_rect.Left + (uv_rect.Width () * percent);

				percent						= ((clipped_rect.Top - screen_rect.Top) / screen_rect.Height ());
				clipped_uv_rect.Top		= uv_rect.Top + (uv_rect.Height () * percent);

				percent						= ((clipped_rect.Bottom - screen_rect.Top) / screen_rect.Height ());
				clipped_uv_rect.Bottom	= uv_rect.Top + (uv_rect.Height () * percent);

				//
				//	Use the clipped rectangles to render
				//
				screen_rect = clipped_rect;
				uv_rect		= clipped_uv_rect;
			}
		}

		if (add_quad) {
			uv_rect *=  1.0F / ((float)desc.Width);
			curr_renderer->Add_Quad (screen_rect, uv_rect, color);

			//
			//	Add this rectangle to the total draw extents
			//
			if (DrawExtents.Width () == 0) {
				DrawExtents = screen_rect;
			} else {
				DrawExtents += screen_rect;
			}
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Record_Sentence_Chunk
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Record_Sentence_Chunk (void)
{
	//
	//	Do we have anything to store?
	//
	int width = TextureOffset.I - TextureStartX;
	if (width > 0) {
		float char_height = Font->Get_Char_Height ();

		//
		//	Build a structure that contains enough information
		// to hold this portion of the sentence
		//
		SentenceDataStruct sentence_data;
		sentence_data.Surface = CurSurface;
		sentence_data.Surface->Add_Ref ();
		sentence_data.ScreenRect.Left		= Cursor.X;
		sentence_data.ScreenRect.Right	= Cursor.X + width;
		sentence_data.ScreenRect.Top		= Cursor.Y;
		sentence_data.ScreenRect.Bottom	= Cursor.Y + char_height;
		sentence_data.UVRect.Left			= TextureStartX;
		sentence_data.UVRect.Top			= TextureOffset.J;
		sentence_data.UVRect.Right			= TextureOffset.I;
		sentence_data.UVRect.Bottom		= TextureOffset.J + char_height;

		//
		//	Add this information to our list
		//
		SentenceData.Add (sentence_data);
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Allocate_New_Surface
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Allocate_New_Surface (const WCHAR *text)
{
	//
	//	Unlock the last surface (if necessary)
	//
	if (LockedPtr != NULL) {
		CurSurface->Unlock ();
		LockedPtr = NULL;
	}

	//
	// Calculate the width of the text
	//
	int text_width = 0;
	for (int index = 0; text[index] != 0; index ++) {
		text_width += Font->Get_Char_Spacing (text[index]);
	}

	int char_height = Font->Get_Char_Height ();

	//
	//	Find the best texture size for the remaining text
	//
	CurrTextureSize = 256;
	int best_tex_mem_usage = 999999999;
	for (int pow2 = 6; pow2 <= 8; pow2 ++) {

		int size					= 1 << pow2;
		int row_count			= (text_width / size) + 1;
		int rows_per_texture	= size / (char_height + 1);

		//
		//	Can we even fit one character on this texture?
		//
		if (rows_per_texture > 0) {

			//
			//	How many textures (at this size) would it take to render
			// the remaining text?
			//
			int texture_count	= row_count / rows_per_texture;
			texture_count		= max (texture_count, 1);

			//
			//	Is this the best usage of texture memory we've found yet?
			//
			int texture_mem_usage = (texture_count * size * size);
			if (texture_mem_usage < best_tex_mem_usage) {
				CurrTextureSize		= size;
				best_tex_mem_usage	= texture_mem_usage;
			}
		}
	}

	//
	//	Use whichever is larger, the hint or the calculated size
	//
	CurrTextureSize = max (TextureSizeHint, CurrTextureSize);

	//
	//	Release our extra hold on the old surface
	//
	REF_PTR_RELEASE (CurSurface);

	//
	//	Create the new surface
	//
	CurSurface = NEW_REF (SurfaceClass, (CurrTextureSize, CurrTextureSize, WW3D_FORMAT_A4R4G4B4));
	WWASSERT (CurSurface != NULL);
	CurSurface->Add_Ref ();

	//
	//	Add this surface to our list
	//
	PendingSurfaceStruct surface_info;
	surface_info.Surface = CurSurface;
	PendingSurfaces.Add (surface_info);

	//
	//	Reset to the upper left corner
	//
	TextureOffset.Set (0, 0);
	TextureStartX = 0;
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Build_Sentence
//
////////////////////////////////////////////////////////////////////////////////////
void
Render2DSentenceClass::Build_Sentence (const WCHAR *text)
{
	if (text == NULL) {
		return ;
	}

	if (!DX8Wrapper::Is_Initted()) {
		return;
	}

	if (Font == NULL) {
		return;
	}

	if (HasCachedSentence &&
		CachedFont == Font &&
		CachedWrapWidth == WrapWidth &&
		CachedTabStop == TabStop &&
		CachedTextureSizeHint == TextureSizeHint &&
		CachedSentenceText == text)
	{
		return;
	}

	//
	//	Start fresh
	//
	Reset_Sentence_Data ();
	Cursor.Set (0, 0);

	//
	//	Ensure we have a surface to start with
	//
	if (CurSurface == NULL) {
		Allocate_New_Surface (text);
	}

	float char_height = Font->Get_Char_Height ();

	//
	//	Loop over all the characters in the string
	//
	const WCHAR *original_text = text;
	while (text != NULL) {
		WCHAR ch = *text++;

		//
		//	Determine how much horizontal space this character requires
		//
		float char_spacing = Font->Get_Char_Spacing (ch);

		bool exceeded_texture_width	= ((TextureOffset.I + char_spacing) >= CurrTextureSize);
		bool encountered_break_char	= (IS_BREAK_CHAR (ch) || ch == L'\n' || ch == 0 || ch == L'\t');

		//
		//	Do we need to record this portion of the sentence to its own chunk?
		//
		if (exceeded_texture_width || encountered_break_char) {
			Record_Sentence_Chunk ();

			//
			//	Adjust the positions
			//
			Cursor.X			+= (TextureOffset.I - TextureStartX);
			TextureStartX	= TextureOffset.I;

			//
			//	Adjust the output coordinates
			//
			if (IS_BREAK_CHAR (ch)) {

				if (ch == L' ') {
					Cursor.X += char_spacing;
				}

				//
				// Check to see if we need to wrap on this word-break
				//
				if (WrapWidth > 0) {

					//
					//	Find the length of the next word
					//
					const WCHAR *word	= text;
					float word_width	= (ch == L' ') ? 0 : char_spacing;
					while ((*word != 0) && ((*word > L' ') && !IS_BREAK_CHAR (*word))) {
						word_width += Font->Get_Char_Spacing (*word++);
					}

					//
					//	Should we wrap the next word?
					//
					if ((Cursor.X + word_width) >= WrapWidth) {
						Cursor.X = 0;
						Cursor.Y += char_height;
					}
				}

			} else if (ch == L'\n') {
				Cursor.X = 0;
				Cursor.Y += char_height;
			} else if (ch == 0) {
				break;
			} else if (ch == L'\t') {
				float tab_spacing = (char_spacing * TabStop);
				float tab_pos = (floor(Cursor.X / tab_spacing) * tab_spacing);
				Cursor.X = (tab_pos + tab_spacing);
			}

			//
			//	Did the text extend past the edge of the texture?
			//
			if (exceeded_texture_width) {
				TextureStartX		= 0;
				TextureOffset.I	= TextureStartX;
				TextureOffset.J	+= char_height;

				//
				//	Did the text extent completely off the texture?
				//
				if ((TextureOffset.J + char_height) >= CurrTextureSize) {
					Allocate_New_Surface (text);
				}
			}
		}

		if (ch != L'\n' && ch != L' ' && ch != L'\t') {

			//
			//	Ensure the surface is locked
			//
			if (LockedPtr == NULL) {
				LockedPtr = (uint16 *)CurSurface->Lock (&LockedStride);
				WWASSERT (LockedPtr != NULL);
			}

			//
			//	Check to ensure the text will fit on this texture
			//
			WWASSERT (((TextureOffset.I + char_spacing) < CurrTextureSize) && ((TextureOffset.J + char_height) < CurrTextureSize));

			//
			//	Blit the character to the surface
			//
			Font->Blit_Char (ch, LockedPtr, LockedStride, TextureOffset.I, TextureOffset.J);
			TextureOffset.I += char_spacing;
		}
	}

	CachedSentenceText = original_text;
	CachedWrapWidth = WrapWidth;
	CachedTabStop = TabStop;
	CachedTextureSizeHint = TextureSizeHint;
	CachedFont = Font;
	HasCachedSentence = true;

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void	Render2DSentenceClass::Force_Alpha( float alpha )
{
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Force_Alpha( alpha );
	}
}



////////////////////////////////////////////////////////////////////////////////////
//
//	FontCharsClass
//
////////////////////////////////////////////////////////////////////////////////////
FontCharsClass::FontCharsClass (void) :
	CurrPixelOffset( 0 ),
	PointSize( 0 ),
	CharHeight( 0 ),
	FontScale( 0.0f ),
	FontPixelHeight( 0.0f ),
	FontAscent( 0 ),
	FontDescent( 0 ),
	FontLineGap( 0 ),
	FontInfo( NULL ),
	UnicodeCharArray( NULL ),
	FirstUnicodeChar( 0xFFFF ),
	LastUnicodeChar( 0 ),
	IsBold (false),
	BufferList(sizeof(PreAllocatedBufferList)/sizeof(uint16*),PreAllocatedBufferList),
	BufferCapacityList(sizeof(PreAllocatedBufferCapacityList)/sizeof(int),PreAllocatedBufferCapacityList)
{
	::memset( ASCIICharArray, 0, sizeof (ASCIICharArray) );
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	~FontCharsClass
//
////////////////////////////////////////////////////////////////////////////////////
FontCharsClass::~FontCharsClass (void)
{
	for (int i=0;i<BufferList.Count(); ++i) {
		delete [] BufferList[i];
	}
	BufferList.Reset_Active();
	BufferCapacityList.Reset_Active();

	Release_Font();
	Free_Character_Arrays();
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Get_Char_Data
//
////////////////////////////////////////////////////////////////////////////////////
const FontCharsClass::CharDataStruct *
FontCharsClass::Get_Char_Data (WCHAR ch)
{
	const CharDataStruct *retval = NULL;

	if ( ch < 256 ) {
		retval = ASCIICharArray[ch];
	} else {
		Grow_Unicode_Array( ch );
		retval = UnicodeCharArray[ch - FirstUnicodeChar];
	}

	//
	//	If the character wasn't found, then add it to our list
	//
	if ( retval == NULL ) {
		retval = Store_Glyph( ch );
	}

	WWASSERT( retval->Value == ch );
	return retval;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Get_Char_Width
//
////////////////////////////////////////////////////////////////////////////////////
int
FontCharsClass::Get_Char_Width (WCHAR ch)
{
	const CharDataStruct	* data = Get_Char_Data( ch );
	if ( data != NULL ) {
		return data->Advance;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Get_Char_Spacing
//
////////////////////////////////////////////////////////////////////////////////////
int
FontCharsClass::Get_Char_Spacing (WCHAR ch)
{
	const CharDataStruct	* data = Get_Char_Data( ch );
	if ( data != NULL ) {
		if ( data->Advance != 0 ) {
			return data->Advance + 1;
		}
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Blit_Char
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Blit_Char (WCHAR ch, uint16 *dest_ptr, int dest_stride, int x, int y)
{
	const CharDataStruct	* data = Get_Char_Data( ch );
	if ( data != NULL && data->Width != 0 ) {

		//
		//	Setup the src and destination pointers
		//
		int dest_inc		= (dest_stride >> 1);
		uint16 *src_ptr	= data->Buffer;
		dest_ptr				+= (dest_inc * y) + x;

		//
		//	Simply copy the data from the src buffer to the destination
		//
		for ( int row = 0; row < CharHeight; row ++ ) {
			for ( int col = 0; col < data->Width; col ++ ) {
				const uint16 pixel = *src_ptr++;
				if (pixel != 0) {
					dest_ptr[col] = pixel;
				}
			}
			dest_ptr	+= dest_inc;
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Store_Glyph
//
////////////////////////////////////////////////////////////////////////////////////
const FontCharsClass::CharDataStruct *
FontCharsClass::Store_Glyph (WCHAR ch)
{
	WWASSERT(FontInfo != NULL);

	int advance = 0;
	int left_side_bearing = 0;
	stbtt_GetCodepointHMetrics(FontInfo, static_cast<int>(ch), &advance, &left_side_bearing);

	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;
	stbtt_GetCodepointBitmapBox(FontInfo, static_cast<int>(ch), FontScale, FontScale, &x0, &y0, &x1, &y1);

	const int bitmap_width = std::max(0, x1 - x0);
	const int bitmap_height = std::max(0, y1 - y0);
	const int left_padding = std::max(0, -x0);
	const int advance_width = std::max(0, static_cast<int>(std::ceil(static_cast<float>(advance) * FontScale)));
	const int char_width = std::max(advance_width + left_padding, std::max(1, x1 + left_padding));
	const int baseline = static_cast<int>(std::ceil(static_cast<float>(FontAscent) * FontScale));

	//
	//	Get a pointer to the surface that this character should use
	//
	Update_Current_Buffer( char_width );
	uint16 *curr_buffer = BufferList[BufferList.Count () - 1];
	curr_buffer += CurrPixelOffset;
	::memset(curr_buffer, 0, sizeof(uint16) * char_width * CharHeight);

	if (bitmap_width > 0 && bitmap_height > 0) {
		std::vector<unsigned char> glyph_bitmap(static_cast<size_t>(bitmap_width) * static_cast<size_t>(bitmap_height));
		stbtt_MakeCodepointBitmap(
			FontInfo,
			glyph_bitmap.data(),
			bitmap_width,
			bitmap_height,
			bitmap_width,
			FontScale,
			FontScale,
			static_cast<int>(ch));

		const int top = baseline + y0;
		const int left = left_padding + x0;
		for (int row = 0; row < bitmap_height; ++row) {
			const int dest_y = top + row;
			if (dest_y < 0 || dest_y >= CharHeight) {
				continue;
			}

			for (int col = 0; col < bitmap_width; ++col) {
				const int dest_x = left + col;
				if (dest_x < 0 || dest_x >= char_width) {
					continue;
				}

				const uint8 pixel_value = glyph_bitmap[static_cast<size_t>(row) * static_cast<size_t>(bitmap_width) + static_cast<size_t>(col)];
				uint16 pixel_color = 0;
				if (pixel_value != 0) {
					pixel_color = 0x0FFF;
				}

				const uint8 alpha_value = static_cast<uint8>((pixel_value >> 4) & 0x0F);
				curr_buffer[dest_y * char_width + dest_x] = static_cast<uint16>(pixel_color | (alpha_value << 12));
			}
		}
	}

	//
	//	Save information about this character in our list
	//
	CharDataStruct *char_data	= new CharDataStruct;
	char_data->Value				= ch;
	char_data->Width				= static_cast<short>(char_width);
	char_data->Advance			= static_cast<short>(std::max(advance_width, 0));
	char_data->Buffer				= BufferList[BufferList.Count () - 1] + CurrPixelOffset;

	//
	//	Insert this character into our array
	//
	if ( ch < 256 ) {
		ASCIICharArray[ch] = char_data;
	} else {
		UnicodeCharArray[ch - FirstUnicodeChar] = char_data;
	}

	//
	//	Advance the character position
	//
	CurrPixelOffset += (char_width * CharHeight);

	//
	//	Return the index of the entry we just added
	//
	return char_data;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Update_Current_Buffer
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Update_Current_Buffer (int char_width)
{
	const int required_pixels = char_width * CharHeight;

	//
	//	Check to see if we need to allocate a new buffer
	//
	bool needs_new_buffer = (BufferList.Count () == 0);
	if (needs_new_buffer == false) {

		//
		//	Would we extend past this buffer?
		//
		const int current_capacity = BufferCapacityList[BufferCapacityList.Count () - 1];
		if ( (CurrPixelOffset + required_pixels) > current_capacity ) {
			needs_new_buffer = true;
		}
	}

	//
	//	Do we need to create a new surface?
	//
	if (needs_new_buffer) {
		const int buffer_capacity = std::max(CHAR_BUFFER_LEN, required_pixels);
		uint16 *new_buffer = new uint16[buffer_capacity];
		BufferList.Add( new_buffer );
		BufferCapacityList.Add( buffer_capacity );
		CurrPixelOffset = 0;
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Load_Font
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Load_Font (const char *font_name)
{
	Release_Font();

	std::string font_source;
	if (!Read_Font_Data(font_name, IsBold, FontFileData, font_source)) {
		WWDEBUG_SAY(("Failed to read font data for '%s'\n", font_name));
		return;
	}
	FontInfo = new stbtt_fontinfo;
	if (!stbtt_InitFont(FontInfo, FontFileData.data(), 0)) {
		delete FontInfo;
		FontInfo = NULL;
		FontFileData.clear();
		WWDEBUG_SAY(("Failed to initialize stb font '%s'\n", font_source.c_str()));
		return;
	}

	FontPixelHeight = (static_cast<float>(PointSize) * LogicalDpi) / 72.0f;
	FontScale = stbtt_ScaleForPixelHeight(FontInfo, FontPixelHeight);
	stbtt_GetFontVMetrics(FontInfo, &FontAscent, &FontDescent, &FontLineGap);
	CharHeight = std::max(1, static_cast<int>(std::ceil((static_cast<float>(FontAscent - FontDescent + FontLineGap)) * FontScale)));
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Release_Font
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Release_Font (void)
{
	if (FontInfo != NULL) {
		delete FontInfo;
		FontInfo = NULL;
	}

	FontFileData.clear();
	FontScale = 0.0f;
	FontPixelHeight = 0.0f;
	FontAscent = 0;
	FontDescent = 0;
	FontLineGap = 0;
	CharHeight = 0;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Initialize_Font
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Initialize_Font (const char *font_name, int point_size, bool is_bold)
{
	//
	//	Build a unique name from the font name and its size
	//
	Name.Format ("%s%d", font_name, point_size);

	//
	//	Remember these settings
	//
	FontName	= font_name;
	PointSize	= point_size;
	IsBold		= is_bold;

	//
	//	Create the actual font object
	//
	Load_Font (font_name);
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Is_Font
//
////////////////////////////////////////////////////////////////////////////////////
bool
FontCharsClass::Is_Font (const char *font_name, int point_size, bool is_bold)
{
	bool retval = false;

	//
	//	Check to see if both the name and height matches...
	//
	if (	(FontName.Compare_No_Case (font_name) == 0) &&
			(point_size == PointSize) &&
			(is_bold == IsBold))
	{
		retval = true;
	}

	return retval;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Grow_Unicode_Array
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Grow_Unicode_Array (WCHAR ch)
{
	//
	//	Don't do anything if character is in the ASCII range
	//
	if ( ch < 256 ) {
		return ;
	}

	//
	//	Don't do anything if character is in the currently allocated range
	//
	if ( ch >= FirstUnicodeChar && ch <= LastUnicodeChar ) {
		return ;
	}

	uint16 first_index	= min( FirstUnicodeChar, static_cast<uint16>(ch) );
	uint16 last_index		= max( LastUnicodeChar, static_cast<uint16>(ch) );
	uint16 count			= (last_index - first_index) + 1;

	//
	//	Allocate enough memory to hold the new cells
	//
	CharDataStruct **new_array = new CharDataStruct *[count];
	::memset (new_array, 0, sizeof (CharDataStruct *) * count);

	//
	//	Copy the contents of the old array into the new array
	//
	if ( UnicodeCharArray != NULL ) {
		int start_offset	= (FirstUnicodeChar - first_index);
		int old_count		= (LastUnicodeChar - FirstUnicodeChar) + 1;
		::memcpy (&new_array[start_offset], UnicodeCharArray, sizeof (CharDataStruct *) * old_count);

		//
		//	Delete the old array
		//
		delete [] UnicodeCharArray;
		UnicodeCharArray = NULL;
	}

	FirstUnicodeChar	= first_index;
	LastUnicodeChar	= last_index;
	UnicodeCharArray	= new_array;
	return ;
}


////////////////////////////////////////////////////////////////////////////////////
//
//	Free_Character_Arrays
//
////////////////////////////////////////////////////////////////////////////////////
void
FontCharsClass::Free_Character_Arrays (void)
{
	if ( UnicodeCharArray != NULL ) {

		int count = (LastUnicodeChar - FirstUnicodeChar) + 1;

		//
		//	Delete each member of the unicode array
		//
		for (int index = 0; index < count; index ++) {
			if ( UnicodeCharArray[index] != NULL ) {
				delete UnicodeCharArray[index];
				UnicodeCharArray[index] = NULL;
			}
		}

		//
		//	Delete the array itself
		//
		delete [] UnicodeCharArray;
		UnicodeCharArray = NULL;
	}

	//
	//	Delete each member of the ascii character array
	//
	for (int index = 0; index < 256; index ++) {
		if ( ASCIICharArray[index] != NULL ) {
			delete ASCIICharArray[index];
			ASCIICharArray[index] = NULL;
		}
	}

	return ;
}

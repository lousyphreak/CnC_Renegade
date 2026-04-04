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
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dialogparser.cpp          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/25/01 3:54p                                              $*
 *                                                                                             *
 *                    $Revision:: 12                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "dialogparser.h"
#include "win.h"
#include "translatedb.h"
#include <commctrl.h>
#include <cstdint>

#if !defined(_WIN32)
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#endif

//////////////////////////////////////////////////////////////////////////////
//	Macros
//////////////////////////////////////////////////////////////////////////////
#define ALIGN_WORD_PTR(p)	(reinterpret_cast<WORD *>((reinterpret_cast<uintptr_t>(p) + 1u) & ~static_cast<uintptr_t>(1u)))
#define ALIGN_DWORD_PTR(p) (reinterpret_cast<DWORD *>((reinterpret_cast<uintptr_t>(p) + 3u) & ~static_cast<uintptr_t>(3u)))


//////////////////////////////////////////////////////////////////////////////
//	Local prototypes
//////////////////////////////////////////////////////////////////////////////
WORD *Skip_Dlg_Field (WORD *src, WCHAR *buffer = NULL, int buffer_len = 0, WORD *ctrl_type = NULL);

#if !defined(_WIN32)
namespace {

#ifndef WS_TABSTOP
#define WS_TABSTOP 0x00010000L
#endif

#ifndef WS_VSCROLL
#define WS_VSCROLL 0x00200000L
#endif

#ifndef BS_RIGHT
#define BS_RIGHT 0x00000200L
#endif

#ifndef BS_CENTER
#define BS_CENTER 0x00000300L
#endif

#ifndef ES_AUTOHSCROLL
#define ES_AUTOHSCROLL 0x0080L
#endif

#ifndef CBS_DROPDOWNLIST
#define CBS_DROPDOWNLIST 0x0003L
#endif

#ifndef CBS_SORT
#define CBS_SORT 0x0100L
#endif

#ifndef LVS_REPORT
#define LVS_REPORT 0x0001L
#endif

#ifndef LVS_NOCOLUMNHEADER
#define LVS_NOCOLUMNHEADER 0x4000L
#endif

#ifndef TBS_BOTH
#define TBS_BOTH 0x0008L
#endif

#ifndef TBS_NOTICKS
#define TBS_NOTICKS 0x0010L
#endif

std::string Trim_Copy(const std::string &value)
{
	const size_t start = value.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) {
		return {};
	}

	const size_t end = value.find_last_not_of(" \t\r\n");
	return value.substr(start, end - start + 1);
}

bool Statement_Needs_Continuation(const std::string &statement)
{
	if (statement.empty()) {
		return false;
	}

	const char last = statement.back();
	return last == ',' || last == '|';
}

std::string Strip_Line_Comment(const std::string &line)
{
	bool in_quote = false;
	for (size_t index = 0; index + 1 < line.size(); ++index) {
		if (line[index] == '"') {
			in_quote = !in_quote;
		}

		if (!in_quote && line[index] == '/' && line[index + 1] == '/') {
			return line.substr(0, index);
		}
	}

	return line;
}

std::string To_Upper_Copy(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
	return value;
}

bool Try_Parse_Int(const std::string &token, int *value)
{
	if (value == NULL) {
		return false;
	}

	char *end = NULL;
	const long parsed = std::strtol(token.c_str(), &end, 0);
	if (end == token.c_str() || *end != '\0') {
		return false;
	}

	*value = static_cast<int>(parsed);
	return true;
}

bool Evaluate_Expression(const std::string &expression, const std::unordered_map<std::string, int> &defines, int *value)
{
	if (value == NULL) {
		return false;
	}

	std::string normalized = expression;
	for (char &ch : normalized) {
		if (ch == '(' || ch == ')' || ch == '\t') {
			ch = ' ';
		}
	}

	std::istringstream stream(normalized);
	std::string token;
	int result = 0;
	int sign = +1;
	bool have_value = false;

	while (stream >> token) {
		if (token == "+") {
			sign = +1;
			continue;
		}
		if (token == "-") {
			sign = -1;
			continue;
		}

		int part = 0;
		if (!Try_Parse_Int(token, &part)) {
			auto it = defines.find(token);
			if (it == defines.end()) {
				return false;
			}
			part = it->second;
		}

		result += sign * part;
		have_value = true;
		sign = +1;
	}

	if (!have_value) {
		return false;
	}

	*value = result;
	return true;
}

std::filesystem::path Find_Project_File(const std::filesystem::path &relative_path)
{
	std::error_code error;
	std::filesystem::path current = std::filesystem::current_path(error);
	if (error) {
		return {};
	}

	while (!current.empty()) {
		const std::filesystem::path candidate = current / relative_path;
		if (std::filesystem::exists(candidate, error) && std::filesystem::is_regular_file(candidate, error)) {
			return candidate;
		}

		if (current == current.root_path()) {
			break;
		}

		current = current.parent_path();
	}

	return {};
}

const std::unordered_map<std::string, int> &Get_Defines()
{
	static std::unordered_map<std::string, int> defines;
	static bool initialized = false;
	if (initialized) {
		return defines;
	}

	initialized = true;
	const std::filesystem::path resource_header = Find_Project_File("Code/Commando/resource.h");
	const std::filesystem::path dialog_header = Find_Project_File("Code/Commando/dialogresource.h");
	const std::filesystem::path headers[] = { resource_header, dialog_header };

	for (const auto &header_path : headers) {
		if (header_path.empty()) {
			continue;
		}

		std::ifstream file(header_path);
		std::string line;
		while (std::getline(file, line)) {
			line = Trim_Copy(Strip_Line_Comment(line));
			if (line.rfind("#define ", 0) != 0) {
				continue;
			}

			std::istringstream stream(line.substr(8));
			std::string name;
			stream >> name;
			std::string expression;
			std::getline(stream, expression);
			expression = Trim_Copy(expression);
			int value = 0;
			if (!name.empty() && !expression.empty() && Evaluate_Expression(expression, defines, &value)) {
				defines[name] = value;
			}
		}
	}

	return defines;
}

uint32 Resolve_Style_Token(const std::string &token)
{
	const std::string upper = To_Upper_Copy(token);
	if (upper == "WS_BORDER") return WS_BORDER;
	if (upper == "WS_GROUP") return WS_GROUP;
	if (upper == "WS_TABSTOP") return WS_TABSTOP;
	if (upper == "WS_DISABLED") return WS_DISABLED;
	if (upper == "WS_VSCROLL") return WS_VSCROLL;
	if (upper == "BS_LEFT") return BS_LEFT;
	if (upper == "BS_RIGHT") return BS_RIGHT;
	if (upper == "BS_CENTER") return BS_CENTER;
	if (upper == "BS_FLAT") return BS_FLAT;
	if (upper == "BS_OWNERDRAW") return BS_OWNERDRAW;
	if (upper == "BS_CHECKBOX") return BS_CHECKBOX;
	if (upper == "BS_AUTOCHECKBOX") return BS_AUTOCHECKBOX;
	if (upper == "BS_BITMAP") return BS_BITMAP;
	if (upper == "SS_BITMAP") return SS_BITMAP;
	if (upper == "SS_CENTERIMAGE") return SS_CENTERIMAGE;
	if (upper == "SS_LEFTNOWORDWRAP") return SS_LEFTNOWORDWRAP;
	if (upper == "ES_MULTILINE") return ES_MULTILINE;
	if (upper == "ES_AUTOVSCROLL") return ES_AUTOVSCROLL;
	if (upper == "ES_AUTOHSCROLL") return ES_AUTOHSCROLL;
	if (upper == "CBS_DROPDOWNLIST") return CBS_DROPDOWNLIST;
	if (upper == "CBS_SORT") return CBS_SORT;
	if (upper == "LVS_REPORT") return LVS_REPORT;
	if (upper == "LVS_NOCOLUMNHEADER") return LVS_NOCOLUMNHEADER;
	if (upper == "TBS_BOTH") return TBS_BOTH;
	if (upper == "TBS_NOTICKS") return TBS_NOTICKS;
	return 0;
}

uint32 Parse_Style_Expression(const std::string &expression)
{
	std::string normalized = expression;
	std::replace(normalized.begin(), normalized.end(), '|', ' ');
	std::replace(normalized.begin(), normalized.end(), ',', ' ');

	std::istringstream stream(normalized);
	std::string token;
	uint32 value = 0;
	while (stream >> token) {
		value |= Resolve_Style_Token(token);
	}

	return value;
}

std::vector<std::string> Split_Csv_Respecting_Quotes(const std::string &text)
{
	std::vector<std::string> parts;
	std::string current;
	bool in_quote = false;

	for (char ch : text) {
		if (ch == '"') {
			in_quote = !in_quote;
			current += ch;
			continue;
		}

		if (ch == ',' && !in_quote) {
			parts.push_back(Trim_Copy(current));
			current.clear();
			continue;
		}

		current += ch;
	}

	if (!current.empty()) {
		parts.push_back(Trim_Copy(current));
	}

	return parts;
}

WideStringClass Make_Wide_Text(const std::string &text)
{
	std::string unquoted = Trim_Copy(text);
	if (unquoted.size() >= 2 && unquoted.front() == '"' && unquoted.back() == '"') {
		unquoted = unquoted.substr(1, unquoted.size() - 2);
	}

	WideStringClass wide;
	wide.Convert_From(unquoted.c_str());
	return wide;
}

void Resolve_Translation(WideStringClass *text)
{
	if (text == NULL) {
		return;
	}

	std::wstring value = text->Peek_Buffer();
	const size_t position = value.find(L"IDS_");
	if (position == std::wstring::npos) {
		return;
	}

	WideStringClass wide_string_id(value.c_str() + position);
	StringClass ascii_string_id;
	wide_string_id.Convert_To(ascii_string_id);
	WideStringClass translation;
	if (TDBObjClass *object = TranslateDBClass::Find_Object(ascii_string_id)) {
		translation = object->Get_English_String().Peek_Buffer();
	} else {
		translation = ascii_string_id.Peek_Buffer();
	}
	value = value.substr(0, position) + std::wstring(translation.Peek_Buffer());
	*text = value.c_str();
}

CONTROL_TYPE Resolve_Control_Class(const std::string &keyword, const std::string &control_class)
{
	const std::string upper_keyword = To_Upper_Copy(keyword);
	const std::string upper_class = To_Upper_Copy(control_class);

	if (upper_keyword == "PUSHBUTTON" || upper_keyword == "DEFPUSHBUTTON") return BUTTON;
	if (upper_keyword == "LTEXT" || upper_keyword == "RTEXT" || upper_keyword == "CTEXT") return STATIC;
	if (upper_keyword == "EDITTEXT") return EDIT;
	if (upper_keyword == "COMBOBOX") return COMBOBOX;
	if (upper_keyword == "LISTBOX") return LIST_BOX;

	if (upper_class.find("BUTTON") != std::string::npos) return BUTTON;
	if (upper_class.find("STATIC") != std::string::npos) return STATIC;
	if (upper_class.find("TRACKBAR") != std::string::npos) return SLIDER;
	if (upper_class.find("TABCONTROL") != std::string::npos) return TAB;
	if (upper_class.find("LISTVIEW") != std::string::npos) return LIST_CTRL;
	if (upper_class.find("HOTKEY") != std::string::npos) return HOTKEY;
	if (upper_class.find("TREEVIEW") != std::string::npos) return TREE_CTRL;
	if (upper_class.find("MAP") != std::string::npos) return MAP;
	if (upper_class.find("VIEWER") != std::string::npos) return VIEWER;
	if (upper_class.find("SHORTCUTBAR") != std::string::npos) return SHORTCUT_BAR;
	if (upper_class.find("MERCHANDISE") != std::string::npos) return MERCHANDISE_CTRL;
	if (upper_class.find("MSCTLS_PROGRESS32") != std::string::npos) return PROGRESS_BAR;
	if (upper_class.find("HEALTHBAR") != std::string::npos) return HEALTH_BAR;
	return BUTTON;
}

bool Resolve_Id(const std::string &token, const std::unordered_map<std::string, int> &defines, int *value)
{
	if (Try_Parse_Int(token, value)) {
		return true;
	}

	auto it = defines.find(token);
	if (it == defines.end()) {
		return false;
	}

	*value = it->second;
	return true;
}

bool Parse_Control_Statement(const std::string &statement, const std::unordered_map<std::string, int> &defines, DynamicVectorClass<ControlDefinitionStruct> *control_list)
{
	if (control_list == NULL) {
		return false;
	}

	std::istringstream keyword_stream(statement);
	std::string keyword;
	keyword_stream >> keyword;
	if (keyword.empty()) {
		return false;
	}

	const std::string rest = Trim_Copy(statement.substr(keyword.length()));
	const std::vector<std::string> parts = Split_Csv_Respecting_Quotes(rest);
	if (parts.empty()) {
		return false;
	}

	ControlDefinitionStruct definition;
	std::string control_class;
	std::string style_expression;

	if (To_Upper_Copy(keyword) == "CONTROL") {
		if (parts.size() < 8) {
			return false;
		}

		definition.title = Make_Wide_Text(parts[0]);
		Resolve_Translation(&definition.title);
		if (!Resolve_Id(parts[1], defines, &definition.id)) {
			return false;
		}
		control_class = parts[2];
		style_expression = parts[3];
		definition.type = Resolve_Control_Class(keyword, control_class);
		if (!Try_Parse_Int(parts[4], &definition.x) || !Try_Parse_Int(parts[5], &definition.y) || !Try_Parse_Int(parts[6], &definition.cx) || !Try_Parse_Int(parts[7], &definition.cy)) {
			return false;
		}
	} else if (To_Upper_Copy(keyword) == "EDITTEXT") {
		if (parts.size() < 5) {
			return false;
		}

		definition.title = L"";
		if (!Resolve_Id(parts[0], defines, &definition.id)) {
			return false;
		}
		definition.type = EDIT;
		if (!Try_Parse_Int(parts[1], &definition.x) || !Try_Parse_Int(parts[2], &definition.y) || !Try_Parse_Int(parts[3], &definition.cx) || !Try_Parse_Int(parts[4], &definition.cy)) {
			return false;
		}
		if (parts.size() > 5) {
			style_expression = parts[5];
		}
	} else if (To_Upper_Copy(keyword) == "COMBOBOX") {
		if (parts.size() < 6) {
			return false;
		}

		definition.title = L"";
		if (!Resolve_Id(parts[0], defines, &definition.id)) {
			return false;
		}
		definition.type = COMBOBOX;
		if (!Try_Parse_Int(parts[1], &definition.x) || !Try_Parse_Int(parts[2], &definition.y) || !Try_Parse_Int(parts[3], &definition.cx) || !Try_Parse_Int(parts[4], &definition.cy)) {
			return false;
		}
		style_expression = parts[5];
	} else {
		if (parts.size() < 6) {
			return false;
		}

		definition.title = Make_Wide_Text(parts[0]);
		Resolve_Translation(&definition.title);
		if (!Resolve_Id(parts[1], defines, &definition.id)) {
			return false;
		}
		definition.type = Resolve_Control_Class(keyword, std::string());
		if (!Try_Parse_Int(parts[2], &definition.x) || !Try_Parse_Int(parts[3], &definition.y) || !Try_Parse_Int(parts[4], &definition.cx) || !Try_Parse_Int(parts[5], &definition.cy)) {
			return false;
		}
		if (parts.size() > 6) {
			style_expression = parts[6];
		}
	}

	definition.style = Parse_Style_Expression(style_expression);
	control_list->Add(definition);
	return true;
}

bool Parse_Template_From_Rc_Source(int res_id, int *dlg_width, int *dlg_height, WideStringClass *dlg_title, DynamicVectorClass<ControlDefinitionStruct> *control_list)
{
	const std::filesystem::path rc_path = Find_Project_File("Code/Commando/chat.rc");
	if (rc_path.empty()) {
		return false;
	}

	const std::unordered_map<std::string, int> &defines = Get_Defines();
	std::ifstream file(rc_path);
	std::string line;
	bool in_target_dialog = false;
	bool in_dialog_body = false;

	while (std::getline(file, line)) {
		line = Trim_Copy(Strip_Line_Comment(line));
		if (line.empty()) {
			continue;
		}

		if (!in_target_dialog) {
			const size_t dialog_pos = line.find(" DIALOG");
			if (dialog_pos == std::string::npos) {
				continue;
			}

			const std::string name = Trim_Copy(line.substr(0, dialog_pos));
			int dialog_id = 0;
			if (!Resolve_Id(name, defines, &dialog_id) || dialog_id != res_id) {
				continue;
			}

			std::smatch matches;
			if (std::regex_search(line, matches, std::regex("(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*$")) && matches.size() == 5) {
				Try_Parse_Int(matches[3].str(), dlg_width);
				Try_Parse_Int(matches[4].str(), dlg_height);
			}

			in_target_dialog = true;
			continue;
		}

		if (!in_dialog_body) {
			if (line.rfind("CAPTION ", 0) == 0) {
				*dlg_title = Make_Wide_Text(line.substr(8));
				Resolve_Translation(dlg_title);
			} else if (line == "BEGIN") {
				in_dialog_body = true;
			}
			continue;
		}

		if (line == "END") {
			return true;
		}

		std::string statement = line;
		while (Statement_Needs_Continuation(statement) && std::getline(file, line)) {
			statement += " " + Trim_Copy(Strip_Line_Comment(line));
		}

		Parse_Control_Statement(statement, defines, control_list);
	}

	return false;
}

} // namespace
#endif


//////////////////////////////////////////////////////////////////////////////
//
//	Skip_Dlg_Field
//
//////////////////////////////////////////////////////////////////////////////
WORD *
Skip_Dlg_Field (WORD *src, WCHAR *buffer, int buffer_len, WORD *ctrl_type)
{
	//
	//	These fields always start on the next word boundary, so align
	//	the source pointer on this boundary.
	//
	WORD *retval = ALIGN_WORD_PTR(src);

	//
	//	Note:  The field codes are as follows:
	//
	//		0xFFFF		- The following WORD is an ordinal value of a system class.
	//		0x0000		- Empty field
	//		Otherwise	- The remaining data is a NULL terminated WCHAR string.
	//
	if (*retval == 0xFFFF) {
		
		//
		//	Move past the field designator
		//
		retval ++;
		
		//
		//	Does the user want information about the ctrl type?
		//
		if (ctrl_type != NULL) {
			*ctrl_type = *retval;
		}

		//
		//	Move past the ctrl type identifier
		//
		retval ++;
	} else if (*retval == 0x0000) {
		
		//
		//	Null terminate the string if the user is expecting data
		//
		if (buffer != NULL) {
			*buffer = 0;
		}

		//
		//	Move past the field designator
		//
		retval ++;
	} else {

		//
		//	The following data is a null-terminated string.  Scan
		// as much data into our desination buffer as possible.
		//	Note:  The data is stored in wide character format.
		//
		while (*retval != 0x0000) {
			if (buffer != NULL && buffer_len > 1) {
				
				//
				//	Store this character in the supplied buffer
				// and decrement the remaining buffer length.
				//
				*buffer++ = *retval;
				buffer_len --;
			}
			retval ++;
		}

		//
		//	Ensure the supplied buffer is NULL terminated
		//
		if (buffer != NULL) {
			*buffer = 0;
		}

		//
		//	Advance to the next field
		//
		retval ++;
	}

	//
	//	Return the new buffer position to the caller
	//
	return retval;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Parse_Template
//
//////////////////////////////////////////////////////////////////////////////
void
DialogParserClass::Parse_Template
(
	int															res_id,
	int *															dlg_width,
	int *															dlg_height,
	WideStringClass *											dlg_title,
	DynamicVectorClass<ControlDefinitionStruct> *	control_list
)
{
	#if !defined(_WIN32)
	if (dlg_width != NULL) {
		*dlg_width = 0;
	}
	if (dlg_height != NULL) {
		*dlg_height = 0;
	}
	if (dlg_title != NULL) {
		*dlg_title = L"";
	}
	if (control_list != NULL) {
		control_list->Delete_All();
	}

	Parse_Template_From_Rc_Source(res_id, dlg_width, dlg_height, dlg_title, control_list);
	return;
	#else
	//
	//	Load the resource file
	//
	HRSRC resource		= ::FindResource (ProgramInstance, MAKEINTRESOURCE (res_id), RT_DIALOG);
	HGLOBAL hglobal	= ::LoadResource (ProgramInstance, resource);
	LPVOID res_buffer	= ::LockResource (hglobal);
	if(res_buffer != NULL) {

		//
		//	The first few bytes of the resource buffer are the DLGTEMPLATE structure
		//
		DLGTEMPLATE *dlg_template = (DLGTEMPLATE *)res_buffer;
		(*dlg_width)	= (int)dlg_template->cx;
		(*dlg_height)	= (int)dlg_template->cy;

		//
		//	Move past the DLGTEMPLATE header to the other fields
		//
		WORD *buffer = (WORD *)(((char *)res_buffer) + sizeof (DLGTEMPLATE));
		
		//
		//	Skip the menu, and window class
		//
		buffer = Skip_Dlg_Field (buffer);
		buffer = Skip_Dlg_Field (buffer);

		//
		//	Read the title
		//
		buffer = Skip_Dlg_Field (buffer, dlg_title->Get_Buffer (96), 96);

		WCHAR *string_id = ::wcsstr (dlg_title->Peek_Buffer (), L"IDS_");
		if (string_id != NULL) {
			WideStringClass wide_string_id = string_id;				
			StringClass ascii_string_id;
			wide_string_id.Convert_To (ascii_string_id);
			(*dlg_title) = TRANSLATE_BY_DESC(ascii_string_id);
		}


		//
		//	Do we need to skip past the font settings?
		//
		if (dlg_template->style & DS_SETFONT) {
			buffer ++;
			while (*buffer != 0x0000) {
				buffer ++;
			}
			buffer ++;
		}

		//
		//	Loop over each control and gather information about them
		//
		for (int index = 0; index < dlg_template->cdit; index ++) {
			DLGITEMTEMPLATE *dlg_item_template = (DLGITEMTEMPLATE *)ALIGN_DWORD_PTR((DWORD *)buffer);
			buffer = (WORD *)(((char *)dlg_item_template) + sizeof (DLGITEMTEMPLATE));

			//
			//	Read the ctrl type
			//
			WCHAR text_buffer[256]	= { 0 };
			WORD ctrl_type				= 0x0000;
			buffer = Skip_Dlg_Field (buffer, text_buffer, 256, &ctrl_type);
			
			//
			//	Wasn't one of the standard types, so see if we can determine
			// what it is by its class name.
			//
			if (ctrl_type == 0) {
				::_wcsupr (text_buffer);
				if (::wcsstr (text_buffer, L"TRACKBAR") != 0) {
					ctrl_type = SLIDER;
				} else if (::wcsstr (text_buffer, L"TABCONTROL") != 0) {
					ctrl_type = TAB;
				} else if (::wcsstr (text_buffer, L"LISTVIEW") != 0) {
					ctrl_type = LIST_CTRL;
				} else if (::wcsstr (text_buffer, L"MAP") != 0) {
					ctrl_type = MAP;
				} else if (::wcsstr (text_buffer, L"VIEWER") != 0) {
					ctrl_type = VIEWER;
				} else if (::wcsstr (text_buffer, L"HOTKEY") != 0) {
					ctrl_type = HOTKEY;
				} else if (::wcsstr (text_buffer, L"SHORTCUTBAR") != 0) {
					ctrl_type = SHORTCUT_BAR;
				} else if (::wcsstr (text_buffer, L"MERCHANDISE") != 0) {
					ctrl_type = MERCHANDISE_CTRL;
				} else if (::wcsstr (text_buffer, L"TREEVIEW") != 0) {
					ctrl_type = TREE_CTRL;
				} else if (::wcsicmp(text_buffer, PROGRESS_CLASSW) == 0) {
					ctrl_type = PROGRESS_BAR;
				} else if (::wcsstr (text_buffer, L"HEALTHBAR") != 0) {
					ctrl_type = HEALTH_BAR;
				}						
			}

			//
			//	Read the window text
			//			
			buffer = Skip_Dlg_Field (buffer, text_buffer, 256);

			WCHAR *string_id = ::wcsstr (text_buffer, L"IDS_");
			if (string_id != NULL) {
				WideStringClass wide_string_id = string_id;				
				StringClass ascii_string_id;
				wide_string_id.Convert_To (ascii_string_id);
				WideStringClass translation = TRANSLATE_BY_DESC(ascii_string_id);
				::wcscpy (string_id, translation);
			}

			//
			//	Add this control definition to the list
			//
			ControlDefinitionStruct definition;
			definition.id		= (int)dlg_item_template->id;
			definition.style	= dlg_item_template->style;
			definition.x		= dlg_item_template->x;
			definition.y		= dlg_item_template->y;
			definition.cx		= dlg_item_template->cx;
			definition.cy		= dlg_item_template->cy;
			definition.type	= (CONTROL_TYPE)ctrl_type;
			definition.title	= text_buffer;
			control_list->Add (definition);

			//
			//	Skip past the extra data
			//
			WORD extra_data_size = *buffer;
			buffer ++;
			if (extra_data_size > 0) {
				buffer = (WORD *)(((char *)ALIGN_WORD_PTR(buffer)) + extra_data_size);
			}
		}
	}

	return ;
	#endif
}

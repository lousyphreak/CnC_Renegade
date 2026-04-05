#include "registry.h"

#include "wwlib_debug.h"

#include <SDL3/SDL_filesystem.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

bool RegistryClass::IsLocked = false;

namespace
{
	using RegistrySection = std::map<std::string, std::string>;
	using RegistryDocument = std::map<std::string, RegistrySection>;

	std::mutex & Registry_Mutex()
	{
		static std::mutex mutex;
		return mutex;
	}

	std::string Registry_Storage_File()
	{
		char * pref_path = SDL_GetPrefPath("Electronic Arts", "Renegade");
		if (pref_path == nullptr) {
			return "renegade_registry.ini";
		}

		std::string result(pref_path);
		SDL_free(pref_path);
		result += "registry_store.ini";
		return result;
	}

	std::string Trim(const std::string & value)
	{
		std::size_t start = 0;
		while (start < value.size() && std::isspace(static_cast<uint8_t>(value[start])) != 0) {
			++start;
		}

		std::size_t end = value.size();
		while (end > start && std::isspace(static_cast<uint8_t>(value[end - 1])) != 0) {
			--end;
		}

		return value.substr(start, end - start);
	}

	std::string Escape(const std::string & value)
	{
		std::string escaped;
		escaped.reserve(value.size());
		for (char ch : value) {
			switch (ch) {
				case '\\': escaped += "\\\\"; break;
				case '\n': escaped += "\\n"; break;
				case '\r': escaped += "\\r"; break;
				case '\t': escaped += "\\t"; break;
				default: escaped += ch; break;
			}
		}
		return escaped;
	}

	std::string Unescape(const std::string & value)
	{
		std::string unescaped;
		unescaped.reserve(value.size());
		for (std::size_t index = 0; index < value.size(); ++index) {
			char ch = value[index];
			if (ch == '\\' && (index + 1) < value.size()) {
				char next = value[++index];
				switch (next) {
					case 'n': unescaped += '\n'; break;
					case 'r': unescaped += '\r'; break;
					case 't': unescaped += '\t'; break;
					case '\\': unescaped += '\\'; break;
					default: unescaped += next; break;
				}
			} else {
				unescaped += ch;
			}
		}
		return unescaped;
	}

	std::string Encode_Hex(const void * data, int size)
	{
		static const char hex_digits[] = "0123456789ABCDEF";
		const uint8_t * bytes = static_cast<const uint8_t *>(data);
		std::string encoded;
		encoded.reserve(static_cast<std::size_t>(size) * 2);
		for (int index = 0; index < size; ++index) {
			encoded += hex_digits[(bytes[index] >> 4) & 0x0F];
			encoded += hex_digits[bytes[index] & 0x0F];
		}
		return encoded;
	}

	int Decode_Hex(const std::string & text, void * buffer, int buffer_size)
	{
		if ((text.size() & 1u) != 0) {
			return 0;
		}

		auto nibble = [](char ch) -> int {
			if (ch >= '0' && ch <= '9') {
				return ch - '0';
			}
			ch = static_cast<char>(std::toupper(static_cast<uint8_t>(ch)));
			if (ch >= 'A' && ch <= 'F') {
				return 10 + (ch - 'A');
			}
			return -1;
		};

		uint8_t * bytes = static_cast<uint8_t *>(buffer);
		const int byte_count = std::min<int>(buffer_size, static_cast<int>(text.size() / 2));
		for (int index = 0; index < byte_count; ++index) {
			const int high = nibble(text[static_cast<std::size_t>(index) * 2]);
			const int low = nibble(text[(static_cast<std::size_t>(index) * 2) + 1]);
			if (high < 0 || low < 0) {
				return index;
			}
			bytes[index] = static_cast<uint8_t>((high << 4) | low);
		}
		return byte_count;
	}

	RegistryDocument Load_Document()
	{
		RegistryDocument document;
		std::ifstream input(Registry_Storage_File());
		if (!input) {
			return document;
		}

		std::string current_section;
		std::string line;
		while (std::getline(input, line)) {
			line = Trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}

			if (line.front() == '[' && line.back() == ']') {
				current_section = line.substr(1, line.size() - 2);
				document[current_section];
				continue;
			}

			const std::size_t divider = line.find('=');
			if (divider == std::string::npos || current_section.empty()) {
				continue;
			}

			const std::string key = Trim(line.substr(0, divider));
			const std::string value = Unescape(line.substr(divider + 1));
			document[current_section][key] = value;
		}

		return document;
	}

	void Save_Document(const RegistryDocument & document)
	{
		std::ofstream output(Registry_Storage_File(), std::ios::trunc);
		for (const auto & section_pair : document) {
			output << '[' << section_pair.first << "]\n";
			for (const auto & value_pair : section_pair.second) {
				output << value_pair.first << '=' << Escape(value_pair.second) << "\n";
			}
			output << "\n";
		}
	}

	void Ensure_Key_Marker(RegistryDocument & document, const std::string & key_path)
	{
		document[key_path]["__key__"] = "k:1";
	}

	std::string Build_Prefix(char prefix, const std::string & payload)
	{
		std::string value;
		value.reserve(payload.size() + 2);
		value += prefix;
		value += ':';
		value += payload;
		return value;
	}

	bool Try_Get_Typed_Value(const RegistryDocument & document, const std::string & key_path, const char * name, char expected_prefix, std::string & payload)
	{
		auto section_it = document.find(key_path);
		if (section_it == document.end()) {
			return false;
		}

		auto value_it = section_it->second.find(name);
		if (value_it == section_it->second.end() || value_it->second.size() < 2 || value_it->second[0] != expected_prefix || value_it->second[1] != ':') {
			return false;
		}

		payload = value_it->second.substr(2);
		return true;
	}

	bool Section_Matches_Tree(const std::string & section_name, const char * path)
	{
		const std::string prefix(path == nullptr ? "" : path);
		if (section_name == prefix) {
			return true;
		}
		if (section_name.size() <= prefix.size()) {
			return false;
		}
		return (section_name.compare(0, prefix.size(), prefix) == 0) && (section_name[prefix.size()] == '\\');
	}

	bool Is_High_Surrogate(uint32_t value)
	{
		return value >= 0xD800 && value <= 0xDBFF;
	}

	bool Is_Low_Surrogate(uint32_t value)
	{
		return value >= 0xDC00 && value <= 0xDFFF;
	}

	void Set_Empty_Wide_String(WideStringClass & out)
	{
		WCHAR * buffer = out.Get_Buffer(1);
		buffer[0] = 0;
	}

	void Append_UTF8(std::string & text, uint32_t codepoint)
	{
		if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
			codepoint = 0xFFFD;
		}

		if (codepoint < 0x80) {
			text += static_cast<char>(codepoint);
		} else if (codepoint < 0x800) {
			text += static_cast<char>(0xC0 | (codepoint >> 6));
			text += static_cast<char>(0x80 | (codepoint & 0x3F));
		} else if (codepoint < 0x10000) {
			text += static_cast<char>(0xE0 | (codepoint >> 12));
			text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			text += static_cast<char>(0x80 | (codepoint & 0x3F));
		} else {
			text += static_cast<char>(0xF0 | (codepoint >> 18));
			text += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
			text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
			text += static_cast<char>(0x80 | (codepoint & 0x3F));
		}
	}

	uint32_t Decode_UTF8_Codepoint(const std::string & value, std::size_t & index)
	{
		const std::size_t start = index;
		const uint8_t lead = static_cast<uint8_t>(value[index++]);
		if (lead < 0x80) {
			return lead;
		}

		int continuation_count = 0;
		uint32_t codepoint = 0;
		uint32_t minimum = 0;

		if ((lead & 0xE0) == 0xC0) {
			continuation_count = 1;
			codepoint = lead & 0x1F;
			minimum = 0x80;
		} else if ((lead & 0xF0) == 0xE0) {
			continuation_count = 2;
			codepoint = lead & 0x0F;
			minimum = 0x800;
		} else if ((lead & 0xF8) == 0xF0) {
			continuation_count = 3;
			codepoint = lead & 0x07;
			minimum = 0x10000;
		} else {
			index = start + 1;
			return 0xFFFD;
		}

		std::size_t read_index = index;
		for (int continuation = 0; continuation < continuation_count; ++continuation) {
			if (read_index >= value.size()) {
				index = start + 1;
				return 0xFFFD;
			}

			const uint8_t next = static_cast<uint8_t>(value[read_index]);
			if ((next & 0xC0) != 0x80) {
				index = start + 1;
				return 0xFFFD;
			}

			codepoint = (codepoint << 6) | (next & 0x3F);
			++read_index;
		}

		if (codepoint < minimum || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
			index = start + 1;
			return 0xFFFD;
		}

		index = read_index;
		return codepoint;
	}

	std::string Wide_To_UTF8(const WCHAR * value)
	{
		if (value == nullptr) {
			return std::string();
		}

		std::string text;
		for (std::size_t index = 0; value[index] != 0; ++index) {
			uint32_t codepoint = static_cast<uint32_t>(value[index]);

			if (sizeof(WCHAR) == sizeof(uint16_t)) {
				if (Is_High_Surrogate(codepoint)) {
					const uint32_t trail = static_cast<uint32_t>(value[index + 1]);
					if (Is_Low_Surrogate(trail)) {
						codepoint =
							0x10000 +
							(((codepoint - 0xD800) << 10) | (trail - 0xDC00));
						++index;
					} else {
						codepoint = 0xFFFD;
					}
				} else if (Is_Low_Surrogate(codepoint)) {
					codepoint = 0xFFFD;
				}
			}

			Append_UTF8(text, codepoint);
		}

		return text;
	}

	void UTF8_To_Wide(const std::string & value, WideStringClass & out)
	{
		if (value.empty()) {
			Set_Empty_Wide_String(out);
			return;
		}

		std::vector<uint32_t> codepoints;
		codepoints.reserve(value.size());
		for (std::size_t index = 0; index < value.size();) {
			codepoints.push_back(Decode_UTF8_Codepoint(value, index));
		}

		if (sizeof(WCHAR) == sizeof(uint16_t)) {
			int code_unit_count = 0;
			for (uint32_t codepoint : codepoints) {
				code_unit_count += (codepoint > 0xFFFF) ? 2 : 1;
			}

			WCHAR * buffer = out.Get_Buffer(code_unit_count + 1);
			int output_index = 0;
			for (uint32_t codepoint : codepoints) {
				if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
					codepoint = 0xFFFD;
				}

				if (codepoint > 0xFFFF) {
					const uint32_t surrogate = codepoint - 0x10000;
					buffer[output_index++] = static_cast<WCHAR>(0xD800 + (surrogate >> 10));
					buffer[output_index++] = static_cast<WCHAR>(0xDC00 + (surrogate & 0x3FF));
				} else {
					buffer[output_index++] = static_cast<WCHAR>(codepoint);
				}
			}
			buffer[output_index] = 0;
			return;
		}

		WCHAR * buffer = out.Get_Buffer(static_cast<int>(codepoints.size()) + 1);
		int output_index = 0;
		for (uint32_t codepoint : codepoints) {
			if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
				codepoint = 0xFFFD;
			}
			buffer[output_index++] = static_cast<WCHAR>(codepoint);
		}
		buffer[output_index] = 0;
	}
}

bool RegistryClass::Exists(const char * sub_key)
{
	if (sub_key == NULL || *sub_key == '\0') {
		return false;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	const RegistryDocument document = Load_Document();
	return document.find(sub_key) != document.end();
}

RegistryClass::RegistryClass(const char * sub_key, bool create)
	: KeyPath(sub_key == NULL ? "" : sub_key), IsValid(sub_key != NULL && *sub_key != '\0')
{
	if (!IsValid) {
		return;
	}

	if (create && !IsLocked) {
		std::lock_guard<std::mutex> lock(Registry_Mutex());
		RegistryDocument document = Load_Document();
		Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
		Save_Document(document);
	}
}

RegistryClass::~RegistryClass(void)
{
	IsValid = false;
}

int RegistryClass::Get_Int(const char * name, int def_value)
{
	WWASSERT(IsValid);
	std::lock_guard<std::mutex> lock(Registry_Mutex());
	std::string payload;
	if (Try_Get_Typed_Value(Load_Document(), KeyPath.Peek_Buffer(), name, 'i', payload)) {
		return std::atoi(payload.c_str());
	}
	return def_value;
}

void RegistryClass::Set_Int(const char * name, int value)
{
	WWASSERT(IsValid);
	if (IsLocked) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
	document[KeyPath.Peek_Buffer()][name] = Build_Prefix('i', std::to_string(value));
	Save_Document(document);
}

bool RegistryClass::Get_Bool(const char * name, bool def_value)
{
	return Get_Int(name, def_value ? 1 : 0) != 0;
}

void RegistryClass::Set_Bool(const char * name, bool value)
{
	Set_Int(name, value ? 1 : 0);
}

float RegistryClass::Get_Float(const char * name, float def_value)
{
	WWASSERT(IsValid);
	std::lock_guard<std::mutex> lock(Registry_Mutex());
	std::string payload;
	if (Try_Get_Typed_Value(Load_Document(), KeyPath.Peek_Buffer(), name, 'f', payload)) {
		return static_cast<float>(std::atof(payload.c_str()));
	}
	return def_value;
}

void RegistryClass::Set_Float(const char * name, float value)
{
	WWASSERT(IsValid);
	if (IsLocked) {
		return;
	}

	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "%f", value);
	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
	document[KeyPath.Peek_Buffer()][name] = Build_Prefix('f', buffer);
	Save_Document(document);
}

int RegistryClass::Get_Bin_Size(const char * name)
{
	WWASSERT(IsValid);
	std::lock_guard<std::mutex> lock(Registry_Mutex());
	std::string payload;
	if (Try_Get_Typed_Value(Load_Document(), KeyPath.Peek_Buffer(), name, 'b', payload)) {
		return static_cast<int>(payload.size() / 2);
	}
	return 0;
}

void RegistryClass::Get_Bin(const char * name, void * buffer, int buffer_size)
{
	WWASSERT(IsValid);
	WWASSERT(buffer != NULL);
	WWASSERT(buffer_size > 0);

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	std::string payload;
	if (Try_Get_Typed_Value(Load_Document(), KeyPath.Peek_Buffer(), name, 'b', payload)) {
		Decode_Hex(payload, buffer, buffer_size);
	}
}

void RegistryClass::Set_Bin(const char * name, const void * buffer, int buffer_size)
{
	WWASSERT(IsValid);
	WWASSERT(buffer != NULL);
	WWASSERT(buffer_size > 0);
	if (IsLocked) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
	document[KeyPath.Peek_Buffer()][name] = Build_Prefix('b', Encode_Hex(buffer, buffer_size));
	Save_Document(document);
}

void RegistryClass::Get_String(const char * name, StringClass & string, const char * default_string)
{
	char buffer[1024];
	Get_String(name, buffer, sizeof(buffer), default_string);
	string = buffer;
}

char * RegistryClass::Get_String(const char * name, char * value, int value_size, const char * default_string)
{
	WWASSERT(IsValid);
	WWASSERT(value != NULL);
	WWASSERT(value_size > 0);

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	std::string payload;
	if (Try_Get_Typed_Value(Load_Document(), KeyPath.Peek_Buffer(), name, 's', payload)) {
		std::snprintf(value, static_cast<std::size_t>(value_size), "%s", payload.c_str());
	} else if (default_string != NULL) {
		std::snprintf(value, static_cast<std::size_t>(value_size), "%s", default_string);
	} else {
		value[0] = '\0';
	}
	return value;
}

void RegistryClass::Set_String(const char * name, const char * value)
{
	WWASSERT(IsValid);
	if (IsLocked) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
	document[KeyPath.Peek_Buffer()][name] = Build_Prefix('s', value == NULL ? std::string() : std::string(value));
	Save_Document(document);
}

void RegistryClass::Get_String(const WCHAR * name, WideStringClass & string, const WCHAR * default_string)
{
	WWASSERT(IsValid);
	std::lock_guard<std::mutex> lock(Registry_Mutex());
	std::string payload;
	const std::string utf8_name = Wide_To_UTF8(name);
	if (Try_Get_Typed_Value(Load_Document(), KeyPath.Peek_Buffer(), utf8_name.c_str(), 'w', payload)) {
		UTF8_To_Wide(payload, string);
	} else if (default_string != NULL) {
		string = default_string;
	} else {
		Set_Empty_Wide_String(string);
	}
}

void RegistryClass::Set_String(const WCHAR * name, const WCHAR * value)
{
	WWASSERT(IsValid);
	if (IsLocked) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
	document[KeyPath.Peek_Buffer()][Wide_To_UTF8(name)] = Build_Prefix('w', Wide_To_UTF8(value));
	Save_Document(document);
}

void RegistryClass::Get_Value_List(DynamicVectorClass<StringClass> & list)
{
	WWASSERT(IsValid);
	std::lock_guard<std::mutex> lock(Registry_Mutex());
	const RegistryDocument document = Load_Document();
	auto section_it = document.find(KeyPath.Peek_Buffer());
	if (section_it == document.end()) {
		return;
	}

	for (const auto & value_pair : section_it->second) {
		if (value_pair.first != "__key__") {
			list.Add(value_pair.first.c_str());
		}
	}
}

void RegistryClass::Delete_Value(const char * name)
{
	if (IsLocked || !IsValid) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	auto section_it = document.find(KeyPath.Peek_Buffer());
	if (section_it != document.end()) {
		section_it->second.erase(name);
		Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
		Save_Document(document);
	}
}

void RegistryClass::Deleta_All_Values(void)
{
	if (IsLocked || !IsValid) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	document[KeyPath.Peek_Buffer()].clear();
	Ensure_Key_Marker(document, KeyPath.Peek_Buffer());
	Save_Document(document);
}

void RegistryClass::Delete_Registry_Tree(char * path)
{
	if (IsLocked || path == NULL) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	for (auto it = document.begin(); it != document.end();) {
		if (Section_Matches_Tree(it->first, path)) {
			it = document.erase(it);
		} else {
			++it;
		}
	}
	Save_Document(document);
}

void RegistryClass::Save_Registry(const char * filename, char * path)
{
	if (filename == NULL || path == NULL) {
		return;
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	const RegistryDocument document = Load_Document();
	std::ofstream output(filename, std::ios::trunc);
	for (const auto & section_pair : document) {
		if (!Section_Matches_Tree(section_pair.first, path)) {
			continue;
		}
		output << '[' << section_pair.first << "]\n";
		for (const auto & value_pair : section_pair.second) {
			output << value_pair.first << '=' << Escape(value_pair.second) << "\n";
		}
		output << "\n";
	}
}

void RegistryClass::Load_Registry(const char * filename, char * old_path, char * new_path)
{
	if (IsLocked || filename == NULL || old_path == NULL || new_path == NULL) {
		return;
	}

	RegistryDocument imported;
	std::ifstream input(filename);
	if (!input) {
		return;
	}

	std::string current_section;
	std::string line;
	while (std::getline(input, line)) {
		line = Trim(line);
		if (line.empty() || line[0] == ';' || line[0] == '#') {
			continue;
		}
		if (line.front() == '[' && line.back() == ']') {
			current_section = line.substr(1, line.size() - 2);
			imported[current_section];
			continue;
		}
		const std::size_t divider = line.find('=');
		if (divider == std::string::npos || current_section.empty()) {
			continue;
		}
		imported[current_section][Trim(line.substr(0, divider))] = Unescape(line.substr(divider + 1));
	}

	std::lock_guard<std::mutex> lock(Registry_Mutex());
	RegistryDocument document = Load_Document();
	const std::string old_prefix(old_path);
	const std::string new_prefix(new_path);
	for (const auto & section_pair : imported) {
		std::string remapped = section_pair.first;
		if (remapped.compare(0, old_prefix.size(), old_prefix) == 0) {
			remapped = new_prefix + remapped.substr(old_prefix.size());
		}
		document[remapped] = section_pair.second;
		Ensure_Key_Marker(document, remapped);
	}
	Save_Document(document);
}

#include "verchk.h"
#include "osdep.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <vector>

namespace
{
	constexpr std::uint16_t PE_DOS_SIGNATURE = 0x5A4Du;
	constexpr std::uint32_t PE_NT_SIGNATURE = 0x00004550u;
	constexpr std::uint16_t PE32_MAGIC = 0x10Bu;
	constexpr std::uint16_t PE32_PLUS_MAGIC = 0x20Bu;
	constexpr std::uint16_t RT_VERSION_RESOURCE_ID = 16u;
	constexpr std::uint32_t VS_FIXEDFILEINFO_SIGNATURE = 0xFEEF04BDu;
	constexpr std::size_t PE32_DATA_DIRECTORY_OFFSET = 96u;
	constexpr std::size_t PE32_PLUS_DATA_DIRECTORY_OFFSET = 112u;
	constexpr std::size_t RESOURCE_DATA_DIRECTORY_INDEX = 2u;

#pragma pack(push, 1)
	struct WWLib_DOS_Header
	{
		std::uint16_t Signature;
		std::uint8_t Reserved[58];
		std::int32_t NtHeaderOffset;
	};

	struct WWLib_PE_File_Header
	{
		std::uint16_t Machine;
		std::uint16_t NumberOfSections;
		std::uint32_t TimeDateStamp;
		std::uint32_t PointerToSymbolTable;
		std::uint32_t NumberOfSymbols;
		std::uint16_t SizeOfOptionalHeader;
		std::uint16_t Characteristics;
	};

	struct WWLib_PE_Section_Header
	{
		char Name[8];
		std::uint32_t VirtualSize;
		std::uint32_t VirtualAddress;
		std::uint32_t SizeOfRawData;
		std::uint32_t PointerToRawData;
		std::uint32_t PointerToRelocations;
		std::uint32_t PointerToLineNumbers;
		std::uint16_t NumberOfRelocations;
		std::uint16_t NumberOfLineNumbers;
		std::uint32_t Characteristics;
	};

	struct WWLib_Resource_Directory
	{
		std::uint32_t Characteristics;
		std::uint32_t TimeDateStamp;
		std::uint16_t MajorVersion;
		std::uint16_t MinorVersion;
		std::uint16_t NumberOfNamedEntries;
		std::uint16_t NumberOfIdEntries;
	};

	struct WWLib_Resource_Directory_Entry
	{
		std::uint32_t Name;
		std::uint32_t OffsetToData;
	};

	struct WWLib_Resource_Data_Entry
	{
		std::uint32_t OffsetToData;
		std::uint32_t Size;
		std::uint32_t CodePage;
		std::uint32_t Reserved;
	};
#pragma pack(pop)

	template <typename T>
	const T * View_Struct(const std::vector<std::uint8_t> & file_data, std::size_t offset)
	{
		if (offset > file_data.size() || (file_data.size() - offset) < sizeof(T)) {
			return nullptr;
		}

		return reinterpret_cast<const T *>(file_data.data() + offset);
	}

	std::size_t Align_Up(std::size_t value, std::size_t alignment)
	{
		return (value + alignment - 1u) & ~(alignment - 1u);
	}

	bool Read_File(const char * filename, std::vector<std::uint8_t> & file_data)
	{
		if (filename == nullptr) {
			return false;
		}

		return renegade_osdep::Read_Entire_File(filename, file_data);
	}

	bool Read_Uint16(const std::vector<std::uint8_t> & file_data, std::size_t offset, std::uint16_t & value)
	{
		if (offset > file_data.size() || (file_data.size() - offset) < sizeof(value)) {
			return false;
		}

		std::memcpy(&value, file_data.data() + offset, sizeof(value));
		return true;
	}

	bool Read_Uint32(const std::vector<std::uint8_t> & file_data, std::size_t offset, std::uint32_t & value)
	{
		if (offset > file_data.size() || (file_data.size() - offset) < sizeof(value)) {
			return false;
		}

		std::memcpy(&value, file_data.data() + offset, sizeof(value));
		return true;
	}

	bool Rva_To_File_Offset(const std::vector<WWLib_PE_Section_Header> & sections, std::uint32_t rva, std::size_t & offset)
	{
		for (const WWLib_PE_Section_Header & section : sections) {
			const std::uint32_t section_span = std::max(section.VirtualSize, section.SizeOfRawData);
			if (rva >= section.VirtualAddress && rva < (section.VirtualAddress + section_span)) {
				offset = static_cast<std::size_t>(section.PointerToRawData + (rva - section.VirtualAddress));
				return true;
			}
		}

		offset = static_cast<std::size_t>(rva);
		return true;
	}

	bool Load_PE_Metadata(
		const char * filename,
		std::vector<std::uint8_t> & file_data,
		WWLib_PE_File_Header & file_header,
		std::vector<WWLib_PE_Section_Header> & sections,
		std::uint32_t & resource_rva,
		std::uint32_t & resource_size)
	{
		file_data.clear();
		sections.clear();
		resource_rva = 0;
		resource_size = 0;

		if (!Read_File(filename, file_data)) {
			return false;
		}

		const WWLib_DOS_Header * dos_header = View_Struct<WWLib_DOS_Header>(file_data, 0);
		if (dos_header == nullptr || dos_header->Signature != PE_DOS_SIGNATURE || dos_header->NtHeaderOffset < 0) {
			return false;
		}

		const std::size_t nt_header_offset = static_cast<std::size_t>(dos_header->NtHeaderOffset);
		std::uint32_t nt_signature = 0;
		if (!Read_Uint32(file_data, nt_header_offset, nt_signature) || nt_signature != PE_NT_SIGNATURE) {
			return false;
		}

		const WWLib_PE_File_Header * header = View_Struct<WWLib_PE_File_Header>(file_data, nt_header_offset + sizeof(std::uint32_t));
		if (header == nullptr) {
			return false;
		}

		file_header = *header;

		const std::size_t optional_header_offset = nt_header_offset + sizeof(std::uint32_t) + sizeof(WWLib_PE_File_Header);
		std::uint16_t optional_magic = 0;
		if (!Read_Uint16(file_data, optional_header_offset, optional_magic)) {
			return false;
		}

		const std::size_t data_directory_offset =
			optional_header_offset +
			((optional_magic == PE32_MAGIC) ? PE32_DATA_DIRECTORY_OFFSET :
			(optional_magic == PE32_PLUS_MAGIC) ? PE32_PLUS_DATA_DIRECTORY_OFFSET : 0u);
		if (data_directory_offset == optional_header_offset) {
			return false;
		}

		const std::size_t resource_directory_offset = data_directory_offset + (RESOURCE_DATA_DIRECTORY_INDEX * sizeof(std::uint32_t) * 2u);
		if (!Read_Uint32(file_data, resource_directory_offset, resource_rva) ||
			!Read_Uint32(file_data, resource_directory_offset + sizeof(std::uint32_t), resource_size)) {
			return false;
		}

		const std::size_t section_table_offset = optional_header_offset + file_header.SizeOfOptionalHeader;
		for (std::uint16_t index = 0; index < file_header.NumberOfSections; ++index) {
			const WWLib_PE_Section_Header * section = View_Struct<WWLib_PE_Section_Header>(
				file_data,
				section_table_offset + (static_cast<std::size_t>(index) * sizeof(WWLib_PE_Section_Header)));
			if (section == nullptr) {
				return false;
			}

			sections.push_back(*section);
		}

		return true;
	}

	const WWLib_Resource_Directory_Entry * Find_Resource_Entry_By_Id(
		const std::vector<std::uint8_t> & file_data,
		std::size_t directory_offset,
		std::uint16_t resource_id)
	{
		const WWLib_Resource_Directory * directory = View_Struct<WWLib_Resource_Directory>(file_data, directory_offset);
		if (directory == nullptr) {
			return nullptr;
		}

		const std::size_t entry_count = static_cast<std::size_t>(directory->NumberOfNamedEntries) + directory->NumberOfIdEntries;
		const std::size_t entry_table_offset = directory_offset + sizeof(WWLib_Resource_Directory);
		for (std::size_t index = 0; index < entry_count; ++index) {
			const WWLib_Resource_Directory_Entry * entry = View_Struct<WWLib_Resource_Directory_Entry>(
				file_data,
				entry_table_offset + (index * sizeof(WWLib_Resource_Directory_Entry)));
			if (entry == nullptr) {
				return nullptr;
			}

			if ((entry->Name & 0x80000000u) == 0u && static_cast<std::uint16_t>(entry->Name & 0xFFFFu) == resource_id) {
				return entry;
			}
		}

		return nullptr;
	}

	const WWLib_Resource_Directory_Entry * Find_First_Resource_Entry(
		const std::vector<std::uint8_t> & file_data,
		std::size_t directory_offset)
	{
		const WWLib_Resource_Directory * directory = View_Struct<WWLib_Resource_Directory>(file_data, directory_offset);
		if (directory == nullptr) {
			return nullptr;
		}

		const std::size_t entry_count = static_cast<std::size_t>(directory->NumberOfNamedEntries) + directory->NumberOfIdEntries;
		if (entry_count == 0u) {
			return nullptr;
		}

		return View_Struct<WWLib_Resource_Directory_Entry>(file_data, directory_offset + sizeof(WWLib_Resource_Directory));
	}

	bool Resource_Entry_Is_Directory(const WWLib_Resource_Directory_Entry * entry)
	{
		return entry != nullptr && (entry->OffsetToData & 0x80000000u) != 0u;
	}

	std::size_t Resource_Entry_Offset(std::size_t resource_base_offset, const WWLib_Resource_Directory_Entry * entry)
	{
		return resource_base_offset + static_cast<std::size_t>(entry->OffsetToData & 0x7FFFFFFFu);
	}

	bool Extract_Fixed_File_Info(
		const std::vector<std::uint8_t> & file_data,
		std::size_t version_info_offset,
		std::uint32_t version_info_size,
		VS_FIXEDFILEINFO * file_info)
	{
		if (file_info == nullptr || version_info_offset >= file_data.size()) {
			return false;
		}

		std::size_t safe_size = std::min<std::size_t>(version_info_size, file_data.size() - version_info_offset);
		if (safe_size < 6u) {
			return false;
		}

		std::uint16_t version_length = 0;
		std::memcpy(&version_length, file_data.data() + version_info_offset, sizeof(version_length));
		if (version_length != 0u) {
			safe_size = std::min<std::size_t>(safe_size, version_length);
		}

		const std::uint8_t * version_info = file_data.data() + version_info_offset;
		std::size_t cursor = 6u;
		for (;;) {
			if ((cursor + sizeof(std::uint16_t)) > safe_size) {
				return false;
			}

			std::uint16_t character = 0;
			std::memcpy(&character, version_info + cursor, sizeof(character));
			cursor += sizeof(character);
			if (character == 0u) {
				break;
			}
		}

		cursor = Align_Up(cursor, 4u);
		if ((cursor + sizeof(VS_FIXEDFILEINFO)) > safe_size) {
			return false;
		}

		std::memcpy(file_info, version_info + cursor, sizeof(*file_info));
		return file_info->dwSignature == VS_FIXEDFILEINFO_SIGNATURE;
	}

	bool Read_PE_Version_Info(const char * filename, VS_FIXEDFILEINFO * file_info)
	{
		std::vector<std::uint8_t> file_data;
		WWLib_PE_File_Header file_header = {};
		std::vector<WWLib_PE_Section_Header> sections;
		std::uint32_t resource_rva = 0;
		std::uint32_t resource_size = 0;
		if (!Load_PE_Metadata(filename, file_data, file_header, sections, resource_rva, resource_size) ||
			resource_rva == 0u ||
			resource_size == 0u) {
			return false;
		}

		std::size_t resource_base_offset = 0;
		if (!Rva_To_File_Offset(sections, resource_rva, resource_base_offset)) {
			return false;
		}

		const WWLib_Resource_Directory_Entry * type_entry =
			Find_Resource_Entry_By_Id(file_data, resource_base_offset, RT_VERSION_RESOURCE_ID);
		if (type_entry == nullptr || !Resource_Entry_Is_Directory(type_entry)) {
			return false;
		}

		const WWLib_Resource_Directory_Entry * name_entry =
			Find_First_Resource_Entry(file_data, Resource_Entry_Offset(resource_base_offset, type_entry));
		if (name_entry == nullptr || !Resource_Entry_Is_Directory(name_entry)) {
			return false;
		}

		const WWLib_Resource_Directory_Entry * language_entry =
			Find_First_Resource_Entry(file_data, Resource_Entry_Offset(resource_base_offset, name_entry));
		if (language_entry == nullptr || Resource_Entry_Is_Directory(language_entry)) {
			return false;
		}

		const WWLib_Resource_Data_Entry * data_entry =
			View_Struct<WWLib_Resource_Data_Entry>(file_data, Resource_Entry_Offset(resource_base_offset, language_entry));
		if (data_entry == nullptr) {
			return false;
		}

		std::size_t version_info_offset = 0;
		if (!Rva_To_File_Offset(sections, data_entry->OffsetToData, version_info_offset)) {
			return false;
		}

		return Extract_Fixed_File_Info(file_data, version_info_offset, data_entry->Size, file_info);
	}

	bool Read_PE_TimeDateStamp(const char * filename, std::uint32_t * timestamp)
	{
		if (timestamp == nullptr) {
			return false;
		}

		std::vector<std::uint8_t> file_data;
		WWLib_PE_File_Header file_header = {};
		std::vector<WWLib_PE_Section_Header> sections;
		std::uint32_t resource_rva = 0;
		std::uint32_t resource_size = 0;
		if (!Load_PE_Metadata(filename, file_data, file_header, sections, resource_rva, resource_size)) {
			return false;
		}

		*timestamp = file_header.TimeDateStamp;
		return true;
	}

	bool WWLib_Get_FileTime(const char * filename, FILETIME * create_time)
	{
		if (filename == nullptr || create_time == nullptr) {
			return false;
		}

		SDL_PathInfo path_info = {};
		if (!renegade_osdep::Get_Path_Info(filename, &path_info)) {
			return false;
		}
		SDL_TimeToWindows(path_info.modify_time, &create_time->dwLowDateTime, &create_time->dwHighDateTime);
		return true;
	}
}

bool GetVersionInfo(char * filename, VS_FIXEDFILEINFO * fileInfo)
{
	if (fileInfo == nullptr) {
		return false;
	}

	std::memset(fileInfo, 0, sizeof(*fileInfo));
	return Read_PE_Version_Info(filename, fileInfo);
}

bool GetFileCreationTime(char * filename, FILETIME * createTime)
{
	return WWLib_Get_FileTime(filename, createTime);
}

int Compare_EXE_Version(int app_instance, const char * filename)
{
	if (filename == nullptr) {
		return 0;
	}

	char current_filename[MAX_PATH] = {};
	if (GetModuleFileName(reinterpret_cast<HINSTANCE>(app_instance), current_filename, MAX_PATH) == 0) {
		return 0;
	}

	std::uint32_t current_timestamp = 0;
	std::uint32_t target_timestamp = 0;
	if (!Read_PE_TimeDateStamp(current_filename, &current_timestamp) ||
		!Read_PE_TimeDateStamp(filename, &target_timestamp)) {
		return 0;
	}

	return (current_timestamp < target_timestamp) ? -1 : (current_timestamp > target_timestamp) ? 1 : 0;
}

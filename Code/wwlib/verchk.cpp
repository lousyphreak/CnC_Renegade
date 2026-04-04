#include "verchk.h"

#include <chrono>
#include <cstring>
#include <filesystem>

namespace
{
	bool WWLib_Get_FileTime(const char * filename, FILETIME * create_time)
	{
		if (filename == nullptr || create_time == nullptr) {
			return false;
		}

		std::error_code error;
		auto write_time = std::filesystem::last_write_time(filename, error);
		if (error) {
			return false;
		}

		auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			write_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(system_time.time_since_epoch()).count();
		constexpr std::uint64_t WINDOWS_TO_UNIX_EPOCH_100NS = 11644473600ull * 10000000ull;
		const std::uint64_t ticks = WINDOWS_TO_UNIX_EPOCH_100NS + static_cast<std::uint64_t>(nanoseconds / 100);

		create_time->dwLowDateTime = static_cast<DWORD>(ticks & 0xFFFFFFFFull);
		create_time->dwHighDateTime = static_cast<DWORD>(ticks >> 32);
		return true;
	}
}

bool GetVersionInfo(char * filename, VS_FIXEDFILEINFO * fileInfo)
{
	if (fileInfo == nullptr) {
		return false;
	}

	std::memset(fileInfo, 0, sizeof(*fileInfo));
	FILETIME create_time = {};
	if (!WWLib_Get_FileTime(filename, &create_time)) {
		return false;
	}

	fileInfo->dwSignature = 0xFEEF04BDu;
	fileInfo->dwFileVersionMS = create_time.dwHighDateTime;
	fileInfo->dwFileVersionLS = create_time.dwLowDateTime;
	fileInfo->dwProductVersionMS = fileInfo->dwFileVersionMS;
	fileInfo->dwProductVersionLS = fileInfo->dwFileVersionLS;
	fileInfo->dwFileDateMS = create_time.dwHighDateTime;
	fileInfo->dwFileDateLS = create_time.dwLowDateTime;
	return true;
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

	FILETIME current_time = {};
	FILETIME target_time = {};
	char current_filename[MAX_PATH] = {};

	if (GetModuleFileName(reinterpret_cast<HINSTANCE>(app_instance), current_filename, MAX_PATH) == 0) {
		return 0;
	}

	if (!WWLib_Get_FileTime(current_filename, &current_time) || !WWLib_Get_FileTime(filename, &target_time)) {
		return 0;
	}

	if (current_time.dwHighDateTime != target_time.dwHighDateTime) {
		return (current_time.dwHighDateTime < target_time.dwHighDateTime) ? -1 : 1;
	}

	if (current_time.dwLowDateTime != target_time.dwLowDateTime) {
		return (current_time.dwLowDateTime < target_time.dwLowDateTime) ? -1 : 1;
	}

	return 0;
}

#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>

namespace renegade_emscripten_assets {

bool Get_Synthetic_Path_Info(const std::string & normalized_path, SDL_PathInfo * info);
void Append_Synthetic_Directory_Entries(const std::string & normalized_directory, std::vector<std::string> & entries);
SDL_IOStream * Open_File(const std::string & normalized_path, const char * mode);
bool Create_Directory(const std::string & normalized_path);
bool Remove_Path(const std::string & normalized_path);
bool Rename_Path(const std::string & old_normalized_path, const std::string & new_normalized_path);
bool Get_File_Attributes(const std::string & normalized_path, uint32_t & attributes);

} // namespace renegade_emscripten_assets

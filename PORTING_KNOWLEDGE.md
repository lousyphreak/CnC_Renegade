# Porting Knowledge

## Disk I/O choke point

- The practical low-level choke point for on-disk file access is now `Code/wwlib/osdep.h`.
- The key helpers added there are:
  - `renegade_osdep::Open_C_File`
  - `renegade_osdep::Open_C_File_Read_Write`
  - `renegade_osdep::Read_C_File`
  - `renegade_osdep::Write_C_File`
  - `renegade_osdep::Seek_C_File`
  - `renegade_osdep::Tell_C_File`
  - `renegade_osdep::Get_C_File_Size`
  - `renegade_osdep::Close_C_File`
  - `renegade_osdep::Read_Entire_File`
  - `renegade_osdep::Get_C_File_Line`
  - `renegade_osdep::Printf_C_File`
  - `renegade_osdep::Collect_Directory_Entries`
  - `renegade_osdep::Collect_Regular_Files_Recursive`
  - `renegade_osdep::Get_Path_Info`
  - `renegade_osdep::Resolve_Existing_Path`

## Layering

- Higher-level game code should continue to prefer `FileClass` / `RawFileClass` / `_TheFileFactory` when possible.
- The low-level `osdep` helpers are for the remaining places that genuinely need low-level stream access, whole-file convenience reads, or directory/path metadata queries.
- `DeleteFile`, `MoveFile`, directory creation, and find-file enumeration were already living in `osdep.h`; disk opens now match that same pattern.
- `RawFileClass` on non-Windows now stores `SDL_IOStream *`, not `FILE *`.

## Why this matters on Linux

- Case-sensitive path fixes only help if every disk-open path goes through the same resolver.
- Before this change, several modernized/ported spots still bypassed the resolver with direct `fopen`, `std::ifstream`, `std::ofstream`, `std::filesystem` traversal, or direct `SDL_IOFromFile`.
- After this change, runtime/library file opens and directory scans that touch disk are routed through the shared SDL-backed resolver helpers, so future filename and path fixes can be made centrally.

## SDL-specific notes

- `SDL_GetPathInfo` is the shared source for path existence, file type, file size, and modification timestamps in the low-level compatibility layer.
- `SDL_GlobDirectory` is used as the base enumeration primitive; recursive scans are built on top of it in `renegade_osdep::Collect_Regular_Files_Recursive`.
- `SDL_OpenFileStorage` / `SDL_GetStorageSpaceRemaining` are sufficient for the savegame free-space gate and avoid falling back to `std::filesystem::space`.
- SDL3 does **not** currently expose a current-working-directory setter, so the bootstrap working-directory setup code remains outside the SDL I/O chokepoint. That code is process setup, not a file read/write path.
- SDL3 also does **not** expose an executable-path query, so the Unix `GetModuleFileName` compatibility path still uses `/proc/self/exe`.

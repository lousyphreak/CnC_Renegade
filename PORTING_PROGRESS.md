# Porting Progress

## SDL/bgfx render-resolution resync

- Traced the SDL window-size path through `Code/Commando/commando_sdl_main.cpp` and the bgfx reset path in `Code/ww3d2/bgfxdynamicbuffer.cpp` / `Code/ww3d2/bgfxrenderer.cpp`.
- The renderer already reset correctly when the exact resize/fullscreen events were seen, but it still depended too heavily on backend-specific event delivery timing.
- Added a main-loop pre-poll reconciliation step that re-queries the current SDL drawable size every frame and reapplies `WW3D::Set_Device_Resolution` whenever the actual pixel size or windowed/fullscreen state diverges from the renderer state.
- Expanded SDL event coverage to include display-mode/content-scale changes for the window's active display, so fullscreen and DPI-driven transitions resync immediately instead of waiting for a narrower subset of window events.
- Hardened the bgfx drawable-size query to fall back to logical window dimensions when SDL cannot provide pixel dimensions during a transition, avoiding failed resets on platforms/backends that briefly report only window units.
- Corrected the Alt+Enter path so fullscreen toggles without an explicit resize request now use desktop fullscreen instead of forcing an exclusive mode near the old window size; this prevents fullscreen from staying at a visibly upscaled window resolution.
- A follow-up runtime exercise resized the live X11 window to `1024x768`, then `1366x768`, then entered and left fullscreen during execution without crashing.
- A targeted Alt+Enter validation run on X11 captured a `3840x2160` bgfx screenshot after the toggle, replacing the previous `1280x720` fullscreen backbuffer and confirming the fullscreen render resolution now tracks the real fullscreen size.

## Load-time filesystem cache

- Investigated slow data/level loading on Linux and traced the hot path to the shared SDL-backed case-correct disk access layer in `Code/wwlib/osdep.h`.
- The main slowdown came from repeated `Resolve_Existing_Path` fallbacks into `Resolve_Path_Case`, which enumerated directory contents with `SDL_GlobDirectory` for each path component whenever requested casing did not exactly match on-disk casing.
- This cost was amplified by `SimpleFileFactoryClass::Get_File`, which probes semicolon-separated search paths by opening candidate files before the real open, causing the same case-resolution work to repeat during asset-heavy loads.
- Added caching for:
  - resolved existing paths
  - directory entry lists used for case-insensitive component matching
- Added cache invalidation for path-mutating operations so case-correct resolution stays accurate when files or directories are created, moved, or deleted.
- This keeps the Linux case-sensitive compatibility behavior intact while removing the repeated directory-globbing work from steady-state asset loads.

## Validation follow-up

- A 200-second `Renegade` soak on the Debug ASAN/UBSAN build exposed an unrelated Mission 02 script bug: `M02_Respawn_Controller` trusted custom-event `param` as an area-table index and received `99` for a 26-entry table.
- Hardened `M02_Respawn_Controller` and the matching demo controller to reject invalid area indices with a debug message instead of indexing past the end of the respawn tracking arrays.

## Centralized SDL disk I/O

- Reworked `Code/wwlib/osdep.h` into the single runtime low-level disk/filesystem chokepoint.
- Switched the shared low-level implementation from stdio / `std::filesystem` to SDL3-backed APIs:
  - `SDL_IOFromFile`, `SDL_ReadIO`, `SDL_WriteIO`, `SDL_SeekIO`, `SDL_TellIO`, `SDL_GetIOSize`, `SDL_FlushIO`, `SDL_CloseIO`
  - `SDL_GetPathInfo`, `SDL_GlobDirectory`, `SDL_CreateDirectory`, `SDL_RemovePath`, `SDL_RenamePath`, `SDL_GetCurrentDirectory`
  - `SDL_OpenFileStorage` / `SDL_GetStorageSpaceRemaining` for the savegame free-space check
- Updated `Code/wwlib/rawfile.cpp` and `Code/wwlib/rawfile.h` so runtime raw disk access now flows through SDL streams instead of Unix `FILE *`.
- Kept the higher-level file stack intact: `FileClass` -> `RawFileClass` -> `BufferedFileClass` / `TextFileClass` / mix/archive users.
- Moved remaining runtime/library bypasses onto the same SDL-backed path, including:
  - registry persistence
  - translation DB text import/export
  - dialog parser source/header reads
  - PE version metadata reads
  - shader binary loads
  - profile/debug/script log writers
  - gameplay/admin text logs, ban lists, and results/history writers
  - package and thumbnail directory scans
  - font discovery and binary font reads
  - vis table `FILE*` serialization helpers

## Behavior-sensitive notes

- Unified Unix `READ|WRITE` raw-file opens with the intended Windows behavior by opening existing files read/write when present and creating them otherwise, instead of truncating through `"w"`.
- Case-correct path resolution for runtime disk opens now comes from one place instead of being split between `RawFileClass`, ad hoc `fopen`, `std::ifstream`, `std::ofstream`, and direct `SDL_IOFromFile` call sites.
- Runtime directory listing and path metadata queries that feed package scans, font lookup, dialog parsing, thumbnail generation, and savegame space checks now use the same SDL-backed helper layer instead of `std::filesystem`.
- The only notable remaining `std::filesystem` usage in the main runtime tree is bootstrap working-directory setup in `commando_sdl_main.cpp` and `commando_dedicated_bootstrap.cpp`; that is process setup rather than file I/O, and SDL3 does not currently expose a working-directory setter.

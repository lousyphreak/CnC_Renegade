# Porting Knowledge

## SDL_mixer 3D audio contract

- Renegade world-space distances are already metric. Evidence in-tree includes:
  - `Code/wwphys/phunits.h`, where the internal physics units are meters
  - `Code/Combat/mapmgr.h`, which describes map scale as pixels per meter of world space
  - multiple gameplay/physics definitions that label ranges and speeds in meters / m/s
- `Sound3DClass` does **not** pass world-space coordinates to the audio backend. It first converts the emitter transform into listener space in `Code/WWAudio/Sound3D.cpp`, then calls the Miles shim with listener-relative coordinates.
- The Miles-side listener is intentionally fixed at the origin; the backend should treat incoming 3D source positions as already relative to the listener.
- Coordinate handedness differs at the backend boundary:
  - WWAudio/Miles shim coordinates are effectively `+X right, +Y up, +Z forward`
  - SDL_mixer `MIX_Point3D` uses `+X right, +Y up, +Z back`
  - The SDL_mixer bridge therefore needs to negate the forward axis when converting 3D positions.
- SDL_mixer's positional mixer uses a fixed inverse-distance attenuation model with a 1-meter reference distance. Renegade sound definitions, however, author linear attenuation with explicit max-volume and dropoff radii.
- To preserve authored WWAudio behavior on SDL_mixer, the backend now converts listener-relative positions into an **equivalent SDL distance** that reproduces the original linear attenuation while keeping the correct relative direction for spatialization.
- For 3D sounds, forced stereo panning and SDL_mixer 3D spatialization are mutually exclusive. The backend should use `MIX_SetTrack3DPosition` for true 3D sounds and reserve `MIX_SetTrackStereo` for 2D/manual pan paths only.

## Sniper listener behavior

- The secondary/sniper listener lives in `Code/Combat/ccamera.cpp`.
- It must be updated every frame while sniper mode is active, not only when the zoom distance changes.
- It also needs the current camera orientation preserved. Replacing the entire transform with `Matrix3D(pos)` makes the listener face the identity orientation and breaks positional audio in sniper mode.

## SDL window size versus render resolution

- `commando_sdl_main.cpp` is the practical choke point for runtime SDL window-change handling in the main client: window events arrive there, and `WW3D::Set_Device_Resolution` is the bridge into the renderer reset path.
- For robust modern-platform behavior, do not rely only on individual SDL resize/fullscreen events. Some backends deliver size changes through different event combinations or with slightly different timing, especially around fullscreen, monitor changes, and content-scale transitions.
- The reliable pattern is:
  - query the current drawable size from SDL
  - compare it against `WW3D::Get_Device_Resolution`
  - call `WW3D::Set_Device_Resolution` only when the drawable size or windowed/fullscreen state actually changed
- Running that reconciliation from the main loop's pre-poll hook keeps the renderer aligned even when the expected SDL event is delayed or skipped.
- Display-level SDL events (`SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED`, `SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED`, and related mode/bounds changes) are also relevant because fullscreen resolution changes may originate from the display rather than a classic window resize event.
- On the bgfx side, `BgfxRenderer::Query_Drawable_Size` should prefer `SDL_GetWindowSizeInPixels`, but it needs a fallback to `SDL_GetWindowSize` for transition windows where SDL temporarily cannot report pixel dimensions yet.
- For `BgfxRenderer::Configure_Window`, treat fullscreen requests without an explicit resize/mode change (`resize_window == false`) as desktop fullscreen (`SDL_SetWindowFullscreenMode(window, NULL)` semantics). That keeps Alt+Enter and similar toggles from locking the game into an exclusive mode chosen from the previous window size.
- Reserve explicit exclusive fullscreen mode selection for calls that are actually requesting a specific render resolution (`resize_window == true` with a concrete width/height).

## Load-time path resolution

- `renegade_osdep::Resolve_Existing_Path` sits directly in the runtime open path for `RawFileClass`, low-level compatibility wrappers, and directory pattern resolution.
- On Linux, exact-case misses previously fell through to `Resolve_Path_Case`, which walked each path component by enumerating the containing directory with `SDL_GlobDirectory` and then linearly matching entries with `strcasecmp`.
- That behavior is correct for case-sensitive filesystems, but it becomes a startup/load-time bottleneck when many requests use legacy Windows-style casing and the same directories are hit repeatedly.
- The shared fix is to cache:
  - normalized requested path -> resolved on-disk path
  - directory -> directory entry list
- Cache invalidation is required on successful create/move/delete/directory-create operations; otherwise case-correct resolution can become stale after runtime file mutations.
- This cache is especially important because `SimpleFileFactoryClass::Get_File` may probe multiple search-path candidates before the real open, multiplying whatever cost exists in the low-level resolver.

## Mission 02 respawn event validation

- `M02_Respawn_Controller` can receive custom events whose `param` is treated as an area index.
- During runtime validation, Mission 02 delivered event `101` with `param == 99`, which exceeded the 26-entry area tables and triggered UBSan.
- The safe fix is to validate the area index at the controller boundary and log the bad event, rather than trusting every sender to stay in range.
- `MissionDemo.cpp` carries the same respawn-controller pattern, so it was hardened the same way to avoid reintroducing the same out-of-bounds behavior in the demo mission scripts.

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

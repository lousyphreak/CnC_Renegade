# Porting Progress

## Emscripten WebGL heap ceiling

- Investigated the Firefox browser exception from `WebGL2RenderingContext.texSubImage2D` during bgfx texture uploads.
- Root cause: the generated Emscripten WebGL glue uses the fast `texSubImage2D(..., heap, srcOffset)` path, and Firefox rejects that path once the wasm heap backing store reaches the 2 GiB boundary.
- Added a source-controlled Emscripten heap ceiling for Renegade:
  - new `RENEGADE_EMSCRIPTEN_MAXIMUM_MEMORY_MB` cache setting
  - default cap set to `2047` MiB when runtime heap growth is enabled
  - configure-time validation now rejects `INITIAL_MEMORY >= MAXIMUM_MEMORY`
- This keeps runtime memory growth available for the web port while preventing the browser heap from reaching the WebGL upload failure threshold.

## Audio multilist duplicate-pruning crash

- Investigated an ASAN/UBSan crash during C4 detonation mode changes and traced it to `Code/WWAudio/SoundScene.cpp` inside `SoundSceneClass::On_Frame_Update`.
- Root cause: the duplicate-audible pruning pass deleted `AudibleInfoClass` objects before removing them from their `MultiListClass` containers, then continued iterating from nodes that could already have been unlinked.
- Reworked that pass to use `MultiListIterator::Remove_Current_Object()` before `delete` and switched the affected loops to manual iterator advancement so removed current nodes are never advanced again.
- This preserves the original behavior of keeping the closer of the duplicate primary/auxiliary sounds while avoiding the stale-object/stale-node path that crashed in `GenericMultiListClass::Internal_Remove`.

## Single-player mission replay recovery

- Investigated missing built-in entries in the single-player load menu and traced the regression to the replay-unlock gate in `Code/Commando/dlgloadspgame.cpp`.
- The menu still enumerated `data\\m*.mix` correctly, but it only shows those mission starts when `Software\\Westwood\\Renegade\\Ranks` contains a non-zero rank for the mission.
- On the affected Linux profile, the shipped save slots and autosave still preserved campaign progress (`M13`, `M01`, `M02`), while the `Ranks` registry section was empty, so every built-in mission replay entry was filtered out.
- Added a compatibility recovery path:
  - it runs only when the mission-rank registry key has no stored values
  - it scans `data\\save\\savegame*.sav`
  - it rebuilds minimal replay ranks from saved single-player progress
  - it explicitly skips the mission currently represented by `autosave.sav`, so the active in-progress mission is not exposed as a replay unlock
- This restores the replay rows for already-completed missions without changing the normal rank-driven behavior for profiles whose mission ranks are still intact.

## SDL3 main callback lifecycle

- Reworked both Commando executables onto SDL3 app callbacks:
  - `Code/Commando/commando_sdl_main.cpp` now uses `SDL_AppInit`, `SDL_AppIterate`, `SDL_AppEvent`, and `SDL_AppQuit` for the client
  - `Code/Commando/commando_dedicated_sdl_main.cpp` now uses the same callback lifecycle for the dedicated server
- `Code/Commando/mainloop.cpp` now exposes only explicit initialize / iterate / shutdown helpers for the shared game-loop body:
  - `Game_Main_Loop_Initialize()`
  - `Game_Main_Loop_Iterate()`
  - `Game_Main_Loop_Shutdown()`
- Removed the old outer `Game_Main_Loop()` wrapper entirely, so no executable still owns an internal `while (RunMainLoop)` loop around the main game loop.
- Preserved the existing bootstrap ordering around:
  - command-line parsing
  - singleton-instance verification
  - dedicated default configuration
  - SDL setup appropriate to each executable (`video/audio/gamepad` for the client, `events` only for dedicated)
  - client exception/version callback registration
  - teardown under `SDL_AppQuit`
- Preserved the existing hidden `--headless-smoke` paths while moving them under `SDL_AppInit` / `SDL_AppQuit`.
- Removed the leftover Launcher `WinMain` shim so the maintained launcher code no longer carries a custom WinMain entrypoint wrapper.
- Fixed dedicated shutdown stability after the callback migration by moving the `Copy_Logs` worker thread object onto a never-destroyed singleton; this avoids static-destruction races on ASAN/UBSAN exit paths.
- Rebuilt the Debug ASAN/UBSAN targets and revalidated the cleaned-up startup flow:
  - `Renegade` completed another 220-second soak and timed out normally
  - `renegade_dedicated` reached the steady-state main loop for 60 seconds under the SDL callback entrypoint using the dedicated validation config

## Emscripten build bring-up and data packaging

- Fixed a Chrome-only graphics startup regression in the current Emscripten bgfx path:
  - `Code/Commando/commando_sdl_main.cpp` now creates the browser SDL window with the same externally managed graphics-context contract used by the desktop bgfx path instead of marking it as an SDL OpenGL-owned window
  - `Code/ww3d2/bgfxrenderer.cpp` no longer creates a second raw Emscripten WebGL context behind SDL's back; it now either passes through an already-current WebGL 2 context or lets bgfx create the canvas context itself
  - the Emscripten target now links with an explicit WebGL 2 contract (`MIN_WEBGL_VERSION=2`, `MAX_WEBGL_VERSION=2`, `FULL_ES3=1`)
  - the bgfx init path now explicitly prefers `OpenGLES` on Emscripten, matching bgfx's browser backend
- Fixed the follow-on browser abort uncovered during validation:
  - post-init `Configure_Window()` / `Update_Platform_Window()` calls were re-querying SDL/Emscripten and changing `platformData.context` after `bgfx::init()`
  - bgfx explicitly forbids changing `context` / `ndt` after initialization, so Chromium aborted inside `bgfx::setPlatformData()`
  - the runtime now preserves the original immutable bgfx platform-data fields after initialization while still updating drawable size, window mode, and the native window handle path
- This removes the browser-sensitive split ownership that let Firefox start while Chrome fell back to the legacy `DirectX 8.0 or later is required` outer error after bgfx initialization failed.

- Added a source-controlled Emscripten packaging path for `Commando`:
  - new CMake options `RENEGADE_EMSCRIPTEN_PACKAGE_GAME_DATA` and `RENEGADE_EMSCRIPTEN_DATA_ROOT`
  - a shared `renegade_configure_emscripten_target(...)` helper in `cmake/RenegadeTargets.cmake`
  - `Code/Commando/CMakeLists.txt` now opts the web target into that helper
- The generated web build now emits the expected runtime artifacts under `build-em/bin`:
  - `Renegade.html`
  - `Renegade.js`
  - `Renegade.wasm`
  - `Renegade.data`
- The Emscripten data bundle is now laid out to match the runtime's existing factory setup:
  - the shipped `Renegade/Data` tree is preloaded as `/Data`
  - optional `HTML` and `Internet` trees are preloaded at `/HTML` and `/Internet`
  - top-level regular files from the shipped tree are also preloaded at `/`
- Confirmed from the generated preload manifest that the web build contains the key runtime paths the game expects, including `/Data/Always2.dat`, `/Data/always.dat`, `/Data/always.dbs`, `/Data/config/...`, `/Data/save/...`, `/HTML/...`, and `/Internet/...`.
- Fixed shared build plumbing so SDL usage requirements propagate to the full codebase by linking `SDL3::SDL3` through `renegade_project_options`.
- Fixed a large batch of Clang/Emscripten compatibility issues that blocked the web build:
  - header declarations with extra qualification that old MSVC accepted
  - old `register` usage rejected by C++17
  - non-trivial `StringClass` / `WideStringClass` objects passed through variadic formatting calls
  - old const-correctness mismatches now enforced by Clang
  - legacy typed-enum bitwise accumulation sites
  - several outdated raw string/pointer assumptions in audio, rendering, combat, and networking code
- Fixed the in-tree Bink decoder Emscripten build by adding the vendored FFmpeg root include directory so `binkdsp.c` can resolve `libavutil/attributes.h`.
- Fixed Emscripten shader-tool execution:
  - `shaderc.js` now runs with `-sNODERAWFS=1`
  - increased stack size avoids the earlier shader preprocessing overflow
- Disabled SDL's Emscripten pthread mode for this build so the final web link stays single-threaded and does not require wasm shared-memory / atomics across the whole project.
- Rebuilt the full Emscripten `Commando` target successfully after the above changes.
- Fixed the current `build-em` regression in the fetched bgfx dependency integration:
  - source-controlled configure-time patching now stops `cmake/bimg/CMakeLists.txt` from building `bimg_encode` unless texture tools are enabled
  - this keeps the web build from compiling NVTT encode sources that do not support the Emscripten target, while preserving the shader tool path the project actually uses
- Fixed the current web-launch artifact regression:
  - `renegade_configure_emscripten_target(...)` now gives browser targets the `.html` suffix Emscripten expects for the launcher page
  - `Commando` therefore emits `Renegade.html` again alongside `Renegade.js`, `Renegade.wasm`, and `Renegade.data`
- Updated `launche.sh` so the normal web flow now:
  - verifies `emcmake` / `emrun` are available
  - recreates `build-em` automatically if it is not an Emscripten Debug tree
  - reconfigures `build-em` as a Debug Emscripten tree with packaged Renegade data
  - cleans the configured tree before building so stale mixed-configuration objects cannot survive across web-port iterations
  - builds with `cmake --build ... -j20`
  - serves `build-em/bin/Renegade.html` through `emrun`
- Fixed the browser-only startup crash that showed up after the web build finally reached the main menu:
  - `MainMenuTransitionClass` now holds ref-counted ownership of the dialogs it animates between instead of storing raw pointers across the transition lifetime
  - the transition validity check now requires the camera and dialog, not just the model and animation
  - `Update_Controls()` now terminates the transition cleanly if the dialog is no longer running and skips any missing menu controls instead of blindly dereferencing them
- Revalidated the full web path in a real browser engine:
  - `Renegade.html` loads under headless Chrome with WebGL enabled
  - the game reaches `MainLoop: Entering main loop`
  - a 210-second browser soak completes without JavaScript exceptions or wasm out-of-bounds traps
- Fixed the remaining Emscripten-only main-menu initialization regression:
  - packaged the runtime RC/parser inputs the non-Windows dialog path actually needs: `Code/Commando/chat.rc`, `resource.h`, `dialogresource.h`, and `Code/Combat/string_ids.h`
  - confirmed the real string/conversation databases come from `Renegade/Data/always.dbs`, not the shipped root `00000409.256` / `00000409.016` files
  - kept the string/conversation DB fallback in `Code/Commando/init.cpp` so startup can load `STRINGS.TDB` / `CONV10.CDB` from `always.dbs` when direct lookup misses
  - fixed the web-only translation failure by forcing the wwtranslatedb persist-factory object files (`translateobj`, `stringtwiddler`, `tdbcategory`) to stay linked, so `STRINGS.TDB` no longer loads with version metadata but zero string objects under Emscripten
  - revalidated in headless Chrome with a captured screenshot showing the main menu rendering proper labels (`Single Player`, `Multiplay Internet`, `Options`, `Quit`, etc.) instead of raw `IDS_MENU_TEXT...` tokens
- Fixed the remaining Emscripten/WebGL lazy texture initialization warnings:
  - `SkinPaletteTexture` was being created without backing data and then updated via partial `bgfx::updateTexture2D(...)` row uploads, which made Chrome warn that `texSubImage` had to clear uninitialized texture storage first
  - normal bgfx textures with mipmapping were also created as full mip chains, but the renderer only uploaded the source mip count; WebGL then lazily initialized the missing tail levels during draw calls
  - `Code/ww3d2/bgfxrenderer.cpp` now initializes the skin palette with one full zero upload immediately after creating the mutable texture, and eagerly defines any missing mip tail levels before a texture is first sampled
  - revalidated in headless Chrome: the web build reaches `MainLoop: Entering main loop`, a 216-second soak completes, and the browser log no longer reports `texSubImage`, `lazy initialization`, or `drawElementsInstanced` texture warnings
- Fixed the follow-on regression in skinned meshes:
  - bgfx treats textures created with initial upload memory as immutable, so the first skin-palette fix accidentally blocked the later per-row `bgfx::updateTexture2D(...)` bone-matrix uploads
  - the renderer now keeps `SkinPaletteTexture` mutable by creating it empty and then issuing one full zero upload before the regular row updates begin
  - rebuilt the Emscripten target and revalidated with another 215-second headless Chrome soak; the game still reaches `MainLoop: Entering main loop` and the WebGL lazy-initialization warnings remain gone

## GPU mesh lighting and point-light support

- Traced the modern mesh-lighting path through `LightEnvironmentClass`, `DX8Wrapper`, `BgfxRenderer`, and `fs_mesh.sc`.
- Reworked the bgfx lighting contract so runtime lights stay as full light descriptors into the shader path instead of being reduced to four directional vectors on the CPU:
  - `LightEnvironmentClass` now preserves shader-facing `D3DLIGHT8` data for selected active lights
  - local-light selection still happens on the CPU, but the actual lighting evaluation now happens in the mesh fragment shader
  - point, spot, and directional lights are all handled in the same GPU path
- Fixed the DX8-wrapper light packing used by the modern renderer:
  - directional and spot lights now derive their light direction from the light transform instead of the old placeholder spot-direction field
  - far-attenuation-disabled point/spot lights no longer inherit a fake linear falloff from the default attenuation range
- Expanded `fs_mesh.sc` to evaluate:
  - scene ambient
  - per-light ambient
  - per-light diffuse
  - point-light distance attenuation
  - spotlight cone attenuation
- Updated the bgfx submit path so lighting no longer forces lit draws off the direct vertex/index buffer path when the buffers are otherwise compatible.
- Rebuilt the Debug ASAN/UBSAN build and completed a 220-second `Renegade` soak with the updated renderer path. The run timed out normally without a crash.

## bgfx Z-bias restoration

- Investigated decal flicker on the modern renderer and traced it past decal mesh generation into the bgfx DX8-wrapper projection bridge.
- Root cause: `CameraClass::Apply` wrote the active projection with `DX8Wrapper::Set_Transform(D3DTS_PROJECTION, ...)`, which clears the wrapper's stored clip-plane range. The bgfx backend currently emulates `D3DRS_ZBIAS` by folding bias into the projection matrix during `Get_Transform`, so once the clip range was lost, all later Z-bias requests silently stopped affecting submitted draws.
- Fixed the active camera path to use `Set_Projection_Transform_With_Z_Bias`, preserving the camera near/far planes for every world render pass.
- Audited the remaining projection override sites and fixed `DazzleRenderObjClass::Render_Dazzle` to save and restore the raw projection plus clip range together, preventing temporary overlay projection changes from breaking or compounding later Z-biased draws.
- This wider fix covers the current in-tree Z-bias users, including decals, multi-pass rigid mesh rendering, and extra-pass wireframe overlays.

## SDL_mixer 3D audio positioning

- Audited the full 3D audio path from `WWAudioClass::On_Frame_Update` through `SoundScene`, `Sound3D`, `SoundPseudo3D`, and the SDL_mixer-backed Miles shim in `Code/WWAudio/sdlmixer_mss.cpp`.
- Confirmed that Renegade world-space units are already meters, so the missing/loud sound issues were not caused by a hidden global distance scale mismatch.
- Confirmed that `Sound3DClass` already converts emitters into listener-relative space before calling the backend, which means the SDL_mixer bridge must treat incoming 3D positions as relative-to-listener data rather than world-space data.
- Reworked the SDL_mixer true-3D sample path to use SDL_mixer's positional API instead of the old custom stereo-only approximation:
  - preserve the listener-relative contract
  - convert the handedness difference between the Miles-style forward axis and SDL_mixer's OpenAL-style `+Z back`
  - keep units explicit at the backend boundary
  - map Renegade's authored linear min/max distance attenuation onto SDL_mixer's fixed inverse-distance model by converting to an equivalent SDL 3D distance per frame
- Kept the 2D/manual pan path on `MIX_SetTrackStereo`, so only true 3D sounds use SDL_mixer's positional mode.
- Fixed `Sound3DClass::Update_Edge_Volume` so sounds recover their full authored volume after moving back inside the inner range; previously the edge fade reduced volume near the dropoff radius but never restored it when the emitter moved closer again.
- Fixed sniper-mode secondary-listener updates in `Code/Combat/ccamera.cpp`:
  - the listener now refreshes every frame while sniper mode is active
  - the listener now preserves camera orientation instead of replacing it with a translation-only matrix
- Completed a 220-second Debug ASAN/UBSAN runtime soak with the updated audio code. The run timed out normally without a crash.

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
- The only notable remaining `std::filesystem` usage in the main runtime tree is bootstrap working-directory setup in `commando_sdl_main.cpp` and `commando_dedicated_sdl_main.cpp`; that is process setup rather than file I/O, and SDL3 does not currently expose a working-directory setter.

# Porting Knowledge

## Assertion macro contract

- In this tree, the main runtime assertion families that disappear in non-debug builds are:
  - `Code/wwdebug/wwdebug.h`: `WWASSERT`, `WWASSERT_PRINT`
  - `Code/wwlib/wwlib_debug.h`: `WWASSERT`, `WWASSERT_PRINT`
  - wrappers such as `fw_assert`, `pm_assert`, and `ds_assert` when they map to those debug-only assertions
- Keep assertion expressions side-effect free. Required work must happen before the assertion, not inside it.
- Safe pattern:
  - compute or mutate state first
  - store the result if needed
  - assert only the final predicate
- Example:
  - `bool ok = DoRequiredWork();`
  - `WWASSERT(ok);`
- Concrete examples fixed in-tree:
  - `WWASSERT(BgfxRenderer::Set_Render_Target(*texture));` was unsafe because the render-target switch disappeared in release
  - `WWASSERT(vertex_buffer->Update_Bgfx_Dynamic_Buffer(...));` was unsafe because the upload disappeared in release
  - `assert(_getcwd(...))` / `assert(_chdir(...))` / `assert(_fullpath(...))` were unsafe because path resolution work disappeared in release
- Future changes should preserve this contract by keeping assertions as validation only.

## SoundScene multilist removal contract

- `SoundSceneClass::On_Frame_Update` builds temporary `MultiListClass<AudibleInfoClass>` collections for primary and auxiliary audible sounds, then prunes duplicates before handing the surviving `sound_obj` entries to the active audible list.
- `AudibleInfoClass` derives from `MultiListObjectClass`, whose destructor already removes the object from every multilist it is still in.
- That means list users must never do `delete obj; list.Remove(obj);` on these records. The safe order is:
  - remove through the active iterator/list first
  - then `delete` the object
- When pruning duplicates from the current iterator position, the loop must also avoid the `for (...; ...; iterator.Next())` pattern unless it compensates for the iterator helper already advancing past the removed node.

## Mission replay unlock compatibility

- The single-player load menu does **not** show built-in mission starts just because `data\\m*.mix` exists. `Code/Commando/dlgloadspgame.cpp` filters those entries through `Get_Game_Rank(...)`, which reads `Software\\Westwood\\Renegade\\Ranks`.
- If that rank table is empty, the menu will still find the mission `.mix` files but will hide every built-in replay entry.
- A compatibility-safe recovery path now lives in `LoadSPGameMenuClass::On_Init_Dialog`:
  - only when the mission-rank key has no stored values
  - scan `data\\save\\savegame*.sav`
  - recover mission replay availability from saved single-player progress
  - exclude the map currently stored in `autosave.sav`, because autosave represents the active in-progress campaign mission rather than a completed replay unlock
- The recovery writes a minimal rank of `1`, which is enough to preserve the original menu contract (`rank > 0` means replayable) without inventing higher star ratings.

## SDL3 main callback contract for Commando

- Both maintained Commando executables now follow SDL3's app-callback lifecycle instead of owning native perpetual loops in `main()`:
  - `SDL_AppInit` performs bootstrap and one-time game-loop initialization
  - `SDL_AppIterate` performs one frame of engine work
  - `SDL_AppEvent` is the normal SDL event path
  - `SDL_AppQuit` performs teardown
- `Code/Commando/mainloop.cpp` is now split into reusable phases:
  - `Game_Main_Loop_Initialize()`
  - `Game_Main_Loop_Iterate()`
  - `Game_Main_Loop_Shutdown()`
- Do not reintroduce a wrapper that loops over these helpers. The SDL callback layer is the only outer driver for the client and dedicated executables.
- `Windows_Message_Handler()` still matters for long, blocking legacy operations on the main thread (for example, load-time loops that already call it), but the steady-state SDL client frame loop should rely on `SDL_AppEvent` + `SDL_AppIterate` instead of polling SDL events internally each frame.
- The dedicated executable initializes only the SDL events subsystem before entering `Game_Main_Loop_Initialize()`. That is enough to keep legacy `Windows_Message_Handler()` calls valid during dedicated startup/load loops without reintroducing a dedicated-owned outer event loop.
- Exit handling at the SDL callback boundary is effectively success vs failure. The currently used in-tree client restart path already exits with `RESTART_EXITCODE == 1`, so mapping non-zero loop exits to `SDL_APP_FAILURE` preserves the active behavior.
- Shutdown-time worker objects that may still be active during error/abort exits must not rely on normal static destruction ordering. The log-copy worker in `init.cpp` is intentionally kept as a never-destroyed singleton so dedicated callback exits do not race its base/derived thread teardown under ASAN/UBSAN.
- Dedicated config filenames are still routed through the legacy `STARTSERVER=...` parser, which uppercases the argument in place. On case-sensitive filesystems, validation configs therefore need matching on-disk casing until that parser is cleaned up separately.

## Emscripten runtime data layout

- The current Renegade startup path does **not** need `always.dat`, `always.dbs`, or `Always2.dat` at the virtual filesystem root for the main game path.
- `Code/Commando/init.cpp` wires those mix files through `RenegadeBaseFileFactory`, whose subdirectory is `DATA`, so packaging them at `/Data/Always2.dat`, `/Data/always.dat`, and `/Data/always.dbs` matches the runtime's actual lookup contract.
- The important web preload layout is therefore:
  - shipped `Renegade/Data/...` -> `/Data/...`
  - shipped optional `Renegade/HTML/...` -> `/HTML/...`
  - shipped optional `Renegade/Internet/...` -> `/Internet/...`
  - selected shipped top-level files -> `/...`
- The generated `Renegade.js` preload manifest is the easiest place to verify that a packaged web build matches the expected virtual paths.
- The non-Windows dialog path used by Emscripten does **not** read compiled Win32 resources. It parses `Code/Commando/chat.rc` at runtime and also needs `Code/Commando/resource.h`, `Code/Commando/dialogresource.h`, and `Code/Combat/string_ids.h` available in the virtual filesystem.
- The shipped top-level files `00000409.256` and `00000409.016` are BMP assets, not the string/conversation databases. Mapping them to `/STRINGS.TDB` or `/CONV10.CDB` produces version/translation failures.
- The real `STRINGS.TDB` and `CONV10.CDB` used by Renegade live inside `Renegade/Data/always.dbs`.
- A successful Emscripten `STRINGS.TDB` load must populate the translation objects themselves, not just the DB header/version fields. If the browser build reports the correct version but still returns `TDBERR` for menu IDs, check whether the wwtranslatedb persist factories were linked in.
- For this codebase, Emscripten/wasm archive linking can drop the wwtranslatedb factory object files when only the translation database subsystem is referenced. Forcing references to the persist-factory statics in `TranslateDBClass::Initialize()` keeps `translateobj.cpp`, `stringtwiddler.cpp`, and `tdbcategory.cpp` linked so `SaveLoadSystemClass::Find_Persist_Factory(CHUNKID_TRANSLATE_OBJ)` succeeds at runtime.

## Emscripten build notes

- Firefox's WebGL 2 `texSubImage2D(..., srcData, srcOffset)` upload path rejects a wasm heap-backed typed array once the underlying heap reaches the 2 GiB boundary. In practice, leaving Emscripten's growth ceiling at `2147483648` bytes is enough to trigger the browser exception even though the upload itself is much smaller.
- For this codebase's browser renderer, keep `-sMAXIMUM_MEMORY` below 2 GiB when `-sALLOW_MEMORY_GROWTH=1` is enabled. The current source-controlled default is `2047` MiB via `RENEGADE_EMSCRIPTEN_MAXIMUM_MEMORY_MB`, which stays compatible with the fast WebGL upload path without disabling runtime growth outright.
- For the current browser renderer path, SDL should create the window/canvas with `SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN` and let bgfx own WebGL context creation, just like the desktop path already lets bgfx own the native renderer/device.
- Do not mix SDL window creation with a second manual `emscripten_webgl_create_context(...)` call for the same canvas. Firefox may tolerate the split ownership, but Chrome can end up with a mismatched current context and fail inside `BgfxRenderer::Init()`.
- `Code/ww3d2/bgfxrenderer.cpp::Query_Native_Window` should only do two things on Emscripten:
  - provide the canvas id through `bgfx::PlatformData.nwh`
  - if some higher layer has already made a context current, validate that it is WebGL 2 before passing it to bgfx through `platformData.context`
- `Code/ww3d2/bgfxrenderer.cpp::Update_Platform_Window` must preserve the immutable bgfx platform-data fields after `bgfx::init()`:
  - `context`
  - `ndt`
  bgfx allows later `setPlatformData()` calls to update the native window/backbuffer side, but changing `context` or `ndt` after initialization aborts the renderer.
- The current Renegade bgfx renderer should treat WebGL 2 as required, not optional:
  - `SkinPaletteTexture` currently requires `RGBA32F` support during `Init_Render_Resources()`
  - a WebGL 1 fallback only converts the real failure into the misleading legacy DirectX startup dialog
- The stable Emscripten link contract for this renderer is:
  - `-sMIN_WEBGL_VERSION=2`
  - `-sMAX_WEBGL_VERSION=2`
  - `-sFULL_ES3=1`
- For bgfx textures on the WebGL path, `createTexture2D(..., hasMips=true, ...)` still implies a full mip chain even if the legacy asset/runtime only provides a shorter mip list.
- If the renderer only uploads the provided mip levels, Chromium reports lazy initialization on the untouched tail levels during later draw calls; eagerly defining the remaining levels in `Code/ww3d2/bgfxrenderer.cpp` avoids the warning and keeps first-use sampling deterministic.
- The skinned-mesh palette texture on Emscripten still needs real storage defined up front, but it also has to remain mutable for the later per-row bone-palette updates.
- In practice, that means `Code/ww3d2/bgfxrenderer.cpp` should create `SkinPaletteTexture` without initial memory and then immediately issue one full-surface zero `bgfx::updateTexture2D(...)` upload. Passing the zero data to `createTexture2D(...)` directly makes the texture immutable and breaks skinned meshes.

- The project-wide SDL include fix belongs in the shared interface target, not in individual libraries. Linking `SDL3::SDL3` through `renegade_project_options` lets the low-level headers include SDL without each legacy target having to remember SDL explicitly.
- Emscripten/Clang is much stricter than the original MSVC toolchain about:
  - extra qualification inside class declarations
  - passing non-trivial wrapper/string objects through `printf`-style varargs
  - old enum bitwise code that relies on implicit integer conversions
  - const-correctness on legacy APIs
- The vendored bgfx shader tool runs as `shaderc.js` during an Emscripten build, so it needs host-filesystem access (`-sNODERAWFS=1`) and a larger stack than the default for this codebase's shader preprocessing workload.
- For the current Renegade web build, SDL pthread support should stay disabled. If SDL is configured with Emscripten pthreads enabled, the final link pulls in `--shared-memory`; that requires every linked object to be compiled with atomics/bulk-memory support and breaks the current single-threaded build.
- The in-tree Bink decoder includes vendored FFmpeg C sources directly. Its target therefore needs the `Code/BinkMovie/ffmpeg` include root in addition to `Code/BinkMovie`, otherwise nested includes such as `libavutil/attributes.h` fail under normal out-of-tree builds.
- The fetched `bgfx.cmake` tree currently includes `cmake/bimg/bimg_encode.cmake` unconditionally, even when `BGFX_BUILD_TOOLS_TEXTURE` is off. That is harmless on native builds, but it breaks the Emscripten build because `bimg_encode` pulls NVTT encode code that does not recognize the web target.
- The stable local fix is to patch fetched `cmake/bimg/CMakeLists.txt` at configure time so `bimg_encode` is only included together with `texturec`. Renegade's web build only needs `shaderc`, so skipping `bimg_encode` preserves the required shader pipeline without carrying the non-portable encode library into the web toolchain.
- `launche.sh` should not assume an already-valid `build-em` tree. Re-running `emcmake cmake -S . -B build-em ...` before `cmake --build` keeps the fetched dependency patches and Emscripten cache options in sync with source-controlled changes.
- Reusing `build-em` across configuration changes can leave ABI-incompatible object files behind (for example, a previous web build produced with a different `CMAKE_BUILD_TYPE`). Cleaning the tree before rebuilding avoids class-layout/thunk mismatches at final wasm link.
- For the browser entrypoint target, Emscripten only emits the launcher HTML page when the final output path ends in `.html`. If the target is left with the default executable suffix, the link step produces only `.js`/`.wasm`/`.data`, and `emrun` will end up serving a directory listing instead of the game page.
- `MainMenuTransitionClass` must keep ref-counted ownership of the dialogs it animates. The dialog manager keeps the transition alive across active-dialog changes, and raw dialog pointers in the transition can turn into wasm `memory access out of bounds` traps once the old dialog unregisters or its controls are queried after teardown.
- The safe transition contract for the web build is:
  - require `Model`, `TransitionAnim`, `Camera`, and `Dialog` in `Is_Valid()`
  - in `Update_Controls()`, stop the transition if the dialog is no longer running
  - tolerate missing controls instead of assuming every menu resource variant carries the full animated control set
- Browser validation for this port is only trustworthy when Chrome is started with WebGL-capable flags. `--disable-gpu` produces a misleading startup failure for the bgfx/WebGL path; the validated headless configuration here used `--use-gl=angle --use-angle=swiftshader --enable-webgl --ignore-gpu-blocklist`.

## bgfx mesh lighting contract

- The bgfx mesh fragment shader should not treat runtime lighting as "4 directional colors" anymore.
- The stable runtime contract is now:
  - `u_meshSceneAmbient`: scene/global ambient
  - `u_meshLightPosType[4]`: light position plus type (`directional`, `point`, `spot`)
  - `u_meshLightDirSpot[4]`: light travel direction plus spotlight outer-cone cosine
  - `u_meshLightDiffuseRange[4]`: diffuse color plus range
  - `u_meshLightAmbientAtten[4]`: ambient color plus the linear-falloff start distance
- Mesh lighting is evaluated in world space in `fs_mesh.sc` using `v_worldPos` and `v_worldNormal`, so the CPU-side bridge only needs to choose the active lights and upload descriptors.
- Because lighting is shader-evaluated, lit draws no longer need a special transient-buffer submission path just because lighting is enabled. Fog and texgen still remain separate submission constraints.

## LightEnvironment bridge

- `LightEnvironmentClass` is still responsible for choosing the most important local lights for an object, but it should preserve full light descriptors for the selected lights instead of collapsing them to directional proxies.
- Weak or overflow lights can still be folded into the global ambient term to preserve the legacy 4-light budget, but the selected active lights should reach the shader as real point/spot/directional lights.
- For the active-light shader path, per-light ambient should stay with the light descriptor; otherwise it gets double-counted if also folded into `D3DRS_AMBIENT`.

## DX8-wrapper light packing

- The `D3DLIGHT8.Direction` field used by the modern renderer should come from the light transform (`-transform.Z`) for directional and spot lights.
- Using `LightClass::SpotDirection` directly as the runtime render-state direction is not reliable for the current game/runtime path and breaks directional-light orientation on the bgfx side.
- If `LightClass::FAR_ATTENUATION` is disabled, the bgfx-facing light descriptor should not synthesize a fake finite linear falloff from the default start/end attenuation values.

## bgfx Z-bias contract

- In the current bgfx renderer, `D3DRS_ZBIAS` is still implemented through the DX8 wrapper, not through native bgfx raster state.
- The active projection matrix alone is **not** enough to preserve this behavior. The wrapper also needs the projection clip-plane range (`ZNear` / `ZFar`) so `DX8Wrapper::Get_Transform(D3DTS_PROJECTION, ...)` can fold the requested bias into the matrix before submission.
- `CameraClass::Apply` therefore must use `DX8Wrapper::Set_Projection_Transform_With_Z_Bias(...)`, not the generic projection `Set_Transform(...)` path.
- Any temporary projection override that restores the old matrix later (for example the dazzle overlay path) must preserve the raw stored projection matrix together with the clip-plane range. Saving the already bias-adjusted matrix from `Get_Transform(D3DTS_PROJECTION, ...)` can reapply the same bias twice on restore.
- Current in-tree consumers of this contract include decal rendering, multi-pass rigid mesh rendering, and scene extra-pass wireframe overlays because they all rely on `D3DRS_ZBIAS`.

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

# Porting Progress

## Recent changes

- Split the bgfx bootstrap "uber" shader into a small set of specialized draw programs and moved more fixed-function emulation work from the CPU submit loop into shader code.
- Root cause: `Code/ww3d2/dx8wrapper.cpp` pushed every triangle draw through one bgfx program pair, so even trivial draws carried fog/lighting/texgen plumbing and the wrapper still resolved unlit material color plus direct-light shading per vertex on the CPU before uploading transient geometry.
- Resolution: `Code/ww3d2/shaders/` now provides dedicated `basic`, `unlit`, `lit_environment`, and `lit_dynamic` shader programs, `Code/ww3d2/CMakeLists.txt` builds those variants instead of the single bootstrap pair, and `dx8wrapper.cpp` now selects the program per draw based on actual renderer state. Unlit material resolution and direct-light shading now run in the shaders, while blend/depth behavior stays in bgfx render state instead of creating extra shader permutations.
- Validation follow-up: the first 300-second smoke also re-exposed two longer-path issues that had to be fixed before this task was complete: `DX8Wrapper::Set_Texture` had drifted back to caching raw `TextureClass*` bindings without refcount ownership, and `Code/WWMath/colmathobbobb.cpp` could pass an uninitialized `Side` value into contact-normal sign selection. The wrapper now uses `REF_PTR_SET` for cached texture bindings again, and `ObbCollisionStruct::Side` is initialized and clamped back to the intended `+/-1` sign behavior.
- Validation: rebuilt with `cmake --build build -j20 --config Debug`, ran `ctest --test-dir build --output-on-failure -j20` (no tests were present), then ran `ASAN_OPTIONS=detect_leaks=0 timeout 300s ./build/bin/Renegade` twice during validation. The final 300-second run completed the full timeout window (`EXIT:124`) and the captured log contained no AddressSanitizer, UndefinedBehaviorSanitizer, or fatal runtime errors.

- Improved bgfx runtime performance by removing forced per-offscreen-scene frame flushes from the D3D wrapper compatibility layer.
- Root cause: `Code/ww3d2/dx8wrapper.cpp` mapped every legacy `Begin_Scene`/`End_Scene` pair onto one bgfx view. Because bgfx framebuffer and viewport state are view-level, the wrapper used `bgfx::frame(BGFX_FRAME_FLUSH)` for `End_Scene(false)` so offscreen render-target passes could safely change targets before the final presented scene. Dynamic/projected shadow rendering uses that offscreen path, so the port was serializing render-target work into extra bgfx frames instead of keeping it inside one gameplay frame.
- Resolution: the bgfx wrapper now allocates an ordered bgfx view per legacy scene, keeps render-target/view state attached to the current scene view, and only calls `bgfx::frame()` when the scene actually flips the presented frame. Offscreen render-target passes now stay in the same bgfx frame without a forced flush.
- Follow-up investigation also confirmed that the remaining active Linux/bgfx `Get_Surface_Level()` call sites are not triggering render-target GPU readback during normal gameplay: the actual render-target reads in `Code/wwphys/pscene_projectors.cpp` are compiled out on bgfx, while the still-active `metalmap`/`enbassetmgr` sites operate on regular textures rather than bgfx render targets.
- Validation: rebuilt with `cmake --build build -j20`, ran `ctest --test-dir build --output-on-failure -j20` (no tests were present), then ran `ASAN_OPTIONS=detect_leaks=0 timeout 60s ./build/bin/Renegade` and `ASAN_OPTIONS=detect_leaks=0 timeout 300s ./build/bin/Renegade`. Both runtime validations survived the full timeout window and exited via timeout (`EXIT:124`) rather than a crash/assert.

- Removed an unnecessary bgfx static-shadow GPU->CPU->GPU round trip and fixed a shutdown-time conversation teardown hazard exposed during validation.
- Root cause: `Code/wwphys/pscene_projectors.cpp` rendered cached static shadows into a render target, read that texture back through `TextureClass::Get_Surface_Level()`, and immediately re-uploaded it as a new texture. The same validation pass also exposed a shutdown ordering bug where `ConversationMgrClass::Shutdown()` notified script observers after `ScriptManager::Shutdown()` had already destroyed them.
- Resolution: bgfx static shadow caching now keeps the generated texture GPU-resident and binds it directly, `Code/ww3d2/texture_bgfx.cpp` allocates render-target readback resources lazily instead of eagerly, and `Code/Combat/combat.cpp` now shuts down `ConversationMgrClass` before `ScriptManager`.
- Validation: rebuilt successfully before the wrapper follow-up work, and the crash found during the first smoke pass no longer reproduces in the later 60-second and 300-second validation runs above.

- Fixed bgfx HUD/mission text glyph selection so dynamic text no longer collapses into the same `0123456789...`-style placeholder sequence.
- Root cause: the bgfx `SurfaceClass::FindBB` port ignored the caller-provided bounding rectangle and scanned the entire surface. `Font3DDataClass::Make_Proportional` relies on `FindBB` to measure each glyph inside its own cell, so the bgfx path was giving many characters the same bounding box/UV region from the shared font atlas.
- Resolution: `Code/ww3d2/surfaceclass_bgfx.cpp` now clamps to and scans only the requested sub-rectangle, matching the original D3D implementation's behavior. That restores per-character atlas bounds for `Font3D` HUD text renderers such as health, ammo, and scripted on-screen messages.
- Validation: rebuilt with `cmake --build build -j20` and ran `timeout 35s ./Renegade` from `build/bin`. The executable stayed alive for the full smoke window (`EXIT:124`), and the captured runtime log did not report any `FONT6x8/FONT8x8/FONT12x16/FONT24x36` load failures while exercising the live renderer path.

- Fixed Linux TGA loading for legacy lowercase includes such as `#include "targa.h"`.
- Root cause: non-Windows targets prepended `Code/compat` to the include path, and `Code/compat/targa.h` was only a stubbed shim. Files like `ww3dformat.cpp` therefore saw a zero-initialized fake `Targa` header, which surfaced at runtime as `TextureClass: Targa has unsupported bitdepth(0)` and `BGFX Surface: failed to load FONT6x8.TGA via TextureLoader`.
- Resolution: removed the stubbed `Code/compat/targa.h` entry point from the active path and updated the renderer-side includes/loaders to use the real `Code/wwlib/TARGA.H` implementation directly, so case-sensitive Linux builds no longer substitute the fake header.
- Validation: rebuilt with `cmake --build build -j20` and ran `./build/bin/Renegade`. The follow-up runtime log no longer reports the `FONT6x8.TGA` load failure or the `unsupported bitdepth(0)` TGA error. The executable now reaches `MainLoop: Entering main loop`; the remaining non-zero exit in this environment is a pre-existing LeakSanitizer report rooted in external RenderDoc/Vulkan library allocations during shutdown.

- Advanced the bgfx renderer from a bootstrap smoke-test path toward a real Direct3D replacement by implementing several previously stubbed `DX8Wrapper` entry points that live gameplay/render code already calls.
- Root cause: the bgfx path still exposed key D3D-era APIs such as `Set_Light_Environment`, `Set_Light`, `Draw_Strip`, `Set_Render_Target`, `Set_Gamma`, `Create_Render_Target`, and `Is_Render_To_Texture` as no-op placeholders. That left fixed-function lighting, strip submission, gamma controls, and render-to-texture behavior missing even though higher-level engine code expected them.
- Resolution: `Code/ww3d2/dx8wrapper.cpp` now ingests light state and approximates fixed-function lighting in the bootstrap path, converts strips to triangle lists for submission, binds bgfx framebuffers for render targets, reports render-to-texture/gamma support, and uploads gamma/brightness/contrast controls to the bootstrap shader. `texture_bgfx.cpp`, `bgfx_compat_resources.h`, and `dx8texman_bgfx.cpp` now track bgfx render-target resources and default-pool texture recreation instead of leaving those paths stubbed out.
- Validation: rebuilt with `cmake --build build --config Debug -j2` and repeatedly ran `./build/bin/Renegade` under `timeout 300s` from `build/bin`. The final validation run completed the full 300-second smoke window and exited via timeout (`EXIT:124`) rather than a crash or assert.

- Fixed several deeper runtime blockers uncovered while validating the stronger bgfx path for the required 300-second smoke window.
- Root cause: once the renderer got far enough into live mission/script paths, ASan/asserts exposed unrelated robustness issues: logical-audio scene removal could invalidate the current object mid-call, combat/logical sound IDs were being forced back into a too-small enum type, several mission scripts assumed target arrays and object pointers were always valid, and `Test_Cinematic::Command_Set_Primary` overflowed a stack buffer when formatting callback IDs.
- Resolution: logical sound/listener removal now keeps temporary references and advances iterators safely during scene teardown, `CombatSound` now stores raw integer type IDs, the mission/test scripts now initialize and bounds-check their target/object state before using it, `ControlClass::Clear_Control` now also clears the pending input bitfields, and `Test_Cinematic.cpp` now formats the callback ID with a bounded buffer.

- Implemented real scene fog in the bgfx renderer instead of leaving `DX8Wrapper::Set_Fog` as a no-op.
- Root cause: the bgfx bootstrap path only forwarded texture/color/alpha-test state, so scene fog range/color never reached the GPU. The bgfx-only `ShaderClass::Enable_Fog` path also still inherited fixed-function fog restrictions and warned on blend modes used by vehicle wheel materials.
- Resolution: the bgfx wrapper now stores fog enable/color/range state, computes a per-vertex fog factor in camera space, passes fog mode/color through bgfx uniforms, and applies the legacy fog behaviors in the bootstrap shaders. The bgfx `shader_headless.cpp` fog selector now also maps additional non-fixed-function blend combinations to `FOG_SCALE_FRAGMENT` instead of warning.

- Fixed sanitizer-detected fog-of-war storage bugs in the UI map control and gameplay map manager.
- Root cause: `MapCtrlClass::Initialize_Cloud` allocated the fog-of-war bit vector with `new[]`, but `Free_Cloud_Data` destroyed it with scalar `delete`. The UI cloud-vector sizing also used a `sizeof(uint32)` divisor instead of a 32-bits-per-word divisor, and both the UI and gameplay cloud bit math offset the bit index by `+ 1`, which can produce undefined `1 << 32` shifts on cell boundaries.
- Resolution: `MapCtrlClass::Free_Cloud_Data` now uses `delete[]`, the UI cloud-vector allocation/reset sizing now matches the gameplay `((cell_count / 32) + 1)` word-count formula, and the fog-of-war bit index calculations in `mapctrl` and `mapmgr` now use the in-word range `0..31` instead of `1..32`.
- Replaced the null WWAudio path with a selectable SDL_mixer-backed backend while preserving the original WWAudio gameplay-facing object model.
- Added `external/SDL_mixer` as a git submodule and introduced the `RENEGADE_AUDIO_BACKEND` CMake option with `SDL_MIXER` and `NULL` values. The default is now `SDL_MIXER`.
- Root build wiring now brings in `external/SDL_mixer` when the SDL backend is selected, and WWAudio switches between the restored portable source set and the null backend source set based on the same option.
- Restored the original WWAudio policy/object code for the SDL backend path, including `AudibleSound`, `Sound3D`, `SoundPseudo3D`, `SoundScene`, logical audio objects, save/load helpers, and sound handles.
- Added `Code/WWAudio/sdlmixer_mss.cpp` as a Miles-compatibility shim on top of SDL3_mixer so legacy WWAudio code can keep using the expected sample, stream, and 3D-handle APIs.
- Implemented SDL_mixer-supported features through that shim: 2D playback, streamed playback, loop counts, per-sound volume and pan, playback-rate adjustment, paused/resumed/stopped state, listener/sample transforms, pseudo-3D attenuation/panning, basic positional 3D approximation, user-data plumbing, and file-factory-backed stream loading.
- Fixed missing music playback in the SDL_mixer backend. Root cause: compressed callback-stream playback for MP3 music was failing in SDL_mixer's seek path, and unknown-size file handles could also fall into an unsafe non-streamed buffer allocation path.
- Resolution: corrected the custom SDL IO wrapper to treat short reads and close semantics properly, made sound-buffer loading robust when `FileClass::Size()` is unknown, defaulted unknown-size 2D assets to the stream-buffer path, and added an SDL_mixer fallback that preloads compressed tracks when direct callback-stream playback cannot seek cleanly.
- Fixed multiple LP64 portability bugs uncovered while restoring the original WWAudio code. Miles user data and file callback payloads were still being truncated through `U32`/`S32`; those paths now use `uintptr_t` so sound object pointers survive on Linux/x86_64.
- Fixed several Linux case-sensitivity and modern-toolchain issues in the restored audio code, including include-path casing mismatches and a loop variable bug that newer compilers rejected.
- Validation: rebuilt `WWAudio`, rebuilt the full project with `cmake --build build -j4`, and ran `./build/bin/Renegade` under `timeout 35s`. The executable stayed alive for the full smoke window, selected `SDL_mixer` as the 3D sound device at startup, and did not report ASan/UBSan failures during the final run. Runtime logs also confirmed streamed WAV UI audio started normally and MP3 music tracks such as `menu.mp3` and `sakura battle theme.mp3` now take the SDL_mixer fallback path instead of failing.

- Fixed Linux gameplay script module discovery so mission logic can find the external `Scripts` shared library when running from the CMake build tree.
- Root cause: `ScriptManager` only tried bare module names such as `SCRIPTS.DLL`, `Scripts.so`, and `libScripts.so`. The current build layout places the executable in `build/bin` and the shared script module in `build/lib`, so the loader never resolved the mission script library from the executable's working directory.
- Resolution: `Code/Combat/scripts.cpp` now expands script-module lookup to also probe paths relative to `SDL_GetBasePath()`, including the executable directory and its sibling `../lib` directory, while preserving the existing compatibility fallbacks for the legacy DLL names.
- Validation: rebuilt with `cmake --build build -j4` and ran `./build/bin/Renegade` under `/usr/bin/timeout 35s`; the executable stayed alive for the full smoke window after the loader change.
- Fixed an AddressSanitizer-detected heap-buffer-overflow in `Code/Commando/datasafe.cpp`.
- Root cause: `GenericDataSafeClass::Swap_Entries` used `long` in the `size == 4` fast path. On LP64 platforms like Linux, `long` is 8 bytes, so the swap read and wrote past 4-byte payloads such as `int` and `float`.
- Resolution: replaced the `long`-based swap with a `uint32`-based swap so the optimized path remains 4 bytes on all supported platforms.
- Fixed multiple LP64 save/load regressions that only appeared once the first mission progressed into live object/script restore.
- Root cause: several gameplay loaders still serialized raw pointer arrays or read pointer-sized fields with `sizeof(pointer)`. Mission data stores 32-bit remap tokens, so those sites mis-read chunks on Linux/x86_64 and produced bogus observer/occupant remap pointers.
- Resolution: `ScriptableGameObj::Load` now reads observer pointers through `ChunkIO_Read_Value`, and `VehicleGameObj` now saves/loads seat occupants through the same 32-bit pointer-token helpers instead of raw native pointer arrays.
- Fixed an AddressSanitizer alloc/dealloc mismatch in `Code/Scripts/scripts.cpp` while validating mission scripting at runtime.
- Root cause: `ScriptImpClass` allocated the argument vector with `new[]` but destroyed it with scalar `delete`.
- Resolution: `ScriptImpClass::Clear_Parameters` now uses `delete[]` for the parameter vector.
- Fixed a UBSan out-of-bounds read in `Code/Combat/viseme.cpp` triggered during the same mission smoke run.
- Root cause: `VisemeManager::Lookup` indexed `gsVisemeTable[start + count - 1]` even when a letter bucket had `count == 0`.
- Resolution: the lookup now returns early for empty buckets before computing the backward search start.
- Validation: rebuilt with `cmake --build build --config Debug -j4` and ran `./build/bin/Renegade` under `/usr/bin/timeout 35s`; the first mission now survives the full 35-second smoke window with no assert, ASan, or UBSan failure.
- Note: `Load Definitions M13.ddb` still logs a missing file during first-mission startup, but `M13.mix` only contains `M13.ldd`/`M13.lsd` and the mission now continues past that point, so the missing `.ddb` is non-fatal for this data set.
- Fixed an AddressSanitizer-detected heap-use-after-free in `Code/ww3d2/texture_bgfx.cpp` reached during executable smoke validation.
- Root cause: `DX8Wrapper` cached raw `TextureClass*` bindings in bgfx render state without retaining a reference, allowing UI textures to be destroyed while still bound for later draw submission.
- Resolution: `DX8Wrapper::Set_Texture` now retains/release-refcounts the currently bound textures, and draw-state shutdown/reset paths release those retained references.
- Validation: rebuilt with `cmake --build build -j32` and ran `./build/bin/Renegade` under `/usr/bin/timeout 35s`; the executable stayed up for the full smoke window with no AddressSanitizer errors.
- Fixed an AddressSanitizer-detected heap-use-after-free in `Code/Combat/conversationmgr.cpp` during the mission/tutorial conversation path.
- Root cause: `ConversationMgrClass::Think` iterated `ActiveConversationList` with raw pointers and called `ActiveConversationClass::Think()` without holding a temporary ref. Script callbacks reached from `Notify_Monitors_On_End` can re-enter the conversation manager, reset the active list, and delete the current conversation before the outer loop calls `Is_Finished()` or removes it.
- Resolution: the manager now keeps a temporary reference across each `ActiveConversationClass::Think()` call and only removes the conversation by index when the same entry is still present in `ActiveConversationList` after any re-entrant mutation.
- Validation: rebuilt with `cmake --build build --config Debug -- -j$(nproc)` and ran `./build/bin/Renegade` under `/usr/bin/timeout 35s`; the executable stayed alive for the full smoke window and the captured log contained no AddressSanitizer or UBSan failures.

## Remaining work

- Continue smoke and gameplay-path validation to catch additional renderer or lifetime issues that only appear after deeper menu/game interaction, especially effects/aggregate paths that still log missing subobjects during the 300-second run.
- Continue pushing bgfx beyond the current bootstrap emulation layer toward fuller fixed-function parity, especially where the original D3D renderer used capabilities that are still only approximated on the CPU side.
- Audit the SDL_mixer Miles shim against more in-game audio content paths, especially long-form music/dialog streams and any feature combinations that previously depended on Miles-specific DSP behavior.

## CPU render hotspot optimizations (profiler-driven)

Runtime profiling of a 120-frame sustained-render window (~16.82 ms/frame average) identified the bgfx CPU submission path as the bottleneck (GPU idle, waitSubmit large). Three targeted port-regression fixes were identified and applied:

### Fix 1: `LightEnvironmentClass::Pre_Render_Update` dead matrix work eliminated
- **File**: `Code/ww3d2/lightenvironment.cpp`
- **Root cause**: The original D3D backend required light directions in camera space; `Pre_Render_Update` computed `Inverse_Rotate_Vector` per light → stored in `OutputLights[]`. In the bgfx port, `Set_Light_Environment` reads world-space directions from `InputLights[]` and the renderer re-rotates via `View_Rotate_Vector` at submission time. `OutputLights[]` is never read anywhere in the bgfx codebase — the per-light loop was pure waste called for every visible physics object per frame.
- **Fix**: Removed the per-light loop body. Kept the `OutputAmbient` clamp since `Get_Equivalent_Ambient()` is consumed by `Set_Light_Environment`. Added a comment explaining the D3D vs bgfx split.
- **Impact**: Up to `LightCount` (max 4) matrix-vector multiplications eliminated per visible object per frame.

### Fix 2: Camera transform cached before mesh loop in `DX8TextureCategoryClass::Render`
- **File**: `Code/ww3d2/dx8renderer.cpp`
- **Root cause**: For ALIGNED and ORIENTED billboard mesh modes, `TheDX8MeshRenderer.Peek_Camera()->Get_Transform()` was called per mesh to extract camera Z-vector and camera position respectively. The camera doesn't change within a single category render, so these were redundant dereferences every iteration.
- **Fix**: Cache `camera_z_vector` and `camera_position` once before the `PolyRenderTaskClass` loop; use cached values in the ALIGNED/ORIENTED branches.

### Fix 3: `DX8Wrapper::Set_Texture` equality guard
- **File**: `Code/ww3d2/dx8wrapper.cpp`
- **Root cause**: `Set_Texture` unconditionally wrote `g_bgfx.textures[stage]` even when the incoming texture pointer was identical to the one already bound.
- **Fix**: Skip the write when `g_bgfx.textures[stage] == texture` (avoids unnecessary cache-line dirty per texture stage per texture category render).

### Rejected ideas
- **Deduplicating `Pre_Render_Update` calls per shared `LightEnvironmentClass`**: Objects can share light environments but the Pre_Render_Update call-site in pscene.cpp doesn't track which envs have already been updated this frame. The fix would require a per-frame generation counter or a visited set, adding complexity and risk.
- **Removing `PolyRenderTaskClass` Add_Ref/Release_Ref**: The ownership semantics are correct and the pool already amortizes allocation cost; changing the ref protocol risks use-after-free in unusual render paths.
- **Restructuring texture category sort/batching**: Would require non-trivial renderer restructuring with high regression risk.

- **Validation**: `cmake --build build -j20` succeeded with no warnings. `./build/bin/Renegade` ran stably under `timeout 310` with no ASAN/UBSAN errors.

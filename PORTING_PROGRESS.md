# Porting Progress

## Recent changes

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

- Continue smoke and gameplay-path validation to catch additional renderer or lifetime issues that only appear after deeper menu/game interaction.
- Audit the SDL_mixer Miles shim against more in-game audio content paths, especially long-form music/dialog streams and any feature combinations that previously depended on Miles-specific DSP behavior.

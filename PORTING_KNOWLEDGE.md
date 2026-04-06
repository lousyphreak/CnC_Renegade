# Porting Knowledge

## TGA include casing on Linux

- This tree contains legacy includes that request the Westwood loader as `targa.h`, `TARGA.H`, and `Targa.h` depending on subsystem and era.
- On case-sensitive filesystems, a lowercase compatibility shim can accidentally shadow the real `Code/wwlib/TARGA.H` header if `Code/compat` appears earlier on the include path.
- If that shim is only a stub, runtime failures look misleading: the loader appears to "open" a TGA, but header fields such as `PixelDepth`, `Width`, and `Height` stay zero and downstream code reports invalid format errors like `unsupported bitdepth(0)`.
- The safe Linux fix in this tree is to remove the stubbed lowercase compatibility header from the active include path and update the renderer-side includes to reference the real `Code/wwlib/TARGA.H` implementation explicitly.

## bgfx font atlas bounding boxes

- `Font3DDataClass::Make_Proportional` does not search the whole font texture for a glyph. It passes the current glyph cell bounds into `SurfaceClass::FindBB` and expects the result to stay inside that rectangle.
- The original D3D `SurfaceClass::FindBB` honors those input bounds by locking only the requested rectangle. If a port scans the entire surface instead, every glyph can inherit the same global bounding box and therefore the same UV slice after atlas repacking.
- On Renegade's HUD this failure mode looks like dynamic text rendering as a repeated deterministic atlas sequence instead of the requested string, because multiple characters now sample the same part of the `Font3D` atlas.
- The bgfx implementation should therefore clamp to the caller's `[min,max)` rectangle and preserve the original "no visible pixels" semantics by returning the input bounds crossed (`real_min = *max`, `real_max = *min`) when nothing opaque is found.

## bgfx DX8 wrapper parity notes

- The current bgfx path was not just missing polish; several `DX8Wrapper` APIs were still literal no-op placeholders even though higher-level renderer/gameplay code actively called them. Before chasing visual mismatches, audit `dx8wrapper.h` for stubbed methods and verify each has a real bgfx implementation.
- The highest-impact missing parity surfaces in this tree were light-state ingestion, triangle-strip submission, gamma controls, render-target binding/creation, and default-pool texture management. Filling in those seams moves bgfx from "test renderer" toward "D3D replacement" much faster than rewriting the full legacy renderer in one pass.
- Render-to-texture support on bgfx needs both wrapper state and resource state. Tracking only a texture handle is not enough; the texture backend must also own a framebuffer handle and expose it back to `DX8Wrapper` so the active view can switch between the main backbuffer and a texture-backed framebuffer.
- The current lighting parity uses a CPU-side approximation in the bootstrap submission path. That is enough to keep legacy light/environment state from being ignored completely, but it is still an emulation layer rather than a full shader-permutation replacement for the original D3D fixed-function pipeline.
- Gamma support must preserve the legacy `Set_Gamma(float gamma, float brightness, float contrast, bool calibrate, bool save)` signature because existing engine/UI code still calls the five-argument form even if the bgfx implementation ignores the trailing booleans.

## bgfx scene fog behavior

- The bgfx renderer does not inherit D3D8 fixed-function fog automatically; `SceneClass::Render` only stays wired up if `DX8Wrapper::Set_Fog` stores the scene fog state and the bgfx draw path forwards it into shader uniforms.
- Renegade camera space is forward-negative-Z. A practical linear fog factor for the bgfx path is therefore based on `max(-view_z, 0)` against the scene's fog start/end distances.
- For the bgfx bootstrap shader, `FOG_ENABLE` should blend fragment RGB toward the scene fog color, `FOG_SCALE_FRAGMENT` should attenuate fragment RGBA by `(1 - fog)` so alpha-driven blend modes also fade out, and `FOG_WHITE` should blend fragment RGB toward white to preserve the original multiplicative-fog behavior.
- The bgfx-only `shader_headless.cpp` path can safely accept more blend combinations than the old fixed-function renderer because the shader can explicitly attenuate the fragment before the backend blend stage. That is the right place to suppress legacy "Unable to fog shader" warnings for bgfx-specific support.

## UI map cloud buffer ownership

- `MapCtrlClass` in `Code/wwui/mapctrl.cpp` owns its fog-of-war bitfield as a dynamic `uint32[]` array.
- Keep the UI cloud buffer sizing consistent with the gameplay map manager: the storage count is `((cell_count / 32) + 1)` 32-bit words, not a byte-count expression based on `sizeof(uint32)`.
- Because the buffer is allocated with `new[]`, teardown must always use `delete[]`. ASan will flag this immediately when the EVA encyclopedia/map tab is destroyed.
- The fog-of-war bit index inside each 32-bit word must stay in the range `0..31`. The original `+ 1` offset turns every 32nd cell into `1 << 32`, which UBSan reports as undefined behavior on modern builds.

## SDL_mixer WWAudio backend notes

- The safest audio-porting seam is the Miles compatibility layer, not the gameplay-facing WWAudio classes. `AudibleSound`, `Sound3D`, `SoundScene`, logical sound objects, and save/load behavior already contain the original gameplay policy and can be kept largely intact.
- SDL3_mixer is good enough to preserve the major Renegade audio behaviors that matter to gameplay: 2D samples, streamed playback, per-track gain, stereo panning, playback-rate scaling, pause/resume/stop, and basic positional approximation.
- Miles-era hardware/DSP features do not map 1:1. Treat true reverb/filter provider behavior as best-effort/no-op unless SDL_mixer exposes an equivalent control.
- `RENEGADE_AUDIO_BACKEND` is the build switch for audio backend selection. Supported values are `SDL_MIXER` and `NULL`, and the intended default is `SDL_MIXER`.
- The `external/SDL_mixer` submodule at `release-3.2.0` did not include the vendored codec dependency trees needed for `SDLMIXER_VENDORED=ON`. On this tree, SDL_mixer must currently be configured with `SDLMIXER_VENDORED=OFF` and use system codec packages.
- Linux/x86_64 exposed latent WWAudio assumptions that Win32 tolerated. Any Miles handle/user-data or file-callback payload that stores object pointers must use `uintptr_t`, not `U32`, `S32`, or pointer-to-int truncating casts like `(S32)this`.
- Restoring original files also reintroduces Windows-era include casing. Audio code that compiles on case-insensitive filesystems may still fail on Linux until header names are matched exactly.
- SDL_mixer callback-stream playback is not equally reliable across codecs. In this port, streamed WAV content worked directly through `MIX_SetTrackIOStream`, but MP3 music hit `mpg123_seek` errors on the custom callback-backed IO path.
- The practical fix is to keep callback-stream playback as the first choice, but fall back to `MIX_LoadAudio_IO` + `MIX_SetTrackAudio` for compressed tracks that fail to start. This preserves music playback while still using true callback streaming where SDL_mixer handles it cleanly.
- `FileClass::Size()` cannot be assumed to be valid for every file-factory source. Some audio assets can report an unknown size; code that casts that value into unsigned buffer lengths will explode on 64-bit builds. Treat non-positive sizes as "unknown" and either stream them or read incrementally instead of preallocating from the reported size.

## DataSafe LP64 hazard

- The original datasafe code assumes `long` is 32 bits in `GenericDataSafeClass::Swap_Entries`.
- That assumption is valid on Win32 but not on modern Linux/x86_64, where `long` is 64 bits.
- Any optimized swap path keyed on `size == 4` must use an explicitly 32-bit type such as `uint32`, not `long`, or it will overflow 4-byte payloads.
- This issue can surface during level loading and gameplay because datasafe shuffling touches entries from multiple threads and is exercised by common safe-wrapped scalar types.

## Renderer texture binding lifetime

- The bgfx-backed `DX8Wrapper` keeps legacy texture bindings in process-local render state between draw calls, similar to the original D3D device state model.
- Those cached `TextureClass*` bindings must participate in refcounting. Storing raw pointers is unsafe because UI and sentence rendering can destroy textures while the wrapper still plans to submit draws that read the current binding.
- When porting more D3D state to bgfx/SDL paths, treat cached resource bindings as owners until they are replaced or the draw state is reset/shutdown.

## Conversation manager re-entrancy

- `ConversationMgrClass::Think` is re-entrant through scripting. `ActiveConversationClass::Stop_Conversation` calls `Notify_Monitors_On_End`, and observer callbacks can immediately invoke script commands such as `Stop_All_Conversations()`.
- Because `ActiveConversationList` stores raw `ActiveConversationClass*` entries, the manager must hold an explicit temporary ref before calling `ActiveConversationClass::Think()`. Otherwise the current conversation can be deleted out from under the outer loop before the post-`Think()` cleanup runs.
- After any callback-driven `Think()` step, do not assume the original list index is still valid. Re-check whether the same pointer still occupies that slot before deleting it, and resync the loop if the active list changed underneath you.

## Script module discovery in the build tree

- Gameplay mission logic depends on the external `Scripts` shared library loaded by `ScriptManager` in `Code/Combat/scripts.cpp`.
- The modern CMake layout emits the main executable to `build/bin` and shared libraries to `build/lib`, so a loader that only probes bare names like `SCRIPTS.DLL` or `Scripts.so` will fail from the executable directory on Linux.
- Use `SDL_GetBasePath()` for runtime-relative probing and include the executable directory plus sibling library directories such as `../lib` in the candidate search list.
- Preserve the old DLL-name compatibility fallbacks (`SCRIPTS.DLL`, `SCRIPTSD.DLL`, `SCRIPTSP.DLL`) because game data and legacy code still request those names even on non-Windows platforms.

## Save/load pointer tokens stay 32-bit

- Legacy mission/save streams do not store native pointers. They store 32-bit remap tokens that are later matched by `PointerRemapClass`.
- `wwlib/chunkio.h` already contains the portability helpers for this (`ChunkIO_Write_Value` / `ChunkIO_Read_Value` pointer overloads plus `SaveLoad_Encode_Pointer_Token`).
- Any gameplay code that bypasses those helpers and reads/writes `sizeof(pointer)` directly will corrupt the stream on 64-bit builds. The failure mode looks like bogus remap targets such as `0x1`, `0xbebebebebebebebe`, or downstream asserts in `pointerremap.cpp`.
- During first-mission validation this showed up in `ScriptableGameObj::Load` and `VehicleGameObj` seat-occupant persistence. Similar raw pointer-array save/load code should be treated as suspicious anywhere else it still exists.

## First-mission data layout note

- `M13.mix` in this data set contains `M13.ldd` and `M13.lsd`, but not `M13.ddb`.
- The engine still attempts `Load Definitions M13.ddb` during mission startup, logs the missing file, and continues loading. Treat that message as non-fatal unless it is directly paired with another load failure.

## Viseme lookup buckets can be empty

- `VisemeManager` builds a per-letter reference table over the viseme combination table.
- Some letters legitimately have zero entries. Any code that computes `start + count - 1` for a bucket must guard `count == 0` first.

## Validation surfaced non-renderer blockers

- Strengthening the bgfx port enough to survive longer gameplay paths will often expose older engine bugs that the initial bootstrap renderer never reached. In this session the additional blockers were lifetime hazards in logical audio scene removal, script-side buffer sizing/array bounds issues, and a few gameplay systems that assumed data was always present.
- `LogicalListenerClass::Remove_From_Scene` and `LogicalSoundClass::Remove_From_Scene` can trigger scene-side removal paths that drop the final reference. Hold a temporary ref across the callback into `SoundSceneClass` so teardown cannot delete the object mid-function.
- `SoundSceneClass::Collect_Logical_Sounds` should advance the iterator before removing a single-shot logical sound from the scene. Removing the current node first invalidates the iterator's view of the list.
- `CombatSound::Type` cannot stay constrained to the fixed `CombatSoundType` enum when logical sounds also use authored script IDs. Store the raw integer type mask/ID instead of truncating it back into the enum.
- `Test_Cinematic::Command_Set_Primary` formatted `MyID` into a 10-byte stack buffer, which is too small for the full signed 32-bit integer range plus the NUL terminator. Use a bounded formatter with a larger buffer for object/callback ID strings in the scripts code.

# Porting Knowledge

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

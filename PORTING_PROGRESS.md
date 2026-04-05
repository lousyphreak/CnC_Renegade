# Porting Progress

## Recent changes

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

## Remaining work

- Continue smoke and gameplay-path validation to catch additional renderer or lifetime issues that only appear after deeper menu/game interaction.

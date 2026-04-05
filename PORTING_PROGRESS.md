# Porting Progress

## Recent changes

- Fixed an AddressSanitizer-detected heap-buffer-overflow in `Code/Commando/datasafe.cpp`.
- Root cause: `GenericDataSafeClass::Swap_Entries` used `long` in the `size == 4` fast path. On LP64 platforms like Linux, `long` is 8 bytes, so the swap read and wrote past 4-byte payloads such as `int` and `float`.
- Resolution: replaced the `long`-based swap with a `uint32`-based swap so the optimized path remains 4 bytes on all supported platforms.
- Fixed an AddressSanitizer-detected heap-use-after-free in `Code/ww3d2/texture_bgfx.cpp` reached during executable smoke validation.
- Root cause: `DX8Wrapper` cached raw `TextureClass*` bindings in bgfx render state without retaining a reference, allowing UI textures to be destroyed while still bound for later draw submission.
- Resolution: `DX8Wrapper::Set_Texture` now retains/release-refcounts the currently bound textures, and draw-state shutdown/reset paths release those retained references.
- Validation: rebuilt with `cmake --build build -j32` and ran `./build/bin/Renegade` under `/usr/bin/timeout 35s`; the executable stayed up for the full smoke window with no AddressSanitizer errors.

## Remaining work

- Continue smoke and gameplay-path validation to catch additional renderer or lifetime issues that only appear after deeper menu/game interaction.

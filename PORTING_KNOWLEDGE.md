# Porting Knowledge

## DataSafe LP64 hazard

- The original datasafe code assumes `long` is 32 bits in `GenericDataSafeClass::Swap_Entries`.
- That assumption is valid on Win32 but not on modern Linux/x86_64, where `long` is 64 bits.
- Any optimized swap path keyed on `size == 4` must use an explicitly 32-bit type such as `uint32`, not `long`, or it will overflow 4-byte payloads.
- This issue can surface during level loading and gameplay because datasafe shuffling touches entries from multiple threads and is exercised by common safe-wrapped scalar types.

## Renderer texture binding lifetime

- The bgfx-backed `DX8Wrapper` keeps legacy texture bindings in process-local render state between draw calls, similar to the original D3D device state model.
- Those cached `TextureClass*` bindings must participate in refcounting. Storing raw pointers is unsafe because UI and sentence rendering can destroy textures while the wrapper still plans to submit draws that read the current binding.
- When porting more D3D state to bgfx/SDL paths, treat cached resource bindings as owners until they are replaced or the draw state is reset/shutdown.

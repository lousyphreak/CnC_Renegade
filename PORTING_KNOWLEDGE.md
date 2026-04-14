# Porting Knowledge

## Shadow mapping

- In the bgfx renderer, fixed-function draws only auto-receive CSM when the shader destination blend is `DSTBLEND_ZERO`.
- The tank-track slope regression was caused by fixed-function mesh passes and sorted draws dropping explicit `receive_shadows` / `cast_shadows` intent before bgfx submission and falling back to that opaque-only default.
- Sorted buffered draws need the same explicit shadow-flag propagation as immediate draws because the later flush path otherwise reclassifies them with the default heuristic and can repaint already-shadowed terrain as lit.
- bgfx shader custom commands in the current CMake setup do not track included `.sh` files as dependencies, so edits under `Code/ww3d2/shaders/*.sh` require touching the dependent `.sc` files or otherwise forcing a shader rebuild.
- In the CSM light view used by `shadowmap.cpp`, casters that sit toward the sun/light-camera side of the receiver slice produce **larger** light-space Z values, so behind-camera caster coverage must expand `max_z` (reducing `z_near`) rather than only pushing `min_z` farther away. Extending the wrong side looks like a gradual receiver cutoff instead of a clean caster pop because the shadow gets clipped by the cascade near plane as the camera moves past the caster.
- The old projected-shadow runtime lived under `wwphys` as a separate system from bgfx shadow maps: `DynamicShadowManagerClass` generated per-object render-to-texture projectors, and static anim shadow projectors generated cached textures that projected onto dynamic units. Removing those paths requires clearing both the object-level managers and the `PhysicsSceneClass` static-shadow generation/config glue.
- `Shadow_Mode` still exists for compatibility with saved settings and UI code, but the runtime should treat every non-zero legacy value as the modern shadow-map mode. This prevents old blob/projection settings from re-enabling deleted shadow projector code while keeping registry migration simple.

## Texture loading

- `BitmapHandlerClass::Copy_Image` has a special bumpmap conversion path for `WW3D_FORMAT_U8V8`, `WW3D_FORMAT_L6V5U5`, and `WW3D_FORMAT_X8L8V8U8` that derives gradients from neighboring source texels. That path must clamp both horizontal and vertical neighbor lookups at the image edges; raw `src_ptr +/- src_bpp` arithmetic on the first or last column will underflow/overflow 24-bit TGA image buffers during level texture loads.
- The crash signature for this bug is an ASan heap-buffer-overflow in `BitmapHandlerClass::Read_B8G8R8A8` while loading an uncompressed mipmap from `TextureLoadTaskClass::Load_Uncompressed_Mipmap`, usually with the invalid address a few bytes before the `Targa::Load` allocation.

## First-person weapon animation lookup

- `StringClass` must be explicitly cast to `const char *` before being passed through `%s` varargs formatters like `StringClass::Format` or `Debug_Say`. C++ will apply implicit conversions for normal function parameters, but not for `...`, and the resulting corrupted strings can look like random asset names instead of valid HAnim identifiers.
- `Code/Combat/weaponview.cpp` builds first-person weapon, hand, and bob animation names via formatted strings. If those `StringClass` values are passed to `%s` without casts, idle/default visuals may still appear while state-driven first-person animations such as reload/fire/enter/exit fail to resolve.

# Porting Knowledge

## Shadow mapping

- In the bgfx renderer, fixed-function draws only auto-receive CSM when the shader destination blend is `DSTBLEND_ZERO`.
- The tank-track slope regression was caused by fixed-function mesh passes and sorted draws dropping explicit `receive_shadows` / `cast_shadows` intent before bgfx submission and falling back to that opaque-only default.
- Sorted buffered draws need the same explicit shadow-flag propagation as immediate draws because the later flush path otherwise reclassifies them with the default heuristic and can repaint already-shadowed terrain as lit.
- bgfx shader custom commands in the current CMake setup do not track included `.sh` files as dependencies, so edits under `Code/ww3d2/shaders/*.sh` require touching the dependent `.sc` files or otherwise forcing a shader rebuild.
- In the CSM light view used by `shadowmap.cpp`, casters that sit toward the sun/light-camera side of the receiver slice produce **larger** light-space Z values, so behind-camera caster coverage must expand `max_z` (reducing `z_near`) rather than only pushing `min_z` farther away. Extending the wrong side looks like a gradual receiver cutoff instead of a clean caster pop because the shadow gets clipped by the cascade near plane as the camera moves past the caster.

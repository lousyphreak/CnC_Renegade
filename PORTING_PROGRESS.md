# Porting Progress

## Renderer

- Fixed the tank-track slope shadow regression by preserving explicit `receive_shadows` / `cast_shadows` intent through the fixed-function mesh path and the sorted renderer flush path instead of falling back to bgfx's default opaque-only heuristic.
- Removed the earlier material-pass/projector/decal shadow-receive experiments once the mesh/sorted submission fix proved to be the real cause.
- Fixed CSM light-space depth padding so off-camera casters are kept on the light-facing side of each cascade instead of extending only the far side, which had been cutting tree/building shadows as those casters moved behind the player camera.

## Combat / first-person presentation

- Fixed first-person weapon and hand animation lookup in `Code/Combat/weaponview.cpp`. The port was building HAnim names with `StringClass` objects passed directly through `%s` varargs formatting, which corrupted the generated names and prevented non-idle first-person weapon states such as reload/fire/enter/exit from resolving correctly. The fix casts the `StringClass` values to `const char *` before formatting the lookup strings and the related debug output.

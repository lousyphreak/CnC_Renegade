# Porting Progress

## Renderer

- Fixed the tank-track slope shadow regression by preserving explicit `receive_shadows` / `cast_shadows` intent through the fixed-function mesh path and the sorted renderer flush path instead of falling back to bgfx's default opaque-only heuristic.
- Removed the earlier material-pass/projector/decal shadow-receive experiments once the mesh/sorted submission fix proved to be the real cause.
- Fixed CSM light-space depth padding so off-camera casters are kept on the light-facing side of each cascade instead of extending only the far side, which had been cutting tree/building shadows as those casters moved behind the player camera.

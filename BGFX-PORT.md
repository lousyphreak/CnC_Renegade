# BGFX port

We want to port from d3d to bgfx, to have a modern renderer that supports multiple platforms, while still being able to run on older hardware that may not support the latest graphics APIs.

There are remnants or an earlier attempt, but you need to **IGNORE** that and start fresh.

## Rules

- stay as close to the d3d original as possible, we want to preserve the original rendering behavior and features as much as possible, while still using bgfx to achieve that.
- make sure to not do a lot of CPU work, we are porting to modern render APIs so use the GPU where feasible, for example for things like skinning, animation, and other things that can be done on the GPU, we should do them on the GPU instead of doing them on the CPU and then sending the results to the GPU.
- no gpu readback, except for screenshots, we want to avoid any gpu readback as much as possible, because it can cause performance issues and stuttering, so if we need to read back data from the GPU, we should try to find a way to do it without causing performance issues, for example by using async readback or by using a staging buffer and reading back the data in a separate thread.
- keep the original code structure and organization as much as possible, do not move files around or change the directory structure, just port the code in place, we want to preserve the original code structure and organization as much as possible, to make it easier to compare the original code with the ported code and to make it easier to track changes and progress.
- keep a state document (e.g. `PORTING_PROGRESS.md`) to track the progress of the port, and update it regularly with detailed notes on what has been done, what is left to do, and any issues or challenges encountered along the way, we want to have a clear and detailed record of the porting process, to make it easier to track progress and to identify any issues or challenges that may arise during the porting process.
- no shims, no stubs, no wrappers - we want to have a complete and functional port as soon as possible, even if it's not perfect or optimized, we want to have a working port as soon as possible, and then we can improve it later, but we don't want to have any shims or stubs that are not fully functional, because that can cause confusion and can make it harder to track progress and to identify any issues or challenges that may arise during the porting process.

**it is better to have compile errors, than disabled code so we find the missing pieces faster**

## Current clean-port milestones

- `BgfxRenderer` now owns bgfx startup, frame begin/end, clear, viewport, and camera submission.
- `ww3d2` now has build-integrated bgfx shader compilation for renderer-owned shader assets.
- The first shared bgfx shader set (`vs_color_tex.sc` / `fs_color_tex.sc`) and varying definition live in `Code/ww3d2/shaders/`.
- `BgfxRenderer` now also owns:
  - a canonical position/color/texcoord vertex layout
  - runtime shader/program loading from build-generated shader binaries
  - a white fallback texture and sampler uniform
  - a dedicated overlay view for screen-space submission
  - surface-to-bgfx texture upload for renderer-native texture binding
  - `ShaderClass` to bgfx render-state translation for API state that belongs in bgfx state bits
- `TextureClass` now has a native bgfx texture path for renderer-driven binding instead of requiring `DX8Wrapper` state submission.
- `Render2D` now submits directly to bgfx using renderer-owned programs, state, and buffers rather than the DX8 dynamic buffer path.

## Immediate next slice

- Expand the same native bgfx submission approach from `Render2D` into the next real material/mesh path, starting with rigid meshes and shared texture ownership cleanup.
- Replace the remaining texture-loader and surface-management DX8 dependencies so bgfx textures are created at source rather than lazily from the legacy backend objects.

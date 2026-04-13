# BGFX port

We want to port from d3d to bgfx, to have a modern renderer that supports multiple platforms, while still being able to run on older hardware that may not support the latest graphics APIs.

There are remnants or an earlier attempt, but you need to **IGNORE** that and start fresh.

## Current shader architecture

The renderer uses two shader programs:

### Overlay shader (`vs_overlay` / `fs_overlay`)
- Used for 2D/UI rendering (`Render2DClass::Render`)
- Trivial: MVP passthrough + vertex color × optional texture
- Single uniform: `u_overlayConfig.x` (1.0 = has texture, 0.0 = vertex color only)
- Compact vertex layout: pos(3f) + color0(4u8n) + uv0(2f) = 20 bytes

### Mesh shader (`vs_mesh` / `fs_mesh`)
- Used for all 3D rendering (rigid mesh, skinned mesh, particles, projectors, decals, etc.)
- Vertex shader: MVP transform, optional directional lighting (ambient + 4 lights), linear vertex fog, texgen (4 modes), texture transforms
- Fragment shader: enum-based stage0/stage1 color+alpha ops via `StageColorOp` enum (9 values), alpha test, fog application (3 modes), specular add
- ~16 vec4 uniforms total for the most complex case

### StageColorOp enum
Maps directly from `ShaderClass` gradient/detail enums, bypassing the DX8Wrapper D3D8 state cache. Values:
- 0=DISABLE, 1=MODULATE, 2=SELECT_TEXTURE, 3=SELECT_CURRENT, 4=ADD, 5=ADDSMOOTH, 6=SUBTRACT, 7=BLEND_TEX_ALPHA, 8=BLEND_CUR_ALPHA

## Rules

- stay as close to the d3d original as possible, we want to preserve the original rendering behavior and features as much as possible, while still using bgfx to achieve that.
- make sure to not do a lot of CPU work, we are porting to modern render APIs so use the GPU where feasible, for example for things like skinning, animation, and other things that can be done on the GPU, we should do them on the GPU instead of doing them on the CPU and then sending the results to the GPU.
- no gpu readback, except for screenshots, we want to avoid any gpu readback as much as possible, because it can cause performance issues and stuttering, so if we need to read back data from the GPU, we should try to find a way to do it without causing performance issues, for example by using async readback or by using a staging buffer and reading back the data in a separate thread.
- keep the original code structure and organization as much as possible, do not move files around or change the directory structure, just port the code in place, we want to preserve the original code structure and organization as much as possible, to make it easier to compare the original code with the ported code and to make it easier to track changes and progress.
- keep a state document (e.g. `PORTING_PROGRESS.md`) to track the progress of the port, and update it regularly with detailed notes on what has been done, what is left to do, and any issues or challenges encountered along the way, we want to have a clear and detailed record of the porting process, to make it easier to track progress and to identify any issues or challenges that may arise during the porting process.
- no shims, no stubs, no wrappers - we want to have a complete and functional port as soon as possible, even if it's not perfect or optimized, we want to have a working port as soon as possible, and then we can improve it later, but we don't want to have any shims or stubs that are not fully functional, because that can cause confusion and can make it harder to track progress and to identify any issues or challenges that may arise during the porting process.

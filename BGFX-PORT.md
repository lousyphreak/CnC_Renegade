# BGFX port

We want to port from d3d to bgfx, to have a modern renderer that supports multiple platforms, while still being able to run on older hardware that may not support the latest graphics APIs.

There are remnants or an earlier attempt, but you need to **IGNORE** that and start fresh.

## Current shader architecture

The renderer uses six shader programs:

### Overlay shader (`vs_overlay` / `fs_overlay`)
- Used for 2D/UI rendering (`Render2DClass::Render`)
- Trivial: MVP passthrough + vertex color × optional texture
- Single uniform: `u_overlayConfig.x` (1.0 = has texture, 0.0 = vertex color only)
- Compact vertex layout: pos(3f) + color0(4u8n) + uv0(2f) = 20 bytes

### Mesh shader variants (multi-shader architecture)
The old monolithic `vs_mesh`/`fs_mesh` uber-shader has been replaced by 4 purpose-built programs. Program selection happens at draw time based on lighting and texgen state.

#### `mesh_unlit` (`vs_mesh_unlit` / `fs_mesh_unlit`)
- Used for prelit meshes, particles, terrain, vertex-colored geometry
- Vertex shader: MVP transform + linear vertex fog
- Fragment shader: stage0/stage1 color+alpha ops, alpha test, fog
- Uniforms: ~4 vec4s (u_meshFogConfig, u_meshFogColor, u_meshFragConfig, u_meshFragConfig2)

#### `mesh_lit` (`vs_mesh_lit` / `fs_mesh_lit`)
- Used for lit meshes with standard UV mapping
- Vertex shader: MVP transform + directional lighting (4 lights) + material color resolution + linear vertex fog
- Fragment shader: same as mesh_unlit
- Uniforms: ~14 vec4s (fog + frag config + u_meshLitConfig + material colors + scene ambient + lights)

#### `mesh_unlit_texgen` (`vs_mesh_unlit_texgen` / `fs_mesh_unlit_texgen`)
- Used for unlit meshes with texgen (screen mappers, projectors)
- Vertex shader: MVP transform + texgen (4 modes) + texture transforms + fog
- Fragment shader: same as mesh_unlit
- Uniforms: ~10 vec4s (fog + frag + texgen mode + tex transform flags + 2 tex transform matrices)

#### `mesh_lit_texgen` (`vs_mesh_lit_texgen` / `fs_mesh_lit_texgen`)
- Used for lit meshes with texgen (env maps, reflections)
- Vertex shader: full feature set — lighting + texgen + fog
- Fragment shader: same as mesh_unlit
- Uniforms: ~20 vec4s (all uniform groups combined)

### Legacy mesh shader (`vs_mesh` / `fs_mesh`)
- Kept during transition, still compiled but no longer submitted
- Will be removed once the new shaders are validated in all edge cases

### Program selection logic
At draw time, `BgfxRenderer::Select_Mesh_Program()` picks the cheapest program that covers the current state:
1. Check `D3DRS_LIGHTING` → lit vs unlit
2. Check texcoord index and tex transform flags → texgen vs not
3. Return the appropriate `MeshShaderProgram` enum value

### Shared fragment code (`mesh_common.sh`)
All fragment shaders include `mesh_common.sh` which defines:
- `ApplyColorOp()` / `ApplyAlphaOp()` — stage color/alpha blending operations
- `ApplyFog()` — fragment fog application (3 modes: enable, scale_fragment, white)
- Stage operation constants matching `StageColorOp` C++ enum

### StageColorOp enum
Maps directly from `ShaderClass` gradient/detail enums, bypassing the DX8Wrapper D3D8 state cache. Values:
- 0=DISABLE, 1=MODULATE, 2=SELECT_TEXTURE, 3=SELECT_CURRENT, 4=ADD, 5=ADDSMOOTH, 6=SUBTRACT, 7=BLEND_TEX_ALPHA, 8=BLEND_CUR_ALPHA

### Dead D3D8 features confirmed unused
The following features were audited and confirmed unused by any game code path:
- BUMPENVMAP / BUMPENVMAPLUMINANCE / DOTPRODUCT3
- EXP / EXP2 fog (only LINEAR used)
- Point/spot light shader calculations (LightEnvironmentClass pre-converts)
- D3DMCS_COLOR2 material source
- D3DTA_TFACTOR / D3DTA_SPECULAR as texture stage arguments
- Specular power lighting
- Stencil operations

## Rules

- stay as close to the d3d original as possible, we want to preserve the original rendering behavior and features as much as possible, while still using bgfx to achieve that.
- make sure to not do a lot of CPU work, we are porting to modern render APIs so use the GPU where feasible, for example for things like skinning, animation, and other things that can be done on the GPU, we should do them on the GPU instead of doing them on the CPU and then sending the results to the GPU.
- no gpu readback, except for screenshots, we want to avoid any gpu readback as much as possible, because it can cause performance issues and stuttering, so if we need to read back data from the GPU, we should try to find a way to do it without causing performance issues, for example by using async readback or by using a staging buffer and reading back the data in a separate thread.
- keep the original code structure and organization as much as possible, do not move files around or change the directory structure, just port the code in place, we want to preserve the original code structure and organization as much as possible, to make it easier to compare the original code with the ported code and to make it easier to track changes and progress.
- keep a state document (e.g. `PORTING_PROGRESS.md`) to track the progress of the port, and update it regularly with detailed notes on what has been done, what is left to do, and any issues or challenges encountered along the way, we want to have a clear and detailed record of the porting process, to make it easier to track progress and to identify any issues or challenges that may arise during the porting process.
- no shims, no stubs, no wrappers - we want to have a complete and functional port as soon as possible, even if it's not perfect or optimized, we want to have a working port as soon as possible, and then we can improve it later, but we don't want to have any shims or stubs that are not fully functional, because that can cause confusion and can make it harder to track progress and to identify any issues or challenges that may arise during the porting process.

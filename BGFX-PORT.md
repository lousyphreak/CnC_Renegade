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

### Shadow shader (`vs_shadow` / `fs_shadow`)
- Used for the shadow depth pass (rendering to shadow atlas)
- Vertex shader: MVP transform only (position output for depth)
- Fragment shader: empty (depth written implicitly by rasterizer)
- Uses `varying_shadow.def.sc` (position + texcoord0 for alpha-test if needed)

### Shadow map sampling (`shadow_common.sh`)
Included by all 4 mesh fragment shaders. Provides:
- `SelectCascade(viewDepth)` — picks cascade 0–2 based on view-space depth
- `WorldToShadowUV(worldPos, cascade)` — transforms world position to shadow atlas UV + depth
- `SampleShadowPCF(shadowUV, bias)` — 3×3 PCF kernel with texel-sized offsets
- `ComputeShadow(worldPos, viewDepth)` — full shadow factor computation (1.0 = lit, configurable darkening)
- Uniforms: `u_shadowLightViewProj[3]` (mat4 per cascade), `u_shadowCascadeSplits` (vec4), `u_shadowConfig` (vec4), `s_shadowMap` (sampler slot 2)

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

## Shadow mapping (CSM)

Replaced the legacy per-object projected texture shadow system with scene-wide Cascaded Shadow Maps (CSM) with PCF filtering.

### Architecture
- **3 cascades** in a single depth atlas (6144×2048, each cascade 2048×2048 side-by-side)
- **bgfx view IDs 0–2** reserved for shadow cascades (render BEFORE main scene which starts at ID 3)
- Shadow depth rendered using `vs_shadow`/`fs_shadow` with `BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW`
- Shadow sampling via `shadow_common.sh` included in all 4 mesh fragment shaders

### Cascade computation (ShadowMapManager)
- Practical split scheme: λ=0.75 blend of logarithmic and uniform splits
- Sphere-based ortho bounds: frustum bounding sphere radius used for X/Y extents (rotation-invariant)
- Texel grid snapping: sphere center projected to light-space XY, snapped to texel increments
- Z bounds: tight AABB of frustum corners in light space, with 2× margin for behind-camera casters
- Sun direction from `PhysicsSceneClass::Get_Sun_Light_Vector()` (light direction, points toward ground)

### Shadow pass integration
- No separate render pass — shadow draws submitted alongside normal mesh draws
- In `Submit_Cached_Fixed_Function_Draw` (bgfxfixedfunction.cpp), after normal `bgfx::submit()`, `ShadowMapManager::Submit_Shadow_Draws()` re-binds VB/IB and submits to all 3 cascade views
- Alpha-blended geometry (DstBlend != ZERO) is excluded from shadow casting
- Receiver selection cannot rely on that same cast rule: some blended passes are still part of the final opaque world surface
- Current fixed-function draws therefore need explicit receiver flags for terrain/world composition passes instead of inferring “receiver” from `DstBlend == ZERO`
- `ShadowMapManager::Bind_Shadow_Uniforms()` binds the shadow atlas texture + uniforms before each mesh draw
- Because the shadow path piggybacks on normal draw submission, `ShadowMapManager` must expose per-cascade world-space cull boxes and `PhysicsSceneClass` must inject any matching shadow casters into the visible static/dynamic/world-space lists before LOD/projector setup. Otherwise caster submission becomes main-camera dependent and shadows pop/swim as the camera rotates.
- The rigid mesh render path also has to treat “intersects a cascade cull box” as a valid registration condition; camera-frustum overlap alone is not sufficient for shadow correctness.

### Key design decisions
- Raw depth sampling (SAMPLER2D) with manual comparison — no hardware comparison (BGFX_SAMPLER_COMPARE_LESS)
- GLSL depth remapping: `depth = depth * 0.5 + 0.5` for OpenGL clip-space convention
- Default shadow distance: 200 units, intensity: 0.6, depth bias: 0.003
- Depth format: D16 preferred (sufficient precision, widely supported for sampling), falls back to D24/D32/D32F/D24S8
- `PRELIT_LIGHTMAP_MULTI_PASS` level geometry should receive the shadow term on its final lightmap/composite pass, not always on pass 0, or it will visibly diverge from single-pass units/meshes
- Terrain alpha layers should also receive the shadow term even though they blend over the base layer; those passes still contribute to one opaque terrain surface

## Rules

- stay as close to the d3d original as possible, we want to preserve the original rendering behavior and features as much as possible, while still using bgfx to achieve that.
- make sure to not do a lot of CPU work, we are porting to modern render APIs so use the GPU where feasible, for example for things like skinning, animation, and other things that can be done on the GPU, we should do them on the GPU instead of doing them on the CPU and then sending the results to the GPU.
- no gpu readback, except for screenshots, we want to avoid any gpu readback as much as possible, because it can cause performance issues and stuttering, so if we need to read back data from the GPU, we should try to find a way to do it without causing performance issues, for example by using async readback or by using a staging buffer and reading back the data in a separate thread.
- keep the original code structure and organization as much as possible, do not move files around or change the directory structure, just port the code in place, we want to preserve the original code structure and organization as much as possible, to make it easier to compare the original code with the ported code and to make it easier to track changes and progress.
- keep a state document (e.g. `PORTING_PROGRESS.md`) to track the progress of the port, and update it regularly with detailed notes on what has been done, what is left to do, and any issues or challenges encountered along the way, we want to have a clear and detailed record of the porting process, to make it easier to track progress and to identify any issues or challenges that may arise during the porting process.
- no shims, no stubs, no wrappers - we want to have a complete and functional port as soon as possible, even if it's not perfect or optimized, we want to have a working port as soon as possible, and then we can improve it later, but we don't want to have any shims or stubs that are not fully functional, because that can cause confusion and can make it harder to track progress and to identify any issues or challenges that may arise during the porting process.

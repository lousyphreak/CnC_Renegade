# bgfx renderer parity audit vs. the original WW3D DX8 renderer

## Scope

This document audits the **actual bgfx build** in this repository against the **original WW3D/DX8 renderer feature set that Renegade itself uses**.

It is based on:

- the bgfx build wiring in `Code/ww3d2/CMakeLists.txt`
- the current bgfx backend in `Code/ww3d2/dx8wrapper.cpp`, `texture_bgfx.cpp`, and `surfaceclass_bgfx.cpp`
- the bgfx-linked compatibility replacements such as `dx8renderer_headless.cpp`, `sortingrenderer_headless.cpp`, `shader_headless.cpp`, and `vertmaterial_headless.cpp`
- the original/full implementations that still exist beside them, especially `pointgr.cpp`, `linegrp.cpp`, `seglinerenderer.cpp`, `textureloader.cpp`, and `texturethumbnail.cpp`

This is a **source audit**. It is strong enough to identify confirmed parity breaks and confirmed preserved systems, but any row marked **Partial** still deserves runtime validation.

### Status legend

- **Implemented**: the bgfx build contains a real code path and the original feature is mostly preserved.
- **Partial**: the feature exists, but semantics are reduced, limited, or depend on code paths that are not fully ported.
- **Missing**: the feature is stubbed, a no-op, or not wired in the bgfx build.

## Executive summary

The bgfx renderer is **not** a full parity port of the original DX8 renderer yet. It is a working compatibility layer with real mesh/sorting/material infrastructure, but it still has several major renderer-feature gaps.

### What is clearly present

- bgfx device/bootstrap, scene begin/end, clear, viewport, resize, fullscreen/windowed toggle, and swap interval
- the core mesh renderer and sorting renderer logic through `dx8renderer_headless.cpp` and `sortingrenderer_headless.cpp`
- CPU-side fixed-function-style lighting, fog state, blend/depth/cull state mapping, and stage-0 texture coordinate generation
- CPU-backed surfaces and textures, including basic DDS/TGA loading and texture-backed render targets
- 2D/UI text paths through `render2dsentence.cpp`, `font3d.cpp`, `surfaceclass_bgfx.cpp`, and `texture_bgfx.cpp`

### Largest confirmed parity breaks

1. **Point particles, line groups, and segmented lines are currently missing in the bgfx build.**
   The build links `particle_renderers_headless.cpp`, and its `PointGroupClass::Render`, `LineGroupClass::Render`, and `SegLineRendererClass::Render` bodies are empty, while the original full implementations still exist in `pointgr.cpp`, `linegrp.cpp`, and `seglinerenderer.cpp`.

2. **Stage-1 / second-texture material support is not actually submitted by the bgfx backend.**
   The high-level material system still configures two stages, but `Submit_Triangles()` only reads stage-0 texture state and only binds `g_bgfx.textures[0]` to `s_texColor`.

3. **The original thumbnail/database/streaming texture path is gone.**
   `texturethumbnail_headless.cpp` is a real stub, the original threaded foreground/background loader behavior is not preserved, and the bgfx texture path collapses textures to one uploaded level.

4. **Public capture/debug APIs are not fully ported.**
   `WW3D::Make_Screen_Shot()` explicitly logs that it is not implemented for the bgfx bootstrap; movie capture remains legacy Windows-only code that depends on old DX8 front-buffer behavior.

5. **Caps/stat reporting is placeholder-level.**
   `DX8Caps` in `dx8wrapper.h` reports optimistic values such as `Support_Texture_Format() == true`, `Support_DXTC() == true`, `Support_ZBias() == true`, while multiple corresponding bgfx features are reduced or absent; the DX8Wrapper perf counters all return `0`.

## Build wiring: what the bgfx build really links

The single most important fact in this audit is that the bgfx build is **not** simply "all real renderer files plus bgfx wrapper." `Code/ww3d2/CMakeLists.txt` keeps many `*_headless.cpp` files in the build, and those filenames are misleading.

### Real bgfx-linked implementations despite the `headless` name

- `dx8renderer_headless.cpp`
- `sortingrenderer_headless.cpp`
- `shader_headless.cpp`
- `vertmaterial_headless.cpp`
- `assetmgr_headless.cpp`
- `textureloader_headless.cpp`

These are not all empty stubs. Several are substantial compatibility replacements.

### True behavior-loss files that the bgfx build still links

- `particle_renderers_headless.cpp`
- `texturethumbnail_headless.cpp`

These are real parity gaps, not naming accidents.

### Files the bgfx build swaps in

When `RENEGADE_WITH_BGFX_RENDERER` is enabled, `CMakeLists.txt` removes:

- `render2dsentence_headless.cpp`
- `surfaceclass_headless.cpp`
- `texture_headless.cpp`
- `ww3d_headless.cpp`

and adds:

- `dx8texman_bgfx.cpp`
- `render2dsentence.cpp`
- `surfaceclass_bgfx.cpp`
- `texture_bgfx.cpp`
- `ww3d.cpp`
- `dx8wrapper.cpp`

That means the bgfx build has a mix of:

- real bgfx backend files
- real compatibility replacements
- and a few still-stubbed subsystems

## Architectural findings by subsystem

### 1. Core renderer/bootstrap

The bgfx backend is centered on `Code/ww3d2/dx8wrapper.cpp`.

Important properties of the current backend:

- It builds a **single bootstrap bgfx program** from `shaders/vs_bootstrap.sc` and `shaders/fs_bootstrap.sc`.
- Submission uses CPU-built transient buffers in `Submit_Triangles()`.
- Only one texture sampler is bound during submission: `s_texColor`.
- CPU-side code computes per-vertex lighting and fog inputs before submission.

This means the backend is closer to a **fixed-function emulation shim** than a full replacement for the original DX8 renderer's material pipeline.

### 2. Mesh renderer and sorting renderer

This area is stronger than the filenames imply.

- `dx8renderer_headless.cpp` is a real implementation and still drives the main mesh render path.
- `sortingrenderer_headless.cpp` is also a real implementation, not a no-op.
- `mesh.cpp`, `meshmdl.cpp`, `matpass.cpp`, `mapper.cpp`, and related higher-level render code are still present and still feed the renderer.

So the bgfx renderer is **not** missing the entire mesh pipeline. The parity problems come from specific backend capabilities, not from the complete absence of mesh rendering.

### 3. Fixed-function emulation: what the bgfx draw path actually consumes

`Submit_Triangles()` in `dx8wrapper.cpp` proves what the bgfx shader path really uses:

- vertex position
- a single diffuse color
- a single UV set for stage 0
- a single fog scalar
- stage-0 texture transform / texcoord generation
- blend, cull, depth test/write, alpha-ref state
- gamma / brightness / contrast uniforms

What it does **not** consume:

- any stage-1 texture binding
- any stage-1 texcoord generation
- secondary gradient data
- detail-color/detail-alpha combiners
- bump-environment matrices
- any richer per-material pixel program behavior beyond the bootstrap shader

So the high-level material system still exists, but the actual bgfx submission path only honors a subset of it.

### 4. Texture coordinate generation

Stage 0 is better than it first looks.

`Generate_Stage0_Texture_Input()` in `dx8wrapper.cpp` supports:

- pass-through UVs
- camera-space position generation
- camera-space normal generation
- camera-space reflection vector generation
- texture transform application
- projected texture coordinates

That is enough to preserve some original stage-0 projected/reflection workflows.

However, the same is **not true for stage 1**. The state is stored for both stages, but the final draw path only uses `texture_stage_states[0]`, `texture_transforms[0]`, and `textures[0]`.

### 5. Lighting, fog, and gamma

These areas are partially preserved:

- CPU-side lighting is implemented through `Compute_Lit_Color()` using material color sources, ambient state, and either a light environment or explicit light list.
- Fog state is implemented through `Set_Fog()` plus shader fog application in `fs_bootstrap.sc`.
- The shader supports the engine's `ShaderClass` fog modes: `FOG_ENABLE`, `FOG_SCALE_FRAGMENT`, and `FOG_WHITE`.
- Gamma/brightness/contrast are applied in the fragment shader.

That said, the modern backend is still much simpler than the original renderer's full material path.

### 6. Z-bias / polygon offset is especially dangerous right now

This is a likely parity bug, not just a cosmetic TODO.

- `DX8Caps::Support_ZBias()` returns `true` in `dx8wrapper.h`.
- The legacy decal code in `decalmsh.cpp` uses a software geometric offset fallback **only when Z-bias is reported as unsupported**.
- But the bgfx wrapper does not appear to apply real Z-bias in submission:
  - `Build_BGFX_State()` does not use `D3DRS_ZBIAS`
  - `Set_Projection_Transform_With_Z_Bias()` ignores the z-bias parameters and just stores the matrix

So the bgfx renderer currently advertises Z-bias support in a way that can disable the original fallback without replacing it with a real offset implementation.

### 7. Texture backend and surfaces

`texture_bgfx.cpp` and `surfaceclass_bgfx.cpp` implement a CPU-backed compatibility layer.

What is preserved:

- CPU-side surface allocation, lock/unlock, clear, copy, stretch copy
- texture creation from dimensions
- texture creation from surfaces
- immediate file loading through the bgfx-compatible surface/texture path
- texture-backed render targets via a bgfx framebuffer/texture pair

What is reduced:

- non-render-target textures are uploaded as plain `RGBA8` textures
- `TextureClass::Get_Mip_Level_Count()` always returns `1`
- `TextureClass::Get_Surface_Level()` only synthesizes a CPU copy of the current bytes
- `TextureClass::Get_D3D_Surface_Level()` returns `NULL`

So the backend preserves the API shape but not the original multi-level D3D texture semantics.

### 8. Texture loading, thumbnails, and streaming

This is one of the largest regressions from the original renderer.

Original renderer behavior from `textureloader.cpp` and `texturethumbnail.cpp`:

- threaded background loader
- foreground/background task queues
- thumbnail database and thumbnail hash lookups
- deferred texture promotion from thumbnail to full texture
- explicit mip-chain locking and copying
- DDS compressed texture loading with per-level copies

Current bgfx behavior:

- `texturethumbnail_headless.cpp` stubs out the entire thumbnail manager
- `textureloader_headless.cpp` makes background loading synchronous (`Finish_Load()` immediately)
- `Flush_Pending_Load_Tasks()` is explicitly a no-op because bgfx loading is synchronous
- compressed DDS files are decoded to RGBA8 instead of preserved in compressed residency
- both compressed and uncompressed load paths end with `MipLevelCount = 1`

This means the old renderer's texture streaming/thumbnail/loading behavior is not just incomplete; it has effectively been replaced by a simpler synchronous loader with single-level textures.

### 9. Render targets, projectors, and shadows

This area is **partially** ported.

What exists:

- `DX8Wrapper::Create_Render_Target()` returns a real `TextureClass` render target
- `Set_Render_Target(TextureClass *)` is implemented
- `TexProjectClass::Compute_Texture()` still uses render-to-texture flow
- `TexProjectClass::Needs_Render_Target()` and `Set_Render_Target(TextureClass *)` are real

What is reduced or missing:

- `Set_Render_Target(IDirect3DSurface8 *surface)` only handles `nullptr`; non-null surfaces are ignored
- render targets are limited to texture-backed 2D framebuffers
- many original projector paths still configure `Mapper1` on stage 1, but stage 1 is not actually submitted by the bgfx backend

So basic texture-backed RTT exists, but **full original projector/shadow semantics are not there yet**.

### 10. Decals

Decal mesh generation and rendering code still exists, but it sits on top of the Z-bias problem described above.

That leaves decals in a **Partial** state:

- the geometry/material path is still there
- but exact depth-offset behavior is not trustworthy yet
- and capability reporting currently pushes code down the "hardware Z-bias exists" branch

### 11. Dazzle / halo / lens flare

`dazzle.cpp` is still a real implementation and uses stage-0 textured additive quads, which makes it a better fit for the current bgfx backend than the missing particle/segline systems.

Current assessment:

- the dazzle system is **not obviously stubbed**
- it uses the normal `DX8Wrapper` state path and stage-0 textures
- but it still needs runtime validation before being called full parity

So dazzle/halo/lens flare should be treated as **Partial**, not Missing.

### 12. Particle points, line groups, and segmented lines

This is the clearest confirmed feature hole in the renderer.

The bgfx build links `particle_renderers_headless.cpp`, where:

- `PointGroupClass::Render(...) {}` is empty
- `LineGroupClass::Render(...) {}` is empty
- `SegLineRendererClass::Render(...) {}` is empty

The original working implementations still exist in:

- `pointgr.cpp`
- `linegrp.cpp`
- `seglinerenderer.cpp`

Higher-level systems still depend on them:

- `part_buf.cpp`
- `segline.cpp`
- other effects/render-objects that delegate into those renderers

So point particles, line groups, and segmented-line effects should all be considered **Missing in the bgfx build today**.

### 13. 2D, sentence rendering, and font rendering

This area looks substantially healthier than the particle path.

- the bgfx build uses the real `render2dsentence.cpp`, not the headless stub
- `render2dsentence.cpp` builds `SurfaceClass` objects and turns them into `TextureClass(..., MIP_LEVELS_1)` textures
- `font3d.cpp` also uses the bgfx `SurfaceClass` path
- `surfaceclass_bgfx.cpp` provides the needed CPU copy/lock/format conversion support

This suggests the 2D text/font pipeline is **implemented**, though still limited by the simplified texture backend.

### 14. Screenshot and movie capture

These are not fully ported.

- `WW3D::Make_Screen_Shot()` in `ww3d.cpp` explicitly logs: `WW3D::Make_Screen_Shot is not implemented for the bgfx bootstrap yet`
- movie capture is wrapped in `_WINDOWS` guards and still depends on old DX8 front-buffer behavior from `WW3D::Update_Movie_Capture()`
- the bgfx wrapper no longer exposes the old DX8 front-buffer helper that this code expects

So both screenshot/capture features should be treated as **Missing** for bgfx parity.

### 15. Caps, device reporting, and performance counters

This is still shim-level, not parity-level.

`DX8Caps` in `dx8wrapper.h` hardcodes answers such as:

- `Support_Render_To_Texture_Format() == true`
- `Support_Texture_Format() == true`
- `Support_DXTC() == true`
- `Support_Gamma() == true`
- `Support_ZBias() == true`
- vendor/device ID `0`
- log string `"bgfx renderer"`

At the same time:

- multiple actual behaviors are reduced or absent
- the DX8Wrapper perf counters (`Get_Last_Frame_DX8_Calls()`, `Get_Last_Frame_Matrix_Changes()`, etc.) all return `0`

So the caps/stat layer should be treated as **placeholder reporting**, not a finished parity implementation.

## Exhaustive checklist

The table below lists every renderer-facing feature surface I could trace from the original WW3D DX8 path, grouped by subsystem.

| Subsystem | Original feature | bgfx status | Evidence | Notes |
| --- | --- | --- | --- | --- |
| Build wiring | Main mesh renderer path | Implemented | `Code/ww3d2/CMakeLists.txt`, `dx8renderer_headless.cpp` | Misleading filename; real implementation is compiled in bgfx build. |
| Build wiring | Sorting renderer | Implemented | `Code/ww3d2/CMakeLists.txt`, `sortingrenderer_headless.cpp` | Real implementation, not a no-op. |
| Build wiring | Shader/material state generation | Implemented | `shader_headless.cpp`, `vertmaterial_headless.cpp` | High-level material system is preserved. |
| Build wiring | Particle renderer backend selection | Missing | `CMakeLists.txt`, `particle_renderers_headless.cpp` | bgfx build links the no-op renderer file instead of `pointgr.cpp` / `linegrp.cpp` / `seglinerenderer.cpp`. |
| Build wiring | Thumbnail manager selection | Missing | `CMakeLists.txt`, `texturethumbnail_headless.cpp` | Entire manager is stubbed. |
| Device/frame | `DX8Wrapper::Init` / Shutdown | Implemented | `dx8wrapper.cpp` init/shutdown path | Real bgfx bootstrap and teardown exist. |
| Device/frame | Begin scene / end scene / present | Implemented | `dx8wrapper.cpp` | bgfx frame lifecycle is wired. |
| Device/frame | Clear color/depth | Implemented | `dx8wrapper.cpp:1203-1218` | Color/depth clear state is applied through bgfx. |
| Device/frame | Viewport changes | Implemented | `dx8wrapper.cpp:1221-1228` | View rect is updated against window or RT target. |
| Device/frame | Resolution changes | Implemented | `dx8wrapper.cpp:1278-1307` | Uses SDL window resize + backbuffer sync. |
| Device/frame | Fullscreen/windowed toggle | Implemented | `dx8wrapper.cpp:1345-1356` | SDL fullscreen toggle path exists. |
| Device/frame | Swap interval / vsync | Implemented | `dx8wrapper.cpp:1358-1369` | Stored and reapplied through reset/sync. |
| Device/frame | Registry save/load of renderer config | Implemented | `dx8wrapper.cpp:1395-1460` | Persisted through `RegistryClass`. |
| Device/frame | Device enumeration and selection | Partial | `dx8wrapper.cpp:1230-1276` | Collapsed to one synthetic device; original multi-device semantics are reduced. |
| Device/frame | Window handle update | Implemented | `dx8wrapper.cpp:1382-1388` | bgfx window is updated and resynced. |
| Device/frame | `Flip_To_Primary()` | Missing | `dx8wrapper.cpp:1200-1201`, `ww3d.cpp:1088-1090` | Legacy API remains, backend implementation is empty. |
| Transforms/state | World transform | Implemented | `dx8wrapper.cpp:1463-1480` | Stored and used during CPU-side submission. |
| Transforms/state | View transform | Implemented | `dx8wrapper.cpp:1463-1480` | Used for lighting, fog, and projection composition. |
| Transforms/state | Projection transform | Implemented | `dx8wrapper.cpp:1463-1480` | Used to build the final MVP. |
| Transforms/state | Stage-0 texture transform | Implemented | `dx8wrapper.cpp:1469-1475`, `915-952` | Consumed by `Submit_Triangles()`. |
| Transforms/state | Stage-1 texture transform | Missing | `dx8wrapper.cpp:1493-1499`, `987-1005` | Stored, but final submission never uses stage 1. |
| Transforms/state | Projection transform with Z-bias | Missing | `dx8wrapper.cpp:1508-1511` | Function ignores z-bias inputs and just stores the matrix. |
| Transforms/state | Shader state snapshot (`Get_Render_State`) | Partial | `dx8wrapper.cpp:1724-1745` | Captures objects and transforms, but not a full D3D-style state machine. |
| Transforms/state | Shader state restore (`Set_Render_State`) | Partial | `dx8wrapper.cpp:1747-1763` | Reapplies tracked objects, but not all original backend semantics. |
| Transforms/state | `Release_Render_State()` | Missing | `dx8wrapper.cpp:1765-1767` | Empty. |
| Transforms/state | `Apply_Render_State_Changes()` | Missing | `dx8wrapper.cpp:1769-1771` | Empty. |
| Transforms/state | Raw render-state shadow cache | Partial | `dx8wrapper.cpp:1627-1632`, `300-375` | Values are stored and some are used for CPU lighting, but many never affect bgfx submission. |
| Shading | CPU-side material color-source handling | Implemented | `dx8wrapper.cpp:309-375` | Ambient/diffuse/emissive material-source logic is preserved. |
| Shading | Light environment | Implemented | `dx8wrapper.cpp:1634-1648` | Ambient + up to 4 directional lights supported. |
| Shading | Explicit directional/point lights | Implemented | `dx8wrapper.cpp:1650-1684` | CPU-side lighting path handles them. |
| Shading | Explicit spot lights | Partial | `dx8wrapper.cpp:1678-1683`, `281-306` | Spot data is stored and attenuated, but still needs runtime confirmation. |
| Shading | Cull mode | Implemented | `Build_BGFX_State()` | Mapped into bgfx state bits. |
| Shading | Depth test/write | Implemented | `Build_BGFX_State()` | Mapped into bgfx state bits. |
| Shading | Alpha blending | Implemented | `Build_BGFX_State()` | Source/dest blend mapping exists. |
| Shading | Alpha test | Partial | `Build_BGFX_State()`, `shaders/fs_bootstrap.sc` | Alpha-ref state exists, but this path should be runtime-validated in bgfx. |
| Shading | Color write enable | Implemented | `Build_BGFX_State()` | Uses color mask portion of `ShaderClass`. |
| Shading | Fill mode / wireframe / point fill | Missing | `dx8wrapper.h` defines `D3DRS_FILLMODE`, submission path ignores it | No bgfx wireframe/point handling in current draw path. |
| Shading | Fog start/end/color | Implemented | `dx8wrapper.cpp`, `shaders/fs_bootstrap.sc` | Fog state is sent to the shader. |
| Shading | `FOG_ENABLE` mode | Implemented | `shader.h`, `fs_bootstrap.sc` | Standard blend-to-fog-color path exists. |
| Shading | `FOG_SCALE_FRAGMENT` mode | Implemented | `shader.h`, `fs_bootstrap.sc` | Darkening/scalar mode exists. |
| Shading | `FOG_WHITE` mode | Implemented | `shader.h`, `fs_bootstrap.sc` | White-fog mode exists. |
| Shading | Gamma/brightness/contrast | Implemented | `dx8wrapper.cpp:1707-1712`, `fs_bootstrap.sc:34-37` | Real shader-side adjustment exists. |
| Shading | Bump env map / bump env luminance | Missing | `dx8wrapper.h:114-116`, stage-1 backend gap | Caps say unsupported and no bgfx shader path exists. |
| Shading | Secondary gradient / detail combiners | Missing | `ShaderClass` exists, `fs_bootstrap.sc` does not consume them | High-level state exists, final shader path does not. |
| Shading | N-patches | Missing | `dx8wrapper.h:114`, `dx8renderer_headless.cpp` comments | Explicitly unsupported in bgfx path. |
| Geometry | Static vertex buffers | Implemented | `dx8wrapper.cpp:1513-1521` | Bound and consumed by submission path. |
| Geometry | Dynamic vertex buffers | Implemented | `dx8wrapper.cpp:1523-1531` | Bound and consumed by submission path. |
| Geometry | Static index buffers | Implemented | `dx8wrapper.cpp:1533-1541` | Bound and consumed by submission path. |
| Geometry | Dynamic index buffers | Implemented | `dx8wrapper.cpp:1543-1551` | Bound and consumed by submission path. |
| Geometry | Indexed triangle draw | Implemented | `dx8wrapper.cpp:1558-1566`, `849-1008` | Main draw path works through `Submit_Triangles()`. |
| Geometry | Triangle strips | Implemented | `dx8wrapper.cpp:1568-1600` | CPU converts strip indices to triangles before submission. |
| Geometry | Translucent sorting | Implemented | `sortingrenderer_headless.cpp`, `ww3d.cpp:1021-1023` | Sorting renderer is real and flushed each frame. |
| Geometry | Delayed/procedural material passes | Partial | `mesh.cpp`, `dx8renderer_headless.cpp` | High-level pass system exists, but any pass expecting full multi-stage shader semantics is reduced. |
| Geometry | DX8Wrapper perf counters | Missing | `dx8wrapper.h:282-287` | All DX8Wrapper counters return `0`. |
| Geometry | Frame polygon/vertex totals | Partial | `ww3d.cpp:1105-1113` | WW3D counters exist via `Debug_Statistics`, but DX8Wrapper-side counters are still stubs. |
| Geometry | Z-bias / polygon offset | Missing | `dx8wrapper.h:120`, `dx8wrapper.cpp:1508-1511`, `Build_BGFX_State()` | Reported as supported but not actually applied in the bgfx submission path. |
| Texture coords | Stage-0 pass-through UVs | Implemented | `Generate_Stage0_Texture_Input()` | Standard UV path works. |
| Texture coords | Stage-0 camera-space position generation | Implemented | `Generate_Stage0_Texture_Input()` | Supported. |
| Texture coords | Stage-0 camera-space normal generation | Implemented | `Generate_Stage0_Texture_Input()` | Supported. |
| Texture coords | Stage-0 reflection-vector generation | Implemented | `Generate_Stage0_Texture_Input()` | Supported. |
| Texture coords | Stage-0 projected texture transform | Implemented | `dx8wrapper.cpp:918-955` | Projection divide is handled for stage 0. |
| Texture coords | Stage-1 generated/projected texcoords | Missing | `Submit_Triangles()` only uses stage 0 | Original two-stage material workflows are not preserved. |
| Textures/surfaces | CPU `SurfaceClass` lock/unlock/copy/clear/stretch | Implemented | `surfaceclass_bgfx.cpp` | Needed for fonts, sentence rendering, and CPU-side conversions. |
| Textures/surfaces | Texture creation from dimensions | Implemented | `texture_bgfx.cpp:408-448` | Real bgfx texture backend object is created. |
| Textures/surfaces | Texture creation from a `SurfaceClass` | Implemented | `texture_bgfx.cpp:470-479` | CPU surface data is copied into backend texture bytes. |
| Textures/surfaces | File texture loading (`DDS` / `TGA`) | Implemented | `surfaceclass_bgfx.cpp:193-214`, `textureloader_headless.cpp` | Immediate load path exists. |
| Textures/surfaces | Stage-0 texture binding | Implemented | `dx8wrapper.cpp:987-1000` | Only stage 0 is actually submitted. |
| Textures/surfaces | Stage-1 texture binding | Missing | `dx8wrapper.cpp:1602-1607` vs `987-1000` | `Set_Texture(1, ...)` stores the pointer, but the backend never binds it. |
| Textures/surfaces | Address mode (wrap/clamp) | Implemented | `texture_bgfx.cpp:339-345` | Sampler flags are built for U/V clamp. |
| Textures/surfaces | Min/mag filtering | Partial | `texture_bgfx.cpp:347-355` | Point/aniso mapping exists, but the backend is simplified. |
| Textures/surfaces | Mip filter selection | Missing | `texture_bgfx.cpp:357-360` | Both branches currently choose `BGFX_SAMPLER_MIP_POINT`. |
| Textures/surfaces | Mip-chain preservation | Missing | `texture_bgfx.cpp:532-535`, `textureloader_headless.cpp:1165-1167`, `1215-1217` | Runtime texture objects collapse to a single level. |
| Textures/surfaces | `Get_Mip_Level_Count()` semantics | Missing | `texture_bgfx.cpp:532-535` | Always returns `1`. |
| Textures/surfaces | `Get_Surface_Level(level)` semantics | Partial | `texture_bgfx.cpp:543-552` | Only synthesizes a CPU copy of current bytes; no true per-level surface access. |
| Textures/surfaces | `Get_D3D_Surface_Level(level)` semantics | Missing | `texture_bgfx.cpp:554-557` | Always returns `NULL`. |
| Textures/surfaces | DDS compressed residency | Partial | `textureloader_headless.cpp:1165-1167`, `1227-1254` | DDS is supported only as a load/decode source; data is uploaded as RGBA8. |
| Textures/surfaces | Texture reduction setting | Partial | `ww3d.cpp:1554-1585`, `textureloader_headless.cpp` | Reduction value is used during load, but the final texture still ends up single-level. |
| Textures/surfaces | Texture invalidation / lazy re-init | Partial | `texture_bgfx.cpp:593-599`, `ww3d.cpp:740-754` | Invalidation exists, but the old thumbnail/streaming behavior is gone. |
| Texture loading | Background texture loading thread | Missing | `textureloader.cpp` vs `textureloader_headless.cpp:788-803` | Original threaded loader is replaced by synchronous `Finish_Load()`. |
| Texture loading | Foreground/background task queues | Missing | `textureloader.cpp` vs `textureloader_headless.cpp` | API shape remains, queue-driven behavior does not. |
| Texture loading | Thumbnail database / thumbnail hash | Missing | `texturethumbnail.cpp` vs `texturethumbnail_headless.cpp` | Entire system is stubbed out. |
| Texture loading | Thumbnail-first texture promotion | Missing | `texture.cpp`, `textureloader.cpp`, `texturethumbnail.cpp` vs bgfx replacements | Original deferred/thumbnail flow is not preserved. |
| Render targets | Texture-backed render target creation | Implemented | `dx8wrapper.cpp:1773-1777`, `texture_bgfx.cpp:120-146` | Real bgfx framebuffer-backed textures are created. |
| Render targets | `Set_Render_Target(TextureClass *)` | Implemented | `dx8wrapper.cpp:1691-1698` | The bgfx view target is updated from the texture RT. |
| Render targets | `Set_Render_Target(IDirect3DSurface8 *)` | Missing | `dx8wrapper.cpp:1700-1705` | Non-null surfaces are ignored; only `nullptr` reset works. |
| Render targets | Projector render-to-texture flow | Partial | `texproject.cpp:1108-1157` | RTT path exists, but original stage-1 projector semantics are reduced. |
| Render targets | Shadow/projector secondary mapper path | Missing | `texproject.cpp:618-622`, `707-711`, `1318-1319` | `Mapper1` is stage 1; bgfx submit path never consumes stage 1. |
| Effects | Base mesh material rendering | Implemented | `mesh.cpp`, `dx8renderer_headless.cpp` | Core mesh/material path still exists. |
| Effects | Two-texture / stage-1 materials | Missing | `dynamesh.cpp`, `texproject.cpp`, `shattersystem.cpp` note, `Submit_Triangles()` | High-level code still configures stage 1; backend does not submit it. |
| Effects | Decals | Partial | `decalmsh.cpp`, `dx8wrapper.h`, `dx8wrapper.cpp` | Geometry/material path exists, but Z-bias support is misreported and likely incomplete. |
| Effects | Dazzle / halo / lens flare | Partial | `dazzle.cpp` | Real implementation exists and uses stage 0, but runtime validation is still needed. |
| Effects | Point particles (`PointGroupClass`) | Missing | `particle_renderers_headless.cpp:79`, `pointgr.cpp:724-933` | bgfx build compiles the empty renderer. |
| Effects | Line groups (`LineGroupClass`) | Missing | `particle_renderers_headless.cpp:137`, `linegrp.cpp:228+` | bgfx build compiles the empty renderer. |
| Effects | Segmented lines (`SegLineRendererClass`) | Missing | `particle_renderers_headless.cpp:164`, `seglinerenderer.cpp:200+` | bgfx build compiles the empty renderer. |
| Effects | Mesh shadow render dispatch | Partial | `mesh.cpp`, `meshmdl.cpp`, `texproject.cpp` | Dispatch exists, but projector/RT/stage-1 limitations keep this from full parity. |
| 2D/text | Core `Render2DClass` path | Implemented | `render2d.cpp`, stage-0 backend | Fits the current single-texture bootstrap path well. |
| 2D/text | `Render2DSentenceClass` | Implemented | `render2dsentence.cpp` | Real sentence renderer is compiled in bgfx build. |
| 2D/text | `Font3DDataClass` / font texture generation | Implemented | `font3d.cpp`, `surfaceclass_bgfx.cpp` | CPU surface path needed by font generation is present. |
| Capture/debug | Screenshot API | Missing | `ww3d.cpp:1246-1250` | Explicitly logged as not implemented for bgfx bootstrap. |
| Capture/debug | Movie capture | Missing | `ww3d.cpp:1268-1538` | Still legacy Windows-only DX8/front-buffer code, not a bgfx port. |
| Capture/debug | Accurate hardware caps/vendor/device reporting | Missing | `dx8wrapper.h:110-125` | Placeholder answers only. |
| Capture/debug | Texture-format support reporting | Missing | `dx8wrapper.h:112-118` | Always-true answers do not match current reduced backend behavior. |
| Capture/debug | App activate/deactivate renderer hooks | Missing | `ww3d.cpp:1691-1730` | Old `WW3D_DX8` path remains; bgfx-specific replacement is not present. |
| Capture/debug | Pixel-center update logic | Missing | `ww3d.cpp:1739-1758` | Legacy `WW3D_DX8` code path only; no bgfx-specific implementation. |

## Confirmed high-priority parity work

If the goal is "close the largest renderer parity gaps first," the most important missing work is:

1. **Replace `particle_renderers_headless.cpp` with real bgfx-backed point/line/segline renderers.**
2. **Implement actual stage-1 texture submission and shading support.**
3. **Restore real texture loading parity: thumbnails or an equivalent deferred path, background loading, and multi-level textures.**
4. **Fix caps/reporting so the engine stops taking unsupported code paths based on optimistic placeholders.**
5. **Port screenshot/capture and validate projector/shadow/decal behavior against the original renderer.**

## Bottom line

The current bgfx renderer is already capable of drawing a significant part of the game because the main mesh, sorting, lighting, surface, and 2D paths are real.

But it is **not feature-complete relative to the original WW3D/DX8 renderer**. The most important missing features today are:

- point/line/segmented-line rendering
- true two-stage material support
- original texture thumbnail/streaming/mip behavior
- screenshot/movie-capture parity
- accurate caps/stat reporting
- trustworthy Z-bias/decal behavior

Until those are fixed, the bgfx renderer should be treated as a **partial renderer port with several confirmed feature regressions**, not as a parity-complete replacement for the original DX8 backend.

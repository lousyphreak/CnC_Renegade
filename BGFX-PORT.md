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

## Shadowing

- bgfx shadow maps are the only supported runtime shadow path. Legacy `wwphys` blob/projected shadow systems must not be kept alive in parallel for units or static anim projectors.
- Keep compatibility surfaces such as serialized `Shadow_Mode` values, but collapse any legacy non-zero mode to the shadow-map path instead of reviving projector-based shadows.
- Generic projector features that are not shadows may remain, but shadow-specific projector generation should be removed rather than hidden behind settings.

## Shader Architecture

The mesh rendering pipeline uses **2 shader programs** (down from 4):

| Program | Vertex Shader | Fragment Shader | Purpose |
|---------|--------------|-----------------|---------|
| `MeshProgram` | `vs_mesh.sc` | `fs_mesh.sc` | Standard meshes |
| `MeshTexgenProgram` | `vs_mesh_texgen.sc` | `fs_mesh.sc` | Meshes with texture coordinate generation (environment maps, projectors) |

Both programs share the same fragment shader (`fs_mesh.sc`), which handles:
- Per-pixel lighting with a tri-state mode uniform (`u_meshLitConfig.x`):
  - `0` = unlit (vertex color passthrough, e.g. prelit meshes, particles)
  - `1` = emissive-only (no normals available, D3D8-compatible behavior)
  - `2` = full per-pixel N·L lighting (4 directional lights + scene ambient)
- Texture stage combining (2 stages, configurable color/alpha ops)
- CSM shadow receiving
- Fog (linear, range-based or planar)
- Alpha test

### Material Classification

Material properties are **pre-computed at category creation time** in `Classify_Material()`, not queried from DX8Wrapper per draw call. The `MaterialClassification` struct stores:
- Which shader program to use (Mesh vs MeshTexgen)
- Fragment config (texture combine ops, alpha test, fog mode)
- Lighting config (mode, color sources — diffuse/ambient/emissive from vertex vs material)
- Material colors (ambient, diffuse, emissive, opacity)

Only **per-mesh scene state** (scene ambient color, light directions/colors) is read from DX8Wrapper at submit time, since these change per mesh instance via `Set_Light_Environment()`.

### Key Design Decisions

- **No specular**: Confirmed unused by all consumers. Not implemented.
- **Max 2 texture stages**: Confirmed across all material/shader usage.
- **Color vertex always enabled**: `D3DRS_COLORVERTEX` is always TRUE; color sources resolved directly from `VertexMaterialClass::Get_*_Color_Source()`.
- **GPU skinning for W3D skins**: Renegade skin meshes in this codebase are rigid single-bone-per-vertex skins, so the bgfx path should keep one immutable base vertex buffer per `MeshModelClass`, pass the bone index as vertex data, and fetch the current bone matrix from a per-frame palette texture in the vertex shader. The palette upload should be cached per visible `MeshClass` so repeated passes/shadow draws reuse the same uploaded transforms instead of re-uploading or re-deforming geometry. Sorted translucent skins are the one exception: the legacy triangle-sorting path still needs CPU-deformed vertices, so that path should stay on a CPU-generated sorting buffer.

### Bump Environment Mapping (EMBM)

DX8 bump environment mapping is implemented in the fragment shader, replicating the original D3D8 fixed-function `D3DTOP_BUMPENVMAP` / `D3DTOP_BUMPENVMAPLUMINANCE` / `D3DTOP_DOTPRODUCT3` texture operations.

**How it works:**
- Stage 0 uses a bump texture (U8V8, L6V5U5, or X8L8V8U8 format) storing signed du/dv perturbation values
- The du/dv values are transformed by a 2×2 rotation/scale matrix (`u_meshBumpEnvMat`), set per-draw by `BumpEnvTextureMapperClass::Apply` which animates the rotation angle
- The transformed perturbation offsets the UV coordinates used for stage 1 sampling
- For the LUMINANCE variant, the bump texture alpha channel modulates brightness via `luminance * scale + offset`
- DOTPRODUCT3 computes a dot product between bias-decoded texture RGB and current color RGB

**Texture format conversion:**
- Signed U/V channels are biased from [-128,127] to [0,255] by XORing with 0x80
- Shader decodes back to [-1,1] with `val * 2.0 - 1.0`
- Luminance channel is unsigned, stored directly in alpha
- Legacy D3D packed-color alpha formats (`A1R5G5B5`, `X1R5G5B5`, `A4R4G4B4`, `X4R4G4B4`) must not be direct-copied into bgfx packed formats just because the bit widths look similar. Their channel layout does not match bgfx `BGR5A1` / `BGRA4`, so the safe upload path is to expand them through the CPU BGRA8 conversion used by `Convert_Surface_Copy_To_BGRA8`.

**Uniforms:**
- `u_meshBumpEnvMat` (vec4): 2×2 bump matrix (mat00, mat01, mat10, mat11)
- `u_meshBumpEnvLum` (vec4): luminance scale and offset (scale, offset, 0, 0)
- Both read from `DX8Wrapper::TextureStageStates` at submit time (dynamic per-draw)
- Match original stage layout: matrix terms come from stage 0, luminance scale/offset from stage 1

**Original runtime users:**
- Authored W3D mesh passes loaded through `meshmdlio.cpp`, not special-case gameplay code
- `VertexMaterialClass::Load_W3D()` creates `BumpEnvTextureMapperClass` from vertex-mapper args
- `Load_Texture()` must preserve `W3DTEXTURE_TYPE_BUMPMAP` even when textures load before renderer init
- `MeshMatDescClass::Post_Load_Process()` must not strip bump passes merely because caps are not initialized yet

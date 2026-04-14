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

## Mesh shader and material pipeline

- **D3DRS_COLORVERTEX is always TRUE**: Set in `DX8Wrapper::Init` and `bgfxdynamicbuffer.cpp`, never set to FALSE anywhere in the codebase. This means color source resolution can query `VertexMaterialClass::Get_*_Color_Source()` directly instead of going through D3D render state.
- **Specular is effectively unused**: Loaded/stored by `VertexMaterialClass` but never rendered. All consumers set `ShaderClass::SECONDARY_GRADIENT_DISABLE`. No need to implement specular in the shader pipeline.
- **Max 2 texture stages confirmed**: `MAX_TEXTURE_STAGES=2` throughout the codebase. No consumer uses more than 2 stages.
- **Material color sources**: `VertexMaterialClass` exposes `Get_Diffuse_Color_Source()`, `Get_Ambient_Color_Source()`, `Get_Emissive_Color_Source()` returning `MATERIAL` (0), `COLOR1` (1), or `COLOR2` (2). In practice only `MATERIAL` and `COLOR1` are used.
- **LightEnvironmentClass CPU reduction**: Reduces arbitrary scene lights to max 4 directional lights + ambient on CPU. This stays as-is; the shader receives 4 directional lights via uniform arrays.
- **FVF has_normals determines lighting mode**: Meshes in a DX8TextureCategoryClass share the same FVF. `(FVF & VERTEX_FORMAT_FLAG_NORMAL) != 0` determines whether full N·L lighting or emissive-only mode is used.
- **Texgen determination at classification time**: Whether a mesh needs texgen is determined by checking `VertexMaterialClass::Peek_Mapper(stage)` for non-null mappers, not by querying live DX8 texture stage state. This allows the program choice to be pre-computed.
- **bgfx uniforms are sticky until changed**: Uniform values persist across submit calls within a frame. Lighting uniforms must always be set before every mesh submit since different meshes have different light environments.

## DX8 Bump Environment Mapping (EMBM)

- **Not tangent-space normal mapping**: W3D mesh format has NO tangent/binormal vertex data. The original D3D8 renderer used the fixed-function `D3DTOP_BUMPENVMAP` operation, which is UV perturbation — NOT modern tangent-space normal mapping.
- **Bump texture formats**: `WW3D_FORMAT_U8V8` (2 bytes: signed U, signed V), `WW3D_FORMAT_L6V5U5` (2 bytes: packed signed U5, signed V5, unsigned L6), `WW3D_FORMAT_X8L8V8U8` (4 bytes: signed U, signed V, unsigned L, padding X). All converted to BGRA8 for bgfx.
- **Signed-to-unsigned bias**: U and V channels are signed bytes (-128 to 127). XOR with 0x80 converts to unsigned [0,255] for storage. Shader decodes via `val * 2.0 - 1.0` to recover [-1,1] range. Luminance (L) channel is unsigned, stored directly in alpha.
- **BumpEnvTextureMapperClass**: Extends LinearOffsetTextureMapperClass. Animates a rotation angle (`CurrentAngle += RadiansPerSecond * delta`), computes a 2×2 rotation/scale matrix, and stores it in DX8Wrapper texture stage state via `D3DTSS_BUMPENVMAT00/01/10/11`.
- **Bump matrix is dynamic per-draw**: Read from `DX8Wrapper::TextureStageStates[stage][D3DTSS_BUMPENVMAT*]` at submit time. Values stored as float-as-DWORD via `F2DW()`, decoded back with `memcpy`.
- **Stage 0 is a passthrough**: The bump stage doesn't produce color output — it only perturbs the UV coordinates of the next stage. Stage 0 alpha op is DISABLE per D3D8 spec.
- **Three gradient types**: `GRADIENT_BUMPENVMAP` (UV perturbation only), `GRADIENT_BUMPENVMAPLUMINANCE` (+ luminance modulation from alpha channel), `GRADIENT_DOTPRODUCT3` (dot product between bias-decoded texture and current color).
- **Real runtime users are file-driven, not game-specific C++ effects**: the original bump path is primarily `meshmdlio.cpp` -> `VertexMaterialClass::Load_W3D()` -> `BumpEnvTextureMapperClass` / bump shader gradients / `W3DTEXTURE_TYPE_BUMPMAP`. I did not find a distinct Renegade gameplay-side hardcoded EMBM factory beyond skeleton/test code.
- **Preserve bump intent before renderer init**: load-time code can run before DX8/bgfx caps are fully initialized. If `meshmatdesc.cpp` kills bump passes or `texture.cpp` downgrades bump textures during that phase, authored bump materials are lost permanently for the session. The bgfx port should preserve those authored bump materials until actual runtime caps are known.
- **BUMPENVMAPLUMINANCE state layout**: this codebase initializes `D3DTSS_BUMPENVMAT*` on stage 0, but `D3DTSS_BUMPENVLSCALE/LOFFSET` on stage 1 (`dx8wrapper.cpp`, `bgfxdynamicbuffer.cpp`). The bgfx uniform upload must mirror that layout for parity with original authored luminance users.

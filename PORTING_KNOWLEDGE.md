# Porting Knowledge

## Renderer architecture

- `Code/ww3d2/ww3d.*` provides the top-level renderer lifecycle used by the game and tools.
- `Code/ww3d2/dx8wrapper.*` is the central Direct3D integration point. It owns device creation, frame begin/end, resource lifecycle hooks, render-state application, and a large amount of static renderer-global state.
- `Code/ww3d2/dx8renderer.*` and `Code/ww3d2/dx8polygonrenderer.*` form the mesh submission pipeline on top of the low-level wrapper.
- Texture and buffer ownership are spread across:
  - `dx8vertexbuffer.*`
  - `dx8indexbuffer.*`
  - `dx8texman.*`
  - `texture.*`
  - `surfaceclass.*`

## Fresh-slate bgfx strategy

- Start from the renderer backend boundary rather than trying to preserve Direct3D internals.
- Keep the higher-level `WW3D` entry surface initially so dependent game code can continue to compile against the same renderer-facing API while the backend is replaced.
- Prefer compile failures that expose missing bgfx work over temporary fallbacks or disabled paths.
- The first slice should replace initialization/device ownership and force the build system to acknowledge bgfx as the renderer backend in `ww3d2`.
- A clean port does not need to preserve `dx8wrapper.h` as the migration seam; higher-level systems can move directly to `BgfxRenderer` when the dependency is only viewport, clear, or matrix submission.

## Fresh blockers uncovered by rebuild

- The largest compile blocker is still the transitive `#include "dx8wrapper.h"` surface, because it drags in `d3d8.h` from many otherwise high-level files.
- Replacing the shared Direct3D SDK structs/enums with an engine-owned renderer vocabulary is viable as an intermediate cleanup step, but it only solves the header dependency if `dx8wrapper.h` also stops inlining calls that dereference D3D COM interfaces.
- The `dx8fvf.*` layer did not need Direct3D at all; it only needed bitfield definitions and vertex-layout sizing. That metadata can live entirely in engine-owned code without a compatibility header.
- `dx8vertexbuffer.h` was one of the highest-impact include points for the old FVF macros, so moving it onto engine-owned flags trims DX8 leakage from many renderer-adjacent compilation units even before the full draw path is ported.
- Some failures are separate Linux/cross-platform hygiene issues rather than renderer design issues:
  - case-sensitive include mismatches such as `audiblesound.h` vs `AudibleSound.h`
  - Windows-only typedef/macros (`DWORD`, `ULONG`, `_strdup`) still embedded in shared headers
- `CameraClass::Apply()` was a narrow, clean dependency on DX8 state submission only. It can be ported directly to bgfx without introducing any compatibility layer by sending viewport and view/projection matrices straight to `BgfxRenderer`.
- `dx8wrapper.h` still mixes two very different roles:
  - renderer-facing cached state and utility APIs that can live in engine-owned code
  - backend-local device calls (`SetTransform`, `SetRenderState`, `SetTexture`, `CopyRects`, `AddRef` / `Release`) that should no longer be inline in a shared header
- After introducing `renderer_types.h`, the next clean architectural step is to push those backend-local inline bodies into `.cpp` implementation so the rest of the engine can compile against the renderer API without needing a concrete Direct3D device type.
- Moving the `dx8wrapper.h` device-calling inlines into `dx8wrapper.cpp` successfully collapses a large class of transitive compile failures. Once that is done, the next blockers become much more concrete:
  - files that still inspect raw `IDirect3DTexture8` / `IDirect3DSurface8` objects directly (`assetmgr.cpp`, `ddsfile.cpp`, `missingtexture.cpp`)
  - backend-local files that still depend on D3DX utility helpers for texture creation, image loading, or mip generation
- `assetmgr.cpp`'s texture logging path is a good example of the next cleanup target: it no longer needs D3DX at all, but it still needs a renderer-owned way to ask a texture for width/height/format instead of calling `GetLevelDesc` on a raw D3D texture.
- `missingtexture.cpp` only needed D3DX for mip generation, not for the missing texture image itself. The mip generation can be expressed directly as a local box-filter over locked surface data; the remaining dependency is ownership/access to the concrete texture and surface interfaces.
- After the shared-header cleanup, build failures are more trustworthy: if a file still breaks on Direct3D types, it is because it truly still owns backend-specific work rather than because `dx8wrapper.h` dragged the device interface into it accidentally.
- A useful clean-port pattern is to move shared code onto existing engine objects before inventing any new renderer API. `TextureClass` and `SurfaceClass` already encapsulate texture/surface description, locking, copying, and format reporting well enough to remove many raw `IDirect3D*` touch points without introducing another abstraction layer.
- `assetmgr.cpp` texture statistics did not need a Direct3D texture at all; switching them to `SurfaceClass::Get_Description()` removes a D3D-ism without affecting renderer behavior.
- `DDSFileClass` is a good seam-reduction target because its real contract is “copy this mip level into a writable surface”, not “copy this mip level into a Direct3D COM object”. Moving that API to `SurfaceClass` keeps the DDS loader renderer-neutral at the shared-engine layer.
- For the remaining backend-local raw return paths, ownership should be concentrated inside `TextureClass` / `SurfaceClass` implementation files rather than repeated in shared callers. That keeps COM-style `AddRef` / `Release` logic out of the broader engine while the DX8 backend still exists.
- `WW3D` frame ownership is a separate seam from mesh/state rendering. Camera submission was already moved to bgfx, and the next clean follow-up was to move top-level frame begin/end/clear/resolution handling there too. That reduces DX8-era code to the parts that still actually implement rendering behavior rather than letting it remain the global frame manager by inertia.
- `BgfxRenderer` now needs to own a small amount of renderer state beyond raw bgfx startup: drawable size, window mode, bit depth, and view-clear submission. Those are renderer-owned concerns and are appropriate to keep there; they are not a compatibility wrapper around Direct3D.
- A clean bgfx port also needs build-owned shader assets, not ad hoc runtime assumptions. `bgfx_compile_shaders(...)` from `bgfxToolUtils.cmake` works correctly in this tree and can generate the per-renderer shader binaries as part of the normal `cmake --build` flow.
- bgfx shader compilation in this project needs two include roots:
  - the local shader directory in `Code/ww3d2/shaders`
  - `${bgfx_cmake_SOURCE_DIR}/bgfx/src` so `bgfx_shader.sh` is available
- `varying.def.sc` must declare both vertex inputs and shader varyings for bgfx's multi-profile shader compiler. Declaring only the varyings is not enough; the vertex shader then fails with missing `a_position` / attribute symbols.
- A useful renderer-owned baseline is now in `BgfxRenderer`:
  - reusable position/color/texcoord vertex layout
  - renderer-profile-aware shader binary loader
  - bgfx program creation helper
  - canonical white texture + sampler uniform for non-textured submission
  - `ShaderClass` to bgfx render-state translation for the state bits that are API state rather than shader logic
- `ShaderClass` features do not map 1:1 into bgfx state:
  - depth compare, depth write, color write, cull, and blend map cleanly to bgfx state bits
  - fog, alpha test, gradient modes, and detail combiners are shader-program concerns and must be handled by shader variants or uniforms rather than by pretending bgfx has DX8 texture-stage state
- `Render2D` is a good clean-port pattern for screen-space rendering:
  - keep the higher-level vertex/color/UV accumulation logic intact
  - use a renderer-owned overlay view in bgfx instead of mutating the main camera view state the way the DX8 path did
  - convert packed engine ARGB colors to bgfx's expected ABGR vertex packing at submission time
- `TextureClass` can now provide a native bgfx texture directly to renderer code. That is a better migration seam than reusing `DX8Wrapper::Set_Texture(...)`, because sampler/addressing policy can be translated into bgfx flags per bind without reviving texture-stage state abstractions.
- `SurfaceClass::CreateCopy()` is currently the cleanest existing engine-level hook for moving legacy texture pixel data into bgfx. It works well for the `Render2D`/font path, but the broader port still needs a renderer-native texture loading path so file-backed textures are not sourced through legacy DX8 objects first.
- A better intermediate seam than “flatten mip 0 and hope” is to let `TextureClass` cache its mip surfaces and make `BgfxRenderer` consume the full chain. That preserves texture reduction, authored mip counts, and compressed DDS payloads without resurrecting DX8 texture-stage abstractions in the draw path.
- DXT textures can stay native in bgfx even before the source loader is fully ported: `SurfaceClass::CreateCopy()` can copy the block-compressed mip payload directly from locked legacy surfaces, and bgfx accepts those levels as `BC1` / `BC2` / `BC3` updates.
- For legacy uncompressed formats, a practical clean-port rule is:
  - preserve native compressed uploads when possible
  - preserve authored mip structure always
  - only convert the formats that do not have a clean 1:1 bgfx upload representation in the current backend slice
- Comparing textures by raw backend pointer identity is a bad seam for the port. Missing-texture detection is safer and cleaner when it compares against the engine-owned singleton `TextureClass` instance instead of comparing `IDirect3DTexture8*`.
- `SurfaceClass` is the right ownership seam for the loader port, not a new renderer shim. It already owns format, locking, copies, and per-level size information, so adding CPU-backed storage there keeps texture data engine-owned without inventing another abstraction layer.
- Once `TextureClass` has real `SurfaceLevels`, “loaded texture” must no longer mean “has a DX8 texture pointer”. Thumbnail-loaded textures are a concrete example: they should be treated as loaded source data immediately, with any remaining DX8 object materialized only on demand for backend-local code.
- `TextureLoader::Load_Surface_Immediate(...)` is a good seam-reduction target because shared callers really want decoded surface data, while only backend-local code still wants to turn that into an `IDirect3DSurface8*`. Returning `SurfaceClass*` there shrinks the raw DX8 boundary without changing higher-level behavior.
- A good hygiene check for the clean port is “does this shared header still mention `IDirect3D*` in its public API?”. If the answer is yes, the seam is probably still in the wrong place. Backend-local files can still bridge to DX8 temporarily, but shared texture APIs should traffic in engine-owned `TextureClass` / `SurfaceClass` data instead.
- Render targets are a separate texture seam from file-backed mip data. A clean bgfx port should treat them as bgfx-owned framebuffer attachments, not as a special case of “make a D3D texture/surface and then recover the bits later”. `WW3D::Begin_Render()` already uses bgfx views, so binding the projector target through `bgfx::setViewFrameBuffer` is the correct direction for that path.

## Repository observations

- `BGFX-PORT.md` explicitly rejects shims, stubs, wrappers, and placeholders.
- The repository root already fetches bgfx via `bgfx.cmake`, so the project-level dependency plumbing exists.
- `Code/ww3d2/CMakeLists.txt` had to be reconstructed to get the renderer target building again.

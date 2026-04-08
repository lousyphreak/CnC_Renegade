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
- Some failures are separate Linux/cross-platform hygiene issues rather than renderer design issues:
  - case-sensitive include mismatches such as `audiblesound.h` vs `AudibleSound.h`
  - Windows-only typedef/macros (`DWORD`, `ULONG`, `_strdup`) still embedded in shared headers
- `CameraClass::Apply()` was a narrow, clean dependency on DX8 state submission only. It can be ported directly to bgfx without introducing any compatibility layer by sending viewport and view/projection matrices straight to `BgfxRenderer`.

## Repository observations

- `BGFX-PORT.md` explicitly rejects shims, stubs, wrappers, and placeholders.
- The repository root already fetches bgfx via `bgfx.cmake`, so the project-level dependency plumbing exists.
- `Code/ww3d2/CMakeLists.txt` had to be reconstructed to get the renderer target building again.

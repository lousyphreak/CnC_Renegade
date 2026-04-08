# Porting Progress

## BGFX renderer fresh start

- Reset the bgfx renderer effort to a clean-slate plan based on `BGFX-PORT.md`.
- Identified the primary renderer migration boundary in `Code/ww3d2`:
  - `dx8wrapper.*` for device lifecycle and render state orchestration
  - `dx8vertexbuffer.*` and `dx8indexbuffer.*` for GPU buffer ownership
  - `dx8texman.*`, `texture.*`, and `surfaceclass.*` for texture and render-target ownership
  - `dx8renderer.*`, `dx8polygonrenderer.*`, and `ww3d.*` for renderer submission and frame lifecycle
- Confirmed that `ww3d2` is a widely shared dependency of `wwui`, `wwphys`, `Combat`, and `Commando`, so the first slice must preserve the top-level `WW3D` entry surface while replacing the backend cleanly.
- Added a dedicated `Code/ww3d2/CMakeLists.txt` so the renderer library builds as its own target again and can link bgfx directly.
- Added an initial `BgfxRenderer` backend that initializes bgfx from the SDL3-owned native window handle.
- Switched `CameraClass::Apply()` off `DX8Wrapper` for viewport/view/projection submission and onto `BgfxRenderer`, making the camera path the first real runtime code to talk to bgfx directly.
- Fixed two Linux-port blockers uncovered by the fresh build:
  - corrected `animatedsoundmgr.cpp` to include `WWAudio/AudibleSound.h` with the filesystem's actual case
  - removed `agg_def.h`'s dependence on Windows-only typedefs/macros for `DWORD`, `ULONG`, and `_strdup`
- Replaced the `dx8fvf.*` vertex-format metadata layer's dependency on `d3d8.h`/`D3dx8core.h` with engine-owned FVF flags and local vertex-size computation.
- Updated `dx8vertexbuffer.*` to consume the new FVF definitions directly instead of raw `D3DFVF_*` macros.
- Dropped the dead `dx8wrapper.h` include from `part_buf.cpp`; an attempted removal from `ddsfile.cpp` showed that DDS upload code is still directly tied to DX8 surface types and needs a later surface-format port rather than a blind include trim.
- Rebuilt after the FVF cleanup and confirmed the next renderer blocker is still the broad `dx8wrapper.h` API surface, which exposes D3D viewport/light/material/transform types to high-level code.

## Next work

- Continue removing `dx8wrapper.h` from high-level runtime files that only need camera/state submission.
- Replace the D3D types in `dx8wrapper.h`/`rddesc.h` with engine-owned renderer data structures so shared headers stop requiring missing Direct3D SDK headers.
- Replace `WW3D::Init()` / frame lifecycle calls with direct bgfx backend calls.
- Replace the D3D-format conversion surface in `formconv.*` and texture loading with backend-neutral or bgfx-backed format handling.

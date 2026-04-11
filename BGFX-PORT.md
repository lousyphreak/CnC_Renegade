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
- The bgfx texture upload path now consumes full `TextureClass` mip chains, preserves native BC1/BC2/BC3 uploads for DXT textures, and keeps per-level surface data available for renderer-native texture creation instead of rebuilding only level 0.
- `TextureClass` no longer materializes fake `IDirect3DTexture8` ownership in the bgfx build. Procedural textures now allocate engine-owned `SurfaceClass` mip levels directly, `dx8texman` no longer pretends to recreate DX8 textures on bgfx resets, and the bgfx-side fake texture COM object has been deleted instead of preserved as a compatibility seam.
- `SurfaceClass` now supports engine-owned CPU texture storage with lazy DX8 materialization at the remaining backend edge, so texture source data is no longer forced to originate in a D3D allocation.
- `TextureLoader` thumbnail and immediate surface loading now produce `SurfaceClass` mip data directly, and thumbnail-backed textures no longer treat the absence of a legacy DX8 texture object as “not loaded”.
- Shared texture-facing headers no longer expose the D3D-returning `MissingTexture` helpers or the unused `TextureClass` DX8 accessors that were leaking `IDirect3D*` back out of the backend boundary.
- `SurfaceClass` utility work in the bgfx build now stays on engine-owned CPU memory instead of materializing fake D3D surfaces:
  - the filename-loading constructor now copies decoded `SurfaceClass` data directly
  - font/hue/pixel utility methods (`FindBB`, `Is_Transparent_Column`, `Get_Pixel`, `DrawPixel`, `DrawHLine`) now operate through `Lock` / `Unlock`
  - the bgfx-side fake `IDirect3DSurface8` implementation and `_Create_DX8_Surface(...)` helpers have been deleted instead of preserved as another compatibility seam
- Render-target textures are starting to move onto bgfx-native ownership: `TextureClass` now carries a bgfx framebuffer handle, `BgfxRenderer` can bind a texture as the active render target, and projector render-to-texture setup no longer needs a D3D surface when bgfx is active.
- `Render2D` now submits directly to bgfx using renderer-owned programs, state, and buffers rather than the DX8 dynamic buffer path.
- The bgfx fixed-function path now advertises the texture-operation coverage it already implements in `fs_fixed_function.sc`: the bgfx-side caps/bootstrap tables expose bump-env, bump-env-luminance, dot3, and current-alpha detail blending so `ShaderClass` no longer silently downgrades those material paths under bgfx.
- Runtime validation has moved beyond startup-only bring-up:
  - bgfx/X11/Vulkan initialization now survives the real `WW3D::Init()` + `DX8Wrapper::Init()` sequence without falling back to headless or failing on repeated init.
  - Linux/X11 startup should currently keep bgfx on its render thread. Re-testing the old single-threaded `bgfx::renderFrame()` workaround against the live menu path showed that it had become a major startup bottleneck, while the threaded path now survives real startup and long-run validation in this tree.
  - SDL native-window bridging now tags Wayland handles with `bgfx::NativeWindowHandleType::Wayland` instead of relying on bgfx's Linux default handle type, preventing Wayland sessions from falling into bgfx's X11 surface path during startup.
  - the bgfx-backed lifecycle now restores the legacy one-time renderer subsystem init/shutdown steps needed by textures, materials, mesh rendering, and related systems while those codepaths are still being ported.
  - late runtime/shutdown ASAN failures in `dx8renderer.cpp`'s deferred delete bookkeeping were fixed, and the executable now survives a live validation run longer than 300 seconds under ASAN/UBSAN.
  - static shadow projector validation/caching still contains a DX8-era CPU-readback assumption (`TextureClass::Get_Surface_Level()` on a render target). bgfx render targets may not expose CPU-readable surfaces, so that path must fail closed rather than crash until the projector cache is ported to a GPU-native copy/sampling flow.
- Startup-specific runtime knowledge from menu bring-up work:
  - do not initialize `AnimatedSoundMgrClass` before the definition hash is live; the null-definition-hash lookup storm is a real seconds-scale startup regression.
  - do not let menu font setup rescan the system font tree per font load; cache font candidates/aliases once and reuse them across `StyleMgrClass` font creation.

## Immediate next slice

- Expand the same native bgfx submission approach from `Render2D` into the next real material/mesh path, starting with rigid meshes and shared texture ownership cleanup.
- Finish the remaining true CPU-readback/render-target cleanup so projector caching and any residual surface-return paths stop assuming bgfx render targets can expose legacy DX8-style CPU surfaces.

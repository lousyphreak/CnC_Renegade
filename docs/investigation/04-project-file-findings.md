# Project File Findings

Last updated: 2026-03-17

This document captures concrete findings from reading representative VC6 `.dsp` project files and both top-level workspaces.

## Why this matters

The legacy project files are the most authoritative source for:

- which targets existed historically
- which outputs are executables vs libraries
- which SDKs and helper libraries the build expected
- which include paths and library paths are now missing
- which parts of the tree were only ever intended to build inside a specific internal Westwood-style environment

## Workspace findings

### `Code/commando.dsw`

This is the primary runtime/game workspace.

Important observations:

- it references the missing `GameSpy` project directly
- it declares explicit dependencies for the `commando` executable target
- it mixes runtime code, engine libraries, audio, tools, and support modules in one VC6 workspace

The `commando` target’s declared project dependencies show a historical core graph:

- `Scripts`
- `wwdebug`
- `wwlib`
- `wwmath`
- `wwnet`
- `wwphys`
- `wwutil`
- `WWAudio`
- `wwsaveload`
- `Combat`
- `wwtranslatedb`
- `wwbitpack`
- `ww3d2`
- `wwui`
- `BinkMovie`
- `BandTest`
- `SControl`
- `GameSpy`

### `Code/tools.dsw`

This is a separate toolchain workspace with at least 27 project entries.

Implication: the historical developer workflow depended on a wide tool ecosystem that cannot be treated as optional background noise.

Notable tool targets include:

- `LevelEdit`
- `LightMap`
- `W3DView`
- `WWConfig`
- `WWCtrl`
- `max2w3d`
- `W3DShellExt`
- `CommandoUpdate`
- `MakeMix`
- `bin2cpp`

## Representative target types

Observed target kinds from project headers:

| Project file | Declared target type |
| --- | --- |
| `Commando/commando.dsp` | `Win32 (x86) Application` |
| `Combat/Combat.dsp` | `Win32 (x86) Static Library` |
| `ww3d2/ww3d2.dsp` | `Win32 (x86) Static Library` |
| `WWAudio/WWAudio.dsp` | `Win32 (x86) Static Library` |
| `Tools/LevelEdit/LevelEdit.dsp` | `Win32 (x86) Application` |

This means a future migration can likely begin by reconstructing libraries first and layering applications on top.

## Configuration patterns

Representative projects use multiple configuration variants beyond simple Debug/Release:

- `Release`
- `Debug`
- `Profile`
- `DebugE`
- `ProfileE`
- special variants such as `Debug Compressed`

Interpretation:

- `Profile` appears to be a profiling/performance-instrumented build flavor
- `E` variants appear related to editor or parameter-editing builds, often with `PARAM_EDITING_ON`
- some build flavors were clearly intended for internal tooling and content workflows rather than shipping binaries

## Main executable project findings: `Commando/commando.dsp`

### Compiler assumptions

Representative include paths for the game target include:

- `..\DirectX\include`
- `..\wwui`
- `..\wwbitpack`
- `..\combat`
- `..\wwaudio`
- `..\miles6\include`
- `..\wolapi`
- `..\wwutil`
- `..\wwlzhl`
- `..\wwnet`
- `..\wwmath`
- `..\ww3d2`
- `..\wwphys`
- `..\wwlib`
- `..\wwdebug`
- `..\wwsaveload`
- `..\wwtranslatedb`
- `..\BinkMovie`
- `..\scontrol`
- `..\GameSpy`

Potential issues:

- `wwlzhl` was not found in the repository inventory
- `GameSpy` was not found in the repository inventory
- `DirectX` and `miles6` were not found in the repository inventory
- several legacy path spellings vary in case or naming style, which may become relevant when porting to case-sensitive filesystems

### Linker assumptions

Representative linker inputs include:

- `d3dx8.lib`
- `dinput.lib`
- `wwui.lib`
- `wwaudio.lib`
- `mss32.lib`
- `wwsaveload.lib`
- `ww3d2.lib`
- `wwdebug.lib`
- `wwlib.lib`
- `wwmath.lib`
- `wwnet.lib`
- `wwphys.lib`
- `wwutil.lib`
- `combat.lib`
- `wwtranslatedb.lib`
- `binkmovie.lib`
- `binkw32.lib`
- `bandtest.lib`
- `scontrol.lib`
- `gamespy.lib`

Representative library paths include:

- `../libs/debug`
- `../libs/release`
- `../libs/profile`
- `../srsdk1x/msvc6/lib/...`
- `../directx/lib`
- `../umbra/lib/win32-x86`

This shows the game executable historically linked against a mix of:

- local engine/game libraries
- Windows platform libraries
- proprietary middleware
- external/internal SDK library trees not present in the public repository

## Engine library findings: `ww3d2/ww3d2.dsp`

This appears to be a central rendering/3D support library.

Evidence:

- DirectX 8 headers and source files are core to the project
- configuration names include `Debug Compressed`
- project sources include renderer-heavy files such as:
  - `dx8renderer.cpp`
  - `dx8wrapper.cpp`
  - `dx8texman.cpp`
  - `dx8vertexbuffer.cpp`
  - `render2d.cpp`

Ghost or missing path references include:

- `..\DirectX\include`
- `..\wwcpuid`
- `..\miles6\include`
- `..\srsdk1x\include`
- `..\srsdk\include`

Interpretation:

- renderer modernization will be one of the hardest parts of the rescue effort
- this library likely sits near the center of the entire runtime and tool graph
- any portable future build will need a deliberate strategy for replacing or encapsulating DX8-era renderer code

## Gameplay library findings: `Combat/Combat.dsp`

This project is a large static library with multiple configurations and editor variants.

Notable details:

- active source set is large and central to gameplay
- include paths reference `miles6`, `DirectX`, and many engine modules
- `PARAM_EDITING_ON` appears in editor-oriented configurations

Interpretation:

- gameplay code is not isolated from engine/platform concerns
- editor-specific behavior leaks into shared game libraries via configuration defines

## Audio library findings: `WWAudio/WWAudio.dsp`

This is a static library with direct dependency on:

- `DirectX`
- `miles6`
- engine support libraries

Interpretation:

- audio replacement is not just about swapping one library call site; there is a dedicated audio subsystem with its own library boundary
- Miles removal will likely touch a wide set of files even if abstraction points exist

## Editor findings: `Tools/LevelEdit/LevelEdit.dsp`

This is one of the strongest examples of external dependency concentration.

Direct evidence includes:

- MFC application target
- `PARAM_EDITING_ON` define
- `NvDXTLib` include and lib paths
- `miles6` include and lib paths
- `directX` include and lib paths
- `srsdk1x` include/lib paths
- `dinput8.lib`, `d3dx8.lib`, `d3d8.lib`
- `shlwapi.lib`

The file also contains historical path oddities such as:

- `..\..\Graphics APIs\directx\lib\...`

That suggests some base configuration history tied to a developer-specific or internal directory layout.

Interpretation:

- `LevelEdit` will likely be significantly harder to recover than core libraries
- it combines MFC, editor-specific defines, content processing, graphics dependencies, and external SDKs
- it should probably not be the first target in a modernization effort unless editor functionality is the primary goal

## Installer and launcher findings

### `Installer/Installer.dsp`

The installer project expects:

- `Cab\Include`
- `cab\lib\fdi.lib`
- `DirectX`
- `Miles6`
- `Bink`

Direct listing of `Code/Installer/` did not show a `Cab/` subdirectory.

### `Launcher/launcher.dsp`

The launcher project contains a `SafeDisk` group referencing files such as `SafeDisk\CdaPfn.h`.

Direct listing of `Code/Launcher/` did not show a `SafeDisk/` subdirectory.

Interpretation:

- installer and launcher recovery will require separate triage and should not be conflated with core game bring-up
- these components are especially likely to depend on obsolete or non-portable third-party code

## Tool-specific one-off findings

### `Tools/LightMap/LightMap.dsp`

References:

- `Lightscape\Inc`
- `Lightscape\Lib`
- `lvsio.lib`
- `lvsiod.lib`

This looks like a strong candidate for deferred recovery rather than early modernization.

### `Tools/RenegadeGR/RenegadeGR.dsp`

References `jni.h`, but that file was not present in a direct listing of the folder.

This suggests an incomplete source snapshot or historical expectation that JDK headers would be copied into the project directory.

### Small renderer sample/test targets

Targets under `ww3d2/` and `Tests/` reference combinations of:

- `wwcpuid`
- `srsdk1x`
- `DirectX`
- `d3d8.lib`
- `d3dx8.lib`

These are useful clues for dependency mapping even if they are not priority build targets.

## Practical conclusions

1. The `.dsp` files are valuable historical maps, but they describe an environment that no longer exists in the repository.
2. Missing directories are only part of the problem; some existing directories also lack referenced files.
3. The build graph is wide, target-rich, and heavily coupled to proprietary and Windows-specific middleware.
4. A sensible recovery plan will likely prioritize a minimal library subset rather than attempting a full workspace resurrection in one shot.

# Dependency Audit

Last updated: 2026-03-17

## Dependencies explicitly called out by `README.md`

The repository’s own README states that successful rebuilds require locating, rewriting, or removing code related to the following libraries:

- DirectX SDK (version 8.0 or higher), expected under `Code/DirectX/`
- RAD Bink SDK, expected around `Code/BinkMovie/`
- RAD Miles Sound System SDK, expected under `Code/Miles6/`
- NvDXTLib SDK, expected under `Code/NvDXTLib/`
- Lightscape SDK, expected under `Code/Lightscape/`
- Umbra SDK, expected under `Code/Umbra/`
- GameSpy SDK, expected under `Code/GameSpy/`
- GNU Regex, expected under `Code/WWLib/`
- SafeDisk API, expected under `Code/Launcher/SafeDisk/`
- Microsoft Cab Archive Library, expected under `Code/Installer/Cab/`
- RTPatch Library, expected under `Code/Installer/`
- Java Runtime Headers, expected under `Code/Tools/RenegadeGR/`

This list alone is enough to classify the repository as not self-contained.

## Confirmed missing dependency directories (first pass)

The following expected directories were checked and are currently absent from the repository snapshot:

- `Code/DirectX/`
- `Code/Miles6/`
- `Code/NvDXTLib/`
- `Code/Lightscape/`
- `Code/Umbra/`
- `Code/GameSpy/`
- `Code/Launcher/SafeDisk/`

Follow-up direct directory listings also confirmed that these paths are not present as subdirectories in the current tree:

- `Code/Installer/Cab/`
- `Code/Launcher/SafeDisk/`

The `Code/Tools/RenegadeGR/` directory exists, but its project metadata references `jni.h`, which was not present in a direct listing of that folder.

## Repository ignore rules corroborate dependency gaps

The root `.gitignore` explicitly excludes several dependency roots and generated outputs, including:

- `Libs/`
- `Miles6/`
- `Bink/`
- `Lightscape/`
- `NvDXTLib/`
- `Umbra/`
- `GameSpy/`
- `DirectX/`

This strongly suggests the repository was never intended to be dependency-complete in public form.

The same ignore rules also exclude selected generated artifacts such as:

- `*_i.c`
- `/Code/Tools/LevelEdit/vss.h`
- `/Code/Commando/build.bin`

Notably, some generated COM source files such as `wolapi/WOLAPI_i.c` and `WOLBrowser/WOLBrowser_i.c` are present anyway, indicating the snapshot contains a mixed set of generated outputs rather than a consistently reproducible generation pipeline.

## No prebuilt rescue libraries are present under `Code/Libs/`

`Code/Libs/Debug/`, `Code/Libs/Profile/`, and `Code/Libs/Release/` currently contain only `.gitignore` placeholders.

This means there are no checked-in static libraries or DLLs in those output directories that could be used as a temporary bridge while source-level build issues are being repaired.

## Additional missing or ghost paths referenced by project files

Beyond the README’s dependency list, representative `.dsp` files reference other paths that were not found in a repository file inventory sweep:

- `Code/srsdk1x/`
- `Code/srsdk/`
- `Code/wwcpuid/`
- `Code/wwlzhl/`

These may represent historical internal SDKs, helper libraries, compression modules, or engine support code that were not included in the public snapshot.

`Code/Installer/Cab/` still requires a dedicated verification pass, but it is already listed as an external expectation by the README.

## Direct source evidence of dependency use

### RAD Bink

`Code/BinkMovie/BINKMovie.cpp` includes:

- `Bink.h`
- `dx8wrapper.h`
- `dx8caps.h`

And directly uses Bink symbols such as:

- `HBINK`
- `BinkOpen`
- `BinkClose`
- `BinkWait`
- `BinkDoFrame`
- `BinkCopyToBuffer`
- `BinkNextFrame`
- `BinkSoundUseDirectSound`

This is a concrete hard dependency, not a dormant mention.

Project metadata also links against `binkw32.lib` from the main game and installer flows, confirming that Bink is part of the actual link stage and not only used in source.

### RAD Miles Sound System

Representative project files including `Commando`, `Combat`, `WWAudio`, `LevelEdit`, `Installer`, `WWConfig`, and `W3DView` reference:

- `..\miles6\include`
- `..\miles6\lib\win\mss32.lib`

This indicates Miles is a pervasive dependency across both runtime and tool targets, not an isolated optional component.

### DirectX 8

DirectX-era headers are in active use across the codebase. Evidence includes:

- `d3d8.h` in `Code/ww3d2/`
- `dinput.h` in gameplay/UI code such as `Combat/` and `Commando/`
- `ddraw.h` in `Code/wwlib/dsurface.h`
- Direct3D 8 interfaces in `BinkMovie/BINKMovie.cpp` such as `IDirect3DTexture8`

This implies:

- graphics modernization is a major task
- input abstraction is needed
- DirectX 8 assumptions likely permeate renderer, UI, tools, and media playback code

Representative linker inputs include:

- `d3dx8.lib`
- `d3d8.lib`
- `dinput.lib`
- `dinput8.lib`
- `ddraw.lib`
- `dsound.lib`
- `dxguid.lib`

The dependency is therefore both compile-time and link-time.

### Umbra

`wwphys.dsp` references:

- include path `..\umbra\interface`
- source files `umbrasupport.cpp` and `umbrasupport.h`

`commando.dsp` and `WWConfig.dsp` also reference Umbra library paths or libraries such as:

- `/libpath:"../umbra/lib/win32-x86"`
- `umbra.lib`
- `umbrad.lib`

This is a concrete visibility/occlusion-related dependency, not just a README mention.

### Lightscape

`Tools/LightMap/LightMap.dsp` references:

- `..\..\Lightscape\Inc`
- `..\..\Lightscape\Lib`
- `lvsio.lib`
- `lvsiod.lib`

This strongly suggests a proprietary or external light-solving / radiosity-related integration used by tooling.

### NvDXTLib

`Tools/LevelEdit/LevelEdit.dsp` references:

- `..\..\NvDXTLib\Inc`
- `..\..\NvDXTLib\Lib`
- `NvDXTLib.lib`

This indicates texture compression or DXT processing functionality in the editor pipeline.

### SafeDisk

`Launcher/launcher.dsp` contains a `SafeDisk` group with entries such as:

- `SOURCE=.\SafeDisk\CdaPfn.h`

But the `Code/Launcher/` directory listing did not contain a `SafeDisk/` subdirectory.

This is a concrete example of repository incompleteness inside an otherwise present project folder.

### Microsoft Cabinet / FDI support

`Installer/Installer.dsp` references:

- include path `Cab\Include`
- library `cab\lib\fdi.lib`

However, a direct listing of `Code/Installer/` did not show a `Cab/` subdirectory.

This makes the installer project currently incomplete even before considering other external dependencies.

### Java / JNI headers for `RenegadeGR`

`Tools/RenegadeGR/RenegadeGR.dsp` references `SOURCE=.\jni.h`, but a direct listing of `Code/Tools/RenegadeGR/` did not contain `jni.h`.

This suggests either:

- the JNI header was expected to be copied into the source folder historically, or
- the public snapshot is missing one or more source/header files for that tool.

### SourceSafe / COM-generated editor artifacts

`Tools/LevelEdit/LevelEdit.dsp` references `VSS.IDL`, and the repository contains `ssauto_i.c`, but `Code/Tools/LevelEdit/vss.h` was not present in a file search and is explicitly ignored by the root `.gitignore`.

This is a strong indicator that at least some tool builds depended on locally generated COM/IDL headers that are not consistently included in the repository.

### Winsock / Windows networking stack

Winsock appears in multiple subsystems:

- `Code/SControl/servercontrolsocket.h`
- `Code/wwnet/packetmgr.h`
- `Code/wwnet/netutil.h`
- `Code/Commando/natsock.h`
- several test and tool modules

This is not inherently proprietary, but it is a portability constraint and may require API modernization from old Winsock patterns.

### MFC / ATL / COM usage

Observed examples:

- `afxwin.h` in `Tools/WWCtrl/StdAfx.h`
- `afxwin.h` in `Tools/WWConfig/StdAfx.h`
- `atlbase.h` in `Commando/DlgWebPage.h` and `Commando/WebBrowser.h`
- `rpc.h`, `rpcndr.h`, `ole2.h` in `WOLBrowser/WOLBrowser.h`

These dependencies increase the cost of cross-platform migration for tools and embedded browser/UI functionality.

## Dependency categories

### Category A: missing and proprietary / non-redistributable

These are likely the biggest blockers to a reproducible public build:

- RAD Bink
- RAD Miles
- SafeDisk
- RTPatch (depending on licensing and provenance)
- GameSpy SDK
- likely vendor SDKs such as Lightscape and Umbra
- NvDXTLib may also need replacement depending on redistribution rights and desired target platforms

### Category B: obsolete but replaceable platform SDKs

- DirectX 8 SDK
- legacy DirectInput / DirectDraw / Direct3D 8 usage
- older Windows shell/UI libraries

These are not “missing” in the same way, but the code targets obsolete APIs and historical SDK layouts.

### Category C: replaceable with open-source alternatives

Potential future replacement directions include:

- Bink → open video playback stack such as FFmpeg + SDL / engine-native decoder path
- Miles → OpenAL Soft, SDL_mixer, FMOD Studio API, or a custom mixer depending on goals
- GameSpy → custom online services layer or community master-server implementation
- DirectInput → SDL input, raw platform abstraction, or a modern cross-platform window/input layer
- Direct3D 8 renderer → modern graphics abstraction (D3D11/12, Vulkan, OpenGL, bgfx, SDL_gpu, etc.)

These are strategy notes only; no replacement decision has been made yet.

## Immediate conclusions

1. The repository currently cannot be considered self-contained.
2. Some missing dependencies are confirmed by absent directories and by live source references.
3. Several dependencies are not just old; they are obsolete proprietary middleware that must be replaced, stubbed, or surgically removed.
4. The dependency problem is intertwined with platform entanglement, especially around Windows multimedia and UI APIs.
5. Project metadata reveals additional missing internal support libraries beyond the README, notably `srsdk1x`, `srsdk`, `wwcpuid`, and `wwlzhl`.
6. Some projects are incomplete even when their containing directory exists, as shown by missing `SafeDisk`, `Cab`, and `jni.h` references.
7. The repository’s own ignore rules reinforce that many dependency roots and generated artifacts were expected to be local-only.
8. There are no prebuilt fallback libraries checked into `Code/Libs/`.

## Follow-up work needed

- inspect representative `.dsp` files for exact include/lib paths and linker inputs
- verify whether any binary SDK remnants exist under `Code/Libs/` or elsewhere
- catalog additional proprietary/third-party references not mentioned in the README
- determine which dependencies are required for the main game versus only for tools, installers, or historical utilities

Much of the first item is now underway; future passes should expand it from representative samples to a repo-wide matrix.

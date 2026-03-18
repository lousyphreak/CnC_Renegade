# Build System and Project Inventory

Last updated: 2026-03-17

## Primary build system state

The repository is organized around legacy Microsoft Visual C++ workspace and project files:

- primary workspace: `Code/commando.dsw`
- secondary workspace: `Code/tools.dsw`
- many per-project `.dsp` files throughout the tree
- at least one newer solution file exists: `Code/Tools/RenegadeSim/RenegadeSim.sln`

This is not a modern portable build system. There is no top-level `CMakeLists.txt`, no Meson configuration, no Premake scripts, and no obvious unified cross-platform build description.

## README-stated build path

The `README.md` describes the canonical build route as:

1. open `Code/commando.dsw` in Visual Studio C++ 6.0
2. batch rebuild all configurations
3. optionally convert through Visual Studio .NET 2003 before newer MSVC versions

Implications:

- the repo still assumes a historical Microsoft toolchain
- project metadata likely encodes old include/lib paths and compiler switches
- modern compiler support is explicitly expected to require extensive code changes

## Projects referenced by `Code/commando.dsw`

First-pass project list observed in the workspace:

- `BandTest`
- `BinkMovie`
- `Combat`
- `GameSpy`
- `LevelEdit`
- `SControl`
- `Scripts`
- `WWAudio`
- `WWConfig`
- `WWCtrl`
- `commando`
- `max2w3d`
- `ww3d2`
- `wwbitpack`
- `wwdebug`
- `wwlib`
- `wwmath`
- `wwnet`
- `wwphys`
- `wwsaveload`
- `wwtranslatedb`
- `wwui`
- `wwutil`

The `commando` workspace also records explicit project dependencies for the main game target. The `commando` project depends on:

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

This is useful because it sketches the minimum historical dependency chain needed to link the main executable.

## Projects referenced by `Code/tools.dsw`

The secondary tool workspace contains a large standalone build graph. Observed project entries include:

- `AMC_imp`
- `Blender2`
- `ChunkView`
- `Clipbord`
- `CommandoUpdate`
- `CopyLocked`
- `LevelEdit`
- `LightMap`
- `MakeMix`
- `MaxFly`
- `MixViewer`
- `RenRem`
- `SimpleGraph`
- `SkeletonHack`
- `VerStamp`
- `VidInit`
- `ViewTrans`
- `W3DShellExt`
- `W3DUpdate`
- `W3DView`
- `WWConfig`
- `WWCtrl`
- `asf_imp`
- `bin2cpp`
- `max2w3d`
- `pluglib`
- `wdump`

This confirms that the original development environment was split across at least two major VC6 workspaces: one for the runtime/game stack and one for authoring, maintenance, conversion, and support tools.

## Confirmed workspace inconsistency

### `GameSpy` project is referenced but absent

`Code/commando.dsw` contains a project entry:

- `Project: "GameSpy"=.\GameSpy\GameSpy.dsp`

However, the current repository layout does not contain a `Code/GameSpy/` directory.

This is a hard evidence item that the workspace is currently incomplete relative to the files needed to open or build all projects as originally configured.

## Build modernization blockers already visible

### 1. Obsolete project format

`.dsw` and `.dsp` are VC6-era artifacts. Modernizing will require at least one of:

- generating a new build system from source structure
- converting project-by-project to modern MSBuild `.vcxproj`
- leapfrogging directly to CMake/Meson and treating old project files as historical references only

### 2. Windows-only build assumptions

The codebase makes heavy use of:

- `windows.h`
- `commctrl.h`
- `shellapi.h`
- `shlobj.h`
- `winsock.h` / `winsock2.h`
- `dinput.h`
- `d3d8.h`
- `ddraw.h`
- MFC headers such as `afxwin.h`
- ATL headers such as `atlbase.h`
- COM/RPC headers such as `rpc.h`, `rpcndr.h`, `ole2.h`

A future cross-platform build system cannot be created cleanly until these dependencies are either isolated behind platform interfaces or replaced.

### 3. Multi-binary ecosystem complexity

This is not a single-target migration. The build system spans:

- game runtime
- engine libraries
- tools
- installers / launchers / updaters
- tests and demos
- plugins / shell extensions / conversion tools

This means a modernization effort will need target prioritization. A practical path will likely focus first on core runtime/engine libraries rather than all historical tools.

### 4. Project files embed missing include and library roots

Representative `.dsp` files reference several include or library paths that are not present in the checked-in repository tree, including:

- `..\DirectX\include`
- `..\miles6\include`
- `..\GameSpy`
- `..\umbra\interface`
- `..\umbra\lib\win32-x86`
- `..\srsdk1x\include`
- `..\srsdk1x\msvc6\lib\...`
- `..\srsdk\include`
- `..\wwcpuid`
- `..\wwlzhl`
- `..\..\Lightscape\Inc`
- `..\..\Lightscape\Lib`
- `..\..\NvDXTLib\Inc`
- `..\..\NvDXTLib\Lib`
- `Cab\Include`
- `Cab\lib`

These are not merely comments in the README; they are active compiler and linker settings in build metadata.

### 5. Multiple target types are in play

Representative project files show different target kinds:

- `Commando/commando.dsp` → `Win32 (x86) Application`
- `Combat/Combat.dsp` → `Win32 (x86) Static Library`
- `ww3d2/ww3d2.dsp` → `Win32 (x86) Static Library`
- `WWAudio/WWAudio.dsp` → `Win32 (x86) Static Library`
- `Tools/LevelEdit/LevelEdit.dsp` → `Win32 (x86) Application`

This matters when reconstructing a modern build graph because the project structure is not flat; there is a real distinction between core libraries and final applications.

## Preliminary migration recommendations

### Suggested phase ordering

1. **Recover project graph**
   - enumerate every `.dsp` target
   - identify outputs, dependencies, include paths, and missing project references
2. **Define a reduced bootstrap build**
   - select the minimum set of libraries/executables needed to build the core runtime
3. **Introduce a modern meta-build**
   - likely CMake for portability and IDE support
4. **Keep historical projects as references only**
   - use `.dsp` files to infer include paths and preprocessor definitions, not as the long-term build system
5. **Split platform-neutral code from Win32-specific layers**
   - this is required before genuine multi-platform work begins

## Open questions for later passes

- which projects are static libraries vs DLLs vs EXEs?
- which `.dsp` files depend on missing third-party SDKs directly?
- whether any project files contain hard-coded absolute paths or developer-machine assumptions
- whether the lone `.sln` under `RenegadeSim` is a useful modernization foothold or an unrelated island

Some of these questions are now partially answered, but they still need a complete repository-wide pass rather than representative sampling.

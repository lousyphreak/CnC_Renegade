# Generated Artifacts and Repository Gaps

Last updated: 2026-03-17

This document records repository-completeness issues that go beyond third-party SDKs: ignored/generated files, empty output directories, selectively included COM artifacts, and other signs that the published tree is not a reproducible standalone source package.

## What `.gitignore` says about repository intent

The root `.gitignore` excludes several important categories:

### Dependency roots

- `Libs/`
- `Miles6/`
- `Bink/`
- `Lightscape/`
- `NvDXTLib/`
- `Umbra/`
- `GameSpy/`
- `DirectX/`

Interpretation:

- these were expected to exist locally in a historical development environment
- the repository should be treated as a source snapshot with external dependencies omitted by design

### Build outputs and intermediates

- `Release*`
- `Profile*`
- `Debug*`
- many Visual Studio-generated files such as `.obj`, `.pdb`, `.ilk`, `.pch`, `.res`, `.tlb`, `.sbr`

Interpretation:

- no prebuilt fallback outputs should be expected in version control

### Generated files

- `*_i.c`
- `/Code/Tools/LevelEdit/vss.h`
- `/Code/Commando/build.bin`

Interpretation:

- at least some COM/IDL outputs and generated headers were historically machine-local or regeneration-based
- if the corresponding generation tooling is not fully preserved, these become silent build blockers

## `Code/Libs/` is present but effectively empty

Direct listings of:

- `Code/Libs/Debug/`
- `Code/Libs/Profile/`
- `Code/Libs/Release/`

showed only `.gitignore`.

Interpretation:

- there are no checked-in static libraries, DLLs, or executables in those output trees
- modern recovery work cannot rely on existing binary outputs as temporary scaffolding

## Selectively present generated COM/IDL artifacts

Some generated COM artifacts are present in the repository:

- `Code/wolapi/WOLAPI_i.c`
- `Code/WOLBrowser/WOLBrowser_i.c`
- `Code/Tools/LevelEdit/ssauto_i.c`
- `Code/Tools/LevelEdit/VSS.IDL`

These show that parts of the COM/IDL pipeline were checked in.

However, the repository is not consistent about it.

## Missing or inconsistent generated/editor artifacts

### `Code/Tools/LevelEdit/vss.h`

Findings:

- `LevelEdit` project files and sources reference VSS/SourceSafe-related COM material
- `Code/Tools/LevelEdit/vss.h` was not found in a file search
- the root `.gitignore` explicitly ignores `/Code/Tools/LevelEdit/vss.h`

Interpretation:

- editor builds may have depended on locally generated or locally copied SourceSafe headers
- this is a concrete missing piece independent of the big SDK dependencies

### `Code/Tools/RenegadeGR/jni.h`

Findings:

- `RenegadeGR.dsp` references `SOURCE=.\jni.h`
- direct directory listing of `Code/Tools/RenegadeGR/` did not show `jni.h`
- no `jni.h` was found anywhere under `Code/`

Interpretation:

- Java/JNI tooling support is incomplete in the checked-in tree

### `Code/Commando/build.bin`

Findings:

- root `.gitignore` explicitly ignores `/Code/Commando/build.bin`
- `Commando/buildnum.cpp` contains placeholder-style build metadata strings

Interpretation:

- historical build numbering/version stamping likely relied on an external generation step

## COM/RPC integration is real, not cosmetic

Direct evidence shows active COM usage, not just leftover headers:

- `WWOnline/WOLSession.cpp` uses `CoCreateInstance` with WOL CLSIDs
- `WWOnline/WOLDownload.cpp` uses COM activation for download services
- `Commando/WebBrowser.cpp` uses `CLSID_WOLBrowser`
- `Installer/Installer.cpp` uses `CLSID_ShellLink` and `CLSID_InternetShortcut`

Interpretation:

- any attempt to rebuild or port online/browser/installer features must account for actual COM runtime behavior, not just generated headers

## Additional repository-gap observations

### Historical documentation outside the repo is referenced

`Commando/commando.dsp` contains shortcut/source entries pointing to paths such as:

- `..\..\..\RenegadeMe\Doc\todo.txt`
- `..\..\..\RenegadeMe\Doc\currentbugs.txt`
- `..\..\..\RenegadeMe\Doc\priorities.txt`

Interpretation:

- some historical developer notes and process documents lived outside this repository snapshot
- they are not required for compilation, but they are evidence that the public tree is only part of the original development environment

### Output/runtime assumptions are Windows-centric

Examples observed include:

- hard-coded installer defaults like `C:\Westwood\Renegade`
- registry-based installation discovery
- DLL loading by fixed Windows filename (`D3D8.DLL`, `DINPUT8.DLL`, `IMAGEHLP.DLL`, `patchw32.dll`)

Interpretation:

- even if source code compiles, some runtime assumptions will still fail outside the original Windows deployment model

## Practical conclusions

1. Repository incompleteness is broader than missing SDK directories.
2. Some missing pieces are generated headers or machine-local artifacts, not just third-party middleware.
3. The presence of some generated COM files but not others suggests the codebase was captured from a live working tree, not re-exported from a clean reproducible build.
4. Empty `Code/Libs/` output directories remove the option of temporarily leaning on checked-in binaries.
5. Tooling and online-related builds are especially vulnerable to missing local artifacts and COM generation drift.

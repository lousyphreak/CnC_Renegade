# Hardcoded Paths and Case-Sensitivity Audit

Last updated: 2026-03-17

This document captures source-level and project-level path hazards that are easy to underestimate during build recovery: absolute machine paths, UNC shares, mixed-case include usage, backslash includes, and hardcoded runtime DLL names.

## Why this matters

These issues are not as dramatic as missing SDKs, but they are often what turns an almost-working migration into a death-by-a-thousand-paper-cuts exercise.

## Missing local headers referenced directly in source

Confirmed examples:

- `BinkMovie/BINKMovie.cpp` → `#include "Bink.h"`
- `Tools/LevelEdit/VSSClass.h` → `#include "vss.h"`
- `Tools/RenegadeGR/RenegadeNet.h` → `#include "jni.h"`
- `Tools/RenegadeGR/RenegadeGR.cpp` → `#include "jni.h"`
- `Launcher/Protect.cpp` → `#include "SafeDisk\CdaPfn.h"`

Interpretation:

- some missing pieces are not merely linker inputs or project metadata references; they are direct compile blockers in source files

## Hardcoded absolute developer-machine paths

### Vintage 3ds Max SDK paths

Representative project-file references include:

- `d:\3dsmax2\maxsdk\include`
- `d:\3dsmax2\maxsdk\lib`

Observed in:

- `Tools/Clipbord/Clipbord.dsp`
- `Tools/Blender2/Blender2.dsp`
- similar plugin/exporter projects in the toolchain

Interpretation:

- these tools are tied to a specific local installation of a very old DCC toolchain
- they are poor candidates for direct modern build recovery

### Hardcoded local install targets

Examples include launcher/project output paths such as:

- `c:\renegade\cdver\renegade.exe`

This is another sign that project files were written against internal/local deployment assumptions rather than relocatable builds.

## Hardcoded UNC paths and corporate network shares

Representative examples:

- `Tools/LevelEdit/FileLocations.h`
  - `\\mobius\project7\projects\renegade\asset management\asset database\srcsafe.ini`
  - `\\mobius\project7\projects\renegade\art\_source\texturemaps`
- `Tools/LevelEdit/PresetLogger.cpp`
  - writes to `\\mobius\project7\projects\renegade\asset management\logs\presets.log`
- `Tools/LevelEdit/Utils.cpp`
  - references `\\mobius\project7\projects\renegade\programming\tools\level edit\`
- `Tools/CommandoUpdate/CommandoUpdateDlg.cpp`
  - references several `\\mobius\project7\projects\renegade\...` tool locations
- `Tools/W3DUpdate/W3DUpdateDlg.cpp`
  - references network asset locations on the same share
- `Tools/LightMap/LightMap.cpp`
  - contains hardcoded `\\Mobius\Project7\Projects\Renegade\Programming\Tools\Lightmap` paths

Interpretation:

- parts of the tool ecosystem assumed access to an internal network share and shared asset database
- these assumptions are incompatible with public reproducibility and modern decentralized workflows

## Runtime DLL name assumptions baked into source

Representative examples:

- `ww3d2/dx8wrapper.cpp` and `Commando/WINMAIN.CPP` load `D3D8.DLL`
- `Combat/directinput.cpp` loads `DINPUT8.DLL`
- `wwutil/stackdump.cpp`, `Combat/debug.cpp`, and `wwlib/Except.cpp` load `IMAGEHLP.DLL`
- `Launcher/patch.cpp` loads `patchw32.dll`

Interpretation:

- even once source compiles, runtime assumptions still bind the code to a specific Windows DLL ecosystem

## Mixed-case and backslash include hazards

Representative examples:

- `BinkMovie/subtitleparser.h` → `#include <wwlib\vector.h>`
- `BinkMovie/subtitlemanager.h` → `#include <wwlib\vector.h>`
- `Commando/AnnounceEvent.cpp` → `#include <wwlib\widestring.h>`
- many `Commando/*` files use forms such as `#include <WWLib\Notify.h>`, `#include <WWLib\Signaler.h>`, `#include <WWLib\WideString.h>`
- `Tools/WWCtrl/Utils.cpp` and other files include `StdAfx.H`
- `Tools/LevelEdit` mixes `stdafx.h`, `StdAfx.h`, and `StdAfx.H`

Why this is risky:

- backslash path separators in includes are Windows-centric
- mixed-case include paths are usually tolerated on Windows but fail on case-sensitive filesystems
- case normalization work will likely be required even for a future Windows build using more modern, stricter tooling

## Practical consequences

These issues affect multiple migration goals:

- **modern Windows builds**: stricter tooling and cleaned-up include paths are still needed
- **Linux/macOS ports**: case sensitivity and backslash includes become immediate blockers
- **CI/reproducible builds**: hardcoded UNC shares and local install paths have to be removed or abstracted

## Suggested handling strategy

1. Create a path/host-assumption audit checklist during any build-system conversion.
2. Normalize include casing and slash direction early, especially in shared/runtime code.
3. Replace hardcoded UNC and drive-letter paths with configurable roots.
4. Treat missing local headers (`vss.h`, `jni.h`, `SafeDisk\...`, `Bink.h`) as first-class repository gaps, not incidental errors.
5. Isolate runtime DLL loading behind platform/service abstraction points.

## Bottom line

The repository contains a large amount of historical environmental coupling at the source level: local drives, corporate shares, Windows-style include paths, mixed casing, and direct DLL names. These issues are secondary to the missing middleware problem, but they will absolutely matter during real-world build recovery and portability work.

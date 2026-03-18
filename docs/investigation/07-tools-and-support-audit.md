# Tools and Support Audit

Last updated: 2026-03-17

This document focuses on the non-runtime side of the repository: editors, asset tools, viewers, converters, installer/launcher code, patching support, and other historical utilities.

## Why this matters

The `Code/Tools/` tree is the largest part of the repository by file count. Even if the main game executable is made buildable, the project will remain hard to maintain unless the asset pipeline and support utilities are understood and triaged.

## Tool and support subsystem map

## Core content and editor tools

### `Tools/LevelEdit/`

Appears to be:

- the main level/world editor
- MFC-based
- DirectX 8-based for rendering/editor visualization
- integrated with SourceSafe / VSS for asset database workflows

Why it is important:

- this is likely the central historical content-authoring tool
- long-term maintainability of maps/world content probably depends on replacing or preserving its functionality

### `Tools/W3DView/`

Appears to be:

- a model/animation/viewer utility for W3D assets

Why it matters:

- useful for validating asset formats during future migration
- a likely candidate for preservation or rewrite as a lightweight viewer

### `Tools/LightMap/`

Appears to be:

- an offline lighting/lightmap-related tool
- dependent on Lightscape and SourceSafe-era infrastructure

Why it matters:

- likely important to historical asset baking
- probably not essential for first runtime bring-up

### `Tools/MakeMix/`, `Tools/MixViewer/`, `Tools/ChunkView/`, `Tools/bin2cpp/`

These appear to be:

- archive/package utilities
- binary inspection/conversion tools
- general pipeline helpers

Why they matter:

- they are useful for reverse-engineering and asset format preservation
- they are less critical than the main editor, but often easier to salvage

## Exporter / plugin ecosystem

Several tool projects are clearly 3ds Max plugin-era exporters/importers, including:

- `Tools/max2w3d/`
- `Tools/Clipbord/`
- `Tools/Blender2/`
- `Tools/AMC_IMP/`
- `Tools/ASF_IMP/`
- `Tools/MaxFly/`

These are especially fragile because they depend on a specific historical DCC environment rather than just generic C/C++ SDKs.

## Launcher, installer, and updater family

### `Code/Launcher/`

Appears to contain:

- launcher logic
- patch application support
- serial/DRM/protection logic
- RTPatch integration
- Blowfish-based helper code

### `Code/Installer/`

Appears to contain:

- setup application UI
- shell shortcut creation
- CAB-related support
- registry/configuration logic
- multimedia integration via Bink/Miles

### Updater/support tools

Relevant projects include:

- `Tools/CommandoUpdate/`
- `Tools/W3DUpdate/`
- `Launcher/patch.cpp`

These reinforce that shipping/updating the game historically involved custom Windows tooling rather than generic package management.

## Other support utilities and side projects

### `Tools/RenegadeGR/`

Appears to be a support/network/recording-related component with missing `jni.h` reference in project metadata.

### `Tools/RenegadeSim/`

Direct listing and file search show:

- `.java` sources
- `RenegadeSim.vjp`
- `RenegadeSim.sln`

Interpretation:

- this toolchain was not purely native C++; there was also a Java-era simulation/support project in the tree

### `Code/SControl/`

Appears to be remote server-control support used by the main game/server ecosystem.

## Build and dependency blockers specific to tools/support apps

## Hardcoded 3ds Max SDK paths

Several exporter/importer plugin projects reference hardcoded paths such as:

- `d:\3dsmax2\maxsdk\include`
- `d:\3dsmax2\maxsdk\lib`

Confirmed examples include:

- `Tools/Clipbord/Clipbord.dsp`
- `Tools/Blender2/Blender2.dsp`

Implication:

- these projects are effectively non-buildable without a specific vintage 3ds Max SDK installation
- this is a much worse problem than a generic missing dependency because the tool host itself is obsolete and proprietary

## SourceSafe / VSS integration is pervasive in `LevelEdit`

Representative evidence includes:

- `Tools/LevelEdit/VSS.IDL`
- `Tools/LevelEdit/VSSClass.cpp`
- `Tools/LevelEdit/FileMgr.cpp`
- `Tools/LevelEdit/LevelEditDoc.cpp`
- `Tools/LevelEdit/PresetsLibForm.cpp`
- resource/UI strings explicitly mentioning VSS and asset database workflows

Notable details:

- `LevelEdit.dsp` expects generated `vss.h`
- `FileLocations.h` contains a hardcoded UNC path:
  - `\\mobius\project7\projects\renegade\asset management\asset database\srcsafe.ini`
- `LevelEdit.rc` contains messaging around `SSUSER` environment variables and VSS read-only mode

Implication:

- this editor is tightly coupled to a historical corporate SourceSafe asset database environment
- a modern replacement will need a completely different asset/version-control model

## MFC-heavy tool surface

Project metadata shows many tools using MFC or `_AFXDLL`, including examples such as:

- `Tools/ChunkView/ChunkView.dsp`
- `Tools/WWCtrl/WWCtrl.dsp`
- `Tools/CommandoUpdate/CommandoUpdate.dsp`
- `Tools/LightMap/LightMap.dsp`
- `Tools/WWConfig/WWConfig.dsp`
- `Tools/LevelEdit/LevelEdit.dsp`

Implication:

- even tools without exotic middleware may still require significant Windows/MFC modernization work

## DirectX 8 and multimedia dependence in tools

Representative dependencies include:

- `LevelEdit` → DirectX 8 + DirectInput + NvDXTLib + Miles
- `W3DView` → DirectX 8 + Miles
- `LightMap` → DirectX 8-era libraries plus Lightscape/SRSDK
- `Installer` → Bink + Miles + DirectX 8 libs

Implication:

- tooling modernization is not separable from renderer/media modernization

## RTPatch and SafeDisk in launcher/updater code

Representative evidence:

- `Launcher/launcher.dsp` links `patchw32.lib`
- `Launcher/patch.cpp` loads `patchw32.dll` and looks up `RTPatchApply32@12`
- `Launcher/launcher.dsp` has a `SafeDisk` group
- `Launcher/Protect.cpp` includes `SafeDisk\CdaPfn.h`

Implication:

- the legacy launcher/update path is anchored in obsolete proprietary delivery/DRM systems
- this is not worth resurrecting verbatim for a modern project

## Missing pieces and repo gaps specific to tools/support code

Key tool/support gaps include:

- missing `Code/Launcher/SafeDisk/`
- missing `Code/Installer/Cab/`
- missing `Code/Tools/RenegadeGR/jni.h`
- missing `Code/Tools/LevelEdit/vss.h`
- missing `Code/srsdk1x/`
- missing `Code/Lightscape/`
- missing `Code/NvDXTLib/`
- missing `Code/DirectX/`
- missing `Code/Miles6/`

These are on top of the runtime-side gaps such as `GameSpy`, `Umbra`, `wwcpuid`, and `wwlzhl`.

## Which tools are probably critical vs optional

## Likely critical to long-term maintainability

### `LevelEdit`

Reason:

- appears to be the primary world/content editor
- deeply tied to authoring workflows and asset management

### `Scripts`

Reason:

- scripting/gameplay extension path appears essential if the project is to remain moddable or gameplay-changeable

### `W3DView` / package utilities (`MakeMix`, `MixViewer`)

Reason:

- useful for asset inspection, validation, and migration
- likely much easier to preserve or rewrite than the full editor

## Historically important but probably not worth direct preservation in current form

### 3ds Max plugin family

Reason:

- hardwired to vintage Max SDK layout
- likely better treated as reverse-engineering references for a future standalone converter or Blender-based path

### `LightMap` in current form

Reason:

- tightly coupled to Lightscape and SourceSafe-era assumptions

### `W3DShellExt`

Reason:

- Explorer shell extension value is low relative to maintenance cost

### launcher/installer DRM/update stack

Reason:

- modern projects do not need SafeDisk/RTPatch revival
- replacement is preferable to preservation

## Suggested modernization / triage order for tools and support components

### Tier 1: preserve the minimum viable content pipeline

Best candidates:

- `Scripts`
- `W3DView`
- `MakeMix`
- `MixViewer`
- other smaller format/package utilities

Rationale:

- these provide immediate value for understanding assets and gameplay data without demanding a full editor rewrite

### Tier 2: replace rather than port the distribution stack

Targets:

- `Launcher`
- `Installer`
- updater utilities

Rationale:

- rewrite with modern packaging/update methods instead of reviving SafeDisk, RTPatch, CAB, old shell APIs, and codec integrations

### Tier 3: plan an editor succession strategy

Target:

- `LevelEdit`

Rationale:

- likely too important to ignore long-term
- likely too coupled and fragile to be the first modernization target
- better to document its workflows thoroughly, then replace or rehost functionality gradually

### Tier 4: archive the obsolete plugin ecosystem as reference

Targets:

- all hardcoded 3ds Max plugins/importers/exporters

Rationale:

- use as documentation of W3D/export semantics
- do not expect direct modern build recovery to be practical

## Especially risky or obsolete files/components

High-risk support/tooling components observed so far:

- `Tools/LevelEdit/VSS.IDL`
- `Tools/LevelEdit/VSSClass.cpp`
- `Tools/LevelEdit/FileMgr.cpp`
- `Tools/LevelEdit/LevelEditDoc.cpp`
- `Tools/LevelEdit/FileLocations.h`
- `Launcher/patch.cpp`
- `Launcher/Protect.cpp`
- `Installer/Installer.cpp`
- `Installer/Utilities.cpp`
- `Tools/max2w3d/*`
- `Tools/Clipbord/Clipbord.dsp`
- `Tools/Blender2/Blender2.dsp`
- `Tools/RenegadeGR/*`
- `Tools/RenegadeSim/*`

## Bottom line

The tools/support side of the repository is not an afterthought; it is a second major modernization project. The biggest problems are concentrated in `LevelEdit`, the 3ds Max exporter family, and the launcher/installer/update stack. For long-term project health, the likely winning strategy is to preserve smaller inspection/utilities, replace the distribution stack, and treat the main editor as a succession project rather than a straightforward port.

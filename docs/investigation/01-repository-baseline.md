# Repository Baseline

Last updated: 2026-03-17

## Root structure

Top-level repository layout observed:

- `.git/`
- `Code/`
- `Run/`
- `README.md`
- `LICENSE.md`

The code and tools are almost entirely concentrated under `Code/`.

## Major top-level directories under `Code/`

Observed directories:

- `BandTest/`
- `BinkMovie/`
- `Combat/`
- `Commando/`
- `Installer/`
- `Launcher/`
- `Libs/`
- `SControl/`
- `Scripts/`
- `Tests/`
- `Tools/`
- `wolapi/`
- `WOLBrowser/`
- `ww3d2/`
- `WWAudio/`
- `wwbitpack/`
- `wwdebug/`
- `wwlib/`
- `WWMath/`
- `wwnet/`
- `WWOnline/`
- `wwphys/`
- `wwsaveload/`
- `wwtranslatedb/`
- `wwui/`
- `wwutil/`

## Quantitative inventory

Approximate per-directory file counts gathered during the first pass:

| Directory | Approx. file count |
| --- | ---: |
| `BandTest` | 6 |
| `BinkMovie` | 9 |
| `Combat` | 266 |
| `Commando` | 436 |
| `Installer` | 55 |
| `Launcher` | 66 |
| `Libs` | 3 |
| `SControl` | 5 |
| `Scripts` | 100 |
| `Tests` | 140 |
| `Tools` | 1429 |
| `wolapi` | 11 |
| `WOLBrowser` | 4 |
| `ww3d2` | 264 |
| `WWAudio` | 47 |
| `wwbitpack` | 10 |
| `wwdebug` | 9 |
| `wwlib` | 263 |
| `WWMath` | 80 |
| `wwnet` | 34 |
| `WWOnline` | 49 |
| `wwphys` | 196 |
| `wwsaveload` | 34 |
| `wwtranslatedb` | 11 |
| `wwui` | 87 |
| `wwutil` | 7 |

## Extension-level inventory

Most common file extensions found in the repository on the first pass:

| Extension | Count |
| --- | ---: |
| `.h` | 1685 |
| `.cpp` | 1481 |
| `.ico` | 140 |
| `.bmp` | 76 |
| `.dsp` | 61 |
| `.rc` | 31 |
| `.w3d` | 22 |
| `.rc2` | 15 |
| `.dsw` | 14 |
| `.tga` | 11 |
| `.txt` | 11 |
| `.def` | 11 |
| `.c` | 9 |

Interpretation:

- this is a large monorepo, not a small isolated game project
- there is a substantial Windows GUI / resource / tooling surface
- the asset and tool formats are part of the development story, not just the runtime game binary

## Immediate structural observations

### 1. The repository contains far more than the main game executable

The presence of major subsystems and tool areas suggests a broad internal engine + game + authoring tool stack:

- runtime/game logic: `Commando/`, `Combat/`
- engine/rendering: `ww3d2/`, `wwphys/`, `WWMath/`, `wwlib/`
- audio: `WWAudio/`
- networking / online: `wwnet/`, `WWOnline/`, `wolapi/`, `WOLBrowser/`
- UI and localization: `wwui/`, `wwtranslatedb/`
- installers / launchers / patchers: `Installer/`, `Launcher/`
- content/toolchain: `Tools/`
- tests / experiments: `Tests/`, `BandTest/`

### 2. Tooling is a first-class concern

`Code/Tools/` is the largest area in the repository. Any modernization plan that ignores tools will likely fail in practice, because the pipeline for producing or editing game data appears deeply coupled to the codebase.

### 3. Repository completeness is suspect by design

The `README.md` explicitly states that rebuilds require locating or replacing multiple external SDKs. Therefore, the repository should be treated as intentionally incomplete from a build reproducibility perspective.

### 4. The codebase is preservation-grade, not turnkey

The repository appears published for historical preservation and reference rather than direct modern compilation. This is reinforced by the README’s note that the archive is provided without support.

### 5. Several build-time paths point outside the currently present tree

Representative project files reference directories or SDK roots that are not present in the repository snapshot, including:

- `DirectX`
- `Miles6`
- `GameSpy`
- `Umbra`
- `Lightscape`
- `NvDXTLib`
- `srsdk1x`
- `srsdk`
- `wwcpuid`
- `wwlzhl`
- `Installer/Cab`
- `Launcher/SafeDisk`

This means the repository is incomplete not just at a high README level, but at the level of actual compiler and linker inputs.

## First-pass risk assessment

### High risk

- missing third-party SDKs
- legacy VC6 project system
- extensive Win32 / DirectX 8 coupling
- likely compiler incompatibilities with modern MSVC / Clang / GCC
- possible assumptions about 32-bit layout, ABI, packing, and inline assembly

### Medium risk

- custom tools with MFC/ATL/COM dependencies
- installer / launcher code that depends on obsolete third-party components
- older networking stack assumptions (Winsock 1.x / 2.x era patterns)

### Lower but still relevant risk

- tests and support utilities that may bit-rot independently from the main game
- project references that may only matter for some historical workflows

## What needs deeper inspection next

- exact workspace/project graph from `commando.dsw` and `tools.dsw`
- include/lib path expectations in representative `.dsp` files
- missing referenced directories and files beyond the README list
- portability blockers in engine, networking, UI, installer, and tooling layers
- evidence of architecture assumptions such as x86-only code or compiler-specific constructs

# Renegade Repository Investigation

Last updated: 2026-03-17

This document set tracks a thorough audit of the `CnC_Renegade` source tree with the long-term objective of making the project buildable, maintainable, and eventually portable across multiple platforms.

## Scope of this investigation

This audit is focused on:

- the current build system and project layout
- missing or proprietary dependencies
- obvious code portability blockers
- missing directories, SDK stubs, and referenced projects not present in the repository
- subsystem-level risk areas that will complicate modernization
- likely sequencing for a future rescue/porting effort

## Document map

- `01-repository-baseline.md` — root facts, quantitative inventory, and major code areas
- `02-build-system-and-projects.md` — current build system, project graph, and migration blockers
- `03-dependency-audit.md` — missing SDKs/libraries and where they appear in the tree
- `04-project-file-findings.md` — concrete `.dsp` findings, ghost include paths, and representative target details
- `05-runtime-portability-audit.md` — subsystem map, Windows/DX8 coupling, x86-only code, and modernization risk
- `06-generated-artifacts-and-repo-gaps.md` — ignored/generated files, empty output directories, and additional repository completeness gaps
- `07-tools-and-support-audit.md` — editor/toolchain, launcher, installer, patching, Max plugins, and support-app triage
- `08-hardcoded-paths-and-case-sensitivity.md` — source-level include issues, absolute machine paths, UNC shares, and runtime DLL assumptions
- `09-modernization-priority-roadmap.md` — recommended rescue order, target tiers, and practical next steps

## Current executive summary

The repository is a large late-1990s / early-2000s Windows C/C++ codebase centered around Microsoft Visual C++ 6.0 workspace files (`.dsw`) and project files (`.dsp`). The codebase is not in a modern buildable state. The top-level `README.md` explicitly states that successful rebuilds require multiple missing or proprietary third-party libraries.

The first three investigation passes confirm several important facts:

1. The primary build orchestration is still `Code/commando.dsw`.
2. The workspace references at least one project directory that is not present in the repository: `Code/GameSpy/`.
3. Multiple dependency directories named in `README.md` do not exist in the repository, including `DirectX`, `Miles6`, `NvDXTLib`, `Lightscape`, `Umbra`, `GameSpy`, and `Launcher/SafeDisk`.
4. The source tree is heavily tied to Win32 APIs, DirectX 8-era headers, MFC/ATL/COM, and older Winsock usage.
5. A significant tool ecosystem exists under `Code/Tools/`, meaning modernization is not just about the game executable; there is a large supporting content and utility toolchain.
6. Representative project files reference additional missing or non-repository paths such as `srsdk1x`, `srsdk`, `wwcpuid`, `wwlzhl`, `Installer/Cab`, and `Launcher/SafeDisk`.
7. The tool workspace `Code/tools.dsw` contains a large secondary build graph of content and utility programs, confirming that the historical development workflow depended on far more than the runtime executable.
8. At least one existing folder contains a missing referenced file: `Code/Tools/RenegadeGR/` exists, but its project file references `jni.h`, which was not present in a direct directory listing.
9. The runtime code contains substantial x86-only inline assembly, Win32 stack walking, registry access, and dynamic DLL loading, especially in `WWMath`, `wwdebug`, `wwutil`, `ww3d2`, `Commando`, `Launcher`, and `Installer`.
10. `.gitignore` explicitly excludes many dependency roots and generated files, reinforcing that the repository is a source snapshot rather than a self-contained build environment.
11. `Code/Libs/Debug`, `Code/Libs/Profile`, and `Code/Libs/Release` exist but currently contain only `.gitignore`, so there are no checked-in prebuilt libraries to bridge the missing dependency gap.
12. The toolchain is itself a major modernization problem: `LevelEdit` depends on MFC, DirectX 8, SourceSafe/VSS, and external asset-database infrastructure; multiple exporter plugins are hardwired to `d:\3dsmax2\maxsdk`; and launcher/installer code still depends on RTPatch, SafeDisk, Bink, Miles, and shell/COM APIs.
13. `Tools/RenegadeSim/` is a Java-era sidecar project (`.java`, `.vjp`, `.sln`), indicating that the historical tooling ecosystem was not even single-language, let alone single-build-system.

## Repository shape at a glance

Approximate file counts gathered from the repository on this investigation pass:

- headers: 1685
- C++ source files: 1481
- legacy Visual C++ project files (`.dsp`): 61
- legacy workspaces (`.dsw`): 14
- resource scripts (`.rc`): 31
- definition files (`.def`): 11
- C source files (`.c`): 9
- one modern-ish solution file: `Code/Tools/RenegadeSim/RenegadeSim.sln`

Largest top-level code areas by file count observed so far:

- `Code/Tools/` — 1429 files
- `Code/Commando/` — 436 files
- `Code/Combat/` — 266 files
- `Code/ww3d2/` — 264 files
- `Code/wwlib/` — 263 files
- `Code/wwphys/` — 196 files
- `Code/Tests/` — 140 files
- `Code/Scripts/` — 100 files

## Preliminary conclusions

The project is currently best understood as:

- a preservation snapshot rather than a reproducible build
- a Windows-first codebase with deep platform entanglement
- a codebase with several missing external SDKs and project folders
- a codebase whose eventual modernization will likely require a staged approach:
  1. build graph recovery on Windows
  2. dependency replacement / abstraction
  3. compiler compatibility cleanup
  4. subsystem isolation
  5. cross-platform migration

## Notes on investigation method

Because the codebase is large, this documentation is being updated incrementally. Findings are expected to become more detailed over time, and some entries may be refined or corrected as more files are read.

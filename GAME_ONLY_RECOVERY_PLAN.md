# Game-Only Recovery Plan

Last updated: 2026-03-19

This document is the **implementation reference** for getting the repository from its current archival state to a **fully working game build**.

This plan is intentionally **game-only**.

It does **not** include restoring the editor/toolchain as part of the critical path. Tools are documented elsewhere under `docs/investigation/`, but they are explicitly **out of scope for the first working game**.

## Goal

Move the repository from:

- VC6-era workspaces and project files
- missing SDKs and internal dependencies
- no reproducible build
- extensive Win32 / DirectX 8 / x86 assumptions

To:

- a modern, **cross-platform CMake** build (Windows + Linux/macOS supported)
- a game executable that can be built with a modern compiler on all supported platforms
- a portable foundation that uses **SDL3** where applicable for windowing, input, audio, and timing
- missing functionality hidden behind controlled feature flags during bring-up
- incremental restoration of game features until the game is fully functional

## Scope and non-goals

## In scope

- `Code/Commando/`
- `Code/Combat/`
- `Code/ww3d2/`
- `Code/wwphys/`
- `Code/wwlib/`
- `Code/WWMath/`
- `Code/wwutil/`
- `Code/wwdebug/`
- `Code/WWAudio/`
- `Code/wwui/`
- `Code/wwnet/`
- `Code/wwsaveload/`
- `Code/wwbitpack/`
- `Code/wwtranslatedb/`
- `Code/BinkMovie/`
- `Code/Scripts/`
- any runtime-side online pieces needed for final game parity (`WWOnline`, `wolapi`, `WOLBrowser`) — **but only after offline/LAN play works**

## Out of scope for the critical path

- `Code/Tools/`
- `Code/Installer/`
- `Code/Launcher/`
- `Code/Tests/`
- `Code/BandTest/`
- `Code/SControl/` unless later proven to be a hard runtime dependency
- historical exporter/plugin ecosystem (`max2w3d`, `Clipbord`, `Blender2`, etc.)

## Working definition of “working”

The recovery effort should target these milestones in order:

1. **CMake configure succeeds** for a minimal bootstrap on a supported platform (initially Linux x64) and the build system is structured to support Windows and macOS targets from day one.
2. **Core runtime libraries compile** with a modern compiler.
3. **The game executable links** with stubs for missing middleware.
4. **The executable starts and enters the main loop** with null/stubbed renderer/audio/online paths.
5. **The executable renders a window / first frame**.
6. **Offline single-player / local gameplay becomes playable**.
7. **Audio, movies, and remaining UX features are restored**.
8. **LAN / non-service-dependent multiplayer works**.
9. **Legacy online features are either restored via replacement backends or deliberately replaced with modern equivalents**.

Do **not** try to jump directly from step 0 to step 9.

## Evidence that drives this order

The recommended order is based on the current audits and the source tree itself.

### Build graph evidence

- `Code/commando.dsw` shows the main game historically depended on:
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
- `Commando/commando.dsp` links missing middleware directly: `mss32.lib`, `binkw32.lib`, `gamespy.lib`, DirectX 8 libs, Umbra library paths.

### Missing middleware evidence

- `BinkMovie/BINKMovie.cpp` includes `Bink.h`.
- `WWAudio` project metadata references `..\miles6\include` and `mss32.lib`.
- `Commando/CDKeyAuth.cpp` includes multiple `GameSpy\...` headers.
- `wwphys/dynamicphys.cpp` includes Umbra code under `#if (UMBRASUPPORT)`.
- `ww3d2/dx8wrapper.cpp` loads `D3D8.DLL`.
- `Combat/directinput.cpp` loads `DINPUT8.DLL`.

### Compiler and architecture blocker evidence

- `WWMath/wwmath.h` contains `__asm`, `__declspec(naked)`, `__fastcall`.
- `WWMath/vp.cpp`, `WWMath/matrix3d.cpp`, `WWMath/quat.cpp` contain x86 asm.
- `wwdebug/wwprofile.cpp` and `wwdebug/wwmemlog.cpp` contain x86-only asm.
- `wwutil/stackdump.cpp`, `wwlib/Except.cpp`, and `Combat/debug.cpp` load `IMAGEHLP.DLL` and depend on Win32 stack walking.

### Why Linux x64 is the initial runtime target (but the build must be cross-platform)

Because the code contains pervasive x86 asm and Win32 assumptions, the first successful modern runtime target should be:

- **Linux x64** (modern compiler toolchain, portable runtime)
- **Windows x86/x64** (secondary runtime targets that must remain supported)
- **SRD-driven cross-platform compatibility** using SDL3 where applicable (windowing, input, audio, timing)

However, the build system and codebase should be structured from the very beginning to support **macOS** (and other platforms) as first-class targets. This means:

- using CMake and cross-platform abstractions from day one
- gating Win32/x86-only code behind clear feature/platform flags
- keeping platform-agnostic code compilable on all platforms (with stubs where necessary)

Do **not** start with:

- ARM64
- a brand-new renderer rewrite before first boot

Those can come later after we establish a portable build baseline and confirm the game boots on the initial target.

## High-level strategy

The implementation sequence should be:

1. add CMake without changing behavior
2. compile the lowest-level libraries first
3. introduce a **single central feature-flag system**
4. hide missing middleware behind `#if`/generated config defines
5. get a **linkable headless/null-feature executable**
6. restore visible functionality in the order that gives the fastest feedback
7. restore the remaining runtime features until the game is complete

## Central rule: use a generated feature-config header

Do **not** scatter ad-hoc `#ifdef SOME_RANDOM_TEMP_HACK` across the codebase.

Create a single generated build configuration header, for example:

- `Code/renegade_build_config.h.in`
- generated to something like `build/generated/renegade_build_config.h`

Every missing subsystem gate should be driven from that header and corresponding CMake options.

## Required first-wave feature flags

The first implementation step must introduce explicit feature flags for the missing or high-risk systems.

Suggested initial flags:

| Flag | Initial state | Purpose | Primary evidence / touchpoints |
| --- | --- | --- | --- |
| `RENEGADE_WITH_X86_ASM` | `OFF` | Disable fragile x86 asm during bring-up | `WWMath/wwmath.h`, `WWMath/vp.cpp`, `wwdebug/wwprofile.cpp`, `wwdebug/wwmemlog.cpp` |
| `RENEGADE_WITH_WIN32_STACKTRACE` | `OFF` | Disable ImageHlp stack walking / SEH extras | `wwutil/stackdump.cpp`, `wwlib/Except.cpp`, `Combat/debug.cpp` |
| `RENEGADE_WITH_UMBRA` | `OFF` | Remove missing Umbra dependency early | `wwphys/dynamicphys.cpp`, `Commando/console.cpp`, `Commando/consolefunction.cpp` |
| `RENEGADE_WITH_BINK` | `OFF` | Hide missing Bink codec during bootstrap | `BinkMovie/BINKMovie.cpp`, `Commando/commando.dsp` |
| `RENEGADE_WITH_MILES` | `OFF` | Hide missing Miles dependency and allow null audio | `WWAudio.dsp`, `Commando/commando.dsp` |
| `RENEGADE_WITH_GAMESPY` | `OFF` | Disable missing GameSpy SDK and internet-server integration | `Commando/CDKeyAuth.cpp`, `gamespyadmin.*`, `GameSpy_QnR.*`, `commando.dsp` |
| `RENEGADE_WITH_LEGACY_WOL` | `OFF` | Disable COM/browser/WOL service paths until late | `Commando/WebBrowser.cpp`, `WWOnline/*`, `wolapi/*`, `WOLBrowser/*` |
| `RENEGADE_WITH_DX8_RENDERER` | `OFF` initially | Allow null renderer / compile-first strategy | `ww3d2/dx8wrapper.cpp`, `ww3d2/dx8renderer.cpp`, `Commando/WINMAIN.CPP` |
| `RENEGADE_WITH_DIRECTINPUT` | `OFF` initially | Allow null input path until rendering exists | `Combat/directinput.cpp` |
| `RENEGADE_WITH_SCRIPT_DLL` | `ON`, but allow stub implementation | Preserve runtime script loading contract while bootstrapping | `Combat/scripts.cpp` |
| `RENEGADE_WITH_BANDTEST` | `OFF` | Remove non-game audio test dependency from early builds | `commando.dsw` dependency list |
| `RENEGADE_WITH_SCONTROL` | `OFF` | Remove remote server control from early game-only builds | `commando.dsw` dependency list |

These flags are not the final architecture; they are the **bootstrap survival kit**.

## Target order for the first CMake build

The first CMake milestone should only target what is needed to build the game executable.

Recommended initial target order:

1. `wwdebug`
2. `wwlib`
3. `WWMath`
4. `wwutil`
5. `wwsaveload`
6. `wwbitpack`
7. `wwtranslatedb`
8. `wwui`
9. `wwnet`
10. `WWAudio` (null/stub backend first)
11. `wwphys` (Umbra off)
12. `ww3d2` (null or compatibility renderer first)
13. `Combat`
14. `Scripts`
15. `BinkMovie` (stub first)
16. `Commando`

Reasoning:

- this follows the historical dependency direction from `commando.dsw`
- it puts the compiler- and platform-problem libraries first
- it avoids wasting time on tools, installer, launcher, and side utilities

## Detailed implementation phases

## Phase 0 — Choose the bootstrap platform and freeze the scope

### Objective

Get everyone building the same thing first.

### Required decisions

- Platform: **Linux x64** as the initial bootstrap target (with Windows x86/x64 and macOS supported as first-class follow-on targets)
- Architecture: **x64 (primary), with x86/x64 support for Windows**
- Compiler: **modern GCC/Clang on Linux**, and **MSVC on Windows**
- Scope: **game only**, no editor/installer/launcher/tool restoration on the critical path

### Phase 0 actions

1. Treat all tool, installer, and launcher projects as **out of scope** for the initial CMake graph.
2. Define the bootstrap deliverable as:
   - `Renegade.exe` equivalent, or a temporary renamed bootstrap executable (`renegade_bootstrap` during initial bring-up)
3. Define a separate list of **deferred directories**:
   - `Code/Tools`
   - `Code/Installer`
   - `Code/Launcher`
   - `Code/Tests`
4. Vendor **SDL3** from the official upstream repository as a pinned git submodule at `external/SDL` so windowing, input, audio, and timing all share a single cross-platform baseline during bring-up.

### Phase 0 completion criteria

- there is no ambiguity about the first supported compiler/platform combination
- implementors are not trying to solve Linux/macOS/editor problems before the game builds
- the repository contains a pinned SDL3 submodule and a bootstrap target that can prove the cross-platform foundation builds

### Phase 0 implementation status (2026-03-18)

- [x] Bootstrap platform frozen to **Linux x64**.
- [x] Follow-on platform structure kept ready for **Windows x86/x64** and **macOS**.
- [x] Scope frozen to **game only**; tools, installer, launcher, and tests remain off the critical path.
- [x] SDL3 selected as the portable platform layer and vendored at `external/SDL` from the official upstream repo.
- [x] SDL3 pin recorded at `release-3.4.2`.
- [x] Foundation library targets (`wwdebug`, `wwlib`, `WWMath`, `wwutil`) are online in CMake.

## Phase 1 — Introduce the CMake skeleton

### Phase 1 objective

Replace VC6 project orchestration with a modern build system **without** trying to solve every code problem at once.

### Phase 1 files to create

At minimum:

- `CMakeLists.txt` at repo root
- `.gitmodules`
- `external/SDL/`
- `cmake/RenegadeOptions.cmake`
- `cmake/RenegadeCompilerSettings.cmake`
- `cmake/RenegadeTargets.cmake` or equivalent shared helper file
- `Code/CMakeLists.txt`
- `Code/bootstrap/CMakeLists.txt`
- `Code/bootstrap/sdl_bootstrap_main.cpp`
- one `CMakeLists.txt` per runtime library directory as they are brought online
- `Code/renegade_build_config.h.in`

### Phase 1 concrete steps

1. Add a root `CMakeLists.txt` that:
   - requires a modern CMake version
   - declares the project
   - documents and enforces the **Linux x64** bootstrap assumptions while keeping Windows/macOS structurally supported
   - includes the options/config modules
   - vendors SDL3 from `external/SDL`
   - adds `Code/` as the main source tree
2. Add CMake options for every first-wave feature flag listed above.
3. Generate the build config header from CMake.
4. Add a temporary `renegade_bootstrap` target that links SDL3 and proves the configure/build path before the historical game targets are migrated.
5. Start with **empty or placeholder** subdirectory CMake files and add targets in dependency order.
6. Make the first configure target succeed even if many targets are not yet enabled.

### Phase 1 rules

- Do not copy VC6 flags blindly.
- Do not try to model every old configuration (`DebugE`, `ProfileE`, etc.) initially.
- Start with just:
  - `Debug`
  - `RelWithDebInfo`

### Phase 1 completion criteria

- `cmake -S . -B build` succeeds
- `cmake --build build --target renegade_bootstrap` succeeds
- feature options can be toggled centrally
- SDL3 is integrated as the portable platform layer for windowing/input/audio/timing
- target scaffolding exists for the core runtime graph

### Phase 1 implementation status (2026-03-18)

- [x] Root `CMakeLists.txt`, shared CMake modules, and generated build-config plumbing are in place.
- [x] The official SDL3 repository is vendored as a git submodule at `external/SDL` and pinned to `release-3.4.2`.
- [x] `cmake -S . -B build` succeeds on the Linux x64 bootstrap target.
- [x] `cmake --build build --target renegade_bootstrap` succeeds.
- [x] `renegade_bootstrap --headless-smoke` succeeds with dummy SDL video/audio drivers.
- [x] The first-wave foundation targets (`wwdebug`, `wwlib`, `WWMath`, `wwutil`) are migrated into the CMake graph.
- [ ] Historical runtime libraries beyond the Phase 2 foundation set are not yet migrated into the CMake graph.

## Phase 2 — Make the foundation libraries compile first

### Phase 2 objective

Remove the biggest compiler and architecture blockers before touching the game executable.

### Phase 2 targets

- `wwdebug`
- `wwlib`
- `WWMath`
- `wwutil`

### Phase 2 concrete tasks

#### 2.1 `WWMath`

Touchpoints:

- `WWMath/wwmath.h`
- `WWMath/vp.cpp`
- `WWMath/matrix3d.cpp`
- `WWMath/quat.cpp`

Actions:

1. Gate all x86 asm behind `RENEGADE_WITH_X86_ASM`.
2. Add portable C++ fallback implementations for every gated routine.
3. Preserve public APIs and signatures; only swap implementation paths.
4. Do **not** try to optimize first — correctness first, speed later.

#### 2.2 `wwdebug`

Touchpoints:

- `wwdebug/wwprofile.cpp`
- `wwdebug/wwmemlog.cpp`

Actions:

1. Disable asm-based profiling/memlog code behind `RENEGADE_WITH_X86_ASM`.
2. Provide simpler mutex/atomic or no-op fallbacks where appropriate.
3. Treat profiling as optional during bootstrap.

#### 2.3 `wwutil` / `wwlib`

Touchpoints:

- `wwutil/stackdump.cpp`
- `wwlib/Except.cpp`
- `wwlib/cpudetect.cpp`
- any other ImageHlp or asm-based helpers

Actions:

1. Gate ImageHlp stack walking and advanced exception tracing behind `RENEGADE_WITH_WIN32_STACKTRACE`.
2. Provide reduced crash-reporting / plain logging fallback.
3. Gate or replace asm-heavy CPU detection paths.
4. Normalize headers/includes enough for modern compilation.

### Phase 2 completion criteria

- these four libraries compile under modern MSVC in the CMake build
- no required target depends on x86 asm or ImageHlp just to compile

### Phase 2 visible progress

- first real modern-compiler library builds succeed
- bootstrap no longer depends on legacy VC6-only behavior for the lowest layers

### Phase 2 implementation status (2026-03-18)

- [x] `wwdebug`, `wwlib`, `WWMath`, and `wwutil` build in the active Linux x64 bootstrap CMake configuration.
- [x] `RENEGADE_WITH_X86_ASM=OFF` is wired through the active foundation build, with audited fallback paths in `WWMath` and `wwdebug`.
- [x] `RENEGADE_WITH_WIN32_STACKTRACE=OFF` is wired through the active foundation build, with stubbed stack-dump / exception paths selected outside the supported legacy Win32 stacktrace configuration.
- [x] asm-heavy CPU detection is replaced by the active `cpudetect_stub.cpp` path in the bootstrap build, so the foundation no longer requires CPUID / RDTSC asm to compile.
- [x] the active build no longer depends on ImageHlp or x86 asm just to compile the Phase 2 foundation targets.
- [ ] the full historical `wwlib` project is not yet migrated into CMake; the current target is still a bootstrap subset with stubbed exception / CPU-detect pieces.
- [ ] Phase 2 is not yet closed against its full completion criteria because the modern MSVC validation pass is still pending.

## Phase 3 — Bring up utility/runtime support libraries

### Phase 3 objective

Get the middle layer of game-support code compiling before the renderer and executable.

### Phase 3 targets

- `wwsaveload`
- `wwbitpack`
- `wwtranslatedb`
- `wwui`
- `wwnet`

### Phase 3 concrete tasks

1. Add each target to CMake with clear include paths and public/private dependencies.
2. Fix compile-only issues caused by modern headers, stricter type checking, and path normalization.
3. Keep online-service-specific code disabled if it requires missing middleware.
4. Treat registry- or COM-heavy helpers as opt-in late paths where possible.

### Phase 3 special note on `wwnet`

- keep the basic socket/runtime code building
- do not make GameSpy or WOL service code a prerequisite for this phase

### Phase 3 completion criteria

- all support/runtime libraries above compile
- the remaining blockers are primarily renderer, audio, gameplay, scripts, and app startup

### Phase 3 implementation status (2026-03-18)

- [x] `wwsaveload`, `wwbitpack`, `wwtranslatedb`, `wwui`, and `wwnet` are present in the active CMake graph.
- [x] `cmake --build build --target wwsaveload wwbitpack wwtranslatedb wwui wwnet -j1` succeeds in the Linux x64 bootstrap configuration.
- [x] `wwsaveload`, `wwbitpack`, and `wwtranslatedb` build from their broad historical source sets with Linux portability fixes applied in shared compatibility layers and targeted source cleanups.
- [x] `wwui` builds on non-Windows in a bootstrap subset configuration with Win32 IME/resource-parser and later-phase-heavy control sources fenced out until renderer/input-adjacent phases land.
- [x] `wwnet` builds on non-Windows in a bootstrap subset configuration with the lower-level socket/runtime/object layers online and the gameplay/session-heavy connection layer deferred.
- [x] Phase 3 no longer requires GameSpy, WOL, COM, or other legacy service integrations just to compile the support/runtime library slice.
- [ ] The full historical non-Windows source sets for `wwui` and `wwnet` are not restored yet; their deferred files remain later-phase work tied to renderer, input, or higher-level gameplay/network bring-up.

## Phase 4 — Stub the missing middleware before trying to link the game

### Phase 4 objective

Make every missing external dependency non-fatal to the build.

### 4.1 Bink

Evidence:

- `BinkMovie/BINKMovie.cpp` includes `Bink.h`
- `commando.dsp` links `binkw32.lib`

Actions:

1. Create a `RENEGADE_WITH_BINK` gate around the real Bink path.
2. Add a stub BinkMovie implementation that:
   - compiles without `Bink.h`
   - returns failure or “movie unsupported” cleanly
   - lets the game skip cinematics rather than fail to link
3. Keep the public `BinkMovie` API intact.

### 4.2 Miles / audio

Evidence:

- `WWAudio` and `commando.dsp` reference `mss32.lib` and `Miles6`

Actions:

1. Add `RENEGADE_WITH_MILES` gates around Miles-specific code.
2. Implement a **NullAudio backend** first.
3. Make `WWAudio` compile and link even when all sound creation calls become harmless no-ops.
4. Preserve API shape so the rest of the game can compile unchanged.

### 4.3 Umbra

Evidence:

- `wwphys/dynamicphys.cpp` uses `#if (UMBRASUPPORT)` and includes Umbra code
- `Commando/consolefunction.cpp` already has an “Umbra support not compiled into this build” path

Actions:

1. Drive Umbra entirely from `RENEGADE_WITH_UMBRA`.
2. Make the build default to `OFF`.
3. Prefer compile-time omission over elaborate fake implementations.
4. Keep memory/stat console code returning sensible zero/disabled values.

### 4.4 GameSpy / legacy internet services

Evidence:

- `Commando/CDKeyAuth.cpp` includes multiple `GameSpy\...` headers
- `commando.dsp` links `gamespy.lib`
- `gamespyadmin.*`, `GameSpy_QnR.*`, `gamespyauthmgr.*`, `GameSpyBanList.*` are integrated into `Commando`

Actions:

1. Add `RENEGADE_WITH_GAMESPY` and default it `OFF`.
2. Stub the GameSpy service/admin/auth layers behind the same public interfaces.
3. Force internet-server listing/authentication paths into disabled/unavailable mode while keeping LAN and offline paths buildable.
4. Do **not** delete the code; wall it off cleanly.

### 4.5 Legacy WOL / browser / COM online UI

Evidence:

- `Commando/WebBrowser.cpp`
- `WWOnline/WOLSession.cpp`
- `wolapi/WOLAPI.h`
- `WOLBrowser/WOLBrowser.h`

Actions:

1. Add `RENEGADE_WITH_LEGACY_WOL` and default it `OFF`.
2. Stub browser/login/matchmaking UI integration.
3. Keep offline and local-network gameplay decoupled from WOL/COM pieces.

### Phase 4 completion criteria

- every missing third-party runtime dependency is hidden behind a controlled flag
- the game can be linked without Bink, Miles, Umbra, GameSpy, or WOL

### Phase 4 implementation status (2026-03-19)

- [x] `WWAudio` is now present in the active CMake graph with a bootstrap null backend (`wwaudio_null.cpp`), and `cmake --build build --target WWAudio -j1` succeeds in the Linux x64 bring-up configuration.
- [x] `BinkMovie` is now present in the active CMake graph with a `RENEGADE_WITH_BINK=OFF` stub implementation (`binkmovie_stub.cpp`), and `cmake --build build --target BinkMovie -j1` succeeds.
- [x] Umbra is now driven from the central feature configuration instead of a hardcoded local define; `wwphys/umbrasupport.h` and `Code/wwphys/CMakeLists.txt` both honor `RENEGADE_WITH_UMBRA`.
- [x] The Linux/bootstrap compatibility layer now covers the legacy Win32, Miles, DirectInput, ImageHlp, and several DX8-adjacent include surfaces needed to compile deeper into the runtime graph.
- [ ] GameSpy and legacy WOL service layers are still untouched on the `Commando` side; they remain later Phase 4 work after the gameplay/runtime libraries build cleanly.
- [ ] Phase 4 is not yet closed: `wwphys` is now green, but the game executable is still not linkable because the downstream gameplay/script targets are still being stabilized and the GameSpy/WOL service layers remain deferred.

## Phase 5 — Get gameplay and script loading online

### Phase 5 objective

Compile the real gameplay layer and preserve the script loading contract.

### Phase 5 targets

- `Combat`
- `Scripts`

### 5.1 `Combat`

Actions:

1. Add `Combat` to CMake after all prerequisite runtime libs compile.
2. Keep editor-only defines (`PARAM_EDITING_ON`) out of the game-only bootstrap build.
3. Gate DirectInput-specific files so `Combat` can build with null input first.
4. Preserve gameplay logic; do not start rewriting game systems while the build is still unstable.

### 5.2 `Scripts`

Evidence:

- `Combat/scripts.cpp` loads a DLL and resolves script entry points dynamically

Actions:

1. Build `Scripts` as a DLL early, even if functionality is initially reduced.
2. If needed, provide a temporary minimal script DLL that exports the required entry points and allows the engine to continue.
3. Once the bootstrap executable links and starts, restore the real script contents incrementally.

### Phase 5 completion criteria

- `Combat` compiles and links
- a `Scripts` DLL exists that satisfies the runtime loader contract

### Phase 5 visible progress

- major gameplay code is now in the build graph
- the future executable can boot further without dying on missing DLL exports

### Phase 5 implementation status (2026-03-19)

- [x] `Combat` and `Scripts` now exist as real CMake targets in the active runtime graph.
- [x] `Combat` excludes `directinput.cpp` when `RENEGADE_WITH_DIRECTINPUT=OFF` or on non-Windows bootstrap builds.
- [x] `Scripts` is now built as a shared library target with the historical runtime-facing name `Scripts`.
- [x] `Combat/scripts.cpp` preserves the legacy DLL-loading contract while adding non-Windows `.so` fallback candidates (`Scripts.so`, `libScripts.so`, etc.) so the bootstrap runtime can keep the old call sites.
- [x] `RENEGADE_WITH_SCRIPT_DLL` now gates script-module loading cleanly instead of forcing unconditional runtime failure.
- [ ] Full `Combat` compile validation is still pending because `Code/Combat/ccamera.cpp` currently fails on `soundscene.h`.
- [ ] Full `Scripts` compile validation is still pending because `Code/Scripts/Common.h` currently fails on `dprint.h`.
- [ ] The real script implementation path has not been validated end-to-end yet; only the target wiring and loader compatibility work are in place.

## Phase 6 — Bring up the renderer and input in two stages

### Phase 6 objective

Get from “the game links” to “the game produces visible output and accepts input” with the least risk.

### Stage 6A — null renderer / null input bootstrap

#### Renderer

Evidence:

- `ww3d2/dx8wrapper.cpp` loads `D3D8.DLL`
- `ww3d2/dx8renderer.cpp` is central to rendering
- `Commando/WINMAIN.CPP` also loads `D3D8.DLL`

Actions:

1. Add `RENEGADE_WITH_DX8_RENDERER` and default it `OFF` for the first linkable bootstrap.
2. Provide a null renderer path sufficient to let the app initialize, log, and advance the main loop.
3. If necessary, compile out DX8-specific source files and replace them with temporary stub translation units that satisfy the same interfaces.

#### Input

Evidence:

- `Combat/directinput.cpp` loads `DINPUT8.DLL`

Actions:

1. Add `RENEGADE_WITH_DIRECTINPUT` and default it `OFF` initially.
2. Provide a null input backend that reports “no input” but keeps the app stable.

### Phase 6A implementation status (2026-03-19)

- [x] `RENEGADE_WITH_DX8_RENDERER=OFF` is now exercised by a real stub branch in `ww3d2/dx8wrapper.h` plus bootstrap compatibility headers for `dx8renderer`, `dx8vertexbuffer`, `dx8indexbuffer`, `dx8caps`, and `targa`.
- [x] `RENEGADE_WITH_DIRECTINPUT=OFF` is now respected in `Code/Combat/CMakeLists.txt`, and a non-Windows `dinput.h` shim exists for compile-time DirectInput keycode dependencies.
- [x] `wwphys` now builds cleanly in the Linux x64 bootstrap configuration, so the remaining bring-up failures have moved downstream into gameplay-layer portability gaps rather than renderer-adjacent `wwphys` fallout.
- [ ] The null-renderer bootstrap is still incomplete: the current stop point has moved into `Combat` and `Scripts`, which are now reaching for portability-only headers such as `soundscene.h` and `dprint.h`.
- [ ] Before `Commando` can be validated downstream, the remaining `Combat`/`Scripts` header surfaces need to be normalized so those targets can finish compiling cleanly.

### Stage 6B — first real frame / first real input

#### Recommended short-term strategy

For fastest visible progress, **do not** start by rewriting the entire renderer to Vulkan/OpenGL.

Instead:

1. Keep the existing `ww3d2` API shape.
2. Restore a **Windows-only compatibility rendering path first**.
3. Make the existing DX8-oriented code compile and run via the least invasive compatibility approach possible.

Only after the game is visibly running should a deeper render-backend refactor become a priority.

#### Input re-enable order

1. Restore enough input for menu navigation and quitting.
2. Restore gameplay input after the first frame exists.
3. Only then revisit deeper abstraction if desired.

### Phase 6 completion criteria

- the game opens a window
- the main loop runs stably
- the first frame is visible
- basic input works

### Phase 6 visible progress

- this is the first “it is obviously alive” milestone

## Phase 7 — Link and boot `Commando`

### Phase 7 objective

Get the real game executable online with the stubbed subsystems above.

### Phase 7 primary targets/files

- `Commando/WINMAIN.CPP`
- `Commando/init.cpp`
- `Commando/cnetwork.cpp`
- `Commando/combatgmode.cpp`
- `Commando/console.cpp`
- `Commando/consolefunction.cpp`
- any startup/config/resource files needed for the base shell

### Phase 7 concrete tasks

1. Add the `Commando` target last, after all prerequisite libraries exist.
2. Remove non-essential historical dependencies from the first bootstrap link if they are not truly required for runtime:
   - `BandTest`
   - `SControl`
   - `GameSpy`
3. Keep internet-service-dependent menus/features hidden or disabled when `RENEGADE_WITH_GAMESPY=OFF` and `RENEGADE_WITH_LEGACY_WOL=OFF`.
4. Keep movie/audio startup paths tolerant of `RENEGADE_WITH_BINK=OFF` and `RENEGADE_WITH_MILES=OFF`.
5. Prefer runtime-visible “feature unavailable” behavior over link-time breakage.

### Phase 7 completion criteria

- the CMake build produces a real game executable
- the executable starts and reaches the idle/main loop path

## Phase 8 — Reach a first playable offline build

### Phase 8 objective

Move from “the exe starts” to “you can actually play the game offline.”

### Phase 8 priorities

1. rendering stable enough for menus/world display
2. input stable enough for real gameplay
3. script DLL integration stable
4. required data/package loading stable
5. audio still optional if needed
6. movies still optional if needed

### Phase 8 concrete steps

1. Verify the game can boot without internet-service features.
2. Make sure missing GameSpy/WOL code does not block local/offline game modes.
3. Fix whatever runtime asset/package assumptions are required to load the title screen / main menu / first mission.
4. Keep all non-critical features behind clear disable paths rather than restoring them prematurely.

### Phase 8 completion criteria

- the user can launch into at least one offline playable path
- the core game loop is usable
- crashes are now primarily feature-specific, not architectural

### Phase 8 visible progress

- **first playable build**

## Phase 9 — Restore audio, movies, and remaining player-facing runtime features

### Phase 9 objective

Turn the offline playable build into a reasonably complete single-player/local build.

### 9.1 Audio restoration

Recommended order:

1. keep NullAudio until first playable build exists
2. replace Miles with a modern backend
3. re-enable 2D/UI audio first
4. then 3D positional sound, streaming, and advanced behavior

Touchpoints:

- `WWAudio/*`
- any game-side creation/use sites revealed during testing

### 9.2 Movie restoration

Recommended order:

1. keep cinematics skippable while Bink is stubbed
2. replace Bink playback later with a modern decoder path
3. re-enable intro/cutscene playback after core gameplay is stable

Touchpoints:

- `BinkMovie/*`
- `Commando` movie call sites

### Phase 9 completion criteria

- the game is enjoyable offline without obvious missing player-facing systems
- missing multimedia is no longer a blocker for normal play

## Phase 10 — Restore LAN and non-service-dependent multiplayer

### Phase 10 objective

Recover networked play without depending on dead proprietary internet services.

### Phase 10 why this is not earlier

Because the runtime/game loop must already be stable before debugging multiplayer behavior is worth the cost.

### Phase 10 concrete steps

1. Keep `wwnet` in the build from earlier phases.
2. Ensure LAN or direct-connect-style flows work without GameSpy/WOL.
3. Separate service discovery/authentication from the actual game networking path.
4. Make service integration optional rather than central.

### Phase 10 completion criteria

- LAN / direct local multiplayer is functional without GameSpy/WOL

## Phase 11 — Restore or replace legacy online services

### Phase 11 objective

Only after offline and LAN play work should the project attempt full online parity.

### Phase 11 subsystems involved

- `Commando` GameSpy integration files
- `WWOnline/*`
- `wolapi/*`
- `WOLBrowser/*`
- any UI/login/browser/auth code that depends on them

### Phase 11 concrete strategy

1. Do **not** try to rebuild against the original proprietary GameSpy stack if it remains unavailable.
2. Replace old service assumptions with project-controlled interfaces.
3. Preserve old in-game flow where practical, but substitute modern/community-owned backends.
4. Make sure the final architecture never again requires unavailable SDKs just to build.

### Phase 11 completion criteria

- the project has a fully working online path appropriate for current infrastructure
- or the legacy internet path is deliberately replaced and documented as such

## Phase 12 — Cleanup and hardening

### Phase 12 objective

Remove bring-up debt once the game works.

### Phase 12 required tasks

1. Audit every bootstrap feature flag.
2. Remove temporary stubs that are no longer needed.
3. Keep permanent optional features only where they represent real product choices.
4. Collapse duplicate code paths created only for bring-up.
5. Add automated build/test smoke checks for:
   - configure
   - core libs
   - executable link
   - basic startup
   - at least one offline playable path

## What to stub first, in exact order

This is the concrete stub order that should be followed.

1. `RENEGADE_WITH_X86_ASM=OFF`
   - because `WWMath`, `wwdebug`, and low-level code otherwise block modern compilation immediately
2. `RENEGADE_WITH_WIN32_STACKTRACE=OFF`
   - because `IMAGEHLP.DLL`/stack walking is not needed for first boot
3. `RENEGADE_WITH_UMBRA=OFF`
   - because the code already contains optional-style Umbra behavior and it is not required for correctness
4. `RENEGADE_WITH_BINK=OFF`
   - because movies are non-critical for first playable build
5. `RENEGADE_WITH_MILES=OFF`
   - because NullAudio is acceptable during bring-up
6. `RENEGADE_WITH_GAMESPY=OFF`
   - because missing GameSpy SDK blocks build and is not needed for offline bring-up
7. `RENEGADE_WITH_LEGACY_WOL=OFF`
   - because COM/browser/internet login is not needed for offline bring-up
8. `RENEGADE_WITH_DX8_RENDERER=OFF` temporarily
   - only until the executable links and boots; then re-enable visible rendering as the next major milestone
9. `RENEGADE_WITH_DIRECTINPUT=OFF` temporarily
   - only until the renderer/main loop is stable enough to support real input restoration

## What to re-enable first, in exact order

Once the game links and boots, re-enable missing functionality in this order:

1. **Renderer / first frame**
2. **Basic input**
3. **Script DLL real implementation**
4. **Offline single-player path**
5. **UI audio / basic sound**
6. **3D / positional audio**
7. **Movies / cinematics**
8. **LAN multiplayer**
9. **Legacy online replacement path**

This order maximizes visible progress and minimizes time spent restoring features that do not help prove the game is alive.

## Things that must not be put on the critical path

Do **not** block the game-only plan on any of the following:

- restoring `LevelEdit`
- restoring SourceSafe/VSS integration
- restoring `Launcher` / `Installer`
- resurrecting SafeDisk
- resurrecting RTPatch
- rebuilding the 3ds Max exporter plugin ecosystem
- porting to Linux/macOS/x64 before Win32 works
- writing a brand-new renderer before the first working boot

## Immediate next implementation actions

The next concrete implementation steps from the current repository state are:

1. keep the root CMake skeleton, generated config header, SDL3 submodule, and `renegade_bootstrap` healthy while expanding the runtime graph
2. normalize the remaining `Combat` portability headers, starting with `Code/Combat/ccamera.cpp` and its `soundscene.h` include
3. normalize the remaining `Scripts` portability headers, starting with `Code/Scripts/Common.h` and its `dprint.h` include
4. rerun `cmake --build build --target Combat -j1` and then `cmake --build build --target Scripts -j1` until both targets are green
5. once `Combat` and `Scripts` are compiling, finish the still-missing Phase 4 service stubs for GameSpy and legacy WOL before attempting `Commando`
6. add `Commando`, get a linkable null-feature executable, and only then continue into first-frame / real-input restoration
7. keep Phase 2 MSVC validation and broader historical-source restoration on the backlog, but do not let them distract from the current Linux game-only critical path

## Current stopping point (2026-03-19)

This is the handoff state for the current bring-up pass.

- Last attempted `Combat` command: `cmake --build build --target Combat -j1`
- Current `Combat` failure: `Code/Combat/ccamera.cpp` includes `soundscene.h`, which still does not resolve on Linux/bootstrap
- Last attempted `Scripts` command: `cmake --build build --target Scripts -j1`
- Current `Scripts` failure: `Code/Scripts/Common.h` includes `dprint.h`, which still does not resolve on Linux/bootstrap
- Current meaning: `wwphys` is now green; the remaining bring-up work has moved into portability cleanup in `Combat` and `Scripts` rather than renderer-adjacent `wwphys` fallout
- Already verified in this pass:
   - `cmake -S . -B build` still succeeds
   - `cmake --build build --target WWAudio -j1` succeeds
   - `cmake --build build --target BinkMovie -j1` succeeds
   - `cmake --build build --target wwphys -j1` succeeds
   - `cmake --build build --target Combat -j1` now gets past the `wwphys` layer and fails later in `ccamera.cpp`
   - `cmake --build build --target Scripts -j1` now gets past the `CustomEvents`/`ScriptFactory` casing issues and fails later in `Common.h`
- Recommended resume order:
   1. fix `soundscene.h` in `Combat`
   2. fix `dprint.h` in `Scripts`
   3. rerun `cmake --build build --target Combat -j1`
   4. rerun `cmake --build build --target Scripts -j1`
   5. once those are green, return to the service-layer stubs and `Commando`

## Bottom line

The shortest path to a fully working game is **not** “solve every missing dependency immediately.”

The shortest path is:

- modernize the build first
- isolate all missing functionality behind controlled feature flags
- compile the foundation first
- link the game with stubs
- restore visible runtime behavior in the order that proves progress fastest
- continue re-enabling systems until the game is complete

That gives implementors a concrete path from today’s archive snapshot to a real, buildable, testable game.

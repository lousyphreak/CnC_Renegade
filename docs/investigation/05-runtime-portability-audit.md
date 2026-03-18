# Runtime Portability Audit

Last updated: 2026-03-17

This document focuses on the code itself rather than just build metadata. It summarizes the major runtime/engine subsystems, portability blockers, compiler-compatibility risks, and likely sequencing concerns for a future multi-platform rescue effort.

## Subsystem map

### `Code/Commando/`

Role:

- main client executable
- UI dialogs and shell/game-mode logic
- game-side networking and WOL integration
- startup, shutdown, WinMain, and main loop orchestration

Why it matters:

- this is where platform-specific startup and client/UI concerns are concentrated
- it appears to be the top-level application layer over engine/game libraries

### `Code/Combat/`

Role:

- core gameplay simulation
- actors, weapons, buildings, AI, pathfinding, objectives, inventory, damage, and scripted game behavior

Why it matters:

- this looks like the most gameplay-centric and relatively reusable runtime library
- it is still entangled with engine/input/debug infrastructure, but conceptually it is closer to portable game logic than the shell or renderer

### `Code/ww3d2/`

Role:

- renderer and 3D asset/runtime layer
- meshes, materials, animations, particles, render states, textures, and DirectX 8 wrappers

Why it matters:

- this is one of the largest and most critical blockers for modern compilation and cross-platform work
- it is deeply centered around Direct3D 8-era assumptions

### `Code/wwphys/`

Role:

- physics, collision, pathing support, world interaction

Why it matters:

- broadly important to runtime correctness
- some integration points appear optional or optimization-related, especially Umbra support

### `Code/wwlib/`, `Code/WWMath/`, `Code/wwutil/`, `Code/wwdebug/`

Role:

- low-level foundation layer
- strings, files, threading, exceptions, math, debugging, profiling, stack dumping, and miscellaneous utilities

Why it matters:

- these directories contain many of the hardest x86/Win32 assumptions
- if they are not cleaned up first, higher-level modernization work will remain fragile

### `Code/WWAudio/`

Role:

- audio abstraction and 3D sound management

Why it matters:

- strongly tied to Miles and some Windows threading/platform behavior
- large enough to require a deliberate replacement or compatibility strategy

### `Code/wwui/`

Role:

- custom UI/dialog framework

Why it matters:

- not pure MFC, but still tied to Windows APIs and DirectInput in places
- sits between the shell/game code and the platform layer

### `Code/wwnet/`, `Code/WWOnline/`, `Code/wolapi/`, `Code/WOLBrowser/`

Role:

- networking, online services, WOL/WESTWOOD Online integration, COM-based browser/api pieces

Why it matters:

- these areas combine sockets, COM, generated IDL artifacts, and service-specific logic
- they are likely not a good first target for cross-platform bring-up unless multiplayer is the immediate goal

## Major portability blockers

## Windows API dependence

Examples observed include:

- registry access in `Launcher/findpatch.cpp`, `Launcher/Protect.cpp`, `Launcher/main.cpp`, `Commando/WebBrowser.cpp`, `Commando/WOLBuddyMgr.cpp`, and installer code
- `LoadLibrary` / `GetProcAddress` usage in `ww3d2/dx8wrapper.cpp`, `Combat/directinput.cpp`, `Launcher/patch.cpp`, `Combat/scripts.cpp`, and `wwutil/stackdump.cpp`
- module/file path retrieval via `GetModuleFileName`
- thread and synchronization primitives via Windows-specific headers and types
- shell/COM integration in installer and online/browser code

Implication:

- the codebase is not just Windows-biased; it is structurally built around Win32 facilities

## DirectX 8 and related multimedia APIs

Representative evidence:

- `ww3d2/dx8wrapper.cpp`
- `ww3d2/dx8wrapper.h`
- `ww3d2/dx8renderer.cpp`
- `ww3d2/dx8caps.cpp`
- `Combat/directinput.cpp`
- `BinkMovie/BINKMovie.cpp`
- `wwlib/dsurface.h`

Representative API/library assumptions:

- `D3D8.DLL`
- `IDirect3D*8` interfaces
- `DINPUT8.DLL`
- `d3d8.lib`, `d3dx8.lib`, `dinput.lib`, `dinput8.lib`, `ddraw.lib`, `dsound.lib`, `dxguid.lib`

Implication:

- the graphics/input/media stack is one of the single biggest modernization barriers

## MFC / ATL / COM / RPC dependence

Examples:

- MFC in tool projects such as `Tools/WWCtrl` and `Tools/WWConfig`
- COM shell usage in `Installer/Installer.cpp`
- COM browser / WOL objects in `Commando/WebBrowser.cpp` and `WWOnline/WOLSession.cpp`
- RPC/IDL-generated interfaces in `wolapi/WOLAPI.h` and `WOLBrowser/WOLBrowser.h`

Implication:

- online/browser/editor/installer/tooling layers are heavily Windows-specific even when the core simulation code may be more portable

## x86-only inline assembly and calling conventions

Some of the most serious architecture blockers are in foundational code.

Representative examples:

- `WWMath/wwmath.h` and `Scripts/wwmath.h`
  - `__asm`
  - `__declspec(naked)`
  - `__fastcall`
  - x87 FPU conversion code
- `WWMath/vp.cpp`
  - SSE assembly macros and register operations
- `WWMath/matrix3d.cpp`
- `WWMath/quat.cpp`
- `wwdebug/wwmemlog.cpp`
- `wwdebug/wwprofile.cpp`
- `wwnet/packetmgr.cpp`

Implication:

- 64-bit portability is not just a compiler flag problem
- ARM64 and non-x86 targets are currently unrealistic without rewriting math/debug/profiling internals

## Win32 stack walking and exception/debug infrastructure

Representative examples:

- `wwutil/stackdump.cpp`
- `Combat/debug.cpp`
- `wwlib/Except.cpp`

Observed concepts:

- `IMAGEHLP.DLL`
- `SymInitialize`
- `SymLoadModule`
- `StackWalk`
- x86 register / stack frame assumptions

Implication:

- crash handling and profiling/debug facilities are deeply platform- and architecture-specific
- these subsystems should be isolated or replaced early to reduce downstream churn

## Compiler-compatibility risks

## MSVC-specific keywords and pragmas

Representative patterns:

- `__declspec(...)`
- `__cdecl`, `__stdcall`, `__fastcall`
- `#pragma comment(lib, "imm32.lib")` in `wwui/IMEManager.cpp`
- VC6-era project flags and behavior assumptions

Implication:

- modern Clang/GCC support will require systematic compatibility shims or targeted refactors

## Pre-standard / legacy C++ idioms

Examples include:

- old-style debug allocator macro overrides in `Scripts/always.h`
- raw pointer and C-style cast heavy code
- widespread `NULL` rather than modern null-safe idioms
- legacy bool/type definitions and custom platform typedefs in some support code

Implication:

- the code may compile only with careful compatibility work, even before platform porting begins

## Potential case-sensitivity and include-path fragility

Representative examples:

- `BinkMovie/subtitleparser.h` uses a backslash include path: `wwlib\vector.h`
- include/file naming uses mixed case across the tree, such as `StdAfx.h` / `StdAfx.H`
- some project files use mixed path capitalization such as `directX`, `DirectX`, `Wwlib`, `Ww3d2`

Implication:

- case-sensitive platforms and tools will surface additional breakage beyond missing SDKs

## Notable code-health and maintenance signals

The codebase contains many TODO/HACK/XXX markers and legacy placeholders, which are not fatal by themselves but are useful indicators of technical debt.

Representative examples:

- `Commando/console.cpp` contains a `#pragma message("TODO: relocate and provide UI for player communication.")`
- `BinkMovie/subtitlemanager.cpp` contains `// TODO: Make sure entries are sorted by time.`
- `WWMath/colmath*` files contain TODO comments about correctness and overlap tests
- `Commando/buildnum.cpp` contains placeholder strings like `Insert1Build2Number3Here4`
- several serialization/chunk ID files contain `XXX...` placeholder-like identifiers

Implication:

- beyond missing dependencies, there is genuine historical unfinishedness or technical debt preserved in the source snapshot

## Particularly risky files and areas

High-risk files or subsystems observed so far:

- `Commando/WINMAIN.CPP`
- `ww3d2/dx8wrapper.h`
- `ww3d2/dx8wrapper.cpp`
- `ww3d2/dx8renderer.cpp`
- `Combat/directinput.cpp`
- `WWMath/wwmath.h`
- `WWMath/vp.cpp`
- `WWMath/quat.cpp`
- `wwdebug/wwmemlog.cpp`
- `wwdebug/wwprofile.cpp`
- `wwutil/stackdump.cpp`
- `wwlib/Except.cpp`
- `Launcher/Protect.cpp`
- `Launcher/patch.cpp`
- `Installer/Installer.cpp`
- `Commando/WebBrowser.cpp`
- `WWOnline/WOLSession.cpp`

## Suggested rescue sequencing based on code risk

### Phase 1: foundation cleanup

Focus on:

- `wwlib`
- `WWMath`
- `wwutil`
- `wwdebug`

Goals:

- reduce compiler-specific and x86-only assumptions
- isolate or replace stack walking/debug-only code
- establish portable type/utility abstractions

### Phase 2: rendering and input abstraction

Focus on:

- `ww3d2`
- `Combat/directinput.cpp`
- related media integration in `BinkMovie`

Goals:

- define a rendering backend abstraction
- decouple input from DirectInput-specific code

### Phase 3: runtime bring-up

Focus on:

- `Combat`
- `Commando` core runtime paths
- `WWAudio` with either stubs or replacement backend

Goals:

- get a minimal executable/runtime loop working without requiring every historical subsystem

### Phase 4: networking and online features

Focus on:

- `wwnet`
- `WWOnline`
- `wolapi`
- `WOLBrowser`

Goals:

- decide whether to stub, replace, or defer legacy online service integrations

### Phase 5: tooling, launcher, installer

Focus on:

- `Tools/*`
- `Launcher`
- `Installer`

Goals:

- treat these as separate modernization projects unless required early for content workflow or distribution

## Bottom line

The code-level audit confirms that the repository is blocked not only by missing SDKs, but also by a dense layer of Windows-specific architecture, x86-specific implementation details, and VC6-era coding practices. A successful multi-platform rescue will need to combine dependency replacement with deep internal refactoring, especially in the foundational and rendering layers.

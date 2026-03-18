# Modernization Priority Roadmap

Last updated: 2026-03-17

This document turns the investigation findings into a practical rescue order. It is not a project plan with estimates or staffing commitments; it is a recommended sequence for reducing risk while preserving optionality.

## Guiding principle

Do not try to resurrect the entire historical environment at once.

This repository is missing too many SDKs, contains too many Windows/x86 assumptions, and spans too many targets for a one-shot “make everything build” effort to be realistic.

Instead, the likely winning approach is:

1. recover a minimal build graph
2. isolate foundational portability blockers
3. stub or replace the most toxic middleware
4. bring up a minimal runtime
5. only then decide which tools are worth porting, rewriting, or retiring

## Tier 0: documentation and inventory stabilization

Immediate goals:

- keep the repository investigation docs current
- continue turning implicit knowledge into explicit matrices
- identify the true minimal target subset for a first successful build

Why first:

- this repository has enough missing context that undocumented assumptions are themselves a blocker

## Tier 1: foundation layer cleanup

Primary targets:

- `wwlib`
- `WWMath`
- `wwutil`
- `wwdebug`

Primary problems to address:

- x86-only inline assembly
- Win32-only stack walking and exception handling
- compiler-specific pragmas and keywords
- old allocator/debug macros and fragile low-level assumptions

Desired outcome:

- the lowest-level code becomes modern-compiler-friendly enough that higher-level work is not built on sand

## Tier 2: build-system reconstruction on Windows

Primary targets:

- core libraries, not the full historical workspace

Suggested first-pass build subset:

- `wwlib`
- `WWMath`
- `wwutil`
- `wwdebug`
- `wwsaveload`
- `wwbitpack`
- `wwtranslatedb`
- `wwui`
- `WWAudio` (possibly stubbed)
- `wwphys` (possibly with Umbra disabled)
- `ww3d2` (possibly partial or behind temporary shims)
- `Combat`

Why this order:

- it maximizes useful library recovery while postponing the worst application/tooling edges

## Tier 3: renderer and input abstraction

Primary targets:

- `ww3d2`
- `Combat/directinput.cpp`
- `Commando/WINMAIN.CPP`
- relevant media glue in `BinkMovie`

Primary decisions needed:

- whether to preserve old renderer behavior behind compatibility shims, or start moving toward a new backend immediately
- whether initial bring-up should stub graphics/media features to reduce scope

Why this is hard:

- Direct3D 8 assumptions are central, not peripheral
- this is one of the largest single technical risk clusters in the repo

## Tier 4: dependency replacement or deactivation

The following should likely be handled by replacement, abstraction, or temporary removal rather than direct recovery:

- Bink
- Miles
- GameSpy
- SafeDisk
- RTPatch
- Lightscape
- NvDXTLib
- SourceSafe/VSS integration

Likely treatment categories:

- **stub temporarily**: GameSpy, SafeDisk, SourceSafe, some updater code
- **replace strategically**: Bink, Miles, DirectInput/Direct3D paths, installer packaging
- **defer or retire**: old Max plugins, shell extension, some update utilities

## Tier 5: minimal runtime bring-up

Primary targets:

- `Combat`
- minimal `Commando` shell/runtime startup path
- required subset of `ww3d2`, `wwphys`, `WWAudio`, and `wwui`

Desired outcome:

- a program that starts, initializes core systems, and can serve as a foundation for further work

This does **not** need to mean “feature parity with historical Renegade” on first bring-up.

## Tier 6: asset inspection and pipeline preservation

Best early preservation candidates:

- `W3DView`
- `MakeMix`
- `MixViewer`
- `ChunkView`
- smaller utilities that help inspect proprietary data formats

Why these before `LevelEdit`:

- they likely offer high preservation value at much lower implementation risk

## Tier 7: editor succession, not editor resurrection

Primary target:

- `Tools/LevelEdit`

Recommendation:

- treat `LevelEdit` as a workflow/reference system to be studied, documented, and gradually replaced
- do **not** make project success depend on reviving every original SourceSafe/MFC/DirectX 8 editor behavior intact

Why:

- `LevelEdit` is one of the most entangled pieces in the whole tree
- it combines MFC, VSS, DirectX 8, asset DB assumptions, and many tool-only dependencies

## Tier 8: optional/historical components

Likely low-value recovery targets in their original form:

- 3ds Max 2 plugin family (`max2w3d`, `Clipbord`, `Blender2`, `AMC_IMP`, `ASF_IMP`, `MaxFly`)
- `W3DShellExt`
- old updater variants if a modern packaging path is introduced
- Java-era side projects unless they become necessary

Best use of these targets:

- archive as historical references
- mine for file-format or workflow knowledge
- avoid making them schedule-critical

## Recommended immediate next steps after this audit

1. Produce a machine-readable target/dependency matrix from all `.dsp` files.
2. Decide the exact minimal bootstrap build target set.
3. Identify which missing dependencies will be stubbed first versus replaced first.
4. Start a foundation cleanup branch focused on `WWMath`, `wwlib`, `wwutil`, and `wwdebug`.
5. Stand up a modern meta-build scaffold for the bootstrap subset.
6. Defer launcher/installer/editor resurrection until the core runtime direction is stable.

## Bottom line

The safest route is not “port the game” in one jump. It is “recover the foundation, contain the renderer/dependency blast radius, bring up a minimal runtime, and only then rebuild the ecosystem around it.” That sequencing gives the best chance of turning this preservation snapshot into an actually maintainable codebase.

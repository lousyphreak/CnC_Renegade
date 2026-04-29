# Renderer Modernization Plan

Last updated: 2026-04-30

This document turns the findings from `10-static-mesh-and-terrain-batching-audit.md` and `BGFX-PORT.md` into a concrete refactor plan for the runtime renderer. The plan is deliberately aimed at a **modern, high-throughput renderer** rather than a compatibility bridge.

## Goals

- make the runtime renderer explicit, bgfx-native, and materially cheaper on the CPU
- reduce state churn, draw submission overhead, first-use work, and per-object render-time mutation
- guarantee correct rendering for the supported runtime feature set, including conservative shadow-caster inclusion
- remove D3D8 and D3D8-era renderer contracts that block modernization
- update renderer consumers together with the interface break instead of preserving legacy glue indefinitely

## Non-goals

- preserving renderer compatibility layers as a long-lived architecture
- preserving 100% exact historical submission granularity or culling behavior
- keeping editor and tool rendering requirements as a design constraint
- preserving projector-era runtime settings, debug toggles, or pass plumbing for their own sake

## Decisions resolved while preparing this plan

The following design branches were explicitly resolved up front:

1. **Hard renderer contract break first**
   - Do not stage this around a long-lived compatibility interface. Break the renderer API intentionally and update consumers in the same refactor sequence.
2. **3D, 2D/UI, and video stay in the same modernization plan**
   - `Render2DClass`, HUD/menu overlays, and `BINKMovie` should move under the same renderer contract instead of becoming a second cleanup track.
3. **Receiver-side projector replay must go away**
   - Surviving projected effects become explicit renderer-owned systems such as decals or bounded projected-effect passes.
4. **Renderer phases replace object-owned special scheduling**
   - `Special_Render`, delayed material passes, and `RenderInfoClass` additional-pass replay are not the long-term scheduling model.
5. **Light-data architecture is fixed before the exact GPU light-culling scheme**
   - The new contract must remove the four-light DX8 model, but it does not need to lock the implementation to clustered or Forward+ immediately.

## Baseline summary from the audit

The current runtime already contains useful modern pieces, but they are trapped behind D3D8-shaped contracts:

- bgfx already executes shader-based mesh lighting and shadow-map sampling
- the active runtime still describes rendering through `DX8Wrapper`, `D3DRS_*`, `D3DTSS_*`, `D3DLIGHT8`, FVF categories, and pass replay
- static meshes and terrain are split across different renderer architectures
- projector and extra-pass handling still re-enters receivers and explodes draw count
- per-object CPU light environments and user-lighting arrays fragment batch sharing
- 2D/UI/video paths still use legacy shader/state objects instead of a shared explicit overlay contract

The rest of this plan assumes those findings are correct and uses them as the starting point.

## Required end-state architecture

The target runtime should be organized around **explicit scene/view/draw data**, not mutable device state.

### 1. Renderer-owned frame model

The renderer should own a frame model with these responsibilities:

- frame begin/end, render-target lifetime, screenshots, and movie capture
- per-view configuration for main scene, shadow views, overlays, and debug views
- scene-global resource binding for lights, fog, shadow data, and shared textures
- renderer-owned opaque/translucent/decal/overlay/video phase scheduling

### 2. Immutable submission data

All runtime submission should converge on immutable packets or descriptors instead of `Set_*` mutation followed by draw:

- mesh draw packets
- terrain draw packets
- shadow caster packets
- decal / projected-effect packets
- overlay draw packets
- video presenter packets

Each packet should carry explicit material/pipeline identity, geometry ranges, resource bindings, and feature flags.

### 3. Shared scene lighting and shadow data

Lighting and shadowing must be scene/view data, not per-object wrapper state:

- one shared world-space light data build per frame
- one shared shadow-caster collection path for static meshes, terrain, and dynamic objects
- no four-light interface cap
- no CPU object-center light approximation as the ordinary runtime path
- conservative shadow-caster inclusion is acceptable; missing casters is not

### 4. Renderer-owned phases

The renderer should own explicit phases at minimum for:

- shadow depth
- main opaque
- alpha-tested / masked where needed
- sorted translucent
- decals / projected effects
- overlay / UI
- movie/video presentation
- debug visualization

Objects and scenes should provide data to these phases; they should not own the phase scheduling.

### 5. Persistent GPU registration

The modern path should move load-time or scene-activation work out of the frame loop:

- static mesh registration
- terrain page buffer construction
- static projector-to-static receiver association, if any projected effect survives
- material classification and pipeline-key derivation

## Renderer interface refactor target

The current renderer surface is too stateful and too DX8-shaped. The replacement contract should be explicit about what changes and who must move.

| Current contract | Planned replacement | Primary consumers to update |
| --- | --- | --- |
| `WW3D::Set_Transform`, `Set_Material`, `Set_Shader`, `Set_Texture`, `Set_Light_Environment`, `Set_Vertex_Buffer`, `Set_Index_Buffer`, `Submit_Current_Triangles`, `Insert_Sorted_Triangles` | Frame/view submission API built around immutable draw packets and explicit queue insertion | `ww3d2/mesh.cpp`, `wwphys/renegadeterrainpatch.cpp`, `ww3d2/render2d.cpp`, `BinkMovie/BINKMovie.cpp`, scene render code |
| `DX8Wrapper` render state, texture-stage state, light slots, fog state | explicit scene/view/draw descriptors plus renderer-owned uniform/resource binding | `ww3d2/dx8wrapper*`, `ww3d2/bgfxrenderer*`, `ww3d2/scene.cpp`, `wwphys/pscene_lighting.cpp` |
| `ShaderClass`, `VertexMaterialClass`, `MaterialPassClass` as mutable state installers | renderer-facing material key + pipeline state + shader parameter payload | `ww3d2/shader*`, `ww3d2/vertmaterial*`, `ww3d2/matpass*`, `ww3d2/meshmatdesc*`, `ww3d2/meshmdlio.cpp` |
| mapper side effects through `D3DTSS_*` and texture transforms | direct shader parameter blocks for texgen, bump-env, and projection data | `ww3d2/mapper*`, `ww3d2/matrixmapper*`, `ww3d2/bgfxrenderer.cpp` |
| `RenderObjClass::Render` / `Special_Render` as render scheduling hooks | extraction-oriented object/scene contracts that append geometry/effect data into renderer-owned phases | `ww3d2/rendobj.h`, `ww3d2/mesh*`, `ww3d2/hlod*`, `wwphys/phys.cpp`, `wwphys/pscene.cpp` |
| `RenderInfoClass` additional passes and delayed procedural pass replay | explicit shadow/decal/projected-effect collectors and phase queues | `ww3d2/rinfo*`, `ww3d2/dx8renderer*`, `ww3d2/mesh.cpp`, `wwphys/renegadeterrainpatch.cpp`, `wwphys/pscene_projectors.cpp` |
| `Render2DClass` immediate overlay submission | renderer-owned overlay batch collector with persistent dynamic/streaming buffers | `ww3d2/render2d*`, `wwui/*`, HUD/menu code |
| `BINKMovie` bespoke render path | video presenter packets submitted through the overlay/view system | `BinkMovie/BINKMovie.cpp` |

### Interface rules the new contract must enforce

- no draw depends on hidden global render-state mutation
- no live renderer API depends on `D3DLIGHT8`, `D3DMATERIAL8`, `D3DRS_*`, `D3DTSS_*`, or FVF identifiers
- no ordinary runtime draw path depends on `DX8Wrapper::Apply_Render_State_Changes()`
- all views carry fog, lighting, shadow, clear, and camera data explicitly
- overlay and movie rendering use the same frame/view resource lifetime model as 3D rendering

## Planned execution order

The sequence below is the recommended order because later batching work will not hold if the old interfaces, old lighting contract, or receiver-side projector replay remain alive.

### Phase 1: break the renderer contract on purpose

**Objective**

- replace the active runtime renderer interface up front instead of layering new behavior under old `WW3D::Set_*` and `DX8Wrapper` calls

**Primary code areas**

- `Code/ww3d2/ww3d.h`
- `Code/ww3d2/ww3d.cpp`
- `Code/ww3d2/dx8wrapper*`
- `Code/ww3d2/bgfxrenderer*`
- `Code/ww3d2/scene.cpp`
- `Code/ww3d2/render2d*`
- `Code/BinkMovie/BINKMovie.cpp`
- immediate mesh and terrain submission sites

**Work**

- define the new frame/view/draw submission surface
- reduce `WW3D` to lifecycle, diagnostics, and renderer-front-door responsibilities
- stop allowing runtime render code to mutate global state and then submit implicitly
- update 3D, 2D/UI, and video consumers together so the hard break is real

**Exit condition**

- the main runtime no longer renders by chaining `Set_*` calls into `Submit_Current_Triangles`

**Status**

- substantially implemented for the maintained runtime path
- `WW3D::FixedFunctionSubmitDesc` and `WW3D::FixedFunctionStateDesc` now carry mesh, terrain, and sorted draws into explicit bgfx submission instead of replaying the old mutable wrapper contract at submit time
- `DX8TextureCategoryClass::Render`, `MeshClass::Render_Material_Pass`, `RenegadeTerrainPatchClass`, and `SortingRendererClass` now submit explicit draw packets with world/view/projection, shadow flags, lighting packets, and fixed-function state
- `VertexMaterialClass`, `TextureMapperClass`, and the projector-oriented `MatrixMapperClass` now synthesize texcoord-index, texture-transform, and bump-environment submission state directly for the active bgfx path instead of relying on wrapper-side `Apply_Render_State_Changes()` side effects
- overlay and movie submission were already on the explicit submission side and now align with the same renderer-owned packet model
- this phase is **not** globally finished for every legacy/debug path because `WW3D::Set_*`, `DX8Wrapper`, and fixed-function-era state objects still exist outside the maintained runtime path

### Phase 2: replace the lighting and shadow contract

**Objective**

- unify lighting and shadowing around renderer-owned scene/view data

**Primary code areas**

- `Code/wwphys/pscene_lighting.cpp`
- `Code/wwphys/pscene_collision.cpp`
- `Code/wwphys/lightsolve*`
- `Code/ww3d2/lightenvironment*`
- `Code/ww3d2/bgfxrenderer*`
- `Code/ww3d2/shadowmap*`
- `Code/ww3d2/shaders/fs_mesh.sc`
- `Code/ww3d2/shaders/vs_mesh.sc`
- `Code/ww3d2/shaders/shadow_common.sh`

**Work**

- build scene light data once per frame instead of per object
- remove the renderer’s dependency on `LightEnvironmentClass` and DX8 light-slot emulation for ordinary lighting
- carry shadow receive/cast flags explicitly in packets
- make shadow collection conservative enough that no supported caster class is skipped
- preserve truly unlit materials, but stop using prelit/user-lighting contracts as the normal world-lighting path

**Exit condition**

- meshes, terrain, and dynamic objects consume one shared GPU lighting/shadow contract, and the contract does not assume four lights

**Status**

- substantially implemented for ordinary runtime world rendering
- `PhysicsSceneClass` now prepares one frame lighting packet and the maintained runtime path consumes that shared submission directly instead of installing `LightEnvironmentClass` as the normal renderer contract
- the bgfx mesh lighting contract and `fs_mesh.sc` were widened from four lights to sixteen lights, removing the old four-light submission ceiling from the active runtime path
- mesh, terrain, and sorted fixed-function draws now carry receive/cast shadow flags and shared lighting packets explicitly through renderer-owned submission data
- `LightEnvironmentClass` still exists for fallback, legacy, and special-case paths, so the broader codebase is not yet fully free of the old lighting model even though the maintained runtime path has crossed the phase boundary

### Phase 3: move scheduling out of objects and into renderer phases

**Objective**

- replace object-local delayed pass plumbing with renderer-owned queues

**Primary code areas**

- `Code/ww3d2/rendobj.h`
- `Code/ww3d2/rinfo*`
- `Code/ww3d2/scene*`
- `Code/ww3d2/sortingrenderer*`
- `Code/ww3d2/mesh*`
- `Code/wwphys/phys.cpp`
- `Code/wwphys/pscene.cpp`

**Work**

- remove `Additional_Pass_Count()`-style replay from the main world path
- define renderer phases explicitly and give each one a queue owner
- convert `Special_Render` users into explicit debug/effect/shadow/overlay providers or retire them
- keep sorted translucency as a distinct phase, but not as a side channel owned by arbitrary objects

**Exit condition**

- phase ownership is centralized in the renderer and scene extraction, not distributed across render objects

**Status**

- substantially implemented for the maintained runtime world path
- `PhysicsSceneClass` now owns a distinct effect/material-pass phase queue, so main-path extra passes are collected during scene extraction and submitted by the renderer after the base world phase instead of being scheduled from inside `PhysClass::Render()`
- runtime material effects are now gathered into an explicit `RenderEffectCollection` packet during scene extraction instead of mutating `RenderInfoClass` via `Push_Material_Pass()` / `Additional_Pass_Count()` and then copying that legacy state back out into the scene queue
- queued runtime effect passes now submit through explicit mesh/container material-pass entry points, so the maintained world path no longer re-enters the normal object render path just to replay those passes
- `RenderObjClass` now exposes explicit material-pass and visibility providers, and the active runtime visibility path consumes those providers instead of routing through `Special_Render`
- legacy compatibility hooks still exist for projector-era shadow/material replay, debug rendering, terrain compatibility, and older object code paths, so the wider tree is not yet fully free of `Special_Render`/`Additional_Pass_Count()` compatibility plumbing even though the maintained runtime path has crossed the phase boundary

### Phase 4: modernize mesh materials and opaque mesh submission

**Objective**

- replace FVF/category/pass-era mesh submission with persistent mesh registration and explicit material keys

**Primary code areas**

- `Code/ww3d2/mesh.cpp`
- `Code/ww3d2/dx8renderer*`
- `Code/ww3d2/bgfxrenderer.cpp`
- `Code/ww3d2/shader*`
- `Code/ww3d2/vertmaterial*`
- `Code/ww3d2/matpass*`
- `Code/ww3d2/meshmatdesc*`
- `Code/ww3d2/meshmdlio.cpp`

**Work**

- replace ordinary rigid opaque FVF/category submission with renderer-owned submesh/material/pipeline registration
- keep authored material meaning, but stop treating texture-category visible lists as the runtime owner for rigid opaque draws
- pre-classify rigid opaque materials into renderer-facing pipeline keys and cached classification records
- keep registration persistent across mesh instances keyed by model plus user-lighting identity
- keep skins, sorted/translucent meshes, aligned/oriented billboard cases, and terrain on compatibility paths until their dedicated phases land

**Exit condition**

- opaque mesh submission is keyed by persistent geometry registration and explicit material state, not by DX8 wrapper mutation

**Status**

- substantially implemented for ordinary rigid opaque runtime meshes
- `DX8MeshRendererClass` now owns persistent rigid registrations keyed by `(MeshModelClass*, UserLightingArray*)`, with explicit submesh draw ranges, cached material classification, and renderer-owned visible draw queues
- `MeshClass::Render()` now routes eligible rigid opaque base submission through that registration path instead of linking polygon renderers into visible texture-category lists
- `MeshClass` now tracks explicit runtime render-registration state and delegates opaque base submission, material-pass replay, and skin queueing back through renderer-owned entry points instead of probing FVF/category internals
- compatibility effect/material-pass replay for those meshes now goes through renderer-owned registered-draw submission rather than relying on legacy polygon-renderer lists being present
- user-lighting changes now invalidate rigid registrations before mutating baked color/material inputs, so registered opaque buffers rebuild against the correct `(model, user-lighting)` identity instead of reusing stale cached geometry
- dead texture/material category-mutation helpers that only existed for the old polygon-renderer ownership model have been removed from the maintained runtime renderer surface
- legacy compatibility still owns skins, sorted/translucent meshes, aligned/oriented camera-facing meshes, terrain, decal replay, and delayed legacy material-pass plumbing
- rigid registration is still created lazily on first runtime use rather than eagerly at scene activation, so the architectural cutover is in place but the final preload timing goal remains open

### Phase 5: replace terrain’s separate renderer with terrain batch pages

**Objective**

- make terrain a first-class participant in the same modern renderer model

**Primary code areas**

- `Code/wwphys/renegadeterrainpatch*`
- `Code/wwphys/renegadeterrainmaterialpass*`
- `Code/wwphys/terrainmaterial*`
- `Code/wwphys/lightsolve*`
- `Code/wwphys/pscene.cpp`

**Work**

- move terrain buffer construction out of `Render()`
- introduce scene-managed terrain page buffers and per-material/page draw ranges
- preserve patch identity for collision, culling, and gameplay even when pages are merged on the GPU
- keep the authored terrain look, but stop treating patch-local submission and patch-local buffer ownership as architectural requirements

**Exit condition**

- visible terrain renders through scene-managed batches rather than patch-local pass loops

**Status**

- implemented for runtime terrain submission
- `TerrainRenderBatchManagerClass` now owns terrain batch pages keyed by `RenegadeTerrainPatchClass *`, with one combined render vertex/index buffer per terrain page and explicit per-material/pass draw ranges
- `PhysicsSceneClass::Pre_Render_Processing()` prepares visible terrain pages before rendering, and `PhysicsSceneClass::Render_Object()` routes terrain base and material-effect passes through the scene-owned terrain batch manager
- `RenegadeTerrainMaterialPassClass` no longer owns render vertex/index buffers; it now only carries terrain source material/layer data used to build scene pages
- `RenegadeTerrainPatchClass::Render()` remains as a compatibility fallback, but it builds a temporary terrain batch page instead of restoring patch-local buffer ownership
- terrain draw ranges explicitly keep shadow receiving enabled for both base and alpha terrain layers, and terrain material/projector pass replay covers the same terrain layer ranges as the old patch-local loops
- collision, culling, surface lookup, lighting solve data, UV generation, and terrain patch identity remain on `RenegadeTerrainPatchClass`

### Phase 6: delete projector-era runtime ownership and reintroduce only explicit systems

**Objective**

- delete projector-driven receiver replay as a renderer architecture, and keep only explicit projected-effect systems that still justify their cost

**Primary code areas**

- `Code/ww3d2/texproject*`
- `Code/ww3d2/matrixmapper*`
- `Code/ww3d2/mesh.cpp`
- `Code/ww3d2/rinfo*`
- `Code/ww3d2/projector*`
- `Code/wwphys/pscene_projectors.cpp`
- `Code/wwphys/projectormanager*`
- `Code/wwphys/phystexproject.cpp`
- `Code/wwphys/physdecalsys*`
- `Code/Commando/systemsettings.cpp`
- `Code/Commando/dlgconfigperformancetab.cpp`
- `Code/Commando/consolefunction.cpp`

**Work**

- finish collapsing legacy shadow/projector modes onto the shadow-map path
- split surviving effects into clear buckets: shadow maps, decals, bounded projected effects, or removal; do not keep a generic runtime projector bucket
- replace camera-space texgen and receiver-side `MaterialPassClass` replay with explicit world-space effect data owned by the renderer
- if a projected effect survives for runtime content, keep it as renderer-owned data and precompute static projector-to-static-world associations where practical
- remap or remove runtime settings, console hooks, and debug toggles that only advertise dead projector-era behavior
- never reintroduce receiver-side extra-pass replay, transient receiver effect attachment, or per-projector receiver re-render as the default effect mechanism
- do not carry editor-only projector/debug behavior forward as part of this refactor

**Exit condition**

- projectors are no longer a structural reason that static meshes or terrain re-render per receiver, and no maintained runtime path depends on camera-space projector state or receiver-side pass replay

**Status**

- implemented for the maintained runtime path
- `PhysicsSceneClass::Render_Objects()` no longer injects projector replay before world rendering, and the old receiver-side `Apply_Projectors(...)` / `Apply_Projector_To_Objects(...)` path has been removed
- the maintained runtime world path no longer depends on transient receiver effect attachment or `RenderInfoClass` additional-pass replay to express projected effects
- `StaticAnimPhysClass` no longer instantiates legacy object-owned texture projectors; `ProjectorManagerDefClass` is asset-compatibility input only and is no longer a renderer feature boundary
- remaining projector-era runtime settings and debug/config surfaces are cleanup targets or migration shims, not architectural commitments for the modern renderer

### Phase 7: converge dynamic, sorted, skinned, overlay, and video paths under the same renderer ownership

**Objective**

- finish the remaining consumer paths without backsliding into legacy abstractions

**Primary code areas**

- `Code/ww3d2/bgfxdynamicbuffer.cpp`
- `Code/ww3d2/vertexbuffer*`
- `Code/ww3d2/indexbuffer*`
- `Code/ww3d2/mesh*`
- `Code/ww3d2/render2d*`
- `Code/BinkMovie/BINKMovie.cpp`
- selected `wwui`, HUD, and movie-overlay users

**Work**

- collapse ordinary dynamic rendering onto bgfx-native dynamic/streaming buffers
- keep CPU deformation only where sorted translucent geometry truly still requires it
- keep GPU skinning as the default for non-sorted skins
- move overlay/UI/video submission to renderer-owned overlay queues and shared transient/persistent buffer management

**Exit condition**

- every maintained runtime rendering path is owned by one renderer architecture, not by a mix of scene-local and legacy DX8-era helpers

### Phase 8: delete the legacy surfaces and clean the outward-facing runtime settings

**Objective**

- make the modernization irreversible by removing the old contracts instead of keeping dead compatibility layers around

**Primary code areas**

- `Code/ww3d2/dx8wrapper*`
- `Code/ww3d2/dx8renderer*`
- `Code/ww3d2/mapper*`
- `Code/ww3d2/matrixmapper*`
- `Code/Commando/systemsettings.cpp`
- `Code/Commando/dlgconfigperformancetab.cpp`
- `Code/Tools/WWConfig/PerformanceConfigDialog.cpp`
- `Code/Commando/consolefunction.cpp`

**Work**

- delete renderer-facing D3D8 state arrays, texture-stage emulation, FVF/category ownership, and pass replay infrastructure
- rename, remove, or remap runtime settings that still expose projector-era or DX8-era concepts
- verify the runtime render path no longer depends on D3D8-era symbols except where legacy asset interpretation still needs them as source data during loading

**Exit condition**

- the runtime renderer can no longer fall back to DX8-era internal contracts

## Consumer update map

The interface break is only complete if the major consumers move with it.

### `ww3d2`

- own the renderer frontend, queueing, material keys, geometry registration, and bgfx submission
- stop exposing mutable DX8-like state as the language of rendering
- reduce `RenderObjClass` participation to geometry/effect extraction instead of scheduling

### `wwphys`

- provide light data, visibility results, terrain ownership, and projected-effect source data
- stop pushing projector replay and per-object light-environment state into the renderer
- preserve gameplay-driven lighting invalidation events as inputs to light/shadow data rebuilds

### `wwui` and overlay/HUD users

- migrate `Render2DClass` and text/quad users to overlay packet collection
- stop depending on legacy `ShaderClass` state mutation as the overlay contract

### `BinkMovie`

- render movies through the overlay/video phase using explicit texture bindings and view setup
- share upload, sampler, and lifetime rules with the rest of the renderer

### Runtime settings and diagnostics

- clean up `Shadow_Mode`, projector toggles, and related debug/config surfaces
- expose only settings that still map to the new renderer architecture

## Areas where the plan must stay conservative

The refactor is explicitly allowed to be conservative in ways that help correctness or throughput:

- batching more than the exact visible triangle set is acceptable
- including extra shadow casters is acceptable
- preserving material meaning is required, but preserving old draw boundaries is not
- staging the exact GPU light-selection method is acceptable, but the interface must not reintroduce DX8 light slots or per-object light caches

## Areas that need special care

1. **Static-lighting appearance**
   - Baked/static-lighted content must keep the intended look even as the runtime stops using `UserLighting` arrays and terrain prelit branches as the active lighting architecture.
2. **Terrain identity**
   - Terrain patches may merge into larger renderer pages, but gameplay, collision, and culling still reason about the patch/domain ownership in `wwphys`.
3. **Material semantics**
   - Authored unlit, emissive, alpha-tested, texgen, and bump-env semantics must survive even as their implementation becomes explicit shader data instead of DX8 state.
4. **Sorted/translucent geometry**
   - This path should not block opaque-world modernization, but the renderer still needs a deliberate plan for ordering and CPU-generated exceptions.
5. **Shadow correctness**
   - The new shadow collector must include all supported caster classes before aggressive optimization work begins.

## Recommended checkpoints for the implementation effort

Use these as the high-level gates for the eventual implementation work:

1. the main runtime compiles and renders through the new explicit renderer interface without relying on `WW3D::Set_*`-style state mutation
2. meshes, terrain, and dynamic objects share one renderer-owned light/shadow contract
3. receiver-side projector replay is gone from the main world path
4. opaque static meshes and terrain submit through scene-managed batches
5. overlay/UI/video submit through renderer-owned overlay/video queues
6. runtime settings no longer expose projector-era semantics as if they were still first-class runtime architecture
7. the active runtime renderer path no longer depends on D3D8-era internal contracts

## Bottom line

The right refactor is **not** “keep the old D3D8 contract and make bgfx hide it better.” The right refactor is to break the renderer interface deliberately, move all maintained runtime consumers onto explicit scene/view/draw data, centralize scheduling inside the renderer, unify lighting and shadowing, modernize mesh and terrain submission, and then delete the DX8-era layers that currently prevent large-batch, low-CPU rendering.

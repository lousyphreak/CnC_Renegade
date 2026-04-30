# Static Mesh and Terrain Batching Audit

Last updated: 2026-04-29

This document audits the current static mesh, projector, and terrain rendering paths that still reflect the old projector-driven DX8 design. The goal is to define the concrete code areas, data flow, batching blockers, and migration path toward a modern renderer that favors larger batched GPU submissions over many small per-object and per-layer draw calls.

This second pass focuses specifically on which projector and related runtime data is **actually dynamic**, which work is only dynamic because of the legacy DX8 projector implementation, and which parts should move to load or scene-activation time. It intentionally excludes LevelEdit/editor concerns and does **not** preserve code-level backward compatibility with the old projector system.

## Executive summary

The repository currently has **two separate legacy static-world rendering pipelines** that both work against modern GPU batching:

1. **Static mesh rendering in `ww3d2`**
   - uses `DX8MeshRendererClass`, FVF containers, and texture categories to reduce state changes
   - still emits per-task draw submissions and is heavily constrained by per-object transforms, per-object lighting state, sorted paths, and projector-driven extra passes
2. **Terrain rendering in `wwphys`**
   - completely bypasses the ww3d2 mesh-category batching system
   - renders each visible terrain patch as a sequence of per-layer, per-pass draws with patch-local vertex/index buffers

The current lighting model is also split across **three different contracts**:

1. **CPU-selected per-object light environments**
   - `PhysicsSceneClass::Compute_Static_Lighting(...)`
   - `PhysClass::Get_Static_Lighting_Environment()`
   - `LightEnvironmentClass`
2. **CPU-baked vertex lighting for prelit/static content**
   - `LightSolveClass`
   - mesh `UserLighting` arrays
   - terrain `VertexColors[]` + `IsPreLit`
3. **GPU shading and GPU shadow sampling in the bgfx path**
   - `BgfxRenderer::Apply_Lighting_Uniforms(...)`
   - `fs_mesh.sc`
   - `ShadowMapManager`

The old projector system is the largest architectural blocker to a clean modern batching model:

- projectors are not just "shadow data"; they are injected as **additional material passes** through `RenderInfoClass`
- `PhysicsSceneClass::Apply_Projectors()` finds intersecting receivers and attaches `SimpleEffectClass` to each object
- each affected object then re-renders through `MeshClass::Render_Material_Pass()` or terrain's `Render_Procedural_Material_Pass()`
- that turns projector cost into roughly **receiver count × active projectors × material splits**

The path forward is **not** to bolt more batching onto the projector path. The better direction is to:

1. remove projector-driven shadowing and terrain/static-mesh lighting passes from the main static-world path
2. converge static meshes and terrain onto a shared scene-level batch collector
3. preserve only the projector semantics that still matter by moving them into explicit modern systems such as:
   - shadow maps
   - decals
   - limited projected-light/decal passes with explicit batching rules
4. move non-gameplay-static work out of the frame loop:
   - static mesh registration
   - terrain buffer construction
   - static projector receiver association
   - projector material/pipeline derivation

The code review also shows that much of the current projector "dynamic" cost is not true gameplay dynamism:

- `MatrixMapperClass` and `TexProjectClass::Pre_Render_Update(...)` rebuild projector state from the current **camera** because the old path projects from eye space
- static meshes defer registration until the first visible frame even though the work is structurally registration-time work
- terrain defers buffer rebuilds until `Render()` even though shipped terrain data is effectively static after load
- static projectors re-attach transient `SimpleEffectClass` wrappers to static receivers every frame even when neither side changed

The same pattern exists in lighting:

- non-prelit objects still pay per-object CPU light collection and cache maintenance
- static light solves still push lighting into vertex colors and saved user-lighting blobs
- the bgfx renderer already shades and shadows on the GPU, but it still receives lights through a DX8-era four-light emulation layer

## Scope and primary files

### Static mesh pipeline

- `Code/ww3d2/mesh.cpp`
- `Code/ww3d2/mesh.h`
- `Code/ww3d2/meshmdl.h`
- `Code/ww3d2/meshmatdesc.h`
- `Code/ww3d2/meshmdlio.cpp`
- `Code/ww3d2/dx8renderer.cpp`
- `Code/ww3d2/dx8renderer.h`
- `Code/ww3d2/dx8wrapper.cpp`
- `Code/ww3d2/bgfxrenderer.cpp`
- `Code/ww3d2/rinfo.h`
- `Code/ww3d2/scene.cpp`
- `Code/ww3d2/sortingrenderer.cpp`

### Projector and effect injection pipeline

- `Code/ww3d2/projector.h`
- `Code/ww3d2/projector.cpp`
- `Code/ww3d2/texproject.h`
- `Code/ww3d2/texproject.cpp`
- `Code/ww3d2/matpass.h`
- `Code/ww3d2/matpass.cpp`
- `Code/wwphys/materialeffect.h`
- `Code/wwphys/materialeffect.cpp`
- `Code/wwphys/pscene_projectors.cpp`
- `Code/wwphys/phystexproject.cpp`
- `Code/wwphys/projectormanager.h`
- `Code/wwphys/projectormanager.cpp`
- `Code/wwphys/staticanimphys.cpp`

### Terrain pipeline

- `Code/wwphys/renegadeterrainpatch.h`
- `Code/wwphys/renegadeterrainpatch.cpp`
- `Code/wwphys/renegadeterrainmaterialpass.h`
- `Code/wwphys/renegadeterrainmaterialpass.cpp`
- `Code/wwphys/terrainmaterial.h`
- `Code/wwphys/terrainmaterial.cpp`
- `Code/wwphys/lightsolve.cpp`

### Lighting, shadow, and material-state pipeline

- `Code/wwphys/pscene_lighting.cpp`
- `Code/wwphys/pscene_collision.cpp`
- `Code/wwphys/lightsolve.h`
- `Code/wwphys/lightsolve.cpp`
- `Code/ww3d2/light.h`
- `Code/ww3d2/light.cpp`
- `Code/ww3d2/lightenvironment.h`
- `Code/ww3d2/lightenvironment.cpp`
- `Code/ww3d2/vertmaterial.h`
- `Code/ww3d2/vertmaterial.cpp`
- `Code/ww3d2/dx8wrapper.h`
- `Code/ww3d2/dx8wrapper.cpp`
- `Code/ww3d2/bgfxrenderer.h`
- `Code/ww3d2/bgfxrenderer.cpp`
- `Code/ww3d2/shadowmap.h`
- `Code/ww3d2/shadowmap.cpp`
- `Code/ww3d2/shaders/vs_mesh.sc`
- `Code/ww3d2/shaders/fs_mesh.sc`
- `Code/ww3d2/shaders/shadow_common.sh`

### Runtime integration touchpoints

- `Code/wwphys/pscene.cpp`
- `Code/wwphys/staticphys.cpp`
- `Code/wwphys/pscene_lighting.cpp`
- `Code/wwphys/staticaabtreecull.h`
- `Code/wwphys/dynamicaabtreecull.h`
- `Code/wwphys/physdecalsys.h`
- `Code/wwphys/visrendercontext.h`
- `Code/ww3d2/meshmdlio.cpp`
- `Code/ww3d2/meshmatdesc.h`
- `Code/ww3d2/w3d_file.h`
- `Code/Commando/dlgconfigperformancetab.cpp`
- `Code/Commando/systemsettings.cpp`
- `Code/Commando/shutdown.cpp`
- `Code/Tools/WWConfig/PerformanceConfigDialog.cpp`
- `Code/Commando/consolefunction.cpp`

## Explicit non-goals

This audit does **not** treat the following as architectural constraints:

1. `Tools/LevelEdit/*` behavior or data flow
2. `Tools/W3DView/*` behavior or data flow
3. `Tools/max2w3d/*` exporter compatibility
4. code-level compatibility wrappers for old projector-era APIs, UI toggles, or helper paths

If editor/exporter/runtime wrappers need follow-up later, they should adapt to the new runtime renderer rather than shaping it.

## Current frame flow

The static-world render path is assembled in `PhysicsSceneClass`, but the actual submissions split once meshes and terrain start rendering.

### Scene preparation

`PhysicsSceneClass::Pre_Render_Processing()` in `Code/wwphys/pscene.cpp`:

1. collects visible static and dynamic objects
2. updates shadow-map cascade visibility
3. runs `Optimize_LODs(...)`
4. runs `Apply_Projectors(camera)`

That last step is where the legacy projector system injects itself into the frame before normal object rendering.

### Projector injection

`PhysicsSceneClass::Apply_Projectors()` in `Code/wwphys/pscene_projectors.cpp`:

1. frustum-culls static and dynamic projectors
2. applies attenuation and optional RTT assignment
3. calls `Apply_Projector_To_Objects(projector, camera)` for each active projector

`Apply_Projector_To_Objects(...)` then:

1. creates `SimpleEffectClass effect(tex_proj->Peek_Material_Pass())`
2. collects intersecting static and dynamic receivers via the scene culling systems
3. calls `obj->Add_Effect_To_Me(effect)` on each visible receiver
4. calls `tex_proj->Pre_Render_Update(camera_tm)` if the projector is actually needed this frame

This is the key coupling point: **projectors do not submit directly**. They mutate receiver objects by pushing a `MaterialPassClass` into the receiver's effect stack.

### Object render contract

`PhysClass::Render(RenderInfoClass & rinfo)` in `Code/wwphys/phys.cpp`:

1. `Push_Effects(rinfo)`
2. `Model->Render(rinfo)`
3. `Pop_Effects(rinfo)`

`SimpleEffectClass::Render_Push()` in `Code/wwphys/materialeffect.cpp` calls `rinfo.Push_Material_Pass(MatPass)`, so the receiver's render object sees projector passes through `RenderInfoClass`.

## Static mesh submission path

### Base mesh rendering

`MeshClass::Render(RenderInfoClass & rinfo)` in `Code/ww3d2/mesh.cpp`:

1. skips hidden meshes
2. diverts sorted meshes to static sort lists when enabled
3. lazily registers mesh types into `TheDX8MeshRenderer`
4. queues base-pass polygon renderers into texture categories through `polygon_renderer->Get_Texture_Category()->Add_Render_Task(...)`
5. queues projector and other extra passes through:
   - `Add_Visible_Material_Pass(...)`
   - `Add_Delayed_Visible_Material_Pass(...)`
6. queues skin meshes separately when needed

This means mesh rendering is already split into:

- **base-pass task registration**
- **later flush of queued tasks**

### Registration-time batching that already exists

The current system does perform some useful batching work at mesh-registration time.

#### 1. FVF container batching

`DX8RigidFVFCategoryContainer` groups rigid meshes by vertex format and builds shared static vertex/index buffers.

Relevant code:

- `Code/ww3d2/dx8renderer.cpp`
- `Vertex_Split_Table`
- `DX8RigidFVFCategoryContainer::Add_Mesh(...)`
- `DX8FVFCategoryContainer::Generate_Texture_Categories(...)`

This stage:

1. appends mesh vertices into a container VB
2. appends indices into a container IB
3. groups polygons into texture/material/shader buckets

#### 2. Texture-category bucketing

`DX8TextureCategoryClass` is keyed by:

- pass
- textures for both stages
- shader
- vertex material

That reduces state changes, but not enough to qualify as a modern batch model. It still yields many draw submissions and keeps all state decisions in a fixed-function-style global pipeline.

#### 3. Shared model reuse

`DX8MeshRendererClass::_RegisteredMeshTable` reuses polygon-renderer structure for identical `(MeshModelClass*, UserLightingArray)` pairs. This is a form of static instance reuse, but it is still not true modern instancing.

### Actual draw-call boundary for meshes

The most important current boundary is in `DX8TextureCategoryClass::Render()` in `Code/ww3d2/dx8renderer.cpp`.

After state is installed, each visible task is still submitted through `BgfxRenderer::Submit_Classified_Draw(...)`.

For rigid opaque meshes this currently means:

- one FVF-container-level VB/IB bind
- then one draw submission per visible polygon-renderer task

So the current system is better described as **state-change minimization with shared buffers**, not as a true scene-level draw merger.

### Projector/extra-pass mesh path

`MeshClass::Render_Material_Pass(MaterialPassClass * pass, IndexBufferClass * ib)` in `Code/ww3d2/mesh.cpp` is the legacy projector hot path.

It handles three cases:

1. skin meshes
2. rigid meshes with per-polygon cull volume
3. rigid meshes without cull volume

For rigid meshes with cull volume, it:

- generates an active polygon table from the projector volume
- builds a dynamic IB
- submits a draw for the intersecting triangles

For rigid meshes without cull volume, it:

- iterates `PolygonRendererList`
- submits once per polygon renderer

This is where the projector path explodes draw count: every receiver mesh re-enters the renderer through its own projector material pass.

## Terrain submission path

`RenegadeTerrainPatchClass` in `Code/wwphys/renegadeterrainpatch.cpp` is a fully separate rendering system.

### Terrain data model

Each patch stores:

- `Grid[]` positions
- `GridNormals[]`
- `VertexColors[]`
- `QuadFlags[]`
- `MaterialPassList`

Each `RenegadeTerrainMaterialPassClass` stores:

- `VertexAlpha[]`
- `GridUVs[]`
- `QuadList[PASS_BASE/PASS_ALPHA]`
- `VertexRenderList[PASS_BASE/PASS_ALPHA]`
- `VertexIndexMap[...]`
- `VertexBuffers[...]`
- `IndexBuffers[...]`

### Terrain draw flow

`RenegadeTerrainPatchClass::Render(RenderInfoClass & rinfo)`:

1. rebuilds buffers if `AreBuffersDirty`
2. installs transform and light environment
3. loops all material layers and renders:
   - `PASS_BASE`
   - `PASS_ALPHA`
4. loops `rinfo.Additional_Pass_Count()` and calls `Render_Procedural_Material_Pass(matpass)`

`Render_By_Texture(...)` then:

1. binds the patch-local VB/IB for that layer and pass
2. binds texture 0
3. selects `BaseMaterial/BaseShader` or `LayerMaterial/LayerShader`
4. calls `WW3D::Submit_Current_Triangles(...)`

### Terrain batching reality

Terrain does **not** participate in:

- `DX8MeshRendererClass`
- FVF containers
- texture categories
- mesh registration reuse

Instead, each visible patch issues draws as:

- `visible patches × material layers × 2 passes`

And projector/extra passes multiply that again because `Render_Procedural_Material_Pass()` loops the terrain layers again.

### Terrain lighting contract

`VertexSolveClass::Light_Terrain(...)` in `Code/wwphys/lightsolve.cpp` bakes lighting into `VertexColors[]`.

That means the current terrain path depends on:

- per-vertex baked lighting data
- fixed terrain material semantics
- no separate lightmap-texture path

Any batching refactor must preserve this authored/baked-light behavior even if the submission path changes.

## Current lighting and shadow model

The renderer is no longer purely fixed-function, but the lighting contract is still inherited from the old CPU-driven design.

### 1. CPU per-object light environments for ordinary lit objects

`PhysicsSceneClass::Render_Object(...)` calls `PhysClass::Get_Static_Lighting_Environment()` for any non-prelit object, then stores the result in `RenderInfoClass::light_environment`.

That path currently does all of the following on the CPU:

- caches lighting per object in `PhysClass::StaticLightingCache`
- invalidates the cache when visibility/lighting changes through `Invalidate_Static_Lighting_Cache()`
- rebuilds the cache through `PhysicsSceneClass::Compute_Static_Lighting(...)`
- collects overlapping lights from `StaticLightingSystem`
- approximates lighting at the **object center**
- keeps only up to **four** active lights through `LightEnvironmentClass::MAX_LIGHTS`
- folds weaker lights into ambient through the lighting LOD cutoff

The resulting `LightEnvironmentClass` is then installed through:

- `MeshClass::Set_Lighting_Environment(...)`
- `DX8Wrapper::Set_Light_Environment(...)`
- `BgfxRenderer::Apply_Lighting_Uniforms(...)`

So even in the bgfx path, lighting still begins as per-object CPU approximation.

### 2. CPU-baked vertex lighting for static solves and terrain

`LightSolveClass` and `VertexSolveClass` still perform a completely different lighting path for static content:

- `LightSolveClass::Generate_Static_Light_Solve(...)` iterates static objects
- `VertexSolveClass::Light_Mesh(...)` computes ambient/diffuse per vertex on the CPU
- `VertexSolveClass::Add_Light_To_Vertex(...)` can ray-test for occlusion on the CPU
- the result is baked into mesh `UserLighting` arrays through `MeshClass::Install_User_Lighting_Array(...)`
- terrain lighting is baked into `RenegadeTerrainPatchClass::VertexColors[]` and enables `IsPreLit`

This has downstream consequences outside the solve itself:

- `RenderObjClass` persists user-lighting data through `Save_User_Lighting(...)` / `Load_User_Lighting(...)`
- `MeshClass::setup_materials_for_user_lighting()` rewrites material color-source behavior so solved vertex colors drive the result
- terrain flips `BaseMaterial` to `COLOR1` when `IsPreLit`
- `DX8MeshRendererClass::_RegisteredMeshTable` keys reuse on `(MeshModelClass*, UserLightingArray*)`, so static lighting fragments mesh sharing

This is not just "old offline lighting data"; it is a separate runtime render contract.

### 3. GPU lighting and GPU shadowing already exist in the bgfx path

The bgfx renderer is already much closer to the target architecture than the surrounding engine code implies:

- `fs_mesh.sc` performs world-space per-pixel lighting
- `ShadowMapManager` computes cascades and `shadow_common.sh` samples GPU shadow maps
- `BgfxRenderer::Apply_Lighting_Uniforms(...)` uploads ambient + light arrays directly to shader uniforms
- the fragment shader consumes:
  - `u_meshSceneAmbient`
  - `u_meshLightPosType[4]`
  - `u_meshLightDirSpot[4]`
  - `u_meshLightDiffuseRange[4]`
  - `u_meshLightAmbientAtten[4]`

So the codebase already has a GPU lighting destination format. The remaining issue is that it is still fed by:

- DX8-style render-state emulation
- four-light slots
- per-object CPU light selection
- prelit vertex-color bypasses for a large part of the static world

### 4. Dynamic lights are not actually unified today

The current CPU light collection path is incomplete:

- `PhysicsSceneClass::Collect_Lights(...)` has `dynamic_lights` parameters
- `PhysicsSceneClass::Add_Collected_Lights_To_List(...)` still contains `// TODO!!` for dynamic lights

That means a unified GPU lighting model is not only a batching improvement. It is also the cleanest path to a single lighting feature set for:

- static meshes
- terrain
- dynamic objects
- static lights
- dynamic lights

## How the current lighting model blocks modernization

### 1. Per-object CPU light environments fragment draw sharing

Lighting for ordinary objects is still expressed as mutable per-object state:

- `Render_Object(...)` computes/selects a `LightEnvironmentClass`
- `MeshClass::Render(...)` stores it per mesh
- `DX8Wrapper::Set_Light_Environment(...)` mutates the current light slots and ambient render state
- `BgfxRenderer::Apply_Lighting_Uniforms(...)` reads those emulated DX8 light slots back out again

That keeps lighting tied to the individual receiver/object boundary, which works directly against large material-centric static batches.

### 2. CPU light solves create a second render contract for static content

Meshes and terrain that go through `LightSolveClass` stop behaving like normally lit geometry:

- their lighting is baked into vertex colors
- their materials are reconfigured to use color arrays
- large parts of the static world become effectively "prelit/unlit" at draw time

That prevents a single light/shadow model from spanning all world geometry.

### 3. The current CPU approximation is architecturally limited

The existing `LightEnvironmentClass` path is constrained by old assumptions:

- four active lights maximum
- object-center approximation
- ambient folding of weaker lights
- per-object cache invalidation and rebuilds
- special-casing around prelit content

Those are exactly the kinds of compromises that a modern GPU light model is meant to eliminate.

## Additional D3D8-era renderer simplification targets

The projector and lighting audits expose a broader pattern: the bgfx backend is already executing shader-based rendering, but large parts of the runtime still describe rendering through **D3D8-era state emulation layers**. Those layers are now architectural debt in their own right and can be simplified independently of projector removal.

### 1. `DX8Wrapper` is still the main renderer contract

`Code/ww3d2/dx8wrapper.h` and `Code/ww3d2/dx8wrapper.cpp` still define the renderer in terms of:

- `RenderStates[256]`
- `TextureStageStates[stage][state]`
- `D3DLIGHT8`
- `D3DMATERIAL8`
- mutable world/view/projection state
- per-draw `Apply_Render_State_Changes()`

`bgfxdynamicbuffer.cpp` mirrors the same model for the bgfx path.

This is no longer just a backend detail. It forces the runtime to express rendering as "mutate fake device state, then let bgfx reconstruct it", which is the opposite of the explicit draw-state model that modern GPU renderers want.

**Simplification target**

- replace wrapper-wide mutable device state with explicit renderer-owned draw descriptors and uniform blocks
- remove D3D8 render-state IDs as the primary internal representation
- stop round-tripping modern bgfx submission through fake D3D8 state arrays

### 2. `ShaderClass`, `VertexMaterialClass`, and `MaterialPassClass` still encode fixed-function semantics

The following files still model rendering as a set of D3D8-style state objects:

- `Code/ww3d2/shader.h`
- `Code/ww3d2/shader.cpp`
- `Code/ww3d2/vertmaterial.h`
- `Code/ww3d2/vertmaterial.cpp`
- `Code/ww3d2/matpass.h`
- `Code/ww3d2/matpass.cpp`

They still revolve around:

- fixed-function blend/fog/cull/depth flags
- `D3DMCS_*` color-source semantics
- pass installation through wrapper mutation
- per-pass state application instead of immutable pipeline/material keys

This is one of the biggest remaining simplification opportunities. The runtime no longer needs a renderer contract built around `Apply()` methods that mutate global state.

**Simplification target**

- collapse shader/material/pass layers into a renderer-facing material descriptor
- treat fog/blend/depth/cull as pipeline state derived from the material key, not mutable global state
- keep authored material meaning, but stop preserving D3D8 state shape as the runtime API

### 3. Texture mappers are legacy texgen/state writers

The mapper stack remains tightly bound to D3D8 texture-stage behavior:

- `Code/ww3d2/mapper.h`
- `Code/ww3d2/mapper.cpp`
- `Code/ww3d2/matrixmapper.h`
- `Code/ww3d2/matrixmapper.cpp`

These classes still write:

- texture transforms
- `D3DTSS_TEXCOORDINDEX`
- `D3DTSS_TEXTURETRANSFORMFLAGS`
- bump-env stage-state values

The bgfx renderer then reads those DX8-style states back out in:

- `BgfxRenderer::Apply_Texgen_Uniforms(...)`
- `BgfxRenderer::Apply_Bump_Env_Uniforms(...)`

This is unnecessary indirection in a shader renderer.

**Simplification target**

- replace texgen and bump-env mapper side effects with direct shader parameters
- treat projection/env-map/bump-env behavior as shader variants or per-draw uniform payloads
- remove the dependency on DX8 texture-stage state as an intermediate language

### 4. Pass/stage/FVF limits are still shaping the architecture

The codebase still carries hard limits that reflect old device capabilities rather than modern renderer goals:

- `MeshMatDescClass::MAX_PASSES`
- `MeshMatDescClass::MAX_TEX_STAGES`
- `MaterialPassClass::MAX_TEX_STAGES`
- `DX8FVFCategoryContainer`
- `DX8TextureCategoryClass`

Those abstractions made sense for:

- fixed-function texture combiners
- FVF-based pipeline selection
- multi-pass draw replay under D3D8-era constraints

They should no longer define the top-level render architecture.

**Simplification target**

- move from pass/stage/FVF-centric renderer ownership to submesh/material/pipeline ownership
- keep only the limits that still matter for actual authored content, not because the runtime is organized around old hardware categories
- treat old 2-stage / 4-pass shapes as import-time source data, not renderer-wide architecture

### 5. Sorting and delayed pass systems are still legacy batching infrastructure

The following systems still keep draw ownership in scene/object code:

- `Code/ww3d2/sortingrenderer.h`
- `Code/ww3d2/sortingrenderer.cpp`
- static sort lists in `MeshClass::Render(...)`
- delayed procedural-material-pass handling in `dx8renderer.*`

These systems still perform:

- CPU sort-list insertion
- explicit late flushes
- special-case render ordering outside a unified renderer queue

Some of this remains necessary for translucent ordering, but the architecture itself is legacy.

**Simplification target**

- move toward renderer-owned draw-key sorting
- keep explicit alpha/special-pass phases, but stop using object-local static sort list plumbing as the main mechanism
- make delayed/procedural passes explicit render phases instead of hidden side channels inside mesh submission

### 6. Dynamic buffer code still preserves D3D lock/discard-era abstractions

The dynamic buffer path in:

- `Code/ww3d2/vertexbuffer.h`
- `Code/ww3d2/indexbuffer.h`
- `Code/ww3d2/bgfxdynamicbuffer.cpp`

still preserves:

- `BUFFER_TYPE_DYNAMIC_RENDER`
- `BUFFER_TYPE_DYNAMIC_SORTING`
- discard/retire-style write patterns
- CPU staging models inherited from old APIs

The bgfx renderer already supports more direct approaches, but the runtime still carries several layers of compatibility behavior around those old patterns.

**Simplification target**

- collapse to a smaller bgfx-native dynamic/streaming buffer model
- preserve only the distinct paths that still truly need CPU-resident sorting or CPU-generated geometry
- remove generic D3D-style dynamic-buffer emulation from ordinary dynamic submissions

### 7. Fog, light, and scene parameters still flow through wrapper state instead of view data

Fog and lighting are still fed through scene/wrapper compatibility paths:

- `SceneClass` stores fog state and applies it through the wrapper
- `LightEnvironmentClass` builds fake hardware lights
- `BgfxRenderer::Apply_Lighting_Uniforms(...)` reads `D3DRS_AMBIENT` and emulated `D3DLIGHT8`
- `SimpleSceneClass::Customized_Render()` still reflects four-light hardware assumptions

This is another case where the bgfx shader path is already more modern than the runtime contract above it.

**Simplification target**

- move fog and scene lighting inputs into explicit per-view/per-scene renderer data
- stop reading ambient/light data back out of DX8 wrapper state during bgfx submission
- remove the four-light runtime assumption entirely

### 8. Render-object interfaces still expose old renderer scheduling modes

`RenderObjClass` and `MeshClass` still expose renderer-owned behavior through object interfaces:

- `Render()`
- `Special_Render()`
- render-mode-specific behavior for VIS/shadow/special paths
- decal scheduling and pass scheduling from inside object render code

This keeps renderer scheduling distributed across scene code, render objects, and backend glue.

**Simplification target**

- move pass scheduling into renderer phases owned by the renderer
- keep render objects as geometry/material providers instead of mini render schedulers
- retire VIS/shadow special-mode plumbing once modern equivalents exist

### 9. bgfx still round-trips through DX8 concepts in several high-value paths

The current bgfx implementation already exposes the most profitable cleanup targets:

- `Classify_Material()` still interprets DX8-era material/source semantics
- `Apply_Lighting_Uniforms(...)` still consumes wrapper light slots
- `Apply_Bump_Env_Uniforms(...)` still consumes DX8 stage state
- `Apply_Texgen_Uniforms(...)` still consumes DX8 texture-transform state

This means the current renderer modernization should not stop at "make bgfx run the old pipeline". The better goal is to make bgfx own the pipeline directly.

## Where the current design blocks modern batching

### 1. Projectors are modeled as receiver-side re-render passes

This is the most important blocker.

Because projectors become `MaterialPassClass` entries on individual receivers:

- batching is broken at the receiver boundary
- work scales with number of affected objects
- projector state is expressed through mutable global render state instead of explicit batch data

### 2. `MatrixMapperClass` is per-draw global state

`TexProjectClass::Pre_Render_Update()` updates the projector view-to-texture matrix, and the mapper later programs DX8-style texture transform state through the wrapper.

That is fundamentally hostile to merging many projector-affected receivers into a single submission.

### 3. Terrain is a totally separate renderer

Terrain already has its own:

- persistence format
- material-pass representation
- VB/IB ownership
- layer composition logic

A modern batching system cannot just optimize `ww3d2` meshes and call the job done. It must either:

- unify terrain under the same batch collector, or
- create a terrain-specific modern batching path with equivalent architectural goals

### 4. Lighting and user-lighting arrays fragment mesh sharing

Static mesh registration reuse keys on:

- `MeshModelClass*`
- `UserLightingArray*`

Different precomputed lighting data breaks reuse and therefore limits shared batching.

### 5. CPU lighting and shadow state still flow through DX8-era interfaces

Even in the bgfx path, the renderer still leans on:

- `LightEnvironmentClass`
- `D3DLIGHT8`
- `DX8Wrapper::Set_Light_Environment(...)`
- `D3DRS_AMBIENT`
- four emulated hardware light slots

That means the shader backend is already modernizing faster than the scene/render contract above it.

### 6. Sorted and delayed paths keep side channels alive

These paths complicate convergence:

- static sort lists
- `SortingRendererClass`
- delayed material passes
- skin-specific dynamic buffer paths

Even if the new target focuses on opaque static mesh and terrain first, these code paths still impose constraints on how much legacy machinery can be removed cleanly.

## Existing batching and grouping worth preserving

Not everything in the old path should be discarded.

### Useful existing ideas

1. **Shared geometry registration**
   - static meshes with the same model already share registration artifacts
2. **Material bucketing**
   - texture/material/shader grouping is conceptually useful
3. **Visibility and spatial culling**
   - scene culling systems already produce the correct visible static sets
4. **Terrain patch decomposition**
   - terrain is already spatially chunked, which is useful for GPU-friendly page or cluster batching

### What should not be preserved as-is

1. projector additional-pass injection through `RenderInfoClass`
2. global fixed-function-style state installation as the primary render contract
3. patch-local terrain draw loops
4. per-projector receiver replay as the default way to apply static-world projected effects
5. per-object `LightEnvironmentClass` selection as the main light/shadow contract
6. CPU-baked user-lighting arrays as the main static-world lighting path
7. `DX8Wrapper` state mutation as the main language between scene code and bgfx
8. texture-stage/texgen mapper side effects as an intermediate representation
9. pass/stage/FVF-centric renderer ownership as the long-term architecture

## What is actually dynamic vs. what only looks dynamic

### Projector data that is effectively static

For most projectors, `ProjectorManagerClass::Init(...)` in `Code/wwphys/projectormanager.cpp` resolves nearly all meaningful configuration up front:

- projection mode and extents
- additive vs. multiplicative blend setup
- initial intensity
- texture lookup and clamp setup
- static-vs-dynamic projector ownership
- bone-name lookup for animated projectors
- the initial projector transform

For non-animated projectors, the transform itself is also effectively static after initialization. `TexProjectClass` then carries mostly fixed configuration data:

- `HFov`, `VFov`, `XMin`, `XMax`, `YMin`, `YMax`, `ZNear`, `ZFar`
- affect-static / affect-dynamic flags
- depth-gradient usage
- material-pass structure and mapper topology
- resolved texture or render-target binding choice

That is configuration or registration data, not per-frame batch input.

### Projector state that is truly dynamic

Only a smaller subset is inherently runtime-dynamic:

1. animated projector transform updates through `ProjectorManagerClass::Update_From_Model(...)`
2. receiver visibility / volume intersection against the current visible scene
3. intensity interpolation in `TexProjectClass::Pre_Render_Update(...)`
4. distance attenuation if that fade rule remains part of the feature set
5. dynamic-shadow render-target budgeting/assignment if a surviving shadow path still uses projector-owned RTTs

Those are the only parts that need true per-frame ownership in a modern design.

### Projector work that is only legacy-dynamic

Several expensive steps are only dynamic because the old implementation is camera-space and fixed-function-state-driven:

1. `TexProjectClass::Pre_Render_Update(const Matrix3D & camera)` rebuilds the projector transform from `Projection * Mworld-projector * Mcamera^-1`
2. `MatrixMapperClass::Apply(...)` mutates DX8 texture-transform state using camera-space position/normal texture-coordinate generation
3. projector texture size may be lazily resolved inside `Pre_Render_Update(...)`
4. `PhysicsSceneClass::Apply_Projector_To_Objects(...)` creates an auto-removed `SimpleEffectClass` and re-attaches it to every receiver each frame, even for static-projector/static-receiver cases

In a modern world-space shader path, static projector transforms do **not** need to be rebuilt per camera, and static receiver associations do **not** need to be represented as transient extra-pass wrappers.

There is also a strong sign that some legacy dynamic-shadow projector machinery is already vestigial in the current tree: `TexProjectClass::Needs_Render_Target()` only checks `TEXTURE_DIRTY`, and that flag is not actively driven anywhere in-tree. That path should not constrain the redesign unless projector-owned RTT generation is deliberately reintroduced.

### Other data currently treated as dynamic but mostly static

#### Static mesh registration

`MeshClass::Render(...)` still lazily registers rigid meshes when `PolygonRendererList` is empty, but `DX8MeshRendererClass::Register_Mesh_Type(...)` performs obviously registration-time work:

- FVF selection
- shared VB/IB population
- texture-category generation
- registered-mesh reuse table setup

This work should move to load time or scene activation so the first visible frame does not pay the cost and so the successor renderer can build persistent batch records up front.

#### Terrain buffer generation

`RenegadeTerrainPatchClass::Render(...)` rebuilds buffers only because the class defers `Update_Rendering_Buffers()` until draw time. In practice, the dirtying points are dominated by load/build-time operations:

- initial grid/material allocation
- `Update_UVs()`
- `Update_Vertex_Render_Lists()`
- initial material/pass composition

For shipped runtime content, that means terrain VB/IB construction is mostly load/finalization work, not true frame work. A modern path should build terrain page data before the patch ever reaches `Render()`.

#### Static lighting data setup

Static lighting already behaves like baked data:

- `LightSolveClass::Compute_Solve(...)` marks objects as having user lighting
- `MeshClass::Install_User_Lighting_Array(...)` converts baked solve data into packed vertex colors and adjusts material color sources
- `RenegadeTerrainPatchClass::Load_Variables(...)` restores `IsPreLit` at load time

The successor renderer should ingest that solved lighting into persistent batch data during load/scene activation instead of letting it fragment first-render registration.

#### Static projector-to-static-receiver association

For static projectors affecting static receivers, the expensive part is not that the relationship changes each frame. The expensive part is that the old path recomputes and re-materializes it every frame through:

- culling-system overlap queries
- transient effect allocation/linking
- receiver-side additional-pass replay

A modern replacement should precompute static projector-to-static-geometry associations during load/scene activation, then reduce frame-time work to visibility masking and batch submission.

## What a unified GPU lighting model would remove or simplify

### Major simplifications

Moving all light and shadow evaluation to the GPU, with one shared world-space light model for all geometry, would allow the codebase to retire or heavily simplify:

1. **Per-object CPU light caches**
   - `PhysClass::StaticLightingCache`
   - `Invalidate_Static_Lighting_Cache()`
   - `Get_Static_Lighting_Environment()`
   - `PhysicsSceneClass::Compute_Static_Lighting(...)`
2. **`LightEnvironmentClass` as a render-time approximation layer**
   - four-light cap
   - object-center approximation
   - ambient folding / lighting LOD cutoff
   - DX8 render-light packing
3. **CPU static light solves as a runtime rendering dependency**
   - `LightSolveClass`
   - mesh `UserLighting` arrays
   - terrain prelit vertex-color solves
   - user-lighting save/load paths in `RenderObjClass` and `MeshClass`
4. **DX8-style per-draw light state mutation**
   - `DX8Wrapper::Set_Light_Environment(...)`
   - reliance on `D3DRS_AMBIENT` + emulated `D3DLIGHT8` slots
5. **Lighting-specific batch fragmentation**
   - `_RegisteredMeshTable` reuse split by `UserLightingArray*`
   - terrain `IsPreLit` branch changing material color sources

This is a large cleanup, not a minor refactor.

The same conclusion applies to the renderer more broadly: once the runtime stops expressing rendering through fake D3D8 state, a large amount of wrapper, mapper, pass-installation, and category plumbing becomes simplifiable at the same time.

### What can stay as source scene data

The move does **not** require removing the scene's concept of lights. These systems still make sense as source data or culling inputs:

- `LightClass` / `LightPhysClass`
- sun orientation and ambient-light settings
- light enable/disable gameplay state
- static/dynamic culling ownership for light volumes
- shadow-caster / shadow-receiver flags

The key change is that they should feed a **shared GPU light/shadow data set**, not per-object CPU approximations.

### What the existing bgfx path already gives us

The current bgfx renderer already provides a useful bridge:

- world-space lighting in `fs_mesh.sc`
- GPU shadow sampling through `shadow_common.sh`
- per-material lighting mode in `MaterialClassification`

That means the migration does **not** need to start from zero. The more direct path is:

1. stop rebuilding lighting as DX8 light slots
2. expand from a four-light per-draw uniform model to a scene/cluster light data model
3. make terrain, static meshes, and dynamic meshes consume the same GPU lighting contract
4. keep only explicitly unlit materials/effects on the unlit path

The same "already gives us a bridge" logic applies beyond lighting:

- bgfx already owns actual shader execution
- bgfx already has explicit view separation for shadow and main passes
- bgfx already consumes explicit texture/uniform/program bindings

So the remaining work is mostly about deleting the D3D8-shaped intermediate layers between the runtime and that backend.

### Practical architectural target

The most coherent target is:

- one world-space GPU lighting path for opaque scene geometry
- one shadowing model shared across meshes and terrain
- one scene-managed light data build step per frame
- no CPU vertex-light solve path for ordinary runtime lighting
- explicit renderer-owned material/pipeline descriptors instead of wrapper state mutation
- renderer-owned draw sorting/phasing instead of object-local sort/delay plumbing
- shader parameters for texgen/bump-env/projection instead of D3D8 texture-stage emulation
- bgfx-native dynamic/streaming buffer ownership for non-sorting dynamic geometry

That can be implemented with several GPU-lighting strategies, but the codebase constraints point most naturally toward:

- scene light lists or page-local light lists
- clustered/tiled light selection, or a simpler per-batch light list as an intermediate step
- the same light/shadow shader contract across static meshes, terrain, and dynamic meshes

## Required touchpoints outside `ww3d2`

The refactor is not confined to renderer files.

### `wwphys`

#### Scene and object ownership

- `Code/wwphys/pscene.cpp`
- `Code/wwphys/pscene_projectors.cpp`
- `Code/wwphys/phys.cpp`
- `Code/wwphys/staticphys.cpp`

These own:

- visible-object lists
- projector application timing
- terrain/static-phys render dispatch
- pre-lit and static-lighting behavior

#### Projector runtime ownership

- `Code/wwphys/projectormanager.cpp`
- `Code/wwphys/staticanimphys.cpp`
- `Code/wwphys/phystexproject.cpp`

These determine:

- which objects create projectors
- whether they are static or dynamic
- how projector transforms are updated
- how self-shadow/projected-shadow semantics are expressed

#### Terrain lighting and persistence

- `Code/wwphys/lightsolve.cpp`
- `Code/wwphys/lightsolve.h`
- `Code/wwphys/renegadeterrainpatch.cpp`
- `Code/wwphys/renegadeterrainmaterialpass.cpp`
- `Code/wwphys/terrainmaterial.cpp`
- `Code/wwphys/pscene_lighting.cpp`
- `Code/wwphys/pscene_collision.cpp`

These define:

- terrain material/layer semantics
- terrain vertex-light baking
- terrain patch save/load format
- CPU light collection and static-lighting cache behavior

### Commando and runtime settings cleanup

- `Code/Commando/systemsettings.cpp`
- `Code/Commando/dlgconfigperformancetab.cpp`
- `Code/Commando/shutdown.cpp`
- `Code/Tools/WWConfig/PerformanceConfigDialog.cpp`
- `Code/Commando/consolefunction.cpp`

These still expose legacy projector/shadow-era controls and debugging:

- `Shadow_Mode`
- dynamic projector toggles
- projector debug display

These are cleanup surfaces, not compatibility commitments. The refactor should freely remap or remove them so the runtime stops pretending the new architecture is still the old projector system.

### Lighting, visibility, and decal support systems

- `Code/wwphys/pscene_lighting.cpp`
- `Code/wwphys/staticaabtreecull.h`
- `Code/wwphys/dynamicaabtreecull.h`
- `Code/wwphys/visrendercontext.h`
- `Code/wwphys/physdecalsys.h`

These are tightly coupled to the static-world path even though they are not part of the projector implementation itself.

#### Lighting and cache invalidation

`PhysicsSceneClass::Compute_Static_Lighting()` in `Code/wwphys/pscene_lighting.cpp` still matters after the projector refactor because it supplies `LightEnvironmentClass` for non-pre-lit dynamic objects. It is also vis-sector gated through `light->Is_Vis_Object_Visible(vis_object_id)`, so any future lighting replacement still has to preserve the current visibility contract or deliberately replace it.

`PhysicsSceneClass::Invalidate_Lighting_Caches(const AABoxClass &)` is called from gameplay paths such as building power-state changes. That notification chain must survive even if the static-world renderer changes completely.

If lighting moves to a unified GPU path, these systems stop invalidating per-object light caches and instead become inputs for:

- light enable/disable state
- light-list rebuild triggers
- shadow-data rebuild triggers

#### Visibility systems

`StaticAABTreeCullClass::Collect_Visible_Objects(...)` currently splits visible static output into:

- `VisibleStaticObjectList`
- `VisibleWSMeshList`

That world-space mesh split is one of the strongest existing batch-entry seams in the runtime. World-space meshes already avoid normal object transform submission and are the best early candidate set for scene-level static batching.

`DynamicAABTreeCullClass::Get_Dynamic_Object_Vis_ID(...)` feeds into static-lighting visibility for dynamic objects. That means the lighting and vis systems are coupled across static and dynamic render ownership; the batching refactor cannot treat them as unrelated subsystems.

`visrendercontext.h` and the static-vis preprocessing path still use synchronous render-based ID visibility generation. That path is not directly part of frame rendering, but any deep renderer architecture changes should assume vis preprocessing will need review if async or GPU-driven render flows expand.

#### Decals

`PhysDecalSysClass` is still a separate projected/overlay system with ww3d2-era `VertexMaterialClass`, `ShaderClass`, and related submission assumptions. It is distinct from texture projectors, but it occupies the same design space from the perspective of modern batching. The recommended plan is to keep decals as a separate explicit projected-effects system, not to fold them back into receiver-side extra passes.

### Runtime asset loading

- `Code/ww3d2/meshmdlio.cpp`
- `Code/ww3d2/meshmatdesc.h`
- `Code/ww3d2/w3d_file.h`

These matter because the runtime still ingests authored:

- pass count
- texture stages
- per-polygon material variation
- shader/material arrays

The important distinction is:

- data that the runtime still loads as source material information
- data that should stop forcing legacy draw splitting or projector-era behavior

That same distinction applies to lighting-related material flags:

- truly unlit materials should remain unlit
- world geometry should stop abusing prelit/user-lighting paths just to carry CPU-computed lighting
- color-source semantics still matter, but the main lighting result should come from unified GPU evaluation

## Legacy runtime surfaces that should not constrain the refactor

The following runtime/API surfaces exist today, but they should **not** be preserved as design constraints:

- `PhysicsSceneClass::Set_Shadow_Mode(...)`
- `PhysicsSceneClass::Set_Max_Simultaneous_Shadows(...)`
- `PhysicsSceneClass::Set_Shadow_Resolution(...)`
- `PhysicsSceneClass::Set_Shadow_Attenuation(...)`
- `PhysicsSceneClass::Set_Shadow_Normal_Intensity(...)`
- `PhysicsSceneClass::Enable_Static_Projectors(...)`
- `PhysicsSceneClass::Enable_Dynamic_Projectors(...)`
- projector debug display toggles exposed in runtime/tools UI

If these survive at all, they should be temporary migration plumbing or renamed cleanly around the new renderer. No effort should be spent on preserving projector-era semantics just because old UI, console, or config code references them.

## Migration target

The desired end state is:

1. **scene-level static batch collection**
   - visible static meshes and terrain feed explicit batch records
2. **material-centric submission**
   - submissions keyed by modern pipeline/material data, not DX8 wrapper state
3. **shared GPU light/shadow data**
   - meshes and terrain consume the same world-space light/shadow contract
   - no per-object CPU light environment approximation for ordinary lighting
4. **explicit effect systems**
   - shadows, decals, and any remaining projections are separate systems, not receiver-side extra passes
5. **large reusable GPU buffers**
   - no patch-local or object-local buffer churn for ordinary static world rendering

## Recommended migration stages

### Stage 1: replace CPU lighting contracts with one GPU lighting/shadow model

Primary goal:

- stop treating lighting as a mix of per-object `LightEnvironmentClass` caches, CPU light solves, and ad-hoc prelit branches

Required work:

- feed the bgfx shaders from shared scene light data instead of DX8 light-slot emulation
- remove the dependency on `LightEnvironmentClass` for ordinary scene lighting
- replace user-lighting-array and terrain-prelit runtime lighting with unified GPU evaluation
- keep only explicitly unlit materials/effects on the unlit path
- preserve light enable/disable gameplay semantics without preserving per-object lighting caches

### Stage 2: remove DX8 wrapper state as the primary renderer language

Primary goal:

- stop expressing bgfx submission through fake D3D8 render state, light slots, texture-stage state, and pass installation

Required work:

- convert wrapper-owned fog/light/material inputs into explicit renderer-owned scene/view/draw data
- collapse shader/material/pass layers into immutable classification data plus uniforms
- remove mapper round-trips through `D3DTSS_*`-style state for texgen/bump-env/projective paths
- reduce `DX8Wrapper` to the minimum compatibility shell needed during migration, then delete the remaining renderer-facing D3D8 contract

### Stage 3: remove projector ownership from the main static-world path

Primary goal:

- stop treating projector shadowing as `MaterialPassClass` replay on receivers

Required work:

- finish collapsing projector-based shadow modes onto the shadow-map path
- identify any remaining non-shadow projector users that must survive at runtime
- split those survivors into explicit feature buckets:
  - keep
  - replace with decals
  - remove

### Stage 4: introduce a scene-level static batch collector

Primary goal:

- replace receiver-driven immediate submission with collected static draw items

Collector inputs should at minimum include:

- mesh/terrain source
- world transform or world-space page identity
- material key
- index range
- lighting data source
- shadow receive/cast flags

Recommended first collection domains:

1. world-space static meshes from `VisibleWSMeshList`
2. ordinary opaque static meshes that are not in sorted/translucent paths
3. visible terrain patches, collected into terrain page batches rather than patch-local submissions

### Stage 5: modernize static mesh submission

Primary goal:

- keep shared geometry reuse, but move to modern persistent GPU buffers and explicit batch keys

Important subgoals:

- preserve opaque static mesh behavior first
- keep sorted/translucent paths separate until opaque world batching is stable
- decouple draw splitting from legacy pass count where possible

### Stage 6: replace terrain patch-local submission with terrain batch pages

Primary goal:

- move visible terrain patches into larger terrain batches

Likely shape:

- one or more scene-managed terrain page buffers
- per-material-page draw ranges, or a texture-array/indirection-based material scheme
- baked vertex lighting preserved in the new vertex format

### Stage 7: reintroduce only the projected effects that still justify their cost

Primary goal:

- projected effects become explicit, bounded modern systems instead of free-form extra passes

That likely means:

- shadow maps for shadows
- dedicated decal batching for decals
- only limited projected-light/decal features if a concrete authored/runtime need still exists

## Concrete required areas to touch

For planning purposes, the minimum complete refactor surface is:

1. **Projector removal/replacement core**
   - `ww3d2/projector*`
   - `ww3d2/texproject*`
   - `wwphys/pscene_projectors.cpp`
   - `wwphys/phystexproject.cpp`
   - `wwphys/projectormanager*`
2. **Unified lighting/shadow core**
   - `wwphys/pscene_lighting.cpp`
   - `wwphys/pscene_collision.cpp`
   - `wwphys/lightsolve*`
   - `ww3d2/light*`
   - `ww3d2/lightenvironment*`
   - `ww3d2/vertmaterial*`
   - `ww3d2/dx8wrapper*`
   - `ww3d2/bgfxrenderer*`
   - `ww3d2/shadowmap*`
   - `ww3d2/shaders/fs_mesh.sc`
   - `ww3d2/shaders/vs_mesh.sc`
   - `ww3d2/shaders/shadow_common.sh`
3. **Renderer-state and material simplification core**
   - `ww3d2/ww3d.h`
   - `ww3d2/ww3d.cpp`
   - `ww3d2/dx8wrapper*`
   - `ww3d2/shader*`
   - `ww3d2/vertmaterial*`
   - `ww3d2/matpass*`
   - `ww3d2/mapper*`
   - `ww3d2/matrixmapper*`
   - `ww3d2/rendobj.h`
   - `ww3d2/rinfo*`
   - `ww3d2/bgfxdynamicbuffer.cpp`
   - `ww3d2/scene.cpp`
   - `ww3d2/sortingrenderer*`
4. **Static mesh batch path**
   - `ww3d2/mesh.cpp`
   - `ww3d2/dx8renderer.h`
   - `ww3d2/dx8renderer.cpp`
   - `ww3d2/bgfxrenderer.cpp`
   - `ww3d2/rinfo.h`
5. **Terrain batch path**
   - `wwphys/renegadeterrainpatch*`
   - `wwphys/renegadeterrainmaterialpass*`
   - `wwphys/terrainmaterial*`
6. **Scene ownership and visibility integration**
   - `wwphys/pscene.cpp`
   - `wwphys/staticaabtreecull.h`
   - `wwphys/dynamicaabtreecull.h`
7. **Gameplay/light-state preservation**
   - `wwphys/staticphys.cpp`
   - `wwphys/dynamicphys.cpp`
   - `wwphys/phys.cpp`
8. **Projected-effects successor systems**
   - `wwphys/physdecalsys.h`
   - shadow-map integration points already living in `ww3d2`
9. **Runtime settings cleanup**
   - `wwphys/pscene.h`
   - `wwphys/pscene_projectors.cpp`
   - `Commando/systemsettings.cpp`
   - `Commando/dlgconfigperformancetab.cpp`
   - `Tools/WWConfig/PerformanceConfigDialog.cpp`
   - `Commando/consolefunction.cpp`
10. **Runtime asset-load interpretation**
   - `ww3d2/meshmdlio.cpp`
   - `ww3d2/meshmatdesc.h`
   - `ww3d2/w3d_file.h`

`Tools/WWConfig/*` is included here only as a runtime-settings migration surface. `Tools/LevelEdit/*` and `Tools/W3DView/*` are intentionally excluded from the minimum refactor surface.

## Areas that need special care

### Runtime terrain ownership and visibility identity

The renderer can merge multiple terrain patches into larger GPU pages, but it must not erase the runtime patch/visibility ownership that collision, culling, and lighting still reason about in `wwphys`.

### Static lighting and pre-lit behavior

Both meshes and terrain currently carry baked/static-lighting assumptions. A unified GPU lighting path should preserve the intended look where required, but it should stop routing ordinary runtime lighting through `UserLighting` arrays, terrain `IsPreLit` branches, and per-object `LightEnvironmentClass` caches.

### Unlit versus lit material intent

Some materials and helpers are intentionally unlit:

- overlay/debug primitives
- line/point helpers
- effect materials that explicitly disable lighting

Those should remain unlit. The cleanup target is the accidental use of unlit/prelit material modes as a carrier for CPU-computed world lighting.

### Legacy material semantics versus renderer-owned pipeline state

Legacy material data still matters as source content, but the renderer should stop preserving:

- fake hardware light-slot assumptions
- texture-stage-state plumbing
- fixed-function pass-install order
- FVF/category ownership as the top-level submission model

The goal is to preserve authored behavior, not D3D8-era implementation shape.

### Building and gameplay ownership of static meshes

Static-world meshes are not purely visual. Systems such as buildings and static physics classifications depend on the existing object structure, so batching must not erase gameplay ownership.

### Runtime asset interpretation

The runtime may continue to load legacy W3D material/pass data, but the new renderer should treat that as source data for batch synthesis, not as a requirement to preserve projector-era draw splitting, DX8-style texture mappers, or code-level compatibility wrappers.

The same applies to legacy lighting metadata: use it to reconstruct world-space GPU lighting behavior where it still expresses real authored intent, but do not preserve CPU light solves, four-light slot limits, or DX8 light-environment behavior as compatibility goals.

## Bottom line

The old projector system is the architectural hinge that keeps the static world locked to many small draw calls. Static meshes already have partial state bucketing, and terrain is already spatially chunked, but both are still subordinated to a DX8-era pass model.

The practical path forward is:

1. replace CPU light environments and CPU light solves with one shared GPU lighting/shadow model
2. remove DX8 wrapper state and fixed-function intermediate layers as the main renderer contract
3. remove projector-driven receiver replay from the core static-world path
4. unify static meshes and terrain under explicit scene-managed batch collection
5. move all non-gameplay-static work to load/scene activation
6. preserve required runtime behavior through explicit modern systems instead of extra passes

That is the cleanest way to move from the old projector model to a highly optimized modern renderer without carrying forward projector-era editor assumptions, transient extra-pass machinery, or backward-compatibility baggage.

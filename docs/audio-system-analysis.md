# Audio System Analysis

## Summary

The current Linux/modern build does **not** implement gameplay audio yet. `Code/WWAudio/CMakeLists.txt` currently builds `wwaudio_null.cpp`, `AudioSaveLoad_null.cpp`, and defines `WWAUDIO_USE_NULL_BACKEND=1` when `RENEGADE_WITH_MILES` is off, which means the shipped WWAudio API surface exists but the runtime backend is intentionally stubbed.

The original codebase, however, still preserves a very complete audio contract. The port does **not** merely need "playback"; it needs a replacement for the old Miles-based WWAudio stack, including category-aware playback, 2D/3D sound objects, distance-based culling and prioritization, streaming music, pseudo-3D fallback, logical/hearing events for AI/gameplay, text callbacks, background music control, save/load hooks, and environmental filtering behavior.

## Current status in the repository

### What is currently unimplemented

- `Code/WWAudio/wwaudio_null.cpp`
  - Almost every `WWAudioClass` method is stubbed or returns failure/null.
- `Code/WWAudio/AudioSaveLoad_null.cpp`
  - Static/dynamic audio save-load subsystems are effectively empty.
- `Code/WWAudio/AudibleSoundDefinition_headless.cpp`
  - Headless definition factory exists, but `Create_Sound()` and logical creation return `nullptr`.

### What still defines the required behavior

The original Miles-backed implementation is still present and is the best source of truth for parity:

- `Code/WWAudio/WWAudio.h`
- `Code/WWAudio/WWAudio.cpp`
- `Code/WWAudio/AudibleSound.h`
- `Code/WWAudio/AudibleSound.cpp`
- `Code/WWAudio/Sound3D.h`
- `Code/WWAudio/Sound3D.cpp`
- `Code/WWAudio/SoundPseudo3D.h`
- `Code/WWAudio/SoundPseudo3D.cpp`
- `Code/WWAudio/SoundScene.h`
- `Code/WWAudio/SoundScene.cpp`
- `Code/WWAudio/LogicalSound.h`
- `Code/WWAudio/LogicalSound.cpp`
- `Code/WWAudio/LogicalListener.h`
- `Code/WWAudio/LogicalListener.cpp`
- `Code/WWAudio/SoundBuffer.h`
- `Code/WWAudio/SoundBuffer.cpp`
- `Code/WWAudio/sound2dhandle.cpp`
- `Code/WWAudio/sound3dhandle.cpp`
- `Code/WWAudio/soundstreamhandle.cpp`
- `Code/WWAudio/FilteredSound.cpp`

## High-level audio path

### 1. Game systems create sounds through WWAudio

Primary entry points:

- `WWAudioClass::Create_Sound_Effect`
- `WWAudioClass::Create_3D_Sound`
- `WWAudioClass::Create_Sound`
- `WWAudioClass::Create_Continuous_Sound`
- `WWAudioClass::Create_Instant_Sound`
- `WWAudioClass::Set_Background_Music`
- `WWAudioClass::Fade_Background_Music`

Representative gameplay consumers:

- `Code/Combat/weapons.cpp`
- `Code/Combat/soldier.cpp`
- `Code/Combat/powerup.cpp`
- `Code/Combat/elevator.cpp`
- `Code/Combat/doors.cpp`
- `Code/Combat/explosion.cpp`
- `Code/Combat/building.cpp`
- `Code/Combat/scriptcommands.cpp`
- `Code/Commando/init.cpp`
- `Code/Commando/winevent.cpp`
- `Code/Commando/playerkill.cpp`
- `Code/Scripts/*.cpp`

### 2. Definitions drive most authored sound behavior

`AudibleSoundDefinitionClass` in `Code/WWAudio/AudibleSound.cpp` stores authored parameters:

- filename
- 2D vs 3D
- loop count
- priority
- base volume
- volume randomization
- pan
- dropoff radius
- max-volume radius
- sound type: sound effect / music / dialog / cinematic
- start offset
- pitch factor
- pitch randomization
- virtual channel
- display text
- logical sound creation flags
- logical notify delay
- logical dropoff radius

That means the replacement backend must preserve definition-driven behavior, not just raw file playback.

### 3. Buffers are cached, loaded, or streamed

`WWAudioClass` caches decoded/loaded `SoundBufferClass` objects by string ID / filename:

- `WWAudioClass::Find_Cached_Buffer`
- `WWAudioClass::Cache_Buffer`
- `WWAudioClass::Create_Sound_Buffer`
- `WWAudioClass::Free_Cache_Space`

`SoundBufferClass` loads full data into memory. `StreamSoundBufferClass` is used for oversized assets. For streams, the runtime relies on the filename and file callbacks rather than preloading the whole asset.

### 4. Runtime sound objects own playback state

`AudibleSoundClass` tracks:

- playing / paused / stopped state
- timestamp and playback position
- loop counts and loops left
- volume / pan / pitch
- priority and runtime priority
- transform
- cull state
- optional logical sound companion
- optional converted/filtered form

This object is not a passive handle; it contains real game-state that the backend must preserve.

### 5. WWAudio manages scarce sample handles

The original system preallocates limited handle pools and reassigns them dynamically:

- 2D handles: `Allocate_2D_Handles`, `Get_2D_Sample`
- 3D handles: `Allocate_3D_Handles`, `Get_3D_Sample`
- priority stealing: lower-priority sounds can lose their handle
- recovery: `Reprioritize_Playlist`

This is a major parity requirement. The original behavior is not "play everything"; it is "play what matters most, deterministically."

### 6. 3D sounds are scene-managed and culled

`SoundSceneClass` owns:

- primary listener
- optional secondary listener
- dynamic/static culling systems
- audible sound collection
- runtime priority updates based on distance
- logical-sound collection for AI/gameplay hearing

Key behavior:

- sounds outside dropoff radius are culled
- newly audible sounds are resumed / allocated
- no-longer-audible sounds are culled and lose handles
- audible lists are recomputed every frame

### 7. True 3D vs pseudo-3D is a real feature boundary

`WWAudioClass::Validate_3D_Sound_Buffer()` requires true 3D sounds to be:

- mono
- PCM
- not streamed

If the asset fails those requirements, `Create_3D_Sound()` falls back to `SoundPseudo3DClass`, which computes:

- manual distance attenuation
- manual stereo pan from listener-relative direction
- no hardware doppler
- no hardware reverb

This fallback is essential for parity because many authored assets may not meet true-3D constraints.

### 8. Logical sounds are gameplay-significant

Logical sounds are separate from audible sounds:

- `LogicalSoundClass`
- `LogicalListenerClass`
- `SoundSceneClass::Collect_Logical_Sounds`

They are used by gameplay/AI hearing systems:

- `Code/Combat/smartgameobj.cpp`
- `Code/Combat/scriptcommands.cpp`
- `Code/Combat/combatsound.cpp`

This means the audio port must preserve **non-audible gameplay signaling**, not only speaker output.

## Exact requirements for feature parity

The following are the practical requirements implied by the original code.

### A. Core playback and transport

Must have:

- one shared audio system singleton (`WWAudioClass`)
- selectable playback format:
  - stereo/mono
  - 8/16-bit preference in old UI
  - playback rate fallback from 44.1 kHz downward
- backend/device enumeration and selection
- runtime device reopen/reassign behavior
- file-factory based asset loading, not raw OS-path-only loading
- streaming playback for large assets/background music
- seek, pause, resume, stop
- loop counts, including infinite loops
- playback position queries in milliseconds
- pitch/playback-rate adjustment

Evidence:

- `WWAudioClass::Open_2D_Device`
- `WWAudioClass::Load_From_Registry`
- `WWAudioClass::Create_Sound_Buffer`
- `soundstreamhandle.cpp`
- `AudibleSoundClass::{Play,Pause,Resume,Stop,Seek,Initialize_Miles_Handle}`

### B. Sound classes and authored categories

Must have:

- 2D sound effects
- 3D positional sounds
- pseudo-3D positional fallback
- music
- dialog
- cinematic sound
- per-category enable/disable and volume control
- background music set/fade APIs

Evidence:

- `AudibleSoundClass::SOUND_TYPE`
- `WWAudioClass::{Allow_Sound_Effects,Allow_Music,Allow_Dialog,Allow_Cinematic_Sound}`
- `WWAudioClass::{Set_Music_Volume,Set_Dialog_Volume,Set_Cinematic_Volume}`
- `WWAudioClass::{Set_Background_Music,Fade_Background_Music}`

### C. 3D spatial behavior

Must have:

- listener transform/orientation
- source transform/orientation
- source velocity support
- distance attenuation using authored max-volume and dropoff radii
- runtime culling by audibility
- runtime priority based on distance
- optional doppler support for true 3D sounds
- speaker configuration awareness
- true 3D and pseudo-3D split

Evidence:

- `Sound3DClass::Update_Miles_Transform`
- `Sound3DClass::Set_Velocity`
- `Sound3DClass::Update_Edge_Volume`
- `SoundSceneClass::Collect_Audible_Sounds`
- `WWAudioClass::Set_Speaker_Type`
- `SoundPseudo3DClass::{Update_Pseudo_Volume,Update_Pseudo_Pan}`

Note: auto velocity is currently compiled out in `Sound3D.cpp` because the original comment says hardware doppler quality was poor. That implies doppler existed in the backend model, but exact gameplay parity may not require aggressive automatic doppler.

### D. Resource arbitration / handle stealing

Must have:

- limited active voice/channel budgeting
- separate 2D and 3D handle pools
- designer priority
- runtime priority
- stealing lower-priority voices when capacity is exhausted
- restoring bumped sounds when capacity becomes available

Evidence:

- `WWAudioClass::{Get_2D_Sample,Get_3D_Sample}`
- `WWAudioClass::Reprioritize_Playlist`
- `AudibleSoundClass::Get_Priority`
- `SoundSceneClass::Collect_Audible_Sounds`

This is one of the easiest requirements to accidentally lose in a modern rewrite.

### E. Virtual channels

Must have:

- logical exclusivity groups separate from physical mixer channels
- higher-priority sounds can override lower-priority sounds on the same virtual channel

Evidence:

- `WWAudioClass::{Acquire_Virtual_Channel,Release_Virtual_Channel}`
- `AudibleSoundDefinitionClass::m_VirtualChannel`

### F. Format and codec compatibility

The code explicitly claims support for:

- PCM WAV
- ADPCM WAV
- VOC
- MP3

Evidence:

- `WWAudio.h` comments near `Create_Sound_Effect`
- CPU-stat comment mentioning ADPCM and MP3 decompression
- config dialog test music uses `sakura battle theme.mp3`

Important true-3D restriction:

- true 3D requires **mono PCM WAV, non-streaming**
- non-compliant assets must fall back to pseudo-3D

### G. Streaming

Must have:

- streamed playback for large files
- streamed looping
- streamed seek/playback-rate/volume/pan control
- custom file callbacks / engine file system integration

Evidence:

- `soundstreamhandle.cpp`
- `WWAudioClass::{File_Open_Callback,File_Close_Callback,File_Seek_Callback,File_Read_Callback}`
- `WWAudioClass::Create_Sound_Buffer`

### H. Event and callback surface

Must have:

- sound started callback
- sound ended callback
- logical heard callback
- text callback

Evidence:

- `AudioEvents.h`
- `SoundSceneObjClass::On_Event`
- `WWAudioClass::{Register_EOS_Callback,Register_Text_Callback,Fire_Text_Callback}`

Gameplay uses these:

- mission/script monitoring via `EVENT_SOUND_ENDED`
- AI hearing via `EVENT_LOGICAL_HEARD`
- on-screen subtitle / audio text path via `Register_Text_Callback` in `Code/Commando/init.cpp`

### I. Environmental / filtered behavior

Must have or deliberately approximate:

- reverb room type state
- effects level
- filtered/tinny alternate render path for secondary listener usage
- environmental attenuation support used by weather/background systems

Evidence:

- `WWAudioClass::{Get_Effects_Level,Set_Reverb_Room_Type}`
- `FilteredSound.cpp`
- `SoundEnvironment.cpp`
- `WeatherMgr.cpp`
- `backgroundmgr.cpp`

Observations:

- The code references EAX-style reverb/filter concepts.
- Some dialog ducking and filtered secondary-listener behavior is present but partially commented out.
- Even if not every legacy effect is currently exercised heavily, the system contract expects room/effect knobs to exist.

### J. Save/load and object identity

Must have:

- persistent sound object IDs
- save/load for sound definitions and scene objects
- static and dynamic audio save/load support

Evidence:

- `SoundSceneObjClass`
- `Sound3DClass::Save/Load`
- `LogicalSoundClass::Save/Load`
- `LogicalListenerClass::Save/Load`
- `AudioSaveLoad.*`

The current null save/load path skips this entirely, but the original design includes it.

### K. Out-of-band movie audio and non-requirements

Important scope notes from the codebase:

- Bink/FMV playback has its own audio path and is not routed through WWAudio.
- Multiplayer radio commands exist, but there is no in-game VoIP or general network voice-chat stack to preserve.

Evidence:

- `Code/BinkMovie/BINKMovie.*`
- `Code/Commando/init.cpp`
- `Code/Combat/scriptcommands.cpp`

## Feature matrix

| Requirement | Original code expects it | Pure SDL3 | Pure SDL3_mixer | OpenAL Soft | miniaudio |
| --- | --- | --- | --- | --- | --- |
| Device open/enumeration | Yes | Yes | Yes, via mixer/device layer | Yes | Yes |
| Basic 2D playback | Yes | Yes | Yes | Yes | Yes |
| Streaming playback | Yes | Yes, custom | Yes | Yes, custom queueing/buffering | Yes |
| MP3/VOC/ADPCM decode parity | Yes | No, not by itself | Partial/format-dependent, much better than SDL core | No, needs external decoder | Partial, needs validation/integration |
| True 3D positional sources | Yes | Not built in | Basic only | Yes | Yes |
| Doppler | Model expects it | Custom DSP required | No practical parity path | Yes | Possible/custom |
| Environmental reverb/filtering | Yes | Custom DSP required | Limited/custom | Yes via EFX | Custom |
| Pseudo-3D fallback | Yes | Yes, custom | Yes, custom | Yes, easy to layer | Yes |
| Voice budgeting / stealing | Yes | Yes, custom | Yes, custom | Yes, custom | Yes, custom |
| Virtual channels | Yes | Custom | Custom | Custom | Custom |
| Logical sound/hearing layer | Yes | Engine-side custom | Engine-side custom | Engine-side custom | Engine-side custom |
| Text/EOS callbacks | Yes | Custom | Supported with wrapper work | Custom | Custom |
| Closest conceptual match to Miles 3D model | Yes | Low | Medium-low | High | Medium |

## Can pure SDL3 fulfill this?

### Short answer

**Not by itself as a drop-in feature-parity replacement.**

### Detailed answer

SDL3 is a strong **low-level foundation** for:

- audio device management
- stream-based playback
- mixing multiple streams
- resampling and format conversion
- channel maps
- default-device migration

However, SDL3 core does **not** provide the higher-level game-audio behaviors this engine expects:

- authored 3D emitter/listener model
- distance attenuation model matching the old system
- doppler
- environmental reverb / EAX-like behavior
- source voice arbitration / priority stealing
- sound-object scene integration
- virtual channels
- legacy codec parity out of the box

SDL3 also only guarantees very basic loading support from core APIs; WWAudio expects more than WAV PCM playback.

### Verdict on pure SDL3

Pure SDL3 is a **good substrate**, but only if we are willing to build a substantial custom engine on top of it:

- custom mixer policy
- custom 3D math/spatialization
- custom voice manager
- custom streaming layer
- custom codec integration
- custom environmental DSP

That is technically possible, but it is the **highest engineering cost** option.

## Can pure SDL3_mixer fulfill this?

### Short answer

**It can cover a meaningful subset, but not full parity.**

### What SDL3_mixer does well

Per its current documentation, SDL3_mixer provides:

- decoding for common audio formats
- multiple tracks
- streaming from `SDL_AudioStream` or `SDL_IOStream`
- per-track gain
- pitch/speed through frequency ratio
- looping
- fades
- stop callbacks
- tags/groups
- postmix/raw/cooked callbacks
- basic positional audio via `MIX_SetTrack3DPosition`

That means SDL3_mixer could likely cover:

- 2D sound effects
- music/dialog/cinematic category routing
- background music
- streaming
- fades
- most per-track control surface
- custom file-system integration
- basic callback plumbing

### What SDL3_mixer does not look sufficient for

The SDL3_mixer docs explicitly say it offers **basic positional audio** and is **not meant to be a full 3D audio engine**. That is the key problem for Renegade parity.

Likely gaps:

- no strong match for the old provider/listener/source model
- no doppler parity
- no EAX/EFX-style environmental reverb parity
- no natural equivalent to true-3D vs pseudo-3D fallback semantics
- no baked-in voice stealing/priority policy matching WWAudio
- no built-in logical sound/hearing layer

### Verdict on pure SDL3_mixer

SDL3_mixer could support a **serviceable approximation** of the 2D and "basic 3D" portions of WWAudio, but it is a poor fit if feature parity with the original 3D audio behavior is the goal.

It is better than pure SDL3 for:

- decoding
- streaming
- track management
- basic positioning

But it still falls short on the hardest parity requirements.

## What other library is a good fit?

## Best fit: OpenAL Soft

### Why it fits this codebase well

OpenAL Soft is the best match for the original architecture because it already exposes the same core concepts the old WWAudio layer expects:

- audio device/context
- listener
- positional sources
- orientation
- velocity / doppler
- distance attenuation
- streaming buffers/queues
- environmental effects through EFX

This is much closer to the old Miles 3D-provider model than SDL or SDL_mixer.

### OpenAL Soft strengths for Renegade

- strong cross-platform support
- purpose-built 3D audio API
- distance attenuation, directional emitters, doppler
- EFX extension for:
  - environmental reverb
  - filters
  - occlusion-style approximations
- streaming support
- HRTF/headphone handling
- software implementation, so behavior is not tied to dead vendor-specific hardware

### What OpenAL Soft does not solve by itself

We would still need to implement engine-side behavior:

- WWAudio object model
- sound definition loading
- cache management
- virtual channels
- voice budgeting and stealing
- playlist/page behavior
- logical sound/hearing
- subtitle/text callback path
- codec decoding for MP3/VOC/ADPCM where OpenAL alone is not enough

### Best practical shape

The most practical parity-oriented stack is:

- **OpenAL Soft** for 3D/2D output, sources, listener, doppler, reverb/filter support
- **SDL3** for platform integration and file/device abstractions elsewhere in the port
- a small decode layer for legacy asset formats as needed, likely including WAV/ADPCM/MP3 handling and possibly VOC support depending on shipped assets

This gives the closest architectural replacement without reintroducing Windows-only code.

## Secondary candidate: miniaudio

### Why it is interesting

miniaudio is attractive because it offers:

- cross-platform device/output layer
- low-level callback path
- higher-level mixing facilities
- a lightweight integration story

### Why it is not the top recommendation

miniaudio is a better fit if the goal is:

- keep dependencies minimal
- build a custom engine from scratch
- accept more engine-side DSP/control work

It is **not** as close conceptually to the legacy Miles 3D-provider model as OpenAL Soft, and the environmental-effect parity story would likely require more custom work.

### Verdict

Good technical option, but not the easiest parity path.

## Libraries that are technically capable but less aligned

### FMOD / Wwise

These could absolutely deliver the feature set, but they are weaker fits for this project because:

- licensing/distribution complexity
- larger behavioral delta from the original code
- less desirable for an open-source long-term port

## Recommended approach

## Recommendation

Use **OpenAL Soft as the primary runtime audio backend**, with WWAudio retained as the engine-facing API layer.

### Why this is the best fit

It best preserves:

- the original object model
- listener/source semantics
- true 3D vs pseudo-3D split
- doppler/reverb capability
- future Linux/macOS/Windows portability

It also lets us port the backend **in place**, matching the codebase's existing organization, instead of rewriting the entire audio architecture around SDL_mixer's abstractions.

### Suggested implementation split

1. Keep `WWAudioClass`, `AudibleSoundClass`, `Sound3DClass`, `SoundSceneClass`, logical sound/listener classes, and authored definitions as the public engine contract.
2. Replace Miles handles and provider calls with OpenAL Soft source/buffer/effect objects.
3. Preserve:
   - buffer caching
   - handle budgeting
   - priority stealing
   - virtual channels
   - pseudo-3D fallback
   - pages/playlists
   - callback surface
4. Use a decode layer only where needed for formats not handled natively by the chosen stack.
5. Treat SDL3 as platform/runtime support, not the full audio-engine replacement.

## Must-have parity checklist

The port should be considered "feature complete enough" only when all of the following work:

- authored 2D sounds
- authored 3D sounds
- pseudo-3D fallback for non-mono/non-PCM/streamed 3D assets
- loop counts and infinite loops
- playback position / seek
- background music and fade transitions
- category volumes and enable/disable
- sound priorities and runtime priorities
- voice/channel stealing
- virtual channels
- logical hearing events for AI/gameplay
- end-of-sound callbacks
- text/subtitle callbacks
- environment-driven attenuation used by weather/background audio
- save/load object identity paths

## Key risks and unknowns

### 1. Exact asset-format reality needs runtime validation

The code claims PCM WAV, ADPCM WAV, VOC, and MP3 support, but repository inspection alone does not prove which formats are actually used in shipped content and how often.

### 2. Reverb/filter usage may be narrower than the API suggests

The system exposes EAX-style controls, but some related paths are commented or lightly used. We should validate how much real content depends on them before overbuilding.

### 3. Secondary-listener behavior needs verification

`SoundSceneClass` has an auxiliary/second-listener path and filtered-sound conversion hooks. That may matter for cameras, remote views, or special tools, and should not be silently dropped.

### 4. Save/load behavior should be validated with real save data

The object model clearly supports it, but the currently active null save/load path means this has not yet been ported.

### 5. Coordinate-space mapping must be preserved exactly

`Sound3D.cpp` remaps coordinates before passing them to Miles, so any OpenAL Soft port must validate handedness and axis mapping carefully or left/right/front/back spatialization will be wrong.

### 6. Streaming semantics need careful adaptation

Miles attaches streams to preallocated sample-handle concepts; OpenAL-style streaming uses queued buffers on sources. The semantic result is achievable, but the handle-pool logic and stream looping behavior need explicit porting rather than a naive rewrite.

### 7. File-factory/threading behavior needs runtime checks

The original design routes streaming through engine file callbacks. Any replacement backend must preserve archive/file-factory integration and ensure threaded streaming does not violate existing file-system assumptions.

### 8. Filtered/radio sound timbre needs perceptual validation

`FilteredSoundClass` is not just "some filter"; it uses a specific tinny/radio-style effect. Matching that convincingly with OpenAL EFX or software DSP needs listening tests, not just API equivalence.

### 9. Bink audio coexistence must be verified

Bink movie playback appears to own its own audio path. The replacement gameplay backend must coexist cleanly with that path on modern platforms.

## Bottom line

- **Pure SDL3:** viable only as a low-level base, not as a practical parity solution by itself.
- **Pure SDL3_mixer:** strong for 2D/streaming/basic positioning, but not enough for true feature parity.
- **Best fit:** **OpenAL Soft**, with WWAudio kept as the engine-facing layer and SDL3 used as supporting platform infrastructure.

That combination gives the best chance of preserving the original Renegade audio behavior while still meeting the port's cross-platform goals.

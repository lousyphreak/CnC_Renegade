# Multiplayer / Network Inventory

This document is a source-level inventory of the multiplayer and networking stack in the current repository. It covers active runtime code, disabled or legacy online-service code, support tooling, porting hazards, and the work still needed to ship modern multiplayer cleanly on current platforms.

## Executive summary

The repository still contains a large original Renegade multiplayer stack.

- **Implemented and active today:** UDP transport, reliable/unreliable delivery, object replication, bit-packed serialization, LAN discovery, in-game chat/event traffic, dedicated server binary, server-side bandwidth management, and an out-of-band remote admin path.
- **Present but disabled/stubbed in normal builds:** Westwood Online (WOL), GameSpy master-server reporting, CD-key/GameSpy auth, WOL NAT traversal, embedded browser integration, slave-server orchestration.
- **Legacy traces still in tree:** WOL COM DLL interop, GameSpy hooks, old Windows-only tools, Java/JNI game-results tooling, historical bandwidth test utility.
- **Main porting reality:** the low-level game networking stack is mostly alive; the legacy internet-service layer is not.

## 1. Build targets and feature switches

### Runtime targets

| Target | Source | Notes |
|---|---|---|
| `Commando` / `Renegade` | `Code/Commando/CMakeLists.txt` | Main SDL3 game binary. |
| `CommandoDedicated` / `renegade_dedicated` | `Code/Commando/CMakeLists.txt` | Headless dedicated server binary. |

### Core build flags

Defined in `cmake/RenegadeOptions.cmake` and exported through `Code/renegade_build_config.h(.in)`:

| Flag | Default | Practical meaning |
|---|---|---|
| `RENEGADE_WITH_GAMESPY` | `OFF` | GameSpy query/reporting and auth integrations stay disabled. |
| `RENEGADE_WITH_LEGACY_WOL` | `OFF` | Westwood Online COM/ATL stack stays disabled. |
| `RENEGADE_WITH_BANDTEST` | `OFF` | Historical bandwidth-test path disabled. |
| `RENEGADE_WITH_SCONTROL` | `OFF` | Defined, but the `SControl` library is still built and linked unconditionally. |

### Build-system notes

- `Code/CMakeLists.txt` always builds `wwnet`, `SControl`, `Combat`, and `Commando`.
- `Code/SControl/CMakeLists.txt` builds `SControl` as a static library unconditionally.
- `Code/Commando/CMakeLists.txt` links `SControl` into both `CommandoLib` and `CommandoDedicatedLib`.
- `Code/wwnet/CMakeLists.txt` links `ws2_32` only on Windows and defines `_UNIX` on non-Windows builds.

## 2. What is actively available in the runtime stack

### 2.1 Low-level transport (`Code/wwnet/`)

This is the active multiplayer transport layer. It is still the backbone of gameplay networking.

| File(s) | Role | Status |
|---|---|---|
| `winsock.h` | In-tree socket compatibility layer for Windows/POSIX. | **Active** |
| `netutil.h/.cpp` | Socket setup, bind/close helpers, local address discovery, broadcast helpers. | **Active** |
| `connect.h/.cpp` | `cConnection`: per-socket connection manager, remote-host table, send/receive logic, reliable/unreliable traffic, ACK/keepalive/refusal handling. | **Active** |
| `rhost.h/.cpp` | `cRemoteHost`: per-peer state, resend timing, ping, bandwidth, loading/flood state. | **Active** |
| `wwpacket.h/.cpp` | `cPacket`: bit-packed game packet wrapper with headers, IDs, send time, CRC. | **Active** |
| `packetmgr.h/.cpp` | Packet combining and delta-compression layer below `cConnection`. | **Active** |
| `packettype.h` | Wire-level packet types (`UNRELIABLE`, `RELIABLE`, `ACK`, `KEEPALIVE`, `CONNECT_CS`, `ACCEPT_SC`, `REFUSAL_SC`, `FIREWALL_PROBE`). | **Active** |
| `networkobject*.{h,cpp}` | Replication base classes, object registries, factory registries. | **Active** |
| `BWBalance.h/.cpp` | Upload-budget balancing across clients. | **Active** |
| `netstats.h/.cpp`, `msgstat*.{h,cpp}` | Network diagnostics and message statistics. | **Active** |
| `singlepl.h/.cpp` | Loopback transport path for single-player. | **Active**, with TODO comment |
| `lan.cpp` | Historical LAN helper file. | **Thin/mostly vestigial** |

### 2.2 Serialization and replication

The authoritative replication pipeline is still present and wired:

1. `NetworkObjectClass::Export_*` / `Import_*`
2. `Code/wwbitpack/bitstream.h/.cpp`
3. `Code/wwbitpack/BitPacker.h/.cpp`
4. `Code/wwnet/wwpacket.h/.cpp`
5. `Code/wwnet/packetmgr.h/.cpp`
6. `Code/wwnet/connect.h/.cpp`
7. UDP `sendto` / `recvfrom`

Important active traits:

- 4 dirty-bit tiers: creation, rare, occasional, frequent.
- Per-client update bookkeeping in `NetworkObjectClass`.
- Packet combining to stay under a small MTU budget.
- Delta-compression support in `PacketManagerClass`.
- Up to `MAX_CLIENT_COUNT = 128` per replicated object.

### 2.3 Application-level multiplayer layer (`Code/Commando/`)

| File(s) | Role | Status |
|---|---|---|
| `cnetwork.h/.cpp` | Main game networking coordinator. Owns client/server connections, callbacks, update loop, version/CRC checks, connect/refusal/eviction handling. | **Active** |
| `pkthandlers.cpp` | Server/client packet dispatch and object import/export handling. | **Active** |
| `nethandler.h/.cpp` | Bridges gameplay/combat events into network handling. | **Active** |
| `comnetrcv.h`, `comnetrcvinst.h/.cpp` | Dynamic object update receiver/sender path. | **Active** |
| `netevent.h` | Base network event object. | **Active** |
| `priority.h/.cpp` | Object priority scheduling for bandwidth-limited updates. | **Active** |
| `netinterface.h/.cpp` | Multiplayer identity and side-preference settings. | **Active** |
| `netgraphs.cpp` | In-game network graphs/debug overlays. | **Active** |
| `apppacketstats.h/.cpp` | App-level packet statistics display and tracking. | **Active** |

### 2.4 Gameplay packet/event surface

The gameplay protocol surface is large and mostly intact.

Representative server-to-client traffic from `Code/Combat/apppackettypes.h`:

- object creation/update for soldiers, vehicles, buildings, turrets, teams, players, static objects, doors, elevators, base controller, C4, beacon, cinematics
- game options, game-data updates, kill events, win events, server FPS, ping response
- purchase responses, explosion/obelisk/announcement events
- text/chat objects

Representative client-to-server traffic:

- control/input mirrors (`CLIENTCONTROL`)
- text/chat, loading, bio/identity
- team changes, purchases, donation, money events
- damage, suicide, request-kill, console commands
- ping request, FPS reporting, bandwidth-budget-out report

Files worth treating as the gameplay-network surface area:

- `Code/Combat/apppackettypes.h`
- `Code/Combat/netclassids.h`
- `Code/Combat/clientcontrol.h/.cpp`
- `Code/Commando/*event*.{h,cpp}`
- `Code/Combat/staticnetworkobject.h/.cpp`
- `Code/Commando/clientpingmanager.h/.cpp`
- `Code/Commando/clientfps.h/.cpp`
- `Code/Commando/serverfps.h/.cpp`

### 2.5 Multiplayer game-state objects

| File(s) | Role | Status |
|---|---|---|
| `player.h/.cpp` | Networked player object. Includes legacy WOL-facing fields. | **Active** |
| `team.h/.cpp` | Networked team object. | **Active** |
| `playermanager.h/.cpp` | Global player registry and multiplayer player bookkeeping. | **Active** |
| `teammanager.h/.cpp` | Team registry and team-level state. | **Active** |
| `playerdata.h/.cpp` | Per-player server-side tracking. | **Active** |
| `gamedata.h/.cpp` | Match/server configuration and lifecycle state. | **Active** |

### 2.6 LAN play

LAN support is one of the clearly live multiplayer modes.

| File(s) | Role | Status |
|---|---|---|
| `langmode.h/.cpp` | LAN game mode wrapper. | **Active** |
| `lanchat.h/.cpp` | UDP broadcast-based LAN discovery/chat-location broadcasting. | **Active** |
| `dlgmplangamelist.h/.cpp` | LAN game list UI. | **Active** |
| `dlgmplanhostoptions.h/.cpp` | LAN host options UI. | **Active** |
| `dlgmpchangelannickname.h/.cpp` | LAN nickname UI. | **Active** |

### 2.7 Chat / HUD / multiplayer UI

| File(s) | Role | Status |
|---|---|---|
| `dlgmpingamechat.h/.cpp` | In-game all-chat dialog. | **Active** |
| `dlgmpingameteamchat.h/.cpp` | In-game team-chat dialog. | **Active** |
| `textdisplay.h/.cpp` | In-world / HUD text display. | **Active** |
| `Code/Combat/messagewindow.h/.cpp` | Scrolling chat/message window. | **Active** |
| `multihud.h/.cpp` | Multiplayer HUD and score/player display. | **Active** |
| `scorescreen.h/.cpp` | End-of-game score screen. | **Active** |
| `dlgcncbattleinfo.h/.cpp`, `dlgcncteaminfo.h/.cpp`, `dlgcncserverinfo.h/.cpp`, `dlgcncwinscreen.h/.cpp` | Multiplayer informational dialogs and win/battle overlays. | **Active** |

### 2.8 Dedicated server and remote administration

| File(s) | Role | Status |
|---|---|---|
| `commando_dedicated_sdl_main.cpp` | Dedicated server entrypoint. | **Active** |
| `ServerSettings.h/.cpp` | Dedicated/server config parsing; supports `MODE_LAN`, `MODE_WOL`, `MODE_GAMESPY`. | **Active** |
| `gamesideservercontrol.h/.cpp` | Game-side bridge to the out-of-band server-control library. | **Active** |
| `Code/SControl/servercontrol.h/.cpp` | Authenticated command/response server-control protocol. | **Active** |
| `Code/SControl/servercontrolsocket.h/.cpp` | UDP socket handler for SControl traffic, with CRC/encryption support. | **Active** |
| `ConsoleMode.h/.cpp` | Dedicated-server console/window management. | **Partially ported / still Win32-shaped** |
| `slavemaster.h/.cpp` | Multi-server orchestration and slave launching. | **Legacy / effectively disabled** |

SControl is worth calling out separately:

- `ServerControlClass` describes itself as a simple **out-of-band authenticated server command/response system**.
- `GameSideServerControlClass` exposes config keys for control port, password, loopback-only mode, and control IP.
- Default remote-admin port in `gamesideservercontrol.h` is `63999`.

## 3. What is present only as disabled, stubbed, or legacy code

### 3.1 Westwood Online (WOL)

This is the largest legacy online-service subsystem still present in-tree.

#### Library layer: `Code/WWOnline/`

Representative files:

- `WOLSession.h/.cpp`
- `WOLConnect.h/.cpp`
- `WOLChannel.h/.cpp`
- `WOLGame.h/.cpp`
- `WOLGameOptions.h/.cpp`
- `WOLUser.h/.cpp`
- `WOLChatMsg.h/.cpp`
- `WOLChatObserver.h/.cpp`
- `WOLLadder.h/.cpp`
- `WOLSquad.h/.cpp`
- `WOLLoginInfo.h/.cpp`
- `WOLPageMsg.h/.cpp`
- `WOLDownload.h/.cpp`
- `WOLServer.h/.cpp`
- `WOLNetUtilObserver.h/.cpp`
- `PingProfile.h/.cpp`
- `GameResField.h/.cpp`
- `GameResPacket.h/.cpp`
- `WaitCondition.h/.cpp`

Current state:

- Behind `RENEGADE_WITH_LEGACY_WOL`.
- Depends on ATL/COM and WOL COM interfaces.
- Includes Windows-only and case-sensitive-problematic headers such as `<atlbase.h>`, `<WWLib\Notify.h>`, and `<WOLAPI\wolapi.h>`.
- Contains unimplemented callbacks in `WOLNetUtilObserver.cpp` such as `OnGameresSent`, `OnWDTState`, `OnHighscore`.

#### COM interop layer: `Code/wolapi/`

Contains:

- `WOLAPI.dll`
- `WOLDBG.dll`
- `WOLAPI.h`
- `WOLAPI_i.c`
- `chatdefs.h`, `downloaddefs.h`, `ftpdefs.h`, `igrdefs.h`, `netutildefs.h`
- `wolapi.doc`

Current state:

- Windows-only binary dependency.
- No cross-platform equivalent in-tree.
- Functionally a dead dependency unless a full replacement layer is built.

#### Browser layer: `Code/WOLBrowser/`

Contains:

- `WOLBrowser.dll`
- `WOLBrowserD.dll`
- `WOLBrowser.h`
- `WOLBrowser_i.c`

Current state:

- Legacy COM browser integration.
- Disabled in current non-WOL builds.
- Replaced at runtime by an SDL-based external-browser fallback in `commando_service_stubs.cpp`.

### 3.2 WOL-facing game UI and flow code

These files are still present as traces of the original online product surface:

- `wolgmode.h/.cpp`
- `WOLChatMgr.h/.cpp`
- `WOLBuddyMgr.h/.cpp`
- `WOLQuickMatch.h/.cpp`
- `WOLJoinGame.h/.cpp`
- `WOLLoginProfile.h/.cpp`
- `WOLLogonMgr.h/.cpp`
- `WOLGameInfo.h/.cpp`
- `WOLDiags.h/.cpp`
- `wollocalemgr.h/.cpp`
- all `DlgWOL*` dialogs
- `gamechanlist.h/.cpp`
- `gamechannel.h/.cpp`
- `GameResSend.h/.cpp`

Current state:

- Most are behind `RENEGADE_WITH_LEGACY_WOL` or rely on WOL session types.
- On non-WOL builds, `commando_service_stubs.cpp` replaces large parts of this flow with no-op implementations.
- `WolGameModeClass` becomes an object that exists but does almost nothing.

### 3.3 GameSpy

Files:

- `GameSpy_QnR.h/.cpp`
- `gamespyadmin.h/.cpp`
- `gamespyauthmgr.h/.cpp`
- `gamespyscchallengeevent.h/.cpp`
- `gamespycschallengeresponseevent.h/.cpp`
- `CDKeyAuth.h/.cpp`
- `GameSpyBanList.h/.cpp`

Current state:

- Behind `RENEGADE_WITH_GAMESPY`.
- Uses headers like `<GameSpy\gqueryreporting.h>`, `<GameSpy\gcdkeyserver.h>`, `<GameSpy\gcdkeyclient.h>`.
- Query/reporting, auth, and related flows are stubbed when the flag is off.
- No replacement master-server browser or auth backend is wired in.

### 3.4 NAT / firewall traversal

Files:

- `nat.h/.cpp`
- `NAT.cpp`
- `natter.h/.cpp`
- `natsock.h/.cpp`
- `nataddr.h/.cpp`
- `FirewallWait.h/.cpp`

Current state:

- `nat.h` exposes a stubbed `FirewallHelperClass` when WOL is disabled.
- `natter.*` and `NAT.cpp` preserve legacy WOL-driven NAT/mangler/firewall traversal logic.
- `natsock.*` and `nataddr.*` still exist as partial lower-level support.
- Real traversal logic is effectively unavailable in current builds.

### 3.5 Slave-server orchestration

Files:

- `slavemaster.h/.cpp`
- `dlgmpslaveservers.h/.cpp`

Current state:

- Historical system for launching and tracking multiple slave dedicated servers.
- Still exposes Win32 process/window types in the headers.
- Runtime behavior is effectively neutralized by service stubs/non-port paths.

## 4. Historical tools and related traces

These are not the current shipping runtime, but they are relevant traces of the original network ecosystem.

### `Code/BandTest/`

- Windows-only bandwidth test utility.
- Uses raw WinSock headers and registry storage directly.
- Only old `.dsp` / `.dsw` project files are present; no current CMake target.
- Strong evidence of historical pre-game bandwidth qualification work.

### `Code/Tools/wnet/`

- Contains reusable TCP/UDP/packet helpers (`tcp.*`, `udp.*`, `packet.*`, `field.*`).
- Includes its own socket abstractions and direct `<winsock.h>` usage.
- Looks like standalone historical networking utility/test code, not current game runtime.

### `Code/Tools/RenegadeGR/`

- Game-results utility/tooling, including TCP/packet helpers and `RenegadeNet.h`.
- Likely tied to posting results or external network service workflows.
- Uses WinSock-era code and old project files.

### `Code/Tools/RenegadeSim/`

- Java/JNI bridge around `RenegadeGR`.
- `RenegadeNet.java` loads native library `RenegadeGR` and exposes `sendGameResults`.
- Strong trace of historical service integration/testing, not a current port path.

### Other traces worth remembering

- `Code/wolapi/wolapi.doc`
- `Code/Installer/WOL1Dialog.*`, `WOL2Dialog.*`
- `Code/Commando/DlgDownload.*`
- `Code/Commando/dlgwebpage_stub.cpp`

## 5. Stubs and replacement behavior in current builds

The central stub surface is `Code/Commando/commando_service_stubs.cpp`.

What it currently stands in for:

- WOL game mode behavior
- firewall/NAT helpers
- legacy browser/web-page flow
- GameSpy-disabled code paths
- portions of dedicated-console/service plumbing

Concrete examples:

- `WebBrowser::LaunchExternal` now calls `SDL_OpenURL`.
- `WolGameModeClass` methods mostly reduce to empty functions or `false`.
- non-WOL builds instantiate placeholder `FirewallHelper` and `WOLNATInterface` objects.

Also worth noting:

- `Code/compat/shellapi.h` already provides a compatibility `ShellExecute` that maps to `SDL_OpenURL`.
- This means the active `#include <shellapi.h>` in `cnetwork.cpp` is currently backed by an in-tree shim, not a missing Windows SDK dependency.

## 6. Platform, portability, and case-sensitivity hazards

### 6.1 Still-active cross-platform concerns

| Issue | Location | Why it matters |
|---|---|---|
| `SYSTEMTIME GameStartTime` / `GetSystemTime` | `Code/Commando/gamedata.h/.cpp` | Active match state still depends on Win32 time structures. |
| `#define errno (WSAGetLastError())` | `Code/Commando/natsock.h` | Breaks normal POSIX `errno` semantics. |
| `#define errno (WSAGetLastError())` | `Code/SControl/servercontrolsocket.h` | Same issue in active remote-admin path. |
| `HANDLE`, `HWND`, console types | `ConsoleMode.h`, `slavemaster.h`, `natter.h` | Win32-shaped types still leak through multiplayer-related headers. |
| Registry-backed settings | `ServerSettings`, `lanchat`, `gamesideservercontrol`, `WOL*`, `mpsettingsmgr`, `useroptions` and others | Cross-platform settings story is still mixed with legacy registry assumptions. |
| Wide strings in multiplayer-facing types | `gamedata.h`, `player.*`, WOL code | Conflicts with the UTF-8-only direction in the project guidance. |

### 6.2 Case-sensitive / path-style hazards

The network-related code still contains many Windows-style include paths and mixed-case references, especially in disabled online-service code:

- `<WOLAPI\wolapi.h>`
- `<WWLib\Notify.h>`
- `<GameSpy\gqueryreporting.h>`
- `<wwnet\wwpacket.h>`
- `"..\combat\specialbuilds.h"`
- `"..\wwonline\wolchannel.h"`
- `"WOLBrowser\WOLBrowser.h"`

Impact:

- Most of these are inside disabled WOL/GameSpy code paths, which is why the project still builds.
- If those paths are ever re-enabled, Linux/case-sensitive path cleanup will be mandatory first.

### 6.3 Legacy TODO / unimplemented traces directly related to networking

Examples found during inspection:

- `Code/wwnet/singlepl.h` has a TODO note.
- `Code/Commando/lanchat.cpp` starts with a TODO section.
- `Code/WWOnline/WOLNetUtilObserver.cpp` logs several `not implemented` callbacks.
- `Code/Commando/gamespycschallengeresponseevent.cpp` still has `TODO_AUTH`.
- `Code/Commando/gamespyauthmgr.cpp` still has `TODO_AUTH`.

## 7. Architectural gaps and mismatches

### Clearly available today

- Direct gameplay networking over UDP
- Reliable/unreliable packet delivery
- Object replication and dirty-bit updates
- LAN server discovery/broadcast
- In-game chat/event transport
- Dedicated server binary
- Remote admin via SControl

### Clearly missing today

- Modern internet matchmaking
- Modern server browser/master-server replacement
- Working WOL replacement
- Working GameSpy replacement
- Cross-platform NAT traversal
- Cross-platform legacy web/browser integration beyond opening an external URL
- Cross-platform CD-key/auth/bans system equivalent to the original service stack

### Mismatches and open design questions

| Topic | Observation |
|---|---|
| Client count vs player count | `NetworkObjectClass` tracks `MAX_CLIENT_COUNT = 128`, while `playermanager.h` still defines `MAX_PLAYERS = 255`. |
| SControl option vs build reality | `RENEGADE_WITH_SCONTROL` exists, but `SControl` is still built and linked unconditionally. |
| Dedicated server modes | `ServerSettingsClass` still models `MODE_WOL` and `MODE_GAMESPY`, but those backends are disabled. |
| Delta compression portability | The active packet delta path should still be audited for endian/packing assumptions on modern targets. |
| Match start time | Game duration already uses `GameStartTimeMs`, but `SYSTEMTIME` is still preserved in active state. |

## 8. What needs to be done

### P0: active multiplayer portability fixes

1. Replace `SYSTEMTIME` / `GetSystemTime` usage in `cGameData` with a cross-platform representation.
2. Remove the `errno` macro override from `Code/Commando/natsock.h`.
3. Remove the same `errno` macro override from `Code/SControl/servercontrolsocket.h`.
4. Audit the active remote-admin and dedicated-server headers for Win32 handle/window types that still leak into cross-platform code.

### P1: active multiplayer cleanup and risk reduction

1. Audit registry-backed multiplayer settings and move them behind a platform-neutral settings layer where still relevant.
2. Resolve or intentionally document the `MAX_CLIENT_COUNT` vs `MAX_PLAYERS` split.
3. Audit packet delta-compression and replication packing for endian/packing assumptions.
4. Continue removing wide-string dependencies from active multiplayer data structures.

### P2: define the actual internet-multiplayer strategy

1. Decide whether internet play should be:
   - direct-IP only,
   - a new master-server/browser stack,
   - or a replacement lobby/matchmaking service.
2. Decide whether any replacement is needed for:
   - GameSpy query/reporting,
   - account/auth,
   - bans,
   - NAT traversal,
   - post-game result submission.
3. Decide whether WOL-era UI should be removed, frozen behind stubs, or rewritten against a new backend.

### P3: legacy pruning / archaeology follow-up

1. Decide whether to keep or remove `BandTest/`.
2. Decide whether to keep or remove `Tools/wnet`, `Tools/RenegadeGR`, and `Tools/RenegadeSim`.
3. Decide whether `slavemaster.*` should be ported, quarantined, or deleted.
4. Clean up disabled WOL/GameSpy code for path-case correctness if any of it is to remain reachable in the future.

## 9. Bottom line

The codebase already has a substantial **working gameplay networking core**: transport, replication, packet/event flow, LAN, dedicated server, and admin hooks all still exist in recognizable form.

What it **does not** currently have is a modern, portable replacement for the old online-service layer. WOL, GameSpy, NAT traversal, and their browser/auth/lobby ecosystem remain mostly as disabled archaeology plus stubs. The next multiplayer phase should treat the active UDP/replication layer as the reusable foundation and the service stack as a replacement project, not a straight port.

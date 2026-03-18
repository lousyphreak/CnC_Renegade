# Legacy DSP/DSW Build Dependencies (Visual C++ 6 era)

This document is generated from the legacy Visual C++ **.dsw** and **.dsp** project files found under `Code/`. It describes the build targets (libraries and executables) and the dependencies that govern their build order. The goal is to provide a reference for anyone needing to understand or reimplement the old build flow (e.g., when porting to a modern build system).

> **Note:** This analysis ignores the current CMake build system and is based solely on the legacy `.dsw`/`.dsp` files in the repository. It includes targets present in this repo; some referenced projects (e.g. `GameSpy`) are not present as `.dsp` files and are therefore treated as external/missing targets.

---

## Quick dependency overview (repo targets)

This section lists the legacy build targets that directly depend on other repository targets. Targets not listed here have no direct repo-target dependencies.

- `SkeletonHack`: `wwlib`
- `wdump`: `wwlib`
- `collide`: `wwmath`
- `movietest`: `wwmath`
- `meshtest`: `wwdebug`, `wwlib`, `wwmath`
- `W3DShellExt`: `wwdebug`, `wwlib`
- `MixViewer`: `wwdebug`, `wwlib`
- `ChunkView`: `wwdebug`, `wwlib`
- `ww3d2`: `wwdebug`, `wwlib`, `wwmath`
- `RenRem`: `SControl`, `wwdebug`, `wwlib`
- `PhysTest`: `wwdebug`, `wwlib`, `wwmath`, `wwphys`, `wwsaveload`
- `mathtest`: `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `SplineTest`: `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `VidInit`: `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `SimpleGraph`: `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `max2w3d`: `pluglib`
- `skeleton_gth`: `ww3d2`, `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `Installer`: `BinkMovie`, `WWAudio`, `ww3d2`, `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`, `wwtranslatedb`, `wwui`
- `WWConfig`: `WWAudio`, `ww3d2`, `wwdebug`, `wwlib`, `wwmath`, `wwnet`, `wwphys`, `wwsaveload`, `wwtranslatedb`, `wwutil`
- `LightMap`: `ww3d2`, `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `commando`: `BandTest`, `BinkMovie`, `Combat`, `GameSpy`, `SControl`, `Scripts`, `WWAudio`, `ww3d2`, `wwbitpack`, `wwdebug`, `wwlib`, `wwmath`, `wwnet`, `wwphys`, `wwsaveload`, `wwtranslatedb`, `wwui`, `wwutil`
- `LevelEdit`: `Combat`, `WWAudio`, `ww3d2`, `wwbitpack`, `wwdebug`, `wwlib`, `wwmath`, `wwnet`, `wwphys`, `wwsaveload`, `wwtranslatedb`, `wwui`, `wwutil`
- `skeleton`: `ww3d2`, `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`
- `W3DView`: `WWAudio`, `ww3d2`, `wwdebug`, `wwlib`, `wwmath`, `wwsaveload`

## How to read this document

- The **build order** is topologically sorted: if target A depends on target B, then B appears earlier in the list. This makes it easier to reason about build ordering and incremental builds.
- Each target includes:
  - its Visual C++ project type (library / executable / DLL / plugin),
  - its source `.dsp` file,
  - the expected output artifact(s),
  - direct dependencies on *other repo targets* (via project dependencies or linked `.lib` files), and
  - direct external link dependencies (system libraries, 3rd-party libs, etc).

---

## Topological build order (targets)

### 1. `wwphys`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwphys/wwphys.dsp`
- **Outputs:**
  - `../Libs/debug/wwphys.lib`
  - `../Libs/debug/wwphyse.lib`
  - `../Libs/profile/wwphys.lib`
  - `../Libs/profile/wwphyse.lib`
  - `../Libs/release/wwphys.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 2. `wwtranslatedb`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwtranslatedb/wwtranslatedb.dsp`
- **Outputs:**
  - `../Libs/debug/wwtranslatedb.lib`
  - `../Libs/profile/wwtranslatedb.lib`
  - `../Libs/release/wwtranslatedb.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 3. `Clipbord`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/Clipbord/Clipbord.dsp`
- **Outputs:**
  - `Hybrid/clipbord.dlu`
  - `Release/clipbord.dlu`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `kernel32.lib`
  - `maxutil.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `util.lib`
  - `uuid.lib`
  - `winspool.lib`

### 4. `RenegadeGR`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/RenegadeGR/RenegadeGR.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`
  - `wsock32.lib`

### 5. `MaxFly`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/MaxFly/MaxFly.dsp`
- **Outputs:**
  - `Debug/MaxFly.dlu`
  - `Hybrid/MaxFly.dlu`
  - `Release/MaxFly.dlu`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `bmm.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `gfx.lib`
  - `gup.lib`
  - `kernel32.lib`
  - `maxscrpt.lib`
  - `maxutil.lib`
  - `mesh.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `paramblk2.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 6. `bin2cpp`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tools/bin2cpp/bin2cpp.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 7. `VerStamp`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tools/VerStamp/VerStamp.dsp`
- **Outputs:**
  - `../../../Run/VerStamp.exe`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 8. `WWCtrl`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/WWCtrl/WWCtrl.dsp`
- **Outputs:**
  - `../../../Run/WWCtrl.dll`
  - `../../../Run/WWCtrlD.dll`
  - `../../../Run/WWCtrlP.dll`
  - `bin/WWCtrl.dll`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 9. `launcher`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Launcher/launcher.dsp`
- **Outputs:**
  - `../../Run/Launcher.exe`
  - `../../Run/consoleserver.exe`
  - `../../Run/consoleserverD.exe`
  - `../../Run/renegadeserver.exe`
  - `c:/renegade/cdver/renegade.exe`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `patchw32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`

### 10. `wwutil`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwutil/wwutil.dsp`
- **Outputs:**
  - `../Libs/Debug/wwutil.lib`
  - `../libs/profile/wwutil.lib`
  - `../libs/release/wwutil.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 11. `Blender2`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/Blender2/Blender2.dsp`
- **Outputs:**
  - `Hybrid/blender.dlu`
  - `Release/blender.dlu`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `kernel32.lib`
  - `maxutil.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `util.lib`
  - `uuid.lib`
  - `winspool.lib`

### 12. `asf_imp`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/ASF_IMP/ASF_IMP.dsp`
- **Outputs:**
  - `Hybrid/asf_imp.dli`
  - `Release/asf_imp.dli`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `kernel32.lib`
  - `maxutil.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `util.lib`
  - `uuid.lib`
  - `winspool.lib`

### 13. `LocalHost`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tests/LocalHost/LocalHost.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `wsock32.lib`

### 14. `W3DUpdate`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/W3DUpdate/W3DUpdate.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 15. `wwlib`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwlib/wwlib.dsp`
- **Outputs:**
  - `../libs/Debug/wwlib.lib`
  - `../libs/Profile/wwlib.lib`
  - `../libs/Release/wwlib.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 16. `wwnet`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwnet/wwnet.dsp`
- **Outputs:**
  - `../Libs/Debug/wwnet.lib`
  - `../Libs/Profile/wwnet.lib`
  - `../Libs/Release/wwnet.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 17. `wwmath`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/WWMath/wwmath.dsp`
- **Outputs:**
  - `../Libs/debug/wwmath.lib`
  - `../Libs/profile/wwmath.lib`
  - `../Libs/release/wwmath.lib`
  - `../Libs/wwmath.lib`
  - `../Libs/wwmathd.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 18. `AMC_imp`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/AMC_IMP/AMC_IMP.dsp`
- **Outputs:**
  - `Hybrid/amc_imp.dli`
  - `Release/amc_imp.dli`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `kernel32.lib`
  - `maxutil.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `util.lib`
  - `uuid.lib`
  - `winspool.lib`

### 19. `CopyLocked`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/CopyLocked/CopyLocked.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 20. `bandy`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tests/Bandy/bandy.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `bandtest.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`
  - `wsock32.lib`

### 21. `ViewTrans`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/ViewTrans/ViewTrans.dsp`
- **Outputs:**
  - `Debug/viewtrans.dlu`
  - `Release/viewtrans.dlu`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `kernel32.lib`
  - `mesh.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `util.lib`
  - `uuid.lib`
  - `winspool.lib`

### 22. `wwdebug`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwdebug/wwdebug.dsp`
- **Outputs:**
  - `../Libs/debug/wwdebug.lib`
  - `../Libs/debug/wwdebuge.lib`
  - `../Libs/profile/wwdebug.lib`
  - `../Libs/profile/wwdebuge.lib`
  - `../Libs/release/wwdebug.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 23. `CommandoUpdate`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/CommandoUpdate/CommandoUpdate.dsp`
- **Outputs:**
  - `Debug/RenegadeUpdate.exe`
  - `Release/RenegadeUpdate.exe`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 24. `MakeMix`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tools/MakeMix/MakeMix.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`

### 25. `BinkMovie`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/BinkMovie/BinkMovie.dsp`
- **Outputs:**
  - `../Libs/Debug/BinkMovie.lib`
  - `../Libs/Profile/BinkMovie.lib`
  - `../Libs/Release/BinkMovie.lib`
  - `Debug/BinkMovieD.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 26. `SControl`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/SControl/SControl.dsp`
- **Outputs:**
  - `../libs/Debug/SControl.lib`
  - `../libs/Profile/SControl.lib`
  - `../libs/Release/SControl.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 27. `BitPackTest`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tests/BitPackTest/BitPackTest.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 28. `wwsaveload`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwsaveload/wwsaveload.dsp`
- **Outputs:**
  - `../Libs/debug/wwsaveload.lib`
  - `../Libs/debug/wwsaveloade.lib`
  - `../Libs/profile/wwsaveload.lib`
  - `../Libs/profile/wwsaveloade.lib`
  - `../Libs/release/wwsaveload.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 29. `BandTest`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/BandTest/BandTest.dsp`
- **Outputs:**
  - `../../Run/BandTest.dll`
  - `../../Run/BandTestD.dll`
  - `../../Run/BandTestP.dll`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `bandtest.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `ws2_32.lib`

### 30. `Scripts`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Scripts/Scripts.dsp`
- **Outputs:**
  - `../../Run/Scripts.dll`
  - `../../Run/ScriptsD.dll`
  - `../../Run/ScriptsP.dll`
  - `../../run/Debug/Scripts.dll`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `scripts.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 31. `wwui`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwui/wwui.dsp`
- **Outputs:**
  - `../libs/Debug/wwui.lib`
  - `../libs/Profile/wwui.lib`
  - `../libs/Release/wwui.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 32. `pluglib`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/Tools/pluglib/pluglib.dsp`
- **Outputs:**
  - `./Debug/pluglibd.lib`
  - `./GMaxRelease/pluglib.lib`
  - `./GMax_Hybrid/pluglib.lib`
  - `./GMax_Release/pluglib.lib`
  - `./Hybrid/pluglib.lib`
  - `./Lib/pluglib.lib`
  - `./Max4_Release/pluglib.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 33. `WWAudio`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/WWAudio/WWAudio.dsp`
- **Outputs:**
  - `../Libs/Debug/WWAudio.lib`
  - `../Libs/Debug/WWAudioe.lib`
  - `../Libs/Release/WWAudio.lib`
  - `../Libs/profile/WWAudio.lib`
  - `../Libs/profile/WWAudioe.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 34. `Combat`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/Combat/Combat.dsp`
- **Outputs:**
  - `../libs/Debug/Combat.lib`
  - `../libs/Debug/Combate.lib`
  - `../libs/Profile/Combat.lib`
  - `../libs/Profile/Combate.lib`
  - `../libs/Release/Combat.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 35. `wwbitpack`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/wwbitpack/wwbitpack.dsp`
- **Outputs:**
  - `../libs/Debug/wwbitpack.lib`
  - `../libs/Profile/wwbitpack.lib`
  - `../libs/Release/wwbitpack.lib`
- **Direct dependencies (repo targets):** None
- **Direct external link dependencies:** None

### 36. `SkeletonHack`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tools/SkeletonHack/SkeletonHack.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwlib`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 37. `wdump`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/wdump/wdump.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwlib`
- **Direct external link dependencies:**
  - `library.lib`
  - `ww3d.lib`

### 38. `collide`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tests/collide/collide.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwmath`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `library.lib`
  - `libraryd.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winspool.lib`

### 39. `movietest`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tests/movietest/movietest.dsp`
- **Outputs:**
  - `Run/movietest.exe`
  - `Run/movietest_d.exe`
- **Direct dependencies (repo targets):**
  - `wwmath`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `ddraw.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `library.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `sr.lib`
  - `srdb.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `ww3d.lib`

### 40. `meshtest`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tests/MeshTest/meshtest.dsp`
- **Outputs:**
  - `Run/meshtest_d.exe`
  - `Run/meshtest_p.exe`
  - `Run/meshtest_r.exe`
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `ddraw.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `library.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `sr.lib`
  - `srdb.lib`
  - `user32.lib`
  - `uuid.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `ww3d.lib`

### 41. `W3DShellExt`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/W3DShellExt/W3DShellExt.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`

### 42. `MixViewer`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/MixViewer/MixViewer.dsp`
- **Outputs:**
  - `../../../Run/MixViewerD.exe`
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
- **Direct external link dependencies:**
  - `winmm.lib`

### 43. `ChunkView`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/ChunkView/ChunkView.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
- **Direct external link dependencies:** None

### 44. `ww3d2`

- **Type:** Win32 (x86) Static Library
- **Project file:** `Code/ww3d2/ww3d2.dsp`
- **Outputs:**
  - `../Libs/Debug/ww3d2.lib`
  - `../Libs/Debug/ww3d2C.lib`
  - `../Libs/Debug/ww3d2e.lib`
  - `../Libs/Profile/ww3d2.lib`
  - `../Libs/Profile/ww3d2e.lib`
  - `../Libs/Release/ww3d2.lib`
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
- **Direct external link dependencies:** None

### 45. `RenRem`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tools/RenRem/RenRem.dsp`
- **Outputs:**
  - `../../../Run/RenRem.exe`
  - `../../../Run/RenRemD.exe`
  - `../../../Run/RenRemP.exe`
- **Direct dependencies (repo targets):**
  - `SControl`
  - `wwdebug`
  - `wwlib`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `ws2_32.lib`

### 46. `PhysTest`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tests/PhysTest/PhysTest.dsp`
- **Outputs:**
  - `Run/PhysTest.exe`
  - `Run/PhysTestD.exe`
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwphys`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `sr.lib`
  - `srdb.lib`
  - `version.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `ww3d.lib`

### 47. `mathtest`

- **Type:** Win32 (x86) Console Application
- **Project file:** `Code/Tests/mathtest/mathtest.dsp`
- **Outputs:**
  - `run/mathtest_d.exe`
  - `run/mathtest_p.exe`
  - `run/mathtest_r.exe`
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `winmm.lib`
  - `winspool.lib`

### 48. `SplineTest`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tests/SplineTest/SplineTest.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:** None

### 49. `VidInit`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/vidinit/VidInit.dsp`
- **Outputs:**
  - `Run/VidInit.exe`
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `ddraw.lib`
  - `dinput.lib`
  - `dsound.lib`
  - `dxguid.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `sr.lib`
  - `srdb.lib`
  - `user32.lib`
  - `uuid.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `wsock32.lib`
  - `ww3d.lib`

### 50. `SimpleGraph`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/SimpleGraph/SimpleGraph.dsp`
- **Outputs:** None listed in `.dsp` (likely uses per-configuration output paths).
- **Direct dependencies (repo targets):**
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:** None

### 51. `max2w3d`

- **Type:** Win32 (x86) Dynamic-Link Library
- **Project file:** `Code/Tools/max2w3d/max2w3d.dsp`
- **Outputs:**
  - `Debug/Max2w3d.dle`
  - `GMax_Hybrid/GMax2w3d.dle`
  - `GMax_Hybrid/Max2w3d.dle`
  - `GMax_Release/GMax2w3d.dle`
  - `Hybrid/Max2w3d.dle`
  - `Hybrid/max2w3d.dle`
  - `Release/Max2w3d.dle`
  - `Run/Max2w3d.dle`
  - `mainMax4Hybrid/Max2w3d.dle`
- **Direct dependencies (repo targets):**
  - `pluglib`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `bmm.lib`
  - `comctl32.lib`
  - `comdlg32.lib`
  - `core.lib`
  - `gdi32.lib`
  - `geom.lib`
  - `kernel32.lib`
  - `maxscrpt.lib`
  - `maxutil.lib`
  - `mesh.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `paramblk2.lib`
  - `shell32.lib`
  - `user32.lib`
  - `util.lib`
  - `uuid.lib`
  - `winspool.lib`

### 52. `skeleton_gth`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tests/skeleton_gth/skeleton_gth.dsp`
- **Outputs:**
  - `Run/skeleton_gth.exe`
- **Direct dependencies (repo targets):**
  - `ww3d2`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `d3d8.lib`
  - `d3dx8.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`

### 53. `Installer`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Installer/Installer.dsp`
- **Outputs:**
  - `Run/Setup.exe`
  - `Run/SetupD.exe`
- **Direct dependencies (repo targets):**
  - `BinkMovie`
  - `WWAudio`
  - `ww3d2`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
  - `wwtranslatedb`
  - `wwui`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `binkw32.lib`
  - `comdlg32.lib`
  - `d3d8.lib`
  - `d3dx8.lib`
  - `fdi.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `mss32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `version.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`

### 54. `WWConfig`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/WWConfig/WWConfig.dsp`
- **Outputs:**
  - `../../../Run/WWConfig.exe`
  - `../../../Run/WWConfigD.exe`
- **Direct dependencies (repo targets):**
  - `WWAudio`
  - `ww3d2`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwnet`
  - `wwphys`
  - `wwsaveload`
  - `wwtranslatedb`
  - `wwutil`
- **Direct external link dependencies:**
  - `d3d8.lib`
  - `d3dx8.lib`
  - `ddraw.lib`
  - `dinput.lib`
  - `dsound.lib`
  - `dxguid.lib`
  - `mss32.lib`
  - `umbra.lib`
  - `umbrad.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `wsock32.lib`

### 55. `LightMap`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/LightMap/LightMap.dsp`
- **Outputs:**
  - `Run/LightMap.exe`
  - `Run/LightMapD.exe`
- **Direct dependencies (repo targets):**
  - `ww3d2`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `d3d8.lib`
  - `d3dx8.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `lvsio.lib`
  - `lvsiod.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `sr.lib`
  - `srdb.lib`
  - `user32.lib`
  - `uuid.lib`
  - `version.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`

### 56. `commando`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Commando/commando.dsp`
- **Outputs:**
  - `../../Run/Renegade.exe`
  - `../../Run/RenegadeD.exe`
  - `../../Run/RenegadeP.exe`
  - `../../run/cmdo_r.exe`
  - `../../run/commando_fx.exe`
- **Direct dependencies (repo targets):**
  - `BandTest`
  - `BinkMovie`
  - `Combat`
  - `GameSpy`
  - `SControl`
  - `Scripts`
  - `WWAudio`
  - `ww3d2`
  - `wwbitpack`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwnet`
  - `wwphys`
  - `wwsaveload`
  - `wwtranslatedb`
  - `wwui`
  - `wwutil`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `bandtest.lib`
  - `binkw32.lib`
  - `comdlg32.lib`
  - `d3dx8.lib`
  - `ddraw.lib`
  - `dinput.lib`
  - `dsound.lib`
  - `dxguid.lib`
  - `gamespy.lib`
  - `gdi32.lib`
  - `glide2x.lib`
  - `kernel32.lib`
  - `library.lib`
  - `libraryd.lib`
  - `msdsrast.lib`
  - `mss32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `sr.lib`
  - `srvc4.lib`
  - `srvc4db.lib`
  - `user32.lib`
  - `uuid.lib`
  - `version.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`
  - `wsock32.lib`
  - `ww3d.lib`
  - `ww3dd.lib`

### 57. `LevelEdit`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/LevelEdit/LevelEdit.dsp`
- **Outputs:**
  - `../../../Run/LevelEdit.exe`
  - `../../../Run/LevelEditD.exe`
  - `Run/LevelEdit.exe`
  - `Run/LevelEditD.exe`
- **Direct dependencies (repo targets):**
  - `Combat`
  - `WWAudio`
  - `ww3d2`
  - `wwbitpack`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwnet`
  - `wwphys`
  - `wwsaveload`
  - `wwtranslatedb`
  - `wwui`
  - `wwutil`
- **Direct external link dependencies:**
  - `d3d8.lib`
  - `d3dx8.lib`
  - `dinput8.lib`
  - `dxguid.lib`
  - `imagehlp.lib`
  - `mss32.lib`
  - `nvdxtlib.lib`
  - `shlwapi.lib`
  - `sr.lib`
  - `srdb.lib`
  - `version.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `ws2_32.lib`
  - `ww3d.lib`
  - `wwctrl.lib`
  - `wwctrld.lib`

### 58. `skeleton`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/ww3d2/skeleton/skeleton.dsp`
- **Outputs:**
  - `Run/skeleton.exe`
  - `Run/skeletonD.exe`
- **Direct dependencies (repo targets):**
  - `ww3d2`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `advapi32.lib`
  - `comdlg32.lib`
  - `d3d8.lib`
  - `d3dx8.lib`
  - `gdi32.lib`
  - `kernel32.lib`
  - `odbc32.lib`
  - `odbccp32.lib`
  - `ole32.lib`
  - `oleaut32.lib`
  - `shell32.lib`
  - `user32.lib`
  - `uuid.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `winspool.lib`

### 59. `W3DView`

- **Type:** Win32 (x86) Application
- **Project file:** `Code/Tools/W3DView/W3DView.dsp`
- **Outputs:**
  - `Compressed/W3DViewC.exe`
  - `Profile/W3DView.exe`
  - `Run/W3DView.exe`
  - `Run/W3DViewD.exe`
- **Direct dependencies (repo targets):**
  - `WWAudio`
  - `ww3d2`
  - `wwdebug`
  - `wwlib`
  - `wwmath`
  - `wwsaveload`
- **Direct external link dependencies:**
  - `d3d8.lib`
  - `d3dx8.lib`
  - `mss32.lib`
  - `sr.lib`
  - `version.lib`
  - `vfw32.lib`
  - `winmm.lib`
  - `ww3d.lib`
  - `wwctrl.lib`
  - `wwctrld.lib`

---

## External (non-repo) libraries referenced by legacy projects

These are libraries the legacy projects link against, but which are not built from the `.dsp` files in this repo. They include system libraries (e.g., `kernel32.lib`) and third-party libraries (e.g., `binkw32.lib`, `d3dx8.lib`, etc).

- `advapi32.lib`
- `bandtest.lib`
- `binkw32.lib`
- `bmm.lib`
- `comctl32.lib`
- `comdlg32.lib`
- `core.lib`
- `d3d8.lib`
- `d3dx8.lib`
- `ddraw.lib`
- `dinput.lib`
- `dinput8.lib`
- `dsound.lib`
- `dxguid.lib`
- `fdi.lib`
- `gamespy.lib`
- `gdi32.lib`
- `geom.lib`
- `gfx.lib`
- `glide2x.lib`
- `gup.lib`
- `imagehlp.lib`
- `kernel32.lib`
- `library.lib`
- `libraryd.lib`
- `lvsio.lib`
- `lvsiod.lib`
- `maxscrpt.lib`
- `maxutil.lib`
- `mesh.lib`
- `msdsrast.lib`
- `mss32.lib`
- `nvdxtlib.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `ole32.lib`
- `oleaut32.lib`
- `paramblk2.lib`
- `patchw32.lib`
- `scripts.lib`
- `shell32.lib`
- `shlwapi.lib`
- `sr.lib`
- `srdb.lib`
- `srvc4.lib`
- `srvc4db.lib`
- `umbra.lib`
- `umbrad.lib`
- `user32.lib`
- `util.lib`
- `uuid.lib`
- `version.lib`
- `vfw32.lib`
- `winmm.lib`
- `winspool.lib`
- `ws2_32.lib`
- `wsock32.lib`
- `ww3d.lib`
- `ww3dd.lib`
- `wwctrl.lib`
- `wwctrld.lib`

---

## Notes / caveats

- Some projects referenced by `.dsw` solutions are missing from this repo (e.g., `GameSpy`). Those are treated as external dependencies in the graph.
- This analysis is based on parsing the `.dsp` and `.dsw` files; it does not attempt to evaluate conditional configurations (Debug/Release/Profile) beyond what is present in the generated makefile sections.
- Some targets (especially tools/plugins) may output to multiple paths depending on configuration; the `.dsp` files sometimes contain output patterns that vary by configuration. The listed outputs are the ones explicitly found in the `.dsp` file text.
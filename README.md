
# Command & Conquer Renegade

This repository includes source code for Command & Conquer Renegade. This release provides support to the [Steam Workshop](https://steamcommunity.com/workshop/browse/?appid=2229890) for the game.


## Dependencies

If you wish to rebuild the source code and tools successfully you will need to find or write new replacements (or remove the code using them entirely) for the following libraries;

- DirectX SDK (Version 8.0 or higher) (expected path `\Code\DirectX\`)
- RAD Bink SDK - (expected path `\Code\BinkMovie\`)
- RAD Miles Sound System SDK - (expected path `\Code\Miles6\`)
- NvDXTLib SDK - (expected path `\Code\NvDXTLib\`)
- Lightscape SDK - (expected path `\Code\Lightscape\`)
- Umbra SDK - (expected path `\Code\Umbra\`)
- GameSpy SDK - (expected path `\Code\GameSpy\`)
- GNU Regex - (expected path `\Code\WWLib\`)
- SafeDisk API - (expected path `\Code\Launcher\SafeDisk\`)
- Microsoft Cab Archive Library - (expected path `\Code\Installer\Cab\`)
- RTPatch Library - (expected path `\Code\Installer\`)
- Java Runtime Headers - (expected path `\Code\Tools\RenegadeGR\`)


## Compiling (Win32 Only)

To use the compiled binaries, you must own the game. The C&C Ultimate Collection is available for purchase on [EA App](https://www.ea.com/en-gb/games/command-and-conquer/command-and-conquer-the-ultimate-collection/buy/pc) or [Steam](https://store.steampowered.com/bundle/39394/Command__Conquer_The_Ultimate_Collection/).

### Renegade

The quickest way to build all configurations in the project is to open `commando.dsw` in Microsoft Visual Studio C++ 6.0 (SP5 recommended for binary matching to patch 1.037) and select Build -> Batch Build, then hit the “Rebuild All” button.

If you wish to compile the code under a modern version of Microsoft Visual Studio, you can convert the legacy project file to a modern MSVC solution by opening the `commando.dsw` in Microsoft Visual Studio .NET 2003, and then opening the newly created project and solution file in MSVC 2015 or newer.

NOTE: As modern versions of MSVC enforce newer revisions of the C++ standard, you will need to make extensive changes to the codebase before it successfully compiles, even more so if you plan on compiling for the Win64 platform.

When the workspace has finished building, the compiled binaries will be copied to the `/Run/` directory found in the root of this repository. 


### Free Dedicated Server
It’s possible to build the Windows version of the FDS (Free Dedicated Server) for Command & Conquer Renegade from the source code in this repository, just uncomment `#define FREEDEDICATEDSERVER` in [Combat\specialbuilds.h](Combat\specialbuilds.h) and perform a “Rebuild All” action on the Release config.

For the modern CMake build, an additional `CommandoDedicated` target is available alongside the normal `Commando` target. It produces the `renegade_dedicated` executable without changing the existing `Renegade` build.

- `cmake --build build --target Commando`
- `cmake --build build --target CommandoDedicated`

The current Linux bring-up version of `renegade_dedicated` enables the dedicated-only compile-time paths (`FREEDEDICATEDSERVER`), forces exclusive/headless server bootstrap behavior, skips SDL video/audio initialization, and validates the dedicated server startup configuration path. A lightweight smoke validation is available with `./build/bin/renegade_dedicated --headless-smoke`.


### Level Edit (Public Release)
To build the public release build of Level Edit, modify the LevelEdit project settings and add `PUBLIC_EDITOR_VER` to the preprocessor defines.


## Emscripten Docker image

A multi-stage [Dockerfile](Dockerfile) is available for the web build.

- `docker build --target engine -t renegade-web:engine .` builds the Emscripten engine bundle without shipping the game data. Provide the data tree at runtime by mounting it at `/usr/share/nginx/html/Renegade-assets`.
- `docker build --target with-data -t renegade-web:with-data .` bakes the game data into a separate layer before the engine bundle so engine rebuilds keep that larger layer cached.
- Optional HTTP basic auth is configured with `HTTP_BASIC_AUTH_USERNAME` and `HTTP_BASIC_AUTH_PASSWORD`. Set `HTTP_BASIC_AUTH_REALM` to override the browser prompt text.
- Example Kubernetes deployment resources live under `k8s/ea-games/`, and `./dbuild.sh` mirrors the Red Alert flow by building the `with-data` image, pushing `thegitea.lousy.ddnss.org/lousy/renegade-web:latest`, and restarting the `renegade-web` deployment.


## Known Issues

The “Debug” configuration of the “Commando” project (the Renegade main project) will sometimes fail to link the final executable. This is due to Windows Defender incorrectly detecting RenegadeD.exe containing a virus (possibly due to the embedded browser code). Excluding the output `/Run/` folder found in the root of this repository in Windows Defender should resolve this for you.


## Contributing

This repository will not be accepting contributions (pull requests, issues, etc). If you wish to create changes to the source code and encourage collaboration, please create a fork of the repository under your GitHub user/organization space.


## Support

This repository is for preservation purposes only and is archived without support. 


## License

This repository and its contents are licensed under the GPL v3 license, with additional terms applied. Please see [LICENSE.md](LICENSE.md) for details.

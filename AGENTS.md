# Command and Conquer: Renegade

This is the source code for Command and Conquer: Renegade, and our goal is to port it to a modern tech stack, while preserving the original gameplay and experience as much as possible.

Use SDL3 for anything that it provides, to make sure the game runs on as many platforms as possible.

Since we are aiming at porting the project to modern OSes, we will not implement new functionality, besides what is strictly required to run on those OSes. For example, we will not implement new graphics features, but we will implement newer graphics APIs supporting the original features.

Since we also target Linux, we need to have our file system support case-sensitive paths, and we need to make sure that all file paths in the codebase are consistent with the actual file system.

## Running the executables

Executables willl run continuously until the user closes them, so you need to use the `timeout` tool to run them for a limited time, and then kill them.

## Building the project

Use `cmake --build` to build the project. The main windows build is in build-win.

## Renderer porting

We will port the renderer from D3D8 to bgfx. The port will be fully destructive, meaning that we will replace the old D3D8 implementation with a new one using bgfx. No new features should be implemented, only the features that are required to run the game on modern OSes.
The new renderer implementation should be **AS CLOSE AS POSSIBLE** to the old one, in terms of structure and organization, to make it easier to review and to ensure that we are not introducing new bugs.

Keep the headless renderer headless. When porting renderer functionality, do it in place where he old d3d functionality is, and make sure to keep the headless renderer headless, and not introduce new dependencies on the rest of the codebase.
Remove all old d3d types, and replace them with bgfx types, or with our own types if bgfx does not provide them.


- **DO NOT COMMIT** - the user will do that

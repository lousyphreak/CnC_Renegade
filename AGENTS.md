# Command and Conquer: Renegade

This is the source code for Command and Conquer: Renegade, and our goal is to port it to a modern tech stack, while preserving the original gameplay and experience as much as possible.

Use SDL3 for anything that it provides, to make sure the game runs on as many platforms as possible.

Since we are aiming at porting the project to modern OSes, we will not implement new functionality, besides what is strictly required to run on those OSes. For example, we will not implement new graphics features, but we will implement newer graphics APIs supporting the original features.

Since we also target Linux, we need to have our file system support case-sensitive paths, and we need to make sure that all file paths in the codebase are consistent with the actual file system.

## Running the executables

Executables willl run continuously until the user closes them, so you need to use the `timeout` tool to run them for a limited time, and then kill them.

## Collecting screenshots

When validating renderer changes, capture the real rendered output from the game window and compare it against the reference image, especially the background.

On Linux/X11, use this workflow:

1. Launch the executable on a real display with `timeout` so it exits automatically, for example: `DISPLAY=:1 timeout 20s <path-to-executable>`
2. Find the X11 window id after the window appears, for example: `DISPLAY=:1 xdotool search --name 'Command & Conquer Renegade' | head -n1`
3. Capture a single screenshot from that specific window: `DISPLAY=:1 ffmpeg -y -loglevel error -f x11grab -draw_mouse 0 -frames:v 1 -window_id "$WIN_ID" screenshot.png`

If a title search is unreliable, use `DISPLAY=:1 xwininfo -root -tree` to find the correct window id manually. Prefer `-window_id` captures over full-screen grabs so the screenshot matches the renderer output exactly.

## Building the project

Use `cmake --build` to build the project. The main windows build is in build-win.

## Renderer porting

We will port the renderer from D3D8 to bgfx. The port will be fully destructive, meaning that we will replace the old D3D8 implementation with a new one using bgfx. No new features should be implemented, only the features that are required to run the game on modern OSes.
The new renderer implementation should be **AS CLOSE AS POSSIBLE** to the old one, in terms of structure and organization, to make it easier to review and to ensure that we are not introducing new bugs.

Keep the headless renderer headless. When porting renderer functionality, do it in place where he old d3d functionality is, and make sure to keep the headless renderer headless, and not introduce new dependencies on the rest of the codebase.
Remove all old d3d types, and replace them with bgfx types, or with our own types if bgfx does not provide them.

## General guidelines

- use asan and ubsan to catch memory errors and undefined behavior during development
- do not stub any functionality, if you port a part of the codebase, make sure the port is **COMPLETE** and fully functional, even if it's not perfect or optimized. we want to have a working port as soon as possible, and then we can improve it later.
- keep the original code structure and organization as much as possible, do not move files around or change the directory structure, just port the code in place.
- keep a state document (e.g. `PORTING_PROGRESS.md`) to track the progress of the port, and update it regularly with detailed notes on what has been done, what is left to do, and any issues or challenges encountered along the way.
- keep a knowledge document (e.g. `PORTING_KNOWLEDGE.md`) to document any important information, insights, or discoveries made during the porting process, such as how certain systems work, any quirks or edge cases discovered, and any useful resources or references found.
- if there is any decision to be made about how to implement something, or if there are any questions or uncertainties about how to proceed, use the `askQuestions` tool.
- do not modify game code, only the updates to modern integration (like sound, render, input). we want to preserve functionality as much as possible, and not introduce new bugs or changes to the original game logic.

- **DO NOT COMMIT** - the user will do that
- **DO NOT CHANGE THE ORIGINAL GAME CODE** - unless needed to port to new functionality, like changing the file system to use SDL3, or changing the input handling to use SDL3, but do not change the original game logic or behavior unless absolutely necessary. if you need to change something in the original code, make sure to document it in the `PORTING_PROGRESS.md` and `PORTING_KNOWLEDGE.md` files, and explain why the change was needed and how it was implemented.


## **ACCEPTABLE CHANGES**

the following is acceptable:
- compiler/OS fixes (like updating code for modern compilers or case insensitive file opening)
- implementation replacement for cross platform support (for example d3d-->bgfx, native system interaction -->SDL3)
- removal of legacy things for good reasons (like CPU detection, because it is no longer required on modern systems)

the following **MUST** be changed:
- win32 specific behaviour (we want the codebase to be 100% cross platform)

**ANYTHING ELSE IS NOT ACCEPTABLE** - we want to avoid changing behaviour of the original game as much as possible, and we want to preserve the original game logic and functionality as much as possible.

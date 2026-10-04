# Warblade Service Release

A decompile of **Warblade 1.34**, created by Edgar M. Vigdal.

This repository contains no game assets and as such cannot be used without them. Compiling also requires access to the game assets for the icon file.

This decompilation is possible mainly due to the fact that Edgar released 1.34 as a debug build. This made it much more feasible to convert ~660 compiled functions back into their C++ form, on top of being able to cut out the PTK library (which is not available anymore) and delink it back into an object file. However this also means it's not possible to compile an exact byte-matching .exe to the original version, since it contains some debug-time patching that the compiler didn't do.

As a disclaimer, this project did use a lot of LLM work (shoutout to Claude), particularly for determining the build flags, the grunt work in converting x86 back to C++, and delinking PTK. It also helped quite a bit with documentation and cleaning up the code while preserving function-level byte matching, so please excuse the comment litter.

-------------

This repo contains two branches:
- `ptk` for a function-wise binary-matching version of the code with the original PTK engine and BASS library
- `sdl`, built on top of the `ptk` branch, removes PTK and replaces it with SDL, additionally with some improvements for modern PCs

Release SR1 is built off of the `sdl` branch.

### SR branch changes

I intentionally didn't want to change too much about the game without good reason, to preserve the game as it stands. So most changes correspond to the underlying rendering system the game uses, and the others relating to things that don't serve a purpose anymore

- 64bit executable, optimized build
- Uses SDL as the rendering & sound engine. PTK and BASS have been dropped as a result
- Resizable window, letterboxing and scaling is applied automatically
- Fullscreen is now borderless instead of exclusive, and can be toggled while in-game without needing a restart
- Settings not applicable with SDL or modern windows (e.g. 16 bit color, hardware sound mixing) have been removed
- VSync is now an option, enabled by default, toggleable by `Alt + V`
- SDL renderer is now an option that can be cycled through, for options such as DirectX 12, Vulkan, and OpenGL. Default is "auto", meaning that it will pick one that works for your system
- Interpolation is now an option to make the game render at >60FPS, however it has a few caveats (see below)
- Anything relating to online services has been either stubbed out or removed
- Links that the main menu opened now open their archive.org versions instead
- Anti-cheat mechanisms (such as obfuscating score tables by multiplying them by 7, reading the process list for trainer executables, intentionally crashing the game upon finding tampered highscores) have been removed
- Asset loading during startup has been significantly sped up by detatching it from the framerate
- Intro sequence now handles window closing instead of only closing when it reaches the main menu

#### Cheats

Not part of the original game. During play, type `GALAGA` to toggle cheats (the code only uses letters that aren't in-game hotkeys). While cheats are on, the number keys (top row or numpad) apply to the current player:

| Key | Cheat |
| --- | --- |
| `1` | Extra life |
| `2` | +1000 money (up to the wallet size) |
| `3` | Skip level (warp) |
| `4` | Best weapon: WAR.I.PLASMA with max bullets |
| `5` | Full armour |
| `6` | Max rockets + super autofire |
| `7` | Shield |
| `8` | Smart bomb |
| `9` | God mode on/off |

Scores and profile stats from a cheated game are recorded as normal.

#### Interpolation

Due to the way Warblade intertangles logic ticks and rendering, it's not possible to get true >60FPS rendering without altering the game logic, which isn't want I want to do.

As a next best thing interpolation is used instead, which basically means two things:
- Subpixel positioning is enabled (original Warblade would snap to exact pixel coordinates)
- Draw calls for each 60FPS tick are recorded, and any draws of the same sprite that are detected to be moved from their previous position is able to have in-between frames calculated

The original logic + render ticks are instead ran on a fixed 16.66ms timer, and the "draws" from it aren't actually drawing anything, they're just recorded by the interpolation system to determine what to render when the next frame is requested.

While this does work for quite a few things (notably borders and enemies), there are a few caveats:
- Text can appear less solid & with artifacts around each character (this one is due to the subpixel positioning, as the original code assumes the pixel coordinate snapping)
- Text can jitter and appear slightly corrupted during animation
- Because I can't predict frames in the future without simulating them and irreversably updating global state, the displayed frame has to be in the past. Running at 120FPS, this is about 8ms of added input lag
- Since the game still renders internally at 800x600 which is then scaled to the window size, even with subpixel positioning there's not enough pixel space to represent certain fractional steps the interpolation wants to perform. Meaning that some effects that move slowly across the screen (like background stars) are still going to appear like they "snap" rather than move smoothly like you'd expect.

I may revisit this in the future with a completely new rendering system for Warblade, but for now this is the best I can do I think.

If you don't like how it looks, then you can easily turn it off in the settings. The default value for this option is `auto`, meaning that it's only enabled when your monitor's refresh rate is something other than 60 (since it won't make a difference otherwise) or you have VSync disabled.

### Installing

1. Copy `warblade-sr.exe` to your Warblade 1.34 installation folder.
2. Run the file.

All the dependencies `warblade-sr.exe` needs are built into the file, so you don't need to install anything.

You can also replace `warblade.exe` with this version, but I don't recommend it.


# Building


## `ptk` byte-matching branch

### Requirements

- **Visual Studio 2008** C++ RTM (not SP1) (Express version is fine too)
- A Warblade 1.34 installation

### Building

From a command prompt in the repository:

```bat
set WARBLADE_GAME_DIR=C:\path\to\Warblade
build.bat
```

`WARBLADE_GAME_DIR` defaults to `..\game`. The result is `build\warblade.exe` (a debug-CRT build, like the original).

## `sdl` modern branch

### Requirements

- **Visual Studio 2026** with **Desktop development with C++** installed
- vcpkg for SDL and its satellite libraries
- A Warblade 1.34 installation

### Building

From a command prompt in the repository:

```bat
set WARBLADE_GAME_DIR=C:\path\to\Warblade
build.bat
```

- `build.bat` (or `build.bat release`) builds `build\vs2026-release\warblade.exe`;
  `build.bat debug` and `build.bat asan` (AddressSanitizer) build into their own folders.
- `WARBLADE_GAME_DIR` defaults to `..\game`. The build folder is runnable: `data\` in it is a
  link to your installation's `data\` folder.
- `run.bat [release|debug|asan]` starts that build from its folder.

### Linux

The `sdl` branch also builds natively on Linux with GCC or Clang. You still need the data from a Warblade 1.34 installation (its `data/` folder).

```sh
sudo apt install build-essential cmake ninja-build git zlib1g-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev \
    libxtst-dev libasound2-dev libpulse-dev libgl-dev libegl-dev
WARBLADE_GAME_DIR=/path/to/Warblade ./build-linux.sh
cd build/linux-release && ./warblade
```

- `build-linux.sh` builds SDL3, SDL3_image and SDL3_mixer from source into `build/deps` the first time (statically, with the same features as the Windows build), then the game into `build/linux-release` (`build-linux.sh debug`: `build/linux-debug`), with a `data/` link to your installation's.
- Profiles, settings and screenshots go to `~/Documents/warblade` (or `~/warblade` when there is no `Documents` folder).
- The game's Windows paths are translated on the fly (`SysPath` in `src/core/sdl.c`): backslashes become slashes, and file names match regardless of case, as they did on Windows. The Microsoft C runtime calls it uses map onto POSIX in `include/posix/io.h`.

### Browser

The same code also builds to WebAssembly with [Emscripten](https://emscripten.org) and runs in a web page. The build has no game data: the page asks for your Warblade 1.34 folder, copies its `data` into the browser, and keeps it there (IndexedDB), so you only choose it once. Nothing is uploaded.

```sh
source /path/to/emsdk/emsdk_env.sh
./build-web.sh
python3 -m http.server -d build/web 8000     # then open http://localhost:8000
```

- `build-web.sh` builds zlib, SDL3, SDL3_image and SDL3_mixer for WebAssembly into `build/deps-web` the first time, then the game into `build/web` (`index.html`, `warblade.js`, `warblade.wasm`). The page (`web/index.html`) has to be served over HTTP; opening the file directly won't load the WebAssembly.
- The game keeps its blocking main loop thanks to Asyncify: `SysFlip` and `AudioUpdate` yield to the browser.
- Saves, profiles and settings live in the browser too (`/save`, kept in IndexedDB). The page saves the settings every few seconds and when the tab is hidden, since players close the tab rather than quit.
- It starts inside the page; `W` (or the settings page) switches to fullscreen. Frame interpolation is off in the browser.

### Docker

The browser version can also be served from a Docker image, with the game data and the saves on the server: players just open the page, and their profiles, saves, settings and high scores live in a volume. The image has no game data; you mount your own.

```sh
docker build -t warblade-sr .
docker run -d -p 8080:8080 \
    -v /path/to/Warblade/data:/data:ro \
    -v warblade-saves:/saves \
    warblade-sr
```

Then open http://localhost:8080. `docker-compose.yml` does the same with `docker compose up -d`.

Instead of building it, you can pull the image GitHub Actions builds from `sdl` (`.github/workflows/docker.yml`): `ghcr.io/rkj/warbladesr:latest`, also tagged `sdl` and `sha-<commit>`. While the package is private, log in first with a GitHub token that has `read:packages`: `docker login ghcr.io -u <github user>`.

- `/data`: your Warblade 1.34 `data` folder (with `warblade.pac`, `music`, `samples`), read-only. Mounting the whole installation folder works too.
- `/saves`: the game's user folder. The page loads it into the game, and every few seconds (and when the tab is hidden) sends back the files the game changed, so saves follow you between browsers and devices.
- The saves are shared by everyone using the server, like one PC: players get their own profiles in the game's profile menu. Two people playing at the same time can overwrite each other's settings file.
- The server is `docker/server.py` (Python standard library). The build stage runs `build-web.sh` in the official Emscripten image.


# Used libraries

### `ptk` branch

- [PTK](http://ptk.phelios.com)
- [BASS](https://www.un4seen.com) (distributed with your Warblade install)
- [zlib](https://zlib.net), [libpng](http://www.libpng.org/pub/png/libpng.html) and libjpeg, built into PTK

### `sdl` branch

- [SDL3](https://www.libsdl.org), [SDL3_image](https://github.com/libsdl-org/SDL_image) and [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer)
- [zlib](https://zlib.net), [libpng](http://www.libpng.org/pub/png/libpng.html) and [libjpeg-turbo](https://libjpeg-turbo.org), for images
- [libxmp](https://github.com/libxmp/libxmp) and [dr_mp3](https://github.com/mackron/dr_libs), for music

This software is based in part on the work of the Independent JPEG Group.
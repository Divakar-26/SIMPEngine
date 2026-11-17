<p align="center">
  <img src="docs/images/logo.gif" alt="The SIMPEngine wordmark writing itself letter by letter, with its Bézier control points showing, then flowers blooming and the dot of the i turning into a rose" width="760">
</p>

<p align="center">
  <sub>Drawn live by the engine: every letter is a Bézier path in <a href="assets/scripts/logo.lua"><code>assets/scripts/logo.lua</code></a>, written with trim tweens.</sub>
</p>

<p align="center">
  <b>Version 1.0.0</b> · <a href="CHANGELOG.md">Changelog</a> · <a href="LICENSE">MIT License</a>
</p>

# SIMPEngine

A 2D game engine and editor written from scratch in C++20 on SDL3 and OpenGL. It has an EnTT entity-component system, a batched SDF renderer, 2D lighting, particles, glow and bloom, tilemaps, audio, Lua scripting, Flutter-style tweens and vector shapes. The editor has a project hub, Play/Stop, undo and a built-in script editor. Make a game in the editor and ship it as a standalone folder.

## The editor

| | |
|---|---|
| ![Project hub](docs/images/editor-hub.png) | ![A Lua game running inside the editor](docs/images/editor-playing.png) |
| *The project hub: recent projects, new projects from templates, guides* | *Play mode: the game runs in the Game tab while the inspector shows its live state* |

![The built-in Lua script editor](docs/images/editor-script.png)

*The built-in script editor: Lua highlighting, API-aware autocomplete, live syntax checking and VS Code themes.*

## Features

**Engine**
- EnTT ECS with plain-struct components and per-system update order
- Batched OpenGL renderer: sprites, SDF shapes (rect, rounded rect, circle, ring, capsule), crisp lines and SDF text in one shader, z-sorted, per-primitive view culling
- 2D lighting: point and spot lights, ambient colour, flicker, per-pixel falloff (half-resolution HDR light map)
- Particle engine with 16 built-in presets, `.particle` files, emissive particles that glow in the dark
- Glow component (per-entity glow) and screen-space bloom
- Parallax layers, camera follow with dead zone, bounds and shake
- Chunked tilemaps with flip flags, flood fill and Tiled (`.tmj`/`.tsj`) import
- Audio sources on SDL3 audio streams (WAV)
- Rigid bodies through AccelEngine
- YAML scenes that round-trip every component, a virtual file system (`assets://`)
- Lua 5.5 scripts with hot reload and script events, plus native C++ scripts (`ScriptableEntity`)
- Flutter-style tweens (curves, springs, sequences, motion paths) and vector shapes with morphing
- Character Controller component for platformer and top-down movement
- Projects (`project.simp`) and a standalone runtime (`SIMPPlayer`) for shipped games

**Editor**
- Project hub: recent projects, new projects from templates, bundled guides
- Build Game: export a project to a folder that runs anywhere; Run Standalone
- Script editor with Lua autocomplete and VS Code themes; Tween Lab
- Viewport with gizmos, snapping, marquee selection, tile painting, light/emitter/collider overlays
- Game view with Play / Pause / Step / Stop (the scene is restored on Stop)
- Undo/redo for every edit, scene files with unsaved-change prompts
- Scene tree with groups, type filters, sorting, group/ungroup and reveal-on-select
- Inspector for every component, including particle, light, audio and camera settings
- Particle effect browser with live previews, tile palette, animation maker with automatic sprite-sheet slicing
- Content browser, asset picker, console, profiler, three themes

![Particle effect browser](docs/images/particle-picker.png)

*Particle effect browser with live previews*

## Building

Requirements: CMake 3.25+ (for the presets), Ninja, a C++20 compiler (tested with GCC 12 and Clang), and OpenGL 4.6 drivers. Linux is the tested platform.

All third-party libraries are git submodules in `vendor/` and are built from source. SDL3 on Linux additionally needs the system development packages for audio, windowing and input. On Debian/Ubuntu, install everything with:

```sh
sudo apt install build-essential git cmake ninja-build pkg-config \
  libasound2-dev libpulse-dev libjack-dev libsndio-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
  libwayland-dev libxkbcommon-dev libdecor-0-dev libdrm-dev libgbm-dev \
  libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev
```

Debian 12 ships CMake 3.25. On older distributions install a newer CMake (for example `pip install cmake`).

Build as a normal user (no `sudo`) in a folder on a Linux filesystem such as ext4. The build creates symlinks, so it fails on NTFS, exFAT and FAT drives (`failed to create symbolic link ... Operation not permitted`).

Clone recursively:

```sh
git clone --recursive https://github.com/Divakar-26/SIMPEngine.git
cd SIMPEngine
```

(Already cloned? Run `git submodule update --init --recursive`.)

```sh
cmake --preset dev && cmake --build --preset dev        # Debug, build/dev
cmake --preset release && cmake --build --preset release
cmake --preset asan && cmake --build --preset asan      # Clang + AddressSanitizer
```

Run the tests (headless, no window needed):

```sh
ctest --test-dir build/dev --output-on-failure       # or ./build/dev/bin/SIMPTests [filter]
```

Start the editor:

```sh
./build/dev/bin/SIMPEditor
```

It opens the project hub. Create a project from the Platformer template, press F5 to play it, then **Project > Build Game** (Ctrl+B) to make a standalone copy. [docs/SHIPPING.md](docs/SHIPPING.md) covers projects, builds and packaging.

### Packaging the engine

```sh
cmake --preset release && cmake --build --preset release
cpack --config build/release/CPackConfig.cmake      # SIMPEngine-1.0.0-Linux.tar.gz
```

### Install as a library

```sh
cmake --preset release
cmake --build --preset release --target Engine
sudo cmake --install build/release --prefix /usr/local --component Unspecified
```

Then in any CMake project:

```cmake
find_package(SIMPEngine REQUIRED)
target_link_libraries(MyGame PRIVATE SIMPEngine::Engine)
```

The full walkthrough, including the project layout and VS Code autocomplete, is in [docs/USING_THE_LIBRARY.md](docs/USING_THE_LIBRARY.md). The editor is not part of the installed library.

## Documentation

| Guide | What it covers |
|---|---|
| [docs/USING_THE_LIBRARY.md](docs/USING_THE_LIBRARY.md) | Install the engine and use it from your own project; VS Code setup |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | How the engine is put together: loop, layers, scene, systems, rendering, editor |
| [docs/API_GUIDE.md](docs/API_GUIDE.md) | Reference for the public API: application, scene, entities, every component, rendering, lighting, particles, audio, input, assets |
| [docs/SHIPPING.md](docs/SHIPPING.md) | Projects, the project hub, Run Standalone, Build Game, packaging the engine |
| [docs/SCRIPTING.md](docs/SCRIPTING.md) | Lua scripting: hooks, input, component handles, events, the script editor |
| [docs/ANIMATION.md](docs/ANIMATION.md) | Tweens, curves, springs, paths, vector shapes and morphing |
| [docs/HOW_TO.md](docs/HOW_TO.md) | Step-by-step recipes: first game, scripts, sprites and animation, tilemaps, particles, lights, levels, procedural generation, editor workflow |

## Project layout

| Path | Contents |
|---|---|
| `Engine/Engine/` | Engine library (`#include <Engine/...>`) |
| `Editor/` | Editor library: layer, panels, commands, undo, UI kit |
| `Apps/` | `SIMPEditor` (project hub + editor) and `SIMPPlayer` (runtime for shipped games) |
| `assets/` | Runtime data, addressed as `assets://...` |
| `docs/` | Guides and screenshots |
| `vendor/` | Third-party code (SDL3, SDL_image, ImGui, ImGuizmo, EnTT, GLM, glad, spdlog, yaml-cpp, nlohmann/json, nanosvg, stb, AccelEngine) |

## Credits

UI fonts: Inter (OFL) and JetBrains Mono; icons: Lucide (ISC).

## License

SIMPEngine is released under the [MIT License](LICENSE). Third-party libraries in `vendor/` and fonts and art in `assets/` keep their own licenses.

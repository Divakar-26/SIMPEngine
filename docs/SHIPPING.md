# Projects and Shipping Games

SIMPEngine 1.0 works with **projects**: a folder with a `project.simp` file and an `assets/` folder. The editor opens projects from the project hub, and **Build Game** turns a project into a folder you can zip and give to players.

## The project hub

Start `SIMPEditor`. The hub lists recent projects (stored in the user preference folder, not in the project), with:

- **New Project**: name, location and a template. *Empty* has a camera and a title. *Platformer* has a player with a Character Controller, a double jump script, platforms and a follow camera.
- **Open...**: any folder that contains `project.simp`.
- Double-click a row to open it. Right-click it to show it in the file manager or remove it from the list.
- **Learn** opens the guides that ship with the engine.

`SIMPEditor <project folder>` skips the hub and opens that project.

## project.simp

```yaml
Project:
  name: My Game
  version: 0.1.0
  engine: 1.0.0
  start_scene: assets://scenes/Main.yaml
  window:
    title: ""          # empty = the project name
    width: 1280
    height: 720
    fullscreen: false
    resizable: true
    vsync: true
```

Edit it from **Project > Project Settings...**. Changes are saved as you make them. **Use Current Scene** sets the start scene to the scene that is open.

Inside a project, `assets://` points to the project's `assets/` folder. Engine files (the default fonts, editor themes) live under `engine://`, so a project only contains its own files.

## Running the game outside the editor

**Run > Run Standalone** (Ctrl+F5) saves the scene and starts `SIMPPlayer` on the project, in its own window, from the start scene. This is exactly what players will see.

## Building

**Project > Build Game...** (Ctrl+B):

1. Check the start scene and window settings (the **Project Settings...** button opens them).
2. Pick the output folder. The default is `<project>/builds/<platform>` (the template's `.gitignore` excludes `builds/`). A folder from an earlier build is cleaned and reused. Any other non-empty folder is refused, so a build never deletes your files.
3. **Build**, or **Build and Run** to start the game when it finishes.

The build is for the platform the editor runs on. The result:

```
MyGame            the game (a renamed SIMPPlayer; MyGame.exe on Windows)
project.simp      window and start scene
assets/           everything in the project's assets folder (hidden files skipped)
engine/fonts/     Inter, the default font for Text components
lib/              shared libraries next to the game (Linux; DLLs sit next to the .exe on Windows)
```

The game finds `project.simp` next to its executable. It loads the libraries in `lib/` through an `$ORIGIN/lib` runpath, so the folder runs from anywhere. Zip it and ship it.

Scripts in a shipped game must be Lua (`Script: {lua: ...}` in the scene). C++ `ScriptableEntity` classes are compiled into a specific executable and are not part of the generic player.

## SIMPPlayer

`SIMPPlayer [project folder or project.simp]` runs any project. Without an argument it looks next to itself, which is how built games start. If the project can't be loaded it shows a message box with the reason.

## Packaging the engine

```sh
cmake --preset release && cmake --build --preset release
cpack --config build/release/CPackConfig.cmake
```

This makes `SIMPEngine-1.0.0-Linux.tar.gz` (a `.zip` on Windows) with `SIMPEditor`, `SIMPPlayer`, `lib/` and `engine/` (fonts, editor themes and these docs). Unpack it anywhere and run `SIMPEditor`. `cmake --install build/release --component Editor --prefix <dir>` installs the same files without archiving them.

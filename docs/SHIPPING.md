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
    maximized: false
    borderless: false
    orientation: landscape   # Android: landscape, portrait or auto
  display:
    width: 1920              # the game resolution: what one world unit of zoom 1 covers
    height: 1080
    aspect: keep_height      # keep_height, keep_width, expand or keep
    stretch: window          # window or fixed
    render_scale: 1.0        # 0.25 to 1: draw smaller and scale up (phones, slow GPUs)
```

Edit it from **Project > Project Settings...**. Changes are saved as you make them. **Use Current Scene** sets the start scene to the scene that is open.

### Game resolution and window

`window` is the window the player opens with. `display` is the size of the picture the game is made for, like Godot's base viewport. At camera zoom 1 the view is exactly `width x height` world units, so a smaller game resolution makes everything look bigger (use 480x270 or 640x360 for pixel art). The default 1920x1080 matches what earlier projects did.

| `aspect` | When the window has another shape |
|---|---|
| `keep_height` | the view always shows the game height, wider windows show more at the sides |
| `keep_width` | the view always shows the game width, taller windows show more above and below |
| `expand` | the whole game area is always visible, the extra space shows more of the world |
| `keep` | the game keeps its shape and the rest of the window is black bars |

`stretch: window` draws at the window's pixel size (sharp text and UI). `stretch: fixed` draws at the game resolution and scales the result up, nearest-neighbour when the scale is a whole number, the usual way to get crisp pixel art at any window size.

In the editor the **Game** panel has a **Window** menu to preview any window shape (the project's window, the game resolution, 16:9, 4:3, ultrawide, phone landscape and portrait, or a custom size); it renders exactly as the player would, including black bars and fixed-resolution scaling, and mouse input is mapped the same way. **Safe frame** outlines the game resolution inside the window. The yellow frame in the Viewport is the camera's view at the game resolution.

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

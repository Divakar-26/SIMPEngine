# Changelog

## 1.0.0 (09/11/2026)

The first stable release: make a 2D game in the editor, script it in Lua, and ship it as a standalone folder.

### Projects and shipping
- Project hub (`SIMPEditor`): recent projects, New Project with Empty and Platformer templates, Open, Learn page with the bundled guides.
- Projects: `project.simp` (name, version, start scene, window title, size, fullscreen, resizable, VSync) plus an `assets/` folder; Project Settings window in the editor.
- `SIMPPlayer`: the runtime with no editor code. It runs any project and is what built games are made of.
- Build Game (Ctrl+B): copies the player, its shared libraries, the project assets and the default font into a folder that runs anywhere. Build and Run, Open Folder.
- Run Standalone (Ctrl+F5) starts the project in its own window.
- `cpack` produces `SIMPEngine-1.0.0-<system>` archives with the editor, the player, libraries, fonts, themes and docs.
- Engine resources moved to their own `engine://` mount, so `assets://` belongs to the game.

### Scripting and animation
- Lua 5.5 scripting with per-entity instances, hot reload, script events (triggers, collisions, timers, health, paths, fixed update, jump/land) and handles for every component.
- Built-in script editor with highlighting, API-aware autocomplete, live syntax check and VS Code themes.
- Flutter-style tweens (curves, springs, sequences, motion paths), vector shapes with morphing, trim and dashes, and the Tween Lab panel.
- Character Controller component: platformer (coyote time, jump buffer, variable jump, multi-jump, one-way platforms) and top-down movement.

### Engine
- Mouse-to-world now uses the camera that was on screen; the camera updates after gameplay, without a frame of follow lag.
- Window settings through `ApplicationSpecification`; deferred layer changes with `Application::Queue`.
- Input resets when the window loses focus; frame delta clamped to 0.1 s; the loop idles while minimized.
- Render targets keep an opaque alpha channel (separate alpha blending).
- F1 scene reload is opt-in (`RenderingLayer::SetReloadScene`), so shipped games never reload on F1.

### Quality
- Test suite (`SIMPTests`, run with `ctest`): save/load round trip of every component, malformed scenes, VFS, camera mapping, layer stack, tilemaps, curves, tweens, character controller, projects.
- CI on GitHub Actions: Linux GCC and Clang (Debug and Release), AddressSanitizer, Windows MSVC, and release packages as build artifacts.
- Scene files carry a format version; malformed scenes fail with a logged error instead of crashing.
- Fix: rotated cameras rotated around the world origin instead of the camera position.
- Fix: tilemap chunks were saved in a random order, so the same scene could save differently each time.
- Builds with zero warnings in project code under GCC and Clang; the `asan` preset works again and the tests run clean under AddressSanitizer.
- MIT license.

### Editor
- Closing the window with unsaved changes asks to save.
- Stop returns to the tab that was open before Play.
- Long names, paths, labels and section titles shorten with "..." and show the full text on hover.
- Console rows grow to fit multi-line messages such as Lua tracebacks.

### Samples
- Mossveil (exploration platformer), Arrow Out (line puzzle), Glowworm (snake), the curve and morph Playground, and a tic-tac-toe script.

Earlier history is in `Changes.txt`.

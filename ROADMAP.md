# SIMPEngine Roadmap

Based on a full read of the codebase (Engine, Editor, Game, build files, scenes). Three tracks: **1. Bug fixes**, **2. Solidify the engine and add good features**, **3. Editor**.

**Status key:** `[x]` done, `[ ]` not done. Tick items as you finish them. Physics is out of scope for this roadmap (the physics engine is a separate project).

## Where the codebase stands

**Strong:** a clean layered architecture (Application → LayerStack → Layers), EnTT ECS with systems split into their own files, a batched OpenGL renderer (quads, lines and SDF shapes in one pipeline with texture slots and a Z-sorted batch), a VFS (`assets://`), a YAML scene serializer, a Tiled loader, an ImGui editor with docking, hierarchy, inspector, content browser, asset picker and animation maker, and a built-in profiler.

**Weak:** there is no play/stop separation, the engine has no tests or CI, a lot of features are half-wired (`AnimationController`, `ScriptComponent` in the editor, `SceneState`, `AssetManager`, `Tilemap`), the serializer only covers part of the component set, there is no audio, and the README promises gamepad input that doesn't exist. Global state (static `Renderer`, `TextureManager`, `Input`, `VFS`) is convenient now but will make testing and multiple scenes hard later.

## Progress overview

| Track | Done | Total |
|---|---|---|
| 1. Bug fixes | _update as you go_ | |
| 2. Engine | _update as you go_ | |
| 3. Editor | _update as you go_ | |

## Milestones

| | Milestone | Goal | Contents | Done |
|---|---|---|---|---|
| [ ] | **M0** | Trust the build | Build and verify §1.0, enable warnings, ASan Debug build, GL debug callback | |
| [ ] | **M1** | Safe data | Serializer round-trip (zIndex, hierarchy, tilemap, camera), one sprite-size source of truth, selection and capture fixes, shutdown order, tests + CI | |
| [ ] | **M2** | Editor workflow | Scene clone, Play/Stop, editor camera vs game camera, undo/redo, dirty tracking, gizmos | |
| [ ] | **M3** | Engine core | Fixed timestep, component registry, parenting and prefabs, input actions, animation controller, script lifecycle | |
| [ ] | **M4** | Content | Full Tiled support, tilemap editor, audio, particles, asset hot reload, content browser upgrades | |
| [ ] | **M5** | Ship a game | Small platformer or arcade game on the engine, packaged `NO_EDITOR` build, docs and tutorial | |

**Rule of thumb:** M1 and M2 come before any big new engine feature. Most of the pain right now comes from missing foundations (no clone, no round-trip test, no play mode), not from missing features.

---

## 1. Bug fixes

### 1.0 Already fixed in code (verify after building)

The last pass was done without a compiler. Build, then smoke-test each once and leave the box ticked only when verified.

- [x] Fixed: F1 reload no longer double-frees bodies
- [x] Fixed: scene save with a body produces valid YAML (save, reload, compare)
- [x] Fixed: animated sprites advance (`AnimationSystem` updates `animation`)
- [x] Fixed: Z-Index 2–10 no longer clipped (camera near/far)
- [x] Fixed: multi-row sprite sheets and tilesets pick the right frame/row (renderer UVs, Animation Maker preview, frame strip, `Tileset::GetTileRect`)
- [x] Fixed: Add Entity dialog adds components independently and resets all checkboxes
- [x] Fixed: Load Scene replaces the scene instead of appending
- [x] Fixed: collapsed/docked viewport no longer creates a 0×0 framebuffer
- [x] Fixed: layers are destroyed before the window/GL context on exit
- [x] Fixed: `EventDispatcher` no longer clears an already-set `Handled`
- [x] Fixed: `LifetimeSystem` child iteration and double-destroy
- [x] Fixed: `Animation` divide-by-zero and zero-duration infinite loop
- [x] Fixed: `Texture` sizes initialised, RGBA decode, in-memory textures set width/height
- [x] Fixed: `PROFILE_SCOPE` macro `__LINE__` expansion; per-sprite profiling removed
- [ ] Verified on a real build (all of the above)

### 1.1 Remaining bugs (not yet fixed)

**Data loss / correctness**

- [ ] Serializer drops data: `TransformComponent::zIndex` is never saved or loaded. Also missing: `HierarchyComponent`, `TilemapComponent`, camera zoom, `ScriptComponent` binding, animation playback state
- [ ] Sprite size is split across `SpriteComponent::width/height` and `RenderComponent`; the renderer only uses `RenderComponent` (falls back to 1×1). Pick one source of truth
- [ ] `SceneManager::LoadScene` on failure leaves an empty scene registered under that name, so a retry silently returns the empty one
- [ ] `TiledLoader` only reads layer 0, assumes `firstgid == 1`, ignores tile flip flags, and throws on malformed JSON

**Stale state / dangling references**

- [ ] `EditorContext::SelectedEntity` stays set after `Scene::Clear()` (F1) or entity deletion
- [ ] `SpriteSection` / `AnimationSection` picker callbacks capture the selected entity or a component by reference and run later. Capture the handle by value and re-fetch the component
- [ ] `Input::GetMouseWheel` keeps a stale value when nothing reads it, and truncates fractional trackpad deltas to int
- [ ] `Renderer::m_WindowWidth/Height` are never updated after startup, but `ConstellationScript` and the viewport frame rectangle read them

**Lifecycle / shutdown**

- [ ] `ImGuiLayer::OnDetach` is never called (`LayerStack` destructor does not detach layers)
- [ ] `Renderer`'s API, `TextureManager` and `SvgTextureCache` are statics that outlive the GL context. Add explicit shutdown calls
- [ ] `GLRenderingAPI::SetViewMatrix/SetProjectionMatrix` assign then flush, so pending draws use the new matrix (viewport axis lines lag one frame). Flush first, or set camera matrices before drawing the axes

**Build / repo hygiene**

- [ ] `Engine/CMakeLists.txt` lists several sources twice
- [ ] README example (`PushOverlay(new ...)`) no longer matches the `unique_ptr` API, and the README run path differs from where CMake puts `GameApp`
- [ ] `VFS::Mount("assets", "../assets")` depends on the working directory. Resolve relative to the executable
- [ ] `Texture` calls `glGenerateMipmap` but uses `GL_NEAREST` (wasted GPU work)
- [ ] Remove leftovers: `RenderingLayer::m_Scene` and `m_TestGrass`, unused includes in `ImGuiLayer`

### 1.2 Parked (hard-coded on purpose, leave alone for now)

Physics is out of scope for this roadmap. Your physics engine is a separate project that needs its own improvements, so nothing here touches it until you decide to.

- [ ] Bodies created in the inspector are not registered with the physics world; deleting an entity or "Remove Body" leaves a stale pointer in the world
- [ ] `PhysicsSystem::startFrame` loop (20×/frame)
- [ ] Sprites ignoring parent transforms, plus parent/child one-frame lag in `HierarchySystem`
- [ ] `CollisionSystem` empty O(n²) loop
- [ ] `Game/main.cpp` overwriting `Level2.yaml` at startup

### 1.3 Keep bugs from coming back

- [ ] Turn on warnings (`-Wall -Wextra`; `/W4` is already set on MSVC) and fix the current ones
- [ ] Add a Debug-only `GL_DEBUG_OUTPUT` callback so GL errors are logged instead of silent
- [ ] Add a Debug build with AddressSanitizer (Linux/Clang)

---

## 2. Solidify the engine and add good features

### Phase A: Foundation (before adding features)

- [ ] **Pass `TimeStep` consistently** through layers (TODO item 2) and stop converting float ↔ `TimeStep` ad hoc
- [ ] **Real resource ownership.** Pick one texture story (`TextureManager` vs `AssetManager` handles vs ad-hoc `make_shared<Texture>`), route everything through it, serialize handles or virtual paths. Fixes duplicated textures
- [ ] **Scene copy.** `Scene::Clone()` (or serialize to a memory buffer). Play/Stop, undo and prefabs all need it
- [ ] **Scene lifecycle events:** `OnSceneStart` / `OnSceneStop` for scripts and systems, instead of lazily creating script instances in `OnUpdate`
- [ ] **Tests.** Add Catch2 or doctest. Cover `TilemapUtils`, `VFS::Resolve`, `AssetRegistry` load/save, `FileSystem`, `Animation` frame math, `Camera2D` screen↔world, and a serializer round-trip (save, load, compare)
- [ ] **CI.** GitHub Actions: configure and build Debug on Windows and Linux, run tests
- [ ] **Dependency handling.** Pin vendored versions (submodules or `FetchContent`) and document exact SDL3 / ImGui-docking / EnTT versions

### Phase B: Core systems to finish

**Rendering**

- [ ] Finish the Z/layer story: sorting layers plus Z-index, documented stable sort
- [ ] Sprite flipping (X/Y), per-sprite tint/alpha (TODO item 2), pixel-perfect camera option, per-texture nearest/linear filter
- [ ] Texture atlas / `Tileset`-backed sprites to cut texture switches
- [ ] Make `Renderer::DrawSvg` correct for in-world use (SVG rasterizes unflipped, textures are flipped)
- [ ] Frustum culling in `TilemapSystem` and `RenderSystem` (every tile of every chunk is submitted every frame today)
- [ ] Expose render-to-texture for minimaps and post-processing

**Animation**

- [ ] Make `AnimationController` the real path: named clips (`Play("run")`), transitions on conditions, frame events. Have `RenderSystem` draw via the controller. Serialize clips
- [ ] Per-frame durations and a clip-end callback

**Input**

- [ ] Action mapping (`"Jump"` → Space / Gamepad A) instead of raw keycodes in scripts
- [ ] `WasPressedThisFrame` / `WasReleasedThisFrame`
- [ ] Gamepad support through SDL3 (the README already claims it)
- [ ] Fix wheel handling (see §1.1)

**Scene / ECS**

- [ ] Entity parenting as a first-class feature: reparent API that removes from the old parent, parent-first transform update, serialized
- [ ] Prefabs (YAML subtree you can instantiate), tags/layers, `UUID` component so references survive reloads
- [ ] Component registry (name → add / remove / serialize / inspect). Makes serializer, hierarchy "Add Component" menu and inspector data-driven instead of three hand-maintained lists

**Scripting**

- [ ] Real `ScriptableEntity` API: `OnStart`, `OnUpdate`, `OnFixedUpdate`, access to `Input`, `Scene`, other entities by name or UUID
- [ ] Serialize which script is bound and its public fields, show them in the inspector
- [ ] Later: hot reload of the game DLL, or an embedded Lua layer

**Other systems**

- [ ] **Audio:** SDL3 audio (or miniaudio), `AudioSource` component, one-shot and looping sounds, master/SFX/music buses, `Sound` asset type
- [ ] **Particles:** CPU particle system component drawing through the existing quad batch (emitters, lifetime curve, gravity, colour over life)
- [ ] **Game UI:** minimal in-game UI layer (text, buttons, anchored panels) with a font/text renderer in the game renderer
- [ ] **Camera:** follow target with damping and dead-zone, bounds clamping, screen shake, zoom easing. Move WASD / wheel / middle-drag handling out of `Camera2D::Update` into an editor camera controller

### Phase C: Content and tooling

- [ ] **Tilemaps:** multiple layers, proper Tiled support (several tilesets, flip flags, object layers → entities), tile animation, auto-tiling using the bitmask reference images in `assets/`
- [ ] **Asset pipeline:** finished `AssetRegistry` with stable GUIDs, a `.meta` file per asset, file watching and hot reload for textures and scenes, thumbnail cache
- [ ] **Build and packaging:** `NO_EDITOR` runtime build loading a packed asset folder; Release preset strips the profiler (`ENGINE_PROFILER_DISABLED`; it is currently always on)
- [ ] **Documentation:** keep `docs/*.md` accurate and add one "make your first game" tutorial that builds against the real API

### Phase D: Longer-term options

- [ ] Instancing / GPU-side sprite path (SSBO) if you ever hit tens of thousands of sprites (measure first with `RenderStats`)
- [ ] Multithreading: job system for animation/transform updates, background asset loading
- [ ] Networking (separate big project; not before Phase A and B)
- [ ] A second renderer backend only if actually needed (`RenderingAPI` already abstracts it but carries GL-isms)

---

## 3. Editor

### Phase E: Make editing safe and predictable

- [ ] **Play / Pause / Stop / Step.** `SceneState` exists in `EditorContext` and is unused. Toolbar with the existing `play`, `pause`, `step` SVGs. Play = clone the scene, run the clone; Stop = discard it. Remove the "only update while the viewport is focused and hovered" workaround in `EditorLayer::OnUpdate`
- [ ] **Separate editor camera from scene cameras.** Editor camera is always free-fly; Play uses the scene's primary camera (TODO items 9 and 14)
- [ ] **Undo / redo.** Route every edit through `EditorCommands` as command objects (create, delete, rename, transform change, add/remove component, property edit). Group drags into one step
- [ ] **Dirty tracking and save prompts.** `*` in the title, prompt before closing or loading over unsaved changes, "Save As" and "New Scene", and a file dialog (SDL3 has one) instead of the hardcoded `assets/scene.simpscene`
- [ ] **Selection that survives changes.** Validate `SelectedEntity` on scene clear and delete; multi-select (ctrl/shift in hierarchy, box-select in viewport)

### Phase F: Viewport tools

- [ ] **Gizmos.** ImGuizmo is already compiled in. Translate / rotate / scale with snapping, W/E/R shortcuts, local vs world space
- [ ] **Picking** that respects rotation, scale and Z, and works for tilemaps and animated sprites (current picker is axis-aligned)
- [ ] **Debug overlays** with toggles: collision boxes, entity bounds, grid, tile grid, camera frustum, FPS / draw-call stats (`RenderStats` already exists). Covers TODO items 12 and 13
- [ ] Gizmo drawn under popups (TODO item 6), hover problems (item 8), warping on resize (item 15)
- [ ] **Camera controls:** frame-selected (F already works), zoom-to-cursor, pixel-grid view

### Phase G: Panels

- [ ] **Hierarchy:** real tree from `HierarchyComponent` (drag-to-reparent, inline rename, search/filter, duplicate, delete with children, visibility/lock toggles, icons per component)
- [ ] **Inspector:** data-driven from the component registry. Add sections for Tilemap, Script, Hierarchy and camera zoom. Multi-edit, reset-to-default, tooltips, proper ranges for `zIndex`/scale/rotation, world vs local transform
- [ ] **Content browser:** low-res thumbnail cache, rename / delete / new folder / new scene, drag assets into the viewport to spawn an entity or into inspector fields, filter by type, use the asset registry. (The drag-move code accepts a `CONTENT_BROWSER_ITEM` payload that nothing starts.)
- [ ] **Animation Maker:** non-uniform frame lists, multiple clips per sheet, pivot/offset, write clips to an animation asset
- [ ] **Tilemap editor:** paint / erase / fill tools, tile palette, layers (chunk storage already exists)
- [ ] **Log panel:** level filter, search, clear, copy, coloured warnings
- [ ] **Profiler panel:** toggleable and dockable instead of always drawn; frame-time history of draw calls and quads
- [ ] **Project/Settings panel:** window size, VSync (hardcoded off), default scene, input map

### Phase H: Polish

- [ ] Save and restore window layout, panel visibility and last opened scene
- [ ] Keyboard shortcut map (Ctrl+S, Ctrl+Z, Ctrl+D, Delete, F2 rename, F focus)
- [ ] Theme and font scaling for HiDPI
- [ ] Editor-only gizmo icons for cameras and triggers

---

## How to use this roadmap

- Treat each checkbox as a ticket. Items with a TODO number from `TODOs.txt` are already noted where they belong.
- When you finish a phase, tick it here and add a line to `Changes.txt` so the history stays accurate.
- Re-read the code after M2. By then the shape of the engine will have changed and this file should change with it.

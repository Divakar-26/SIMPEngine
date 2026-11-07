# Architecture

This page describes how SIMPEngine is put together. For the API itself see [API_GUIDE.md](API_GUIDE.md); for recipes see [HOW_TO.md](HOW_TO.md).

## Modules

| Module | Path | Role |
|---|---|---|
| Engine | `Engine/Engine/` | Static library. Application loop, layers, scene/ECS, systems, renderer, audio, assets, VFS, input |
| Editor | `Editor/` | Static library. `EditorLayer`, panels, commands, undo history, editor camera, `EditorUI` design kit |
| Game | `Game/` | `GameApp` executable. Builds the scene and pushes layers (Mossveil lives in `Game/Mossveil/`) |

The editor is optional: building with `-DNO_EDITOR=ON` links only the engine and renders the scene straight to the window.

## Frame loop

`Application::Run` does, every frame:

1. `Input::BeginFrame()` clears per-frame edges (just pressed / just released, wheel).
2. SDL events go to the `ImGuiLayer`, then to `Input`, then to layers from top to bottom until one marks the event handled.
3. `OnUpdate(TimeStep)` on every layer, bottom to top, then `AudioEngine::Update()`.
4. Clear, ImGui begin, `OnRender()` on every layer, ImGui end, swap.

`LayerStack` owns layers through `unique_ptr`. `PushLayer`/`PushOverlay` call `OnAttach`; destruction detaches in reverse order. Overlays always sit above normal layers.

A typical game pushes, in order: its own game layer (level switching and other non-entity logic), the `RenderingLayer` (updates and draws the active scene), and, in editor builds, the `EditorLayer`.

## Scene and ECS

`Scene` owns an `entt::registry` and one instance of each system. Components are plain global-namespace structs in `Scene/Component.h`. `Entity` is a light handle (`entt::entity` + `Scene*`).

`Scene::OnUpdate(dt)` runs, in order:

1. physics frame start and transform sync
2. camera (follow, bounds, shake)
3. movement (`VelocityComponent`)
4. collision
5. hierarchy (world transforms)
6. lifetime (`LifetimeComponent` countdown and destroy)
7. scripts (`OnCreate` lazily on first update, then `OnUpdate`)
8. physics step
9. sprite animation
10. particles
11. audio sources

`Scene::OnRender()` runs:

1. hierarchy transforms, then parallax offsets
2. camera matrices (or the `OnRender(view, projection)` override used by the editor viewport)
3. tilemaps, shapes, sprites, non-emissive particles, text
4. script `OnRender`
5. lighting composite (when scene lighting is on)
6. emissive particles, glow components, screen bloom

Renderers read `TransformComponent::worldTransform` through `DecomposeWorld`, so parenting, rotation and scale apply everywhere.

## Rendering

`Renderer` is a static facade over a `RenderingAPI`. The OpenGL backend (`GLRenderingAPI`) collects `Primitive`s, stable-sorts them by z when flushed, and draws them in batches of up to 16 textures per blend mode with one shader (`Shaders/Renderer2DShader.h`). The shader has four modes: textured sprite, SDF shape, SDF text, and analytic light.

- Every primitive is culled against the visible rectangle before it is queued (`Renderer::IsVisible`).
- Changing view, projection or render target flushes the batch.
- Render targets are `Framebuffer`s (RGBA8 or RGBA16F) bound with `Renderer::BeginRenderTarget` / `EndRenderTarget`.

Post-processing passes run inside `Scene::RenderContent` on whatever target is bound:

| Pass | Implementation |
|---|---|
| Lighting | `LightingSystem` renders lights additively into a half-resolution RGBA16F light map cleared to the ambient colour, then multiplies it over the view |
| Emissive particles | Particle effects marked `Emissive` are drawn after the multiply so they are not darkened |
| Glow | `GlowSystem` redraws glowing entities into a half-resolution emission buffer, blurs it with the bloom chain and adds it |
| Bloom | `BloomRenderer` thresholds the frame, runs a 13-tap downsample / tent upsample mip chain and adds it back |

## Assets

All runtime paths are virtual: `assets://textures/foo.png`. `VFS::Mount("assets", "../assets")` in `main()` walks up from the executable until it finds the folder. `TextureManager` caches textures by path; scenes store virtual paths so they load from anywhere.

| Asset | Loader |
|---|---|
| Images | `TextureManager::LoadTexture` (stb_image) |
| SVG | `SvgTextureCache` (nanosvg) |
| Fonts | `Font::Load` (stb_truetype SDF atlas) |
| Scenes | `SceneSerializer` (YAML) |
| Particle effects | `ParticleSerializer` (`.particle`, YAML) |
| Tiled maps and tilesets | `TiledLoader` (JSON, CSV layers) |
| Audio | `AudioEngine` (WAV via SDL) |

## Editor

`EditorLayer` turns off the rendering layer's backbuffer output and simulation, then draws the scene into its own framebuffers:

- **Viewport** — `EditorCamera`, gizmos, picking, tile painting, overlays. Never simulates gameplay (particles still animate for preview).
- **Game** — renders through the scene's primary camera and owns game input while playing.

Play takes a YAML snapshot of the scene (`EditorHistory::Capture`) and Stop restores it, re-binding script factories by entity order. Undo/redo uses the same snapshots: property widgets call `EditorUI::NotifyEdit()`, discrete commands call `EditorCommands::RecordEdit()` before changing anything.

All panels share an `EditorContext` (scene, selection, play state, tool state). UI is built from the `EditorUI` kit (`Editor/UI/`): theme colours, fonts, section headers, property rows, tree rows, tiles and toolbars.

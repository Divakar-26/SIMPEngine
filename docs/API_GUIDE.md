# API Guide

Reference for SIMPEngine's public API as of November 2026. Everything lives in namespace `SIMPEngine` except components, which are plain global structs. Include paths start at `Engine/Engine/`, e.g. `#include <Engine/Scene/Scene.h>`.

- [Application and layers](#application-and-layers)
- [Scene](#scene)
- [Entity](#entity)
- [Components](#components)
- [Scripts](#scripts)
- [Input](#input)
- [Rendering](#rendering)
- [Camera](#camera)
- [Lighting, glow and bloom](#lighting-glow-and-bloom)
- [Particles](#particles)
- [Tilemaps and Tiled](#tilemaps-and-tiled)
- [Audio](#audio)
- [Physics](#physics)
- [Assets, VFS and serialization](#assets-vfs-and-serialization)
- [Logging and profiling](#logging-and-profiling)

---

## Application and layers

```cpp
#include <Engine/Application/Application.h>

class MyGame : public SIMPEngine::Application
{
public:
    MyGame() { /* create scenes, push layers */ }
};

SIMPEngine::Application *SIMPEngine::CreateApplication() { return new MyGame(); }
```

| Member | Description |
|---|---|
| `void PushLayer(std::unique_ptr<Layer>)` | Adds a layer below all overlays; calls `OnAttach` |
| `void PushOverlay(std::unique_ptr<Layer>)` | Adds a layer on top; calls `OnAttach` |
| `Window &GetWindow()` | The SDL window |
| `static Application &Get()` | The running application |

`main()` must initialise logging and mount assets before creating the application:

```cpp
int main()
{
    SIMPEngine::Log::Init();
    SIMPEngine::VFS::Mount("assets", "../assets");
    auto *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
```

### Layer

```cpp
class Layer
{
public:
    Layer(const std::string &name = "Layer");
    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnUpdate(TimeStep ts);   // ts.GetSeconds()
    virtual void OnRender();
    virtual void OnEvent(Event &e);
};
```

### RenderingLayer

Updates and draws the active scene of a `SceneManager`.

| Member | Description |
|---|---|
| `RenderingLayer(std::shared_ptr<SceneManager>)` | |
| `Scene &GetScene()` | Active scene |
| `void SetSimulationEnabled(bool)` | When false, `Scene::OnUpdate` is not called |
| `void SetRenderToBackbuffer(bool)` | When false, the layer does not draw to the window (the editor draws into its own panels) |
| `void Step(float dt)` | Runs one update (used by the editor's Step button) |

### SceneManager

| Member | Description |
|---|---|
| `void AddScene(name, std::shared_ptr<Scene>)` | Registers a scene |
| `void SetActiveScene(name)` | Switches the active scene |
| `bool LoadScene(name, path)` | Loads a YAML scene and activates it; registers it only if loading succeeds |
| `std::shared_ptr<Scene> GetActiveScene()` | |

---

## Scene

`#include <Engine/Scene/Scene.h>`

| Member | Description |
|---|---|
| `Scene(const std::string &name = "untitled")` | |
| `Entity CreateEntity(const std::string &name = "")` | New entity with `TransformComponent` and `TagComponent` |
| `EntityBuilder BuildEntity(const std::string &name = "")` | Fluent creation, see below |
| `Entity GetEntityByName(const std::string &name)` | First entity whose tag matches, or an invalid entity |
| `void DestroyEntity(Entity)` | Destroys immediately. From scripts prefer `LifetimeComponent` |
| `void SetParent(entt::entity child, entt::entity parent)` | Adds `HierarchyComponent` links; the child's transform becomes local to the parent |
| `void Clear()` | Destroys every entity |
| `entt::registry &GetRegistry()` | Raw EnTT access for views |
| `void OnUpdate(float dt)` / `void OnRender()` | Called by `RenderingLayer` |
| `void UpdateTransforms()` | Recomputes world transforms now |
| `void UpdateParticles(float dt)` | Simulates particles only (editor preview) |
| `glm::vec2 ScreenToWorld(const glm::vec2 &viewportPixel)` | Through the active camera |
| `Camera2D &GetActiveCamera()` | Primary `CameraComponent` camera, or a default one |
| `const glm::vec4 &GetBackgroundColor()` / `SetBackgroundColor(glm::vec4)` | Clear colour |
| `SceneLighting &GetLighting()` | `{ bool Enabled; glm::vec4 Ambient; }` |
| `BloomSettings &GetBloom()` | `{ bool Enabled; float Threshold, Knee, Intensity, Radius; }` |
| `void SetLightingVisible(bool)` | Temporarily skip lighting, glow and bloom (editor preview toggle) |
| `void MarkPhysicsDirty()` | Rebuild the physics world next update; call after adding/removing bodies |
| `void StopAudio()` | Stops every audio source's voice |

### EntityBuilder

```cpp
auto e = scene->BuildEntity("Crate")
             .At(120.0f, 40.0f)
             .Scale(2.0f, 2.0f)
             .Rotation(15.0f)
             .ZIndex(1.0f)
             .With<SpriteComponent>()
             .Build();
```

`With<T>(args...)` emplaces a component with constructor arguments.

---

## Entity

`#include <Engine/Scene/Entity.h>`

```cpp
auto &sprite = entity.AddComponent<SpriteComponent>();
if (entity.HasComponent<LightComponent>())
    entity.GetComponent<LightComponent>().intensity = 2.0f;
entity.RemoveComponent<VelocityComponent>();
entt::entity handle = entity.GetHandle();
Scene *scene = entity.GetScene();
if (entity) { /* still alive */ }
```

`Entity` is a value type (`entt::entity` + `Scene*`). `operator bool` checks that the handle is still valid in the registry.

---

## Components

All in `Scene/Component.h`. Sizes are world units (1 unit = 1 pixel at zoom 1). Angles are degrees. Colours are `SDL_Color` (0–255) unless noted.

### TagComponent
`std::string Tag` — the entity name.

### TransformComponent
| Field | Default | Notes |
|---|---|---|
| `glm::vec2 position` | `0,0` | Local to the parent, world when there is no parent |
| `float rotation` | `0` | Degrees, counter-clockwise |
| `glm::vec2 scale` | `1,1` | |
| `float zIndex` | `0` | Draw order; higher draws later. Keep within ±1000 for game cameras |
| `glm::mat4 worldTransform` | | Written by the hierarchy system; read with `DecomposeWorld(transform)` |
| `bool dirty` | | Set to `true` after changing fields from code |

### HierarchyComponent
`entt::entity parent`, `std::vector<entt::entity> children`. Use `Scene::SetParent`.

### SpriteComponent
| Field | Notes |
|---|---|
| `std::shared_ptr<Texture> texture` | |
| `float width, height` | Display size before scale |
| `SDL_Color tint` | Multiplies the texture; alpha fades |
| `bool flipX, flipY` | |
| `bool useRegion` + `SDL_FRect region` | Draw a sub-rectangle in pixels from the image's top-left (sprite sheets) |

### AnimatedSpriteComponent
`std::unique_ptr<Animation> animation` (grid sheet frames, see [Rendering](#rendering)). The display size comes from a `SpriteComponent` (`width`, `height`, `tint`) or a `RenderComponent` on the same entity; with neither, the animation is drawn at the entity's scale in world units (1×1 by default), so add a `SpriteComponent` to size it. Animations can't be flipped; use `SpriteComponent::region` + `flipX` when you need facing.

### RenderComponent
A filled rectangle: `float width, height`, `SDL_Color color`.

### TextComponent
`std::string Text`, `float Size`, `SDL_Color Color`, `int Align` (0 left, 1 centre, 2 right), `std::string FontPath` (empty = default font).

### VelocityComponent
`float vx, vy` — moved by the movement system every update.

### CollisionComponent
`float width, height, offsetX, offsetY` — an axis-aligned box whose bottom-left corner is `position + offset` (the entity's local position, not the world transform). The collision system only tests overlaps and does nothing with them; nothing is pushed apart and no callbacks fire. Use these boxes from your own scripts (see [HOW_TO](HOW_TO.md#move-a-character-with-your-own-collision)) or use rigid bodies.

### PhysicsComponent
`std::unique_ptr<AccelEngine::RigidBody> body` — see [Physics](#physics).

### CameraComponent
| Field | Notes |
|---|---|
| `Camera2D Camera` | The camera itself |
| `bool primary` | The primary camera renders the game |
| `std::string followTarget` | Tag of the entity to follow (empty = none) |
| `glm::vec2 followOffset` | Added to the target position |
| `float followSmoothing` | Catch-up speed; 0 = instant |
| `glm::vec2 deadZone` | Box (width, height) the target can move in without moving the camera |
| `bool useBounds` + `glm::vec2 boundsMin, boundsMax` | Keeps the whole view inside a world rectangle |
| `float shakeStrength` | Maximum shake offset in units |
| `void Shake(float amount)` | Adds trauma (0–1). Shake falls off over time |

The camera system writes the follow result into the camera entity's `TransformComponent`.

### LifetimeComponent
`float remaining` — seconds until the entity is destroyed, together with all its children. Safe to add from scripts (`registry.emplace_or_replace<LifetimeComponent>(e, LifetimeComponent{0.0f})`).

### TilemapComponent
| Field | Notes |
|---|---|
| `std::shared_ptr<Tileset> tileset` | |
| `float tileSize` | Cell size in world units |
| `std::unordered_map<int64_t, TileChunk> chunks` | 16×16 chunks; use the [tilemap helpers](#tilemaps-and-tiled) |
| `SDL_Color tint`, `bool visible` | |
| `bool solid` | Reported by `IsSolidAt` / `OverlapsSolid` |

### ParticleEmitterComponent
`ParticleEffect effect`, `bool playing`, `ParticleState state` (runtime pool, not saved). See [Particles](#particles).

### LightComponent
| Field | Default | Notes |
|---|---|---|
| `LightType type` | `Point` | `Point` or `Spot` |
| `SDL_Color color` | warm white | |
| `float intensity` | 1 | Above 1 widens the fully lit area |
| `float radius` | 240 | Scaled by the entity scale |
| `float falloff` | 1 | Exponent: below 1 stays bright to the edge |
| `float innerAngle, outerAngle` | 20, 35 | Spot half-angles in degrees; the cone points along the entity rotation |
| `float flicker, flickerSpeed` | 0, 8 | Flicker amount 0–1 |
| `bool enabled` | true | |

Lights only show when `scene.GetLighting().Enabled` is true.

### GlowComponent
`SDL_Color color` (multiplies the entity's own colours), `float intensity`, `float spread`, `bool enabled`. Works on sprites, animations, shapes, text, tilemaps and particles.

### ParallaxComponent
`glm::vec2 factor` — how fast the entity and its children follow the camera: 0 sticks to the screen, 1 is normal, below 1 is background, above 1 is foreground. A common pattern is a group entity with the component and the layer's sprites as children.

### AudioSourceComponent
`std::string clip` (WAV path), `float volume`, `float pitch`, `bool loop`, `bool playOnStart`. Runtime: `uint32_t voice`, `bool started`.

### ScriptComponent
Holds a native script; see below. Not saved in scene files.

---

## Scripts

```cpp
#include <Engine/Scene/ScriptableEntity.h>

class Spinner : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override {}
    void OnUpdate(float dt) override
    {
        auto &t = GetComponent<TransformComponent>();
        t.rotation += 90.0f * dt;
        t.dirty = true;
    }
    void OnRender() override {}   // extra drawing in world space
    void OnDestroy() override {}
};

registry.emplace<ScriptComponent>(entity).Bind<Spinner>();
```

| Member | Description |
|---|---|
| `T &GetComponent<T>()` | Component on this script's entity |
| `Entity GetEntity()` | This entity; `GetEntity().GetScene()` for the scene |
| `glm::vec2 GetMouseWorldPosition()` | Mouse in world space, correct inside the editor's Game panel |
| `bool IsMouseInViewport()` | Whether the mouse is over the game view |

Scripts are created lazily on the scene's first update. Don't destroy entities during `OnUpdate`; add a `LifetimeComponent` instead. Rebuilding the whole scene should happen in a layer's `OnUpdate`, not inside a script.

---

## Input

`#include <Engine/Input/Input.h>` and `<Engine/Input/SIMP_Keys.h>` (`SIMPK_A`, `SIMPK_SPACE`, `SIMPK_LSHIFT`, `SIMPK_LEFT`, `SIMPK_1`, …).

| Function | Description |
|---|---|
| `Input::IsKeyPressed(key)` | Held |
| `Input::IsKeyJustPressed(key)` | Went down this frame |
| `Input::IsKeyJustReleased(key)` | Went up this frame |
| `Input::IsMouseButtonPressed(b)` / `JustPressed(b)` / `JustReleased(b)` | `SDL_BUTTON_LEFT` etc. |
| `Input::GetMouseWheel()` | Wheel delta this frame |
| `Input::GetMouseViewportPosition()` | Pixels relative to the game view |
| `Input::IsMouseInViewport()` | |
| `Input::SetEnabled(bool)` / `IsEnabled()` | The editor disables game input outside the Game panel |

---

## Rendering

`#include <Engine/Rendering/Renderer.h>`. Draw calls are valid inside `OnRender` (scripts, layers) after the camera matrices are set; they are batched and sorted by z.

| Function | Description |
|---|---|
| `DrawSprite(const SpriteDesc &)` | Texture with position, size, rotation, z, tint, source rect, flips |
| `DrawTexture(tex, x, y, w, h, tint, rotation, z, srcRect)` | Shorthand |
| `DrawQuad(x, y, w, h, rotation, color, fill, z)` | Rectangle centred at `x, y` |
| `DrawRect(x, y, w, h, thickness, color, rotation, z)` | Outline |
| `DrawRoundedRect(cx, cy, halfW, halfH, radius, color, aa, z)` | |
| `DrawCircle(cx, cy, r, color, aa, z)` / `DrawRing(cx, cy, r, thickness, color, aa, z)` | |
| `DrawLine(x1, y1, x2, y2, color, z)` | One pixel wide at any zoom |
| `DrawLine(x1, y1, x2, y2, width, color, z)` / `DrawCapsuleLine(...)` | |
| `DrawText(text, x, y, size, color, align, rotation, z, font)` | SDF text; `TextAlign::Left/Center/Right` |
| `DrawLight(center, radius, rotation, color, falloff, inner, outer, z)` | Used by the lighting pass |
| `SetBlendMode(BlendMode::Alpha / Additive / Multiply)` | Affects following draws |
| `SetColorMultiplier(glm::vec4)` | Multiplies every following primitive's colour |
| `GetWorldPerPixel()` | World units per screen pixel (size hairlines) |
| `IsVisible(center, halfExtent)` / `GetVisibleBounds(min, max)` | View culling helpers |
| `BeginRenderTarget(Framebuffer &)` / `EndRenderTarget()` | Render into a texture |
| `Flush()` | Draw everything queued so far |

```cpp
SIMPEngine::SpriteDesc desc;
desc.TextureRef = texture;
desc.Position = {0.0f, 0.0f};
desc.Size = {64.0f, 64.0f};
desc.Tint = {255, 255, 255, 255};
SIMPEngine::Renderer::DrawSprite(desc);
```

### Texture and Framebuffer
`Texture`: `GetID()`, `GetWidth()`, `GetHeight()`, `SetFilter(TextureFilter::Nearest | Linear)`, `GetVFSPath()` / `SetVFSPath()`. `Framebuffer(width, height, hdr = false)`, `Resize`, `GetColorAttachment()`.

### Animation
```cpp
auto anim = std::make_unique<SIMPEngine::Animation>(sheet, frameW, frameH, frameCount, frameDuration, true);
anim->SetPlaybackState(startFrame, 0.0f, false);
entity.AddComponent<AnimatedSpriteComponent>().animation = std::move(anim);
```
Frames are read left to right, top to bottom; columns = `sheetWidth / frameW`. `SpriteSheetSlicer::Detect(path)` guesses `FrameWidth`, `FrameHeight` and `FrameCount` from the gaps between frames.

### Font
`Font::Load(path)`, `Font::GetDefault()`, `font->MeasureText(text, size)`.

---

## Camera

`Camera2D` (`Math/Camera2D.h`), usually inside a `CameraComponent`.

| Member | Description |
|---|---|
| `SetPosition(vec2)` / `GetPosition()` | Centre of the view |
| `SetZoom(float)` / `GetZoom()` | View height = 1080 × zoom units |
| `SetRotation(float)` | Degrees |
| `SetManualControl(bool)` | Default true: WASD/wheel/drag move the camera. Turn off for game cameras |
| `ScreenToWorld(vec2)` / `WorldToScreen(vec2)` | |
| `GetVisibleWorldBounds(left, right, top, bottom)` | |

---

## Lighting, glow and bloom

```cpp
auto &lighting = scene.GetLighting();
lighting.Enabled = true;
lighting.Ambient = {0.3f, 0.35f, 0.4f, 1.0f};

auto &bloom = scene.GetBloom();
bloom.Enabled = true;
bloom.Threshold = 0.75f;   bloom.Intensity = 0.6f;

auto lamp = scene.CreateEntity("Lamp");
auto &light = lamp.AddComponent<LightComponent>();
light.color = {255, 200, 140, 255};
light.radius = 400.0f;
light.flicker = 0.2f;

lamp.AddComponent<GlowComponent>().intensity = 1.5f;
```

- Lighting multiplies the frame by the light map, so it can darken but never brighten past the unlit colours.
- Script `OnRender` output (HUDs) is lit too; place a light under HUD text or keep it in a bright area.
- Emissive particles, glow and bloom are added after lighting, so they glow in the dark.

---

## Particles

`#include <Engine/Particles/ParticleEffect.h>`, `ParticleEmitter.h`, `ParticlePresets.h`, `ParticleSerializer.h`.

### ParticleEffect (settings)
| Group | Fields |
|---|---|
| Emission | `Looping`, `Duration`, `Rate` (per second), `Burst` (per loop), `MaxParticles`, `Prewarm`, `WorldSpace` |
| Shape | `Emitter` (`Point`, `Circle`, `Box`, `Line`), `Radius`, `BoxSize` (`x` is the line length), `EmitFromEdge` |
| Motion | `Lifetime` (min, max), `Direction`, `Spread` (±degrees), `Speed` (min, max), `Gravity`, `Drag`, `Radial`, `Orbital`, `Turbulence`, `TurbulenceFrequency` |
| Size | `Size` (min, max), `EndSize` (multiplier at end of life), `Stretch` (lengthen along velocity) |
| Rotation | `StartRotation`, `AngularVelocity` (ranges), `AlignToVelocity` |
| Colour | `StartColor`, `MidColor` + `UseMidColor`, `EndColor` (`glm::vec4` 0–1), `HueVariation`, `FadeIn` |
| Look | `Shape` (`Soft`, `Circle`, `Square`, `Spark`, `Star`, `Smoke`, `Ring`, `Texture` + `CustomTexture`), `Blend`, `Emissive` |

```cpp
auto e = scene.CreateEntity("Campfire");
auto &emitter = e.AddComponent<ParticleEmitterComponent>();
emitter.effect = SIMPEngine::ParticlePresets::Find("Fire")->Effect;
emitter.effect.Rate = 60.0f;
```

| Function | Description |
|---|---|
| `ParticlePresets::All()` / `Find(name)` | Built-ins: Fire, Smoke, Embers, Sparks, Explosion, Dust Puff, Shockwave, Magic Orbit, Heal, Portal, Snow, Rain, Fireflies, Fountain, Bubbles, Confetti |
| `ParticleSerializer::SaveFile(path, effect)` / `LoadFile(path, effect)` | `.particle` files |
| `ParticleEmitter::Restart(state)` | Start over |
| `ParticleEmitter::Emit(effect, state, frame, count)` | Manual burst; frame from `ParticleSystem::FrameOf(transform)` |

For one-shot effects set `Looping = false` and add a `LifetimeComponent` so the entity cleans itself up.

---

## Tilemaps and Tiled

`#include <Engine/Scene/TilemapUtils.h>`. Tile coordinates have y up; a tile id is `row * columns + column` counted from the tileset image's top-left; `-1` is empty.

| Function | Description |
|---|---|
| `SetTile(tm, x, y, id, flags = 0)` | `flags`: `TileFlipX`, `TileFlipY` |
| `GetTile(tm, x, y)` / `GetTileId(tm, x, y)` / `EraseTile(tm, x, y)` | |
| `FloodFill(tm, start, id, flags, boundsMin, boundsMax)` | Returns tiles filled |
| `GetUsedBounds(tm, min, max)` / `CountTiles(tm)` / `PruneEmptyChunks(tm)` | |
| `WorldToTile(tm, transform, world)` / `TileToWorld(tm, transform, cell)` | Respect position and scale |
| `IsSolidAt(registry, world)` / `OverlapsSolid(registry, min, max)` | Query tilemaps marked `solid` |

`Tileset(texture, tileW, tileH, margin = 0, spacing = 0)`.

`TiledLoader` (`Scene/Loaders/TiledLoader.h`): `LoadTileset(path)` for `.tsj`, `Import(mapPath, scene)` creates one entity per tile layer under a root, `Load(mapPath, tilemap, transform)` loads the first layer into one component. Layers must use CSV encoding.

---

## Audio

`#include <Engine/Audio/AudioEngine.h>` — initialised by `Application`.

| Function | Description |
|---|---|
| `VoiceId Play(path, volume = 1, pitch = 1, loop = false)` | Returns 0 on failure |
| `Stop(id)` / `StopAll()` / `IsPlaying(id)` | |
| `SetVolume(id, v)` / `SetPitch(id, p)` / `SetLooping(id, bool)` | |
| `SetMasterVolume(v)` / `SetPaused(bool)` | |
| `Preload(path)` / `GetClipLength(path)` | |

Only WAV is supported. For sounds tied to entities use `AudioSourceComponent`.

---

## Physics

```cpp
auto &pc = entity.AddComponent<PhysicsComponent>();
pc.body = std::make_unique<AccelEngine::RigidBody>();
pc.body->shapeType = AccelEngine::ShapeType::AABB;   // or CIRCLE
pc.body->aabb.halfSize = {16.0f, 16.0f};             // or circle.radius
pc.body->inverseMass = 1.0f;                         // 0 = static
pc.body->restitution = 0.3f;
pc.body->entityID = static_cast<uint32_t>(entity.GetHandle());
glm::vec2 at = entity.GetComponent<TransformComponent>().position;
pc.body->position = {at.x, at.y};
scene.MarkPhysicsDirty();
```

- At the start of every update the body position is copied **into** the transform, so a body whose `position` you never set snaps the entity to the origin. Set it, or call `scene.SyncPhysicsFromTransforms()` after placing entities (the editor does this when you press Play).
- The world is rebuilt from all bodies at the start of the next update after `MarkPhysicsDirty()`.
- Gravity, friction and collision response come from AccelEngine; see its headers in `include/AccelEngine/`.

---

## Assets, VFS and serialization

| API | Description |
|---|---|
| `VFS::Mount(alias, path)` / `VFS::Resolve(virtualPath)` | `assets://x/y.png` → real path |
| `TextureManager::Get().LoadTexture(id, path)` | Cached; call `SetVFSPath(path)` if the texture will be saved in a scene |
| `SceneSerializer s(&scene)` | |
| `s.Serialize(path)` / `s.Deserialize(path)` | YAML files |
| `s.SerializeToString()` / `s.DeserializeFromString(yaml)` | |
| `s.SerializeEntities(handles)` / `s.DeserializeEntities(yaml)` | Copy/paste a subset |

Scene files store every component except scripts. Scripts must be rebound in code after loading.

---

## Logging and profiling

```cpp
LOG_INFO("Player at {} {}", x, y);   // game
CORE_WARN("Texture missing: {}", path);   // engine
PROFILE_SCOPE("Pathfinding");   // Core/Profiler/EngineProfiler.h
```

Logs appear in the terminal and the editor's Console panel. Profiled scopes appear in View → Profiler.

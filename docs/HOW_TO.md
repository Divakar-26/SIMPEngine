# How-To Guide

Short recipes for common tasks. The full API is in [API_GUIDE.md](API_GUIDE.md); the sample game in `Game/Mossveil/` uses almost everything here.

- [Start a new game](#start-a-new-game)
- [Create entities in code](#create-entities-in-code)
- [Write a script](#write-a-script)
- [Move a character with your own collision](#move-a-character-with-your-own-collision)
- [Sprites, sprite sheets and animation](#sprites-sprite-sheets-and-animation)
- [Build a tilemap](#build-a-tilemap)
- [Add particles](#add-particles)
- [Light a scene](#light-a-scene)
- [Make things glow](#make-things-glow)
- [Parallax backgrounds](#parallax-backgrounds)
- [Follow the player with the camera](#follow-the-player-with-the-camera)
- [Draw a HUD](#draw-a-hud)
- [Play sounds](#play-sounds)
- [Switch levels](#switch-levels)
- [Generate a level procedurally](#generate-a-level-procedurally)
- [Save and load scenes](#save-and-load-scenes)
- [Use the editor](#use-the-editor)
- [Keep it fast](#keep-it-fast)

---

## Start a new game

`Game/main.cpp` owns startup. A minimal game:

```cpp
#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/VFS.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/SceneManager.h>

#ifndef NO_EDITOR
#include "EditorLayer.h"
#endif

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        auto scenes = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("Main");
        scenes->AddScene("Main", scene);
        scenes->SetActiveScene("Main");

        BuildWorld(*scene);

        auto rendering = std::make_unique<SIMPEngine::RenderingLayer>(scenes);
        auto *renderingPtr = rendering.get();
        PushOverlay(std::move(rendering));
#ifndef NO_EDITOR
        PushOverlay(std::make_unique<EditorLayer>(renderingPtr));
#endif
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication() { return new MyGame(); }

int main()
{
    SIMPEngine::Log::Init();
    SIMPEngine::VFS::Mount("assets", "../assets");
    auto *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
```

Put game sources under `Game/<YourGame>/` and add every `.cpp` to `Game/CMakeLists.txt`.

---

## Create entities in code

```cpp
auto camera = scene.CreateEntity("Camera");
auto &cc = camera.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f));
cc.primary = true;

auto box = scene.BuildEntity("Box")
               .At(0.0f, 50.0f)
               .With<RenderComponent>()
               .Build();
auto &shape = box.GetComponent<RenderComponent>();
shape.width = shape.height = 64.0f;
shape.color = {240, 180, 80, 255};
```

Group related entities under a parent so the Scene panel stays tidy:

```cpp
auto group = scene.CreateEntity("Decor");
auto rock = scene.CreateEntity("Rock");
scene.SetParent(rock.GetHandle(), group.GetHandle());
```

---

## Write a script

```cpp
class Bobber : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override { m_Base = GetComponent<TransformComponent>().position; }

    void OnUpdate(float dt) override
    {
        m_Time += dt;
        auto &t = GetComponent<TransformComponent>();
        t.position.y = m_Base.y + std::sin(m_Time * 2.0f) * 10.0f;
        t.dirty = true;
    }

private:
    glm::vec2 m_Base{0.0f};
    float m_Time = 0.0f;
};

entity.AddComponent<ScriptComponent>().Bind<Bobber>();
```

- Find other entities with `GetEntity().GetScene()->GetEntityByName("Player")`, or cache handles in `OnCreate`.
- Remove things with `registry.emplace_or_replace<LifetimeComponent>(handle, LifetimeComponent{0.0f})`, never by destroying inside `OnUpdate`.
- Script instances are not saved. Keep gameplay data in components or entity names so Play/Stop in the editor restores it.

---

## Move a character with your own collision

The engine gives you `CollisionComponent` boxes as data. A simple platformer collects them once and resolves movement per axis (this is what `Game/Mossveil/Scripts.h` does):

```cpp
struct Box { glm::vec2 Min, Max; bool Overlaps(const Box &o) const; };

void OnCreate() override
{
    auto &registry = GetEntity().GetScene()->GetRegistry();
    for (auto e : registry.view<TagComponent, TransformComponent, CollisionComponent>())
    {
        auto &t = registry.get<TransformComponent>(e);
        auto &c = registry.get<CollisionComponent>(e);
        glm::vec2 min = t.position + glm::vec2(c.offsetX, c.offsetY);
        if (registry.get<TagComponent>(e).Tag == "Solid")
            m_Solids.push_back({min, min + glm::vec2(c.width, c.height)});
    }
}
```

Then move X, push out of overlapping boxes, move Y, push out again and set `onGround` when you land on a box's top. Split large moves into steps of about 18 units so fast objects don't tunnel.

---

## Sprites, sprite sheets and animation

```cpp
auto texture = SIMPEngine::TextureManager::Get().LoadTexture("assets://hero.png", "assets://hero.png");
texture->SetVFSPath("assets://hero.png");
texture->SetFilter(SIMPEngine::TextureFilter::Nearest);   // pixel art; Linear for painted art

auto &sprite = entity.AddComponent<SpriteComponent>();
sprite.texture = texture;
sprite.width = 64.0f;
sprite.height = 64.0f;
```

**Animate by region** (full control, flip support): set `useRegion = true` and change `region` every frame to the current cell, `flipX` for facing.

**Animate with a component**:

```cpp
auto &anim = entity.AddComponent<AnimatedSpriteComponent>();
anim.animation = std::make_unique<SIMPEngine::Animation>(texture, 32, 32, 8, 0.1f, true);
```

**In the editor**: add *Animated Sprite*, press *Create Animation…*, pick the sheet. The Animation Maker detects frame size and count automatically; adjust and press *Build Animation*.

If your frames are separate PNG files, pack them into a grid first. `Game/Mossveil/tools/prepare_assets.py` shows how: trim every frame to a shared bounding box, scale, and paste into columns of ten.

---

## Build a tilemap

**Editor**: Edit → New Tilemap, drop an image or Tiled `.tsj` onto the Tileset slot, pick tiles in the Tiles panel, paint in the viewport (B brush, R rectangle, G fill, E erase, I picker, X/Y flip, right-drag erases, T toggles tile mode).

**Code**:

```cpp
auto &tm = entity.AddComponent<TilemapComponent>();
tm.tileset = std::make_shared<SIMPEngine::Tileset>(texture, 16, 16);
tm.tileSize = 32.0f;
tm.solid = true;
for (int x = 0; x < 40; ++x)
    SIMPEngine::SetTile(tm, x, 0, 1);
```

**Tiled**: in the FileSystem panel right-click a `.tmj` → *Import Tiled Map*, or call `TiledLoader::Import(path, scene)`. Export maps with CSV layer data.

---

## Add particles

**Editor**: Edit → New Particle Effect…, choose from the grid, then tune it in the inspector. *Save as Asset* writes `assets/particles/<Name>.particle`, which then appears under *Saved* in the browser.

**Code**:

```cpp
auto fx = scene.CreateEntity("Embers");
fx.GetComponent<TransformComponent>().position = {300.0f, 40.0f};
auto &emitter = fx.AddComponent<ParticleEmitterComponent>();
emitter.effect = SIMPEngine::ParticlePresets::Find("Embers")->Effect;
emitter.effect.BoxSize = {400.0f, 10.0f};
```

One-shot burst that cleans itself up:

```cpp
emitter.effect.Looping = false;
fx.AddComponent<LifetimeComponent>().remaining = 2.0f;
```

Set `Emissive = true` on effects that should glow in a dark, lit scene (fire, sparks, magic).

---

## Light a scene

```cpp
auto &lighting = scene.GetLighting();
lighting.Enabled = true;
lighting.Ambient = {0.25f, 0.3f, 0.35f, 1.0f};

auto torch = scene.CreateEntity("Torch");
auto &light = torch.AddComponent<LightComponent>();
light.color = {255, 190, 120, 255};
light.radius = 350.0f;
light.flicker = 0.25f;

auto shaft = scene.CreateEntity("Sunbeam");
shaft.GetComponent<TransformComponent>().rotation = -90.0f;   // point down
auto &spot = shaft.AddComponent<LightComponent>();
spot.type = LightType::Spot;
spot.radius = 900.0f;
spot.outerAngle = 15.0f;
```

In the editor: Edit → New Point Light / New Spot Light. The sun button in the viewport toolbar previews lighting.

Tips:
- Ambient between 0.25 and 0.45 keeps silhouettes readable.
- A light that follows the player (a child entity with a `LightComponent`) makes dark levels playable.
- The background clear colour is lit too; make it brighter than you want it to look.

---

## Make things glow

Add a `GlowComponent` to any visible entity:

```cpp
auto &glow = crystal.AddComponent<GlowComponent>();
glow.color = {120, 220, 255, 255};
glow.intensity = 1.5f;
glow.spread = 1.2f;
```

For a whole-screen effect enable bloom instead: `scene.GetBloom().Enabled = true;`.

---

## Parallax backgrounds

```cpp
auto layer = scene.CreateEntity("Far Hills");
layer.AddComponent<ParallaxComponent>().factor = {0.3f, 1.0f};
for (int i = 0; i < 10; ++i)
{
    auto hill = scene.CreateEntity("Hill");
    hill.GetComponent<TransformComponent>().position = {i * 600.0f, -100.0f};
    hill.GetComponent<TransformComponent>().zIndex = -50.0f;
    // add a SpriteComponent...
    scene.SetParent(hill.GetHandle(), layer.GetHandle());
}
```

An object at world `x` appears at `x + camera.x * (1 - factor)`. To cover a level of length `L`, spread background objects over `[-W, L * factor + W]`, where `W` is the view width.

---

## Follow the player with the camera

```cpp
auto &cc = camera.AddComponent<CameraComponent>(0.75f, spawn);
cc.primary = true;
cc.followTarget = "Player";
cc.followOffset = {0.0f, 200.0f};
cc.followSmoothing = 4.5f;
cc.deadZone = {110.0f, 80.0f};
cc.useBounds = true;
cc.boundsMin = {0.0f, -500.0f};
cc.boundsMax = {8000.0f, 1500.0f};
```

Turn off the debug fly controls in a script's `OnCreate`: `GetComponent<CameraComponent>().Camera.SetManualControl(false);`. Shake on impact with `cc.Shake(0.4f)`.

---

## Draw a HUD

Draw in a script's `OnRender`, positioned from the camera's visible bounds:

```cpp
void OnRender() override
{
    float left, right, top, bottom;
    camera.GetVisibleWorldBounds(left, right, top, bottom);
    SIMPEngine::Renderer::DrawText("Score 10", left + 30.0f, std::max(top, bottom) - 40.0f,
                                   28.0f, {255, 255, 255, 255}, SIMPEngine::TextAlign::Left, 0.0f, 995.0f, m_Font.get());
}
```

With scene lighting on, put a bright point light behind the HUD text (Mossveil moves a "HUD Light" entity to the corner every frame).

---

## Play sounds

```cpp
SIMPEngine::AudioEngine::Play("assets://sfx/jump.wav", 0.8f);

auto music = scene.CreateEntity("Music");
auto &source = music.AddComponent<AudioSourceComponent>();
source.clip = "assets://music/cave.wav";
source.loop = true;
source.volume = 0.5f;
```

Sources start on the first update in Play mode and stop when the entity is destroyed or Play stops.

---

## Switch levels

Rebuild the scene from a layer, never from inside a script:

```cpp
struct GameState { int PendingLevel = 0; };
GameState &State();   // a static instance

class LevelLayer : public SIMPEngine::Layer
{
public:
    explicit LevelLayer(SIMPEngine::Scene *scene) : m_Scene(scene) {}
    void OnUpdate(SIMPEngine::TimeStep) override
    {
        if (int level = State().PendingLevel)
        {
            State().PendingLevel = 0;
            m_Scene->Clear();
            BuildLevel(*m_Scene, level);
        }
    }
private:
    SIMPEngine::Scene *m_Scene;
};
```

Push it before the `RenderingLayer`. Scripts set `State().PendingLevel = 2;`. Store the current level in an entity name (Mossveil uses `Director <level> <seed>`) and read it in a script's `OnCreate`, so editor Play/Stop snapshots stay consistent.

---

## Generate a level procedurally

Use a seeded generator and build the level with the same helpers as hand-made levels (`Game/Mossveil/Levels.cpp`, `BuildShiftingDeep`):

1. Seed `std::mt19937` from a number you can show the player and reuse.
2. Walk from left to right, choosing a segment type with weights: gap, floating platforms, hazard, wide gap, cave.
3. Keep every step inside what the player can do. Measure your jump first: apex `v² / 2g`, horizontal reach `airTime × runSpeed`. Then clamp gaps and height changes below those.
4. Clamp the ground height to a band so the camera bounds stay sensible.
5. Decorate each segment (plants, lights, particles) from the same random stream so the seed reproduces the whole look.
6. Add checkpoints every few segments and the exit on a final safe stretch, then compute camera bounds from the lowest and highest ground.

---

## Save and load scenes

```cpp
SIMPEngine::SceneSerializer serializer(&scene);
serializer.Serialize("assets://scenes/MyLevel.yaml");
serializer.Deserialize("assets://scenes/MyLevel.yaml");
```

The editor's Scene menu does the same (Ctrl+S, Ctrl+O). Rebind scripts after loading in code.

---

## Use the editor

| Action | How |
|---|---|
| Play / Pause / Step / Stop | F5 / F6 / F7 / F5, or the toolbar |
| Move / rotate / scale | W / E / R or the toolbar; hold Ctrl to invert snapping |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| Frame selection | F |
| Rename / duplicate / delete | F2 / Ctrl+D / Del |
| Group / ungroup in the Scene panel | Ctrl+G / Ctrl+Shift+G |
| Show only one kind of entity | Scene panel → filter button |
| Collapse or expand every group | Scene panel toolbar |
| Lighting preview / collider overlay | Sun and box buttons in the viewport toolbar |

---

## Keep it fast

- Large transparent particles are the most expensive thing you can draw. Every visible pixel of every particle is shaded and blended. Prefer a few big, denser particles over many faint ones, and cap `MaxParticles`.
- Off-screen sprites and particles are culled for free; fill on screen is what costs.
- Screen bloom costs a few full-screen passes. Use Glow components on the few things that should shine instead.
- Prewarm runs the simulation at load time; keep `Lifetime` reasonable on prewarmed effects.
- Use the editor's Profiler (View → Profiler) and the stats overlay (draw calls, primitives) to check changes.

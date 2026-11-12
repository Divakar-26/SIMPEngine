#include "TestFramework.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/TilemapUtils.h>

#include <AccelEngine/body.h>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <utility>
#include <string>

using namespace SIMPEngine;

namespace
{
    void FillScene(Scene &scene)
    {
        scene.SetBackgroundColor({0.1f, 0.2f, 0.3f, 1.0f});
        scene.GetLighting().Enabled = true;
        scene.GetLighting().Ambient = {0.2f, 0.25f, 0.3f, 1.0f};
        scene.GetBloom().Enabled = true;
        scene.GetBloom().Intensity = 1.25f;

        Entity e = scene.CreateEntity("Everything");
        auto &t = e.GetComponent<TransformComponent>();
        t.position = {12.5f, -40.0f};
        t.rotation = 30.0f;
        t.scale = {2.0f, 0.5f};
        t.zIndex = 3.0f;

        auto &collision = e.AddComponent<CollisionComponent>();
        collision.width = 40.0f;
        collision.height = 20.0f;
        collision.offsetX = -20.0f;
        collision.offsetY = -10.0f;

        auto &sprite = e.AddComponent<SpriteComponent>();
        sprite.width = 64.0f;
        sprite.height = 32.0f;
        sprite.tint = SDL_Color{200, 100, 50, 255};
        sprite.flipX = true;
        sprite.useRegion = true;
        sprite.region = SDL_FRect{16.0f, 0.0f, 16.0f, 16.0f};

        auto &text = e.AddComponent<TextComponent>();
        text.Text = "Hello";
        text.Size = 40.0f;
        text.Align = 2;

        auto &render = e.AddComponent<RenderComponent>();
        render.width = 10.0f;
        render.height = 12.0f;
        render.color = SDL_Color{1, 2, 3, 4};

        auto &velocity = e.AddComponent<VelocityComponent>();
        velocity.vx = 3.0f;
        velocity.vy = -4.0f;

        e.AddComponent<ScriptComponent>().BindLua("assets://scripts/player.lua");

        e.AddComponent<LifetimeComponent>().remaining = 7.5f;

        auto &tilemap = e.AddComponent<TilemapComponent>();
        tilemap.tileSize = 24.0f;
        tilemap.solid = true;
        SetTile(tilemap, 0, 0, 5);
        SetTile(tilemap, -3, 17, 9, TileFlipX | TileFlipY);

        auto &particles = e.AddComponent<ParticleEmitterComponent>();
        particles.effect.Rate = 55.0f;
        particles.effect.Burst = 12;
        particles.effect.Direction = 45.0f;

        auto &light = e.AddComponent<LightComponent>();
        light.type = LightType::Spot;
        light.radius = 300.0f;
        light.flicker = 0.3f;

        e.AddComponent<ParallaxComponent>().factor = {0.25f, 0.75f};

        auto &audio = e.AddComponent<AudioSourceComponent>();
        audio.clip = "assets://audio/none.wav";
        audio.volume = 0.5f;
        audio.loop = true;
        audio.playOnStart = false;

        auto &glow = e.AddComponent<GlowComponent>();
        glow.intensity = 2.5f;
        glow.spread = 1.5f;

        auto &trigger = e.AddComponent<TriggerComponent>();
        trigger.width = 80.0f;
        trigger.filter = "Player";

        auto &timer = e.AddComponent<TimerComponent>();
        timer.interval = 0.25f;
        timer.repeat = false;

        auto &path = e.AddComponent<PathFollowerComponent>();
        path.points = {{0.0f, 0.0f}, {100.0f, 50.0f}};
        path.mode = PathMode::PingPong;
        path.waitTime = 0.5f;

        auto &health = e.AddComponent<HealthComponent>();
        health.max = 50.0f;
        health.current = 20.0f;
        health.destroyOnDeath = true;

        auto &vector = e.AddComponent<VectorShapeComponent>();
        vector.path = VectorPath::Star(5, 40.0f, 18.0f);
        vector.morphTarget = VectorPath::Circle(30.0f);
        vector.morph = 0.4f;
        vector.stroke = true;
        vector.strokeWidth = 6.0f;
        vector.trimEnd = 0.8f;
        vector.dashLength = 8.0f;
        vector.dashGap = 4.0f;
        vector.gradient = true;

        auto &controller = e.AddComponent<CharacterControllerComponent>();
        controller.mode = ControllerMode::TopDown;
        controller.maxJumps = 3;
        controller.solidTag = "Ground";
        controller.playerInput = false;

        auto &damage = e.AddComponent<DamageComponent>();
        damage.amount = -15.0f;
        damage.interval = 0.0f;
        damage.filter = "Player";
        damage.knockback = 320.0f;
        damage.destroyOnHit = true;

        auto &osc = e.AddComponent<OscillatorComponent>();
        osc.move = {4.0f, 12.0f};
        osc.frequency = 2.0f;
        osc.phase = 0.25f;
        osc.swing = 10.0f;
        osc.spin = 90.0f;
        osc.pulse = 0.1f;

        auto &follow = e.AddComponent<FollowComponent>();
        follow.target = "Hero";
        follow.offset = {5.0f, 9.0f};
        follow.smoothing = 3.0f;
        follow.stopDistance = 40.0f;
        follow.followY = false;

        auto &button = e.AddComponent<ButtonComponent>();
        button.button = 2;
        button.hoverScale = 1.2f;
        button.pressScale = 0.8f;
        button.clickSound = "assets://audio/click.wav";
        button.padding = 4.0f;

        e.AddComponent<PrefabInstanceComponent>().path = "assets://prefabs/Thing.prefab";

        auto &canvas = e.AddComponent<CanvasComponent>();
        canvas.referenceSize = {1920.0f, 1080.0f};
        canvas.scaleMode = 0;
        canvas.match = 0.25f;
        canvas.safeArea = 0.05f;
        canvas.sortOrder = 3;

        auto &uiRect = e.AddComponent<UIRectComponent>();
        uiRect.anchorMin = {0.0f, 1.0f};
        uiRect.anchorMax = {1.0f, 1.0f};
        uiRect.pivot = {0.0f, 1.0f};
        uiRect.position = {4.0f, -6.0f};
        uiRect.size = {200.0f, 60.0f};
        uiRect.margin = {1.0f, 2.0f, 3.0f, 4.0f};
        uiRect.blocksInput = false;

        auto &uiPanel = e.AddComponent<UIPanelComponent>();
        uiPanel.color = {10, 20, 30, 40};
        uiPanel.cornerRadius = 12.0f;
        uiPanel.borderWidth = 2.0f;

        auto &uiLabel = e.AddComponent<UILabelComponent>();
        uiLabel.text = "Coins {coins}";
        uiLabel.size = 30.0f;
        uiLabel.align = 2;
        uiLabel.verticalAlign = 0;
        uiLabel.shadow = true;

        auto &uiImage = e.AddComponent<UIImageComponent>();
        uiImage.preserveAspect = false;
        uiImage.useRegion = true;
        uiImage.region = {1.0f, 2.0f, 3.0f, 4.0f};

        auto &uiButton = e.AddComponent<UIButtonComponent>();
        uiButton.focusable = false;
        uiButton.hoverScale = 1.1f;
        uiButton.clickSound = "assets://audio/click.wav";

        auto &uiLayout = e.AddComponent<UILayoutComponent>();
        uiLayout.direction = 0;
        uiLayout.spacing = 14.0f;
        uiLayout.padding = {1.0f, 2.0f, 3.0f, 4.0f};
        uiLayout.justify = 1;
        uiLayout.stretch = true;

        auto &portal = e.AddComponent<TeleporterComponent>();
        portal.target = "Exit";
        portal.offset = {0.0f, 12.0f};
        portal.keepVelocity = false;

        auto &zone2 = e.AddComponent<CameraZoneComponent>();
        zone2.zoom = 0.6f;
        zone2.speed = 2.0f;
        zone2.setBounds = true;
        zone2.boundsMin = {-100.0f, -200.0f};
        zone2.boundsMax = {300.0f, 400.0f};
        zone2.restoreOnExit = false;

        auto &limits = e.AddComponent<WorldBoundsComponent>();
        limits.min = {-50.0f, -60.0f};
        limits.max = {70.0f, 80.0f};
        limits.mode = BoundsMode::Bounce;
        limits.margin = 8.0f;

        auto &pickable = e.AddComponent<PickableComponent>();
        pickable.button = 1;
        pickable.smoothing = 7.0f;
        pickable.throwStrength = 0.5f;
        pickable.liftScale = 1.3f;
        pickable.bringToFront = false;
        pickable.keepOffset = false;
        pickable.padding = 6.0f;

        auto &pickup = e.AddComponent<PickupComponent>();
        pickup.key = "coins";
        pickup.amount = 5.0f;
        pickup.sound = "assets://audio/coin.ogg";
        pickup.destroyOnPickup = false;

        auto &checkpoint = e.AddComponent<CheckpointComponent>();
        checkpoint.offset = {0.0f, 30.0f};
        checkpoint.sound = "assets://audio/flag.wav";

        auto &respawn = e.AddComponent<RespawnComponent>();
        respawn.delay = 2.5f;
        respawn.useKillY = true;
        respawn.killY = -750.0f;
        respawn.restoreHealth = false;

        auto &music = e.AddComponent<MusicComponent>();
        music.tracks = {"assets://music/a.ogg", "assets://music/b.ogg"};
        music.volume = 0.6f;
        music.fadeIn = 2.0f;
        music.fadeOut = 0.5f;
        music.shuffle = true;
        music.loopPlaylist = false;
        music.playOnStart = false;
        music.onTrigger = true;
        music.stopOnExit = true;
        audio.bus = 1;

        auto &spawner = e.AddComponent<SpawnerComponent>();
        spawner.prototype = "Bat";
        spawner.interval = 2.5f;
        spawner.perSpawn = 3;
        spawner.maxAlive = 7;
        spawner.totalLimit = 20;
        spawner.offset = {10.0f, -5.0f};
        spawner.spread = {30.0f, 0.0f};
        spawner.velocity = {0.0f, 120.0f};
        spawner.lifetime = 4.0f;
        spawner.hidePrototype = false;
        spawner.spawnOnStart = true;

        Entity camera = scene.CreateEntity("Camera");
        auto &cam = camera.AddComponent<CameraComponent>(1.5f, glm::vec2(0.0f, 0.0f));
        cam.primary = true;
        cam.followTarget = "Everything";
        cam.followSmoothing = 3.0f;
        cam.deadZone = {20.0f, 10.0f};
        cam.useBounds = true;
        cam.boundsMin = {-500.0f, -300.0f};
        cam.boundsMax = {500.0f, 300.0f};

        Entity child = scene.CreateEntity("Child");
        child.GetComponent<TransformComponent>().position = {5.0f, 5.0f};
        scene.SetParent(child.GetHandle(), e.GetHandle());

        Entity body = scene.CreateEntity("Body");
        auto rigid = std::make_unique<AccelEngine::RigidBody>();
        rigid->shapeType = AccelEngine::ShapeType::AABB;
        rigid->aabb.halfSize = {16.0f, 8.0f};
        rigid->inverseMass = 0.5f;
        rigid->restitution = 0.2f;
        body.AddComponent<PhysicsComponent>().body = std::move(rigid);
    }
}

TEST_CASE("serializer: save, load, save gives identical YAML")
{
    Scene original("Round Trip");
    FillScene(original);
    const std::string first = SceneSerializer(&original).SerializeToString();

    for (const char *key : {"Transform", "Collision", "Sprite", "Text", "Render", "Velocity", "Camera", "Script", "Lifetime",
                            "Tilemap", "ParticleEmitter", "Light", "Parallax", "AudioSource", "Glow", "Trigger", "Timer",
                            "PathFollower", "Health", "VectorShape", "CharacterController", "Damage", "Oscillator", "Spawner", "Follow", "Button", "Teleporter", "CameraZone", "WorldBounds", "Pickable", "Pickup", "Checkpoint", "Respawn", "Music", "bus", "Hierarchy", "Physics", "lighting", "bloom", "Version"})
    {
        if (first.find(key) == std::string::npos)
            SIMPTest::Fail(__FILE__, __LINE__, std::string("first save is missing ") + key);
    }

    Scene loaded("Loaded");
    CHECK(SceneSerializer(&loaded).DeserializeFromString(first));
    const std::string second = SceneSerializer(&loaded).SerializeToString();
    CHECK(first == second);
    if (first != second)
    {
        size_t at = 0;
        while (at < first.size() && at < second.size() && first[at] == second[at])
            ++at;
        const size_t from = at > 80 ? at - 80 : 0;
        std::printf("    first differs at %zu:\n----\n%s\n----\n%s\n----\n", at, first.substr(from, 200).c_str(), second.substr(from, 200).c_str());
    }
}

TEST_CASE("serializer: loaded values match what was saved")
{
    Scene original("Values");
    FillScene(original);
    Scene loaded("Loaded");
    CHECK(SceneSerializer(&loaded).DeserializeFromString(SceneSerializer(&original).SerializeToString()));

    auto &registry = loaded.GetRegistry();
    int checked = 0;
    for (auto entity : registry.view<TagComponent>())
    {
        if (registry.get<TagComponent>(entity).Tag != "Everything")
            continue;
        ++checked;
        const auto &t = registry.get<TransformComponent>(entity);
        CHECK_NEAR(t.position.x, 12.5f, 1e-4);
        CHECK_NEAR(t.rotation, 30.0f, 1e-4);
        CHECK_NEAR(t.zIndex, 3.0f, 1e-4);
        CHECK(registry.get<SpriteComponent>(entity).flipX);
        CHECK(registry.get<ScriptComponent>(entity).LuaPath == "assets://scripts/player.lua");
        auto &tilemap = registry.get<TilemapComponent>(entity);
        CHECK(GetTileId(tilemap, 0, 0) == 5);
        CHECK(GetTileId(tilemap, -3, 17) == 9);
        CHECK(registry.get<LightComponent>(entity).type == LightType::Spot);
        CHECK(registry.get<PathFollowerComponent>(entity).mode == PathMode::PingPong);
        CHECK_NEAR(registry.get<HealthComponent>(entity).current, 20.0f, 1e-4);
        CHECK(registry.get<CharacterControllerComponent>(entity).mode == ControllerMode::TopDown);
        CHECK(registry.get<CharacterControllerComponent>(entity).maxJumps == 3);
        CHECK_NEAR(registry.get<DamageComponent>(entity).amount, -15.0f, 1e-4);
        CHECK(registry.get<DamageComponent>(entity).filter == "Player");
        CHECK(registry.get<DamageComponent>(entity).destroyOnHit);
        CHECK_NEAR(registry.get<OscillatorComponent>(entity).move.y, 12.0f, 1e-4);
        CHECK_NEAR(registry.get<OscillatorComponent>(entity).spin, 90.0f, 1e-4);
        CHECK(registry.get<SpawnerComponent>(entity).prototype == "Bat");
        CHECK(registry.get<SpawnerComponent>(entity).perSpawn == 3);
        CHECK_NEAR(registry.get<SpawnerComponent>(entity).velocity.y, 120.0f, 1e-4);
        CHECK(!registry.get<SpawnerComponent>(entity).hidePrototype);
        CHECK(registry.get<FollowComponent>(entity).target == "Hero");
        CHECK(!registry.get<FollowComponent>(entity).followY);
        CHECK_NEAR(registry.get<FollowComponent>(entity).stopDistance, 40.0f, 1e-4);
        CHECK(registry.get<ButtonComponent>(entity).button == 2);
        CHECK_NEAR(registry.get<ButtonComponent>(entity).hoverScale, 1.2f, 1e-4);
        CHECK(registry.get<PrefabInstanceComponent>(entity).path == "assets://prefabs/Thing.prefab");
        CHECK_NEAR(registry.get<CanvasComponent>(entity).referenceSize.x, 1920.0f, 1e-4);
        CHECK(registry.get<CanvasComponent>(entity).scaleMode == 0);
        CHECK(registry.get<CanvasComponent>(entity).sortOrder == 3);
        CHECK_NEAR(registry.get<UIRectComponent>(entity).margin.w, 4.0f, 1e-4);
        CHECK_NEAR(registry.get<UIRectComponent>(entity).anchorMin.y, 1.0f, 1e-4);
        CHECK(!registry.get<UIRectComponent>(entity).blocksInput);
        CHECK_NEAR(registry.get<UIPanelComponent>(entity).cornerRadius, 12.0f, 1e-4);
        CHECK(registry.get<UIPanelComponent>(entity).color.b == 30);
        CHECK(registry.get<UILabelComponent>(entity).text == "Coins {coins}");
        CHECK(registry.get<UILabelComponent>(entity).align == 2);
        CHECK(registry.get<UILabelComponent>(entity).shadow);
        CHECK(!registry.get<UIImageComponent>(entity).preserveAspect);
        CHECK_NEAR(registry.get<UIImageComponent>(entity).region.h, 4.0f, 1e-4);
        CHECK(!registry.get<UIButtonComponent>(entity).focusable);
        CHECK(registry.get<UIButtonComponent>(entity).clickSound == "assets://audio/click.wav");
        CHECK(registry.get<UILayoutComponent>(entity).direction == 0);
        CHECK_NEAR(registry.get<UILayoutComponent>(entity).spacing, 14.0f, 1e-4);
        CHECK(registry.get<UILayoutComponent>(entity).stretch);
        CHECK(registry.get<TeleporterComponent>(entity).target == "Exit");
        CHECK(!registry.get<TeleporterComponent>(entity).keepVelocity);
        CHECK_NEAR(registry.get<CameraZoneComponent>(entity).boundsMax.y, 400.0f, 1e-4);
        CHECK(!registry.get<CameraZoneComponent>(entity).restoreOnExit);
        CHECK(registry.get<WorldBoundsComponent>(entity).mode == BoundsMode::Bounce);
        CHECK_NEAR(registry.get<WorldBoundsComponent>(entity).margin, 8.0f, 1e-4);
        CHECK(registry.get<PickableComponent>(entity).button == 1);
        CHECK(!registry.get<PickableComponent>(entity).keepOffset);
        CHECK_NEAR(registry.get<PickableComponent>(entity).throwStrength, 0.5f, 1e-4);
        CHECK(registry.get<PickupComponent>(entity).key == "coins");
        CHECK(!registry.get<PickupComponent>(entity).destroyOnPickup);
        CHECK_NEAR(registry.get<CheckpointComponent>(entity).offset.y, 30.0f, 1e-4);
        CHECK_NEAR(registry.get<RespawnComponent>(entity).killY, -750.0f, 1e-4);
        CHECK(!registry.get<RespawnComponent>(entity).restoreHealth);
        CHECK(registry.get<MusicComponent>(entity).tracks.size() == 2);
        CHECK(registry.get<MusicComponent>(entity).tracks[1] == "assets://music/b.ogg");
        CHECK(registry.get<MusicComponent>(entity).stopOnExit);
        CHECK(registry.get<AudioSourceComponent>(entity).bus == 1);
        CHECK(registry.get<HierarchyComponent>(entity).children.size() == 1);
    }
    CHECK(checked == 1);
    CHECK(loaded.GetLighting().Enabled);
    CHECK_NEAR(loaded.GetBloom().Intensity, 1.25f, 1e-4);
}

TEST_CASE("serializer: malformed scenes fail cleanly")
{
    Scene scene("Bad");
    SceneSerializer serializer(&scene);
    CHECK(!serializer.DeserializeFromString("this: [is not: valid"));
    CHECK(!serializer.DeserializeFromString("just a string"));
    CHECK(!serializer.DeserializeFromString("Entities: []"));
    CHECK(!serializer.DeserializeFromString("Scene: x\nEntities: 5"));
    CHECK(!serializer.DeserializeFromString("Scene: x\nSettings:\n  background: [a, b, c, d]"));

    const bool loaded = serializer.DeserializeFromString(
        "Scene: Partial\nEntities:\n"
        "  - ID: 1\n    Tag: Good\n    Transform: {position: [1, 2], rotation: 0, scale: [1, 1]}\n"
        "  - ID: 2\n    Tag: BadComponent\n    Light: {radius: not-a-number}\n");
    CHECK(loaded);
    int count = 0;
    for (auto entity : scene.GetRegistry().view<TagComponent>())
    {
        (void)entity;
        ++count;
    }
    CHECK(count == 2);
}

TEST_CASE("serializer: newer format versions still load")
{
    Scene scene("Future");
    CHECK(SceneSerializer(&scene).DeserializeFromString("Scene: Future\nVersion: 99\nEntities: []\n"));
}

TEST_CASE("serializer: truncated and corrupted scenes never crash the loader")
{
    Scene original("Fuzz");
    FillScene(original);
    const std::string yaml = SceneSerializer(&original).SerializeToString();

    uint32_t state = 12345u;
    auto next = [&state]
    {
        state = state * 1664525u + 1013904223u;
        return state >> 8;
    };
    const char noise[] = "-:[]{}#&*!|>'\"%@` \n\t0123456789.eE+nan";

    for (int i = 0; i < 400; ++i)
    {
        std::string mutated = yaml;
        if (i % 4 == 0)
            mutated.resize(next() % mutated.size());
        const int edits = 1 + static_cast<int>(next() % 6);
        for (int e = 0; e < edits && !mutated.empty(); ++e)
            mutated[next() % mutated.size()] = noise[next() % (sizeof(noise) - 1)];

        Scene scene("Mutated");
        SceneSerializer(&scene).DeserializeFromString(mutated);
        scene.OnUpdate(1.0f / 60.0f);
        SceneSerializer(&scene).SerializeToString();
    }
    CHECK(true);
}

TEST_CASE("Scene load requests run between updates and fall back on failure")
{
    using namespace SIMPEngine;
    const std::string dir = std::filesystem::temp_directory_path().string();
    const std::string fileA = dir + "/simp_load_a.yaml";
    const std::string fileB = dir + "/simp_load_b.yaml";

    Scene a("a");
    a.BuildEntity("OnlyA").Build();
    CHECK(SceneSerializer(&a).Serialize(fileA));
    Scene b("b");
    b.BuildEntity("B1").Build();
    b.BuildEntity("B2").Build();
    CHECK(SceneSerializer(&b).Serialize(fileB));

    Scene scene("runtime");
    CHECK(SceneSerializer(&scene).Deserialize(fileA));
    CHECK(scene.GetSourcePath() == fileA);
    CHECK(scene.GetRegistry().view<TagComponent>().size() == 1);

    scene.RequestLoad(fileB);
    CHECK(scene.IsTransitioning());
    CHECK(scene.GetSourcePath() == fileA);
    scene.OnUpdate(0.016f);
    CHECK(scene.GetSourcePath() == fileB);
    CHECK(scene.GetRegistry().view<TagComponent>().size() == 2);
    CHECK(!scene.IsTransitioning());

    scene.RequestLoad(fileA, 0.2f);
    scene.OnUpdate(0.016f);
    CHECK(scene.GetSourcePath() == fileB);
    scene.OnUpdate(0.1f);
    CHECK(scene.GetSourcePath() == fileB);
    CHECK_NEAR(scene.GetFadeAlpha(), 0.5f, 0.01f);
    scene.OnUpdate(0.15f);
    CHECK(scene.GetSourcePath() == fileA);
    scene.OnUpdate(0.3f);
    CHECK(!scene.IsTransitioning());

    scene.RequestLoad(dir + "/simp_missing_scene.yaml");
    scene.OnUpdate(0.016f);
    CHECK(scene.GetSourcePath() == fileA);

    scene.RequestReload();
    scene.OnUpdate(0.016f);
    CHECK(scene.GetSourcePath() == fileA);

    std::filesystem::remove(fileA);
    std::filesystem::remove(fileB);
}

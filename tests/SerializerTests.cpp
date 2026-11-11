#include "TestFramework.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/TilemapUtils.h>

#include <AccelEngine/body.h>

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
                            "PathFollower", "Health", "VectorShape", "CharacterController", "Damage", "Oscillator", "Spawner", "Hierarchy", "Physics", "lighting", "bloom", "Version"})
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

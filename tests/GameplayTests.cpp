#include "TestFramework.h"

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/Tween.h>
#include <Engine/Core/GameVariables.h>
#include <Engine/Core/Version.h>
#include <Engine/Input/Input.h>
#include <Engine/Project/Project.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <random>
#include <string>

using namespace SIMPEngine;
namespace fs = std::filesystem;

namespace
{
    void Step(Scene &scene, int frames)
    {
        for (int i = 0; i < frames; ++i)
            scene.OnUpdate(1.0f / 60.0f);
    }

    Entity Ground(Scene &scene, glm::vec2 min, glm::vec2 size, const char *name = "Ground")
    {
        Entity e = scene.CreateEntity(name);
        e.GetComponent<TransformComponent>().position = min;
        auto &collision = e.AddComponent<CollisionComponent>();
        collision.width = size.x;
        collision.height = size.y;
        return e;
    }

    fs::path FreshDirectory(const char *name)
    {
        const fs::path dir = fs::temp_directory_path() / name;
        std::error_code ec;
        fs::remove_all(dir, ec);
        return dir;
    }
}

TEST_CASE("tween: value tween hits the target on time")
{
    Scene scene("Tweens");
    float value = 0.0f;
    bool done = false;
    TweenSystem &tweens = scene.GetTweens();
    const uint32_t id = tweens.Value(10.0f, 20.0f, 0.5f, Curve::Linear(), [&](float v) { value = v; });
    const uint32_t call = tweens.Call([&] { done = true; });
    tweens.Group(true, {id, call});
    for (int i = 0; i < 15; ++i)
        tweens.Update(1.0f / 60.0f);
    CHECK_NEAR(value, 15.0f, 0.2);
    CHECK(!done);
    for (int i = 0; i < 20; ++i)
        tweens.Update(1.0f / 60.0f);
    CHECK_NEAR(value, 20.0f, 1e-4);
    CHECK(done);
}

TEST_CASE("tween: a new tween on the same property replaces the old one")
{
    Scene scene("Tweens");
    Entity e = scene.CreateEntity("Box");
    TweenSystem &tweens = scene.GetTweens();
    TweenTrack track;
    track.Property = TweenProperty::X;
    track.To[0] = 100.0f;
    tweens.To(e.GetHandle(), {track}, 1.0f, Curve::Linear());
    tweens.Update(0.5f);
    track.To[0] = -50.0f;
    tweens.To(e.GetHandle(), {track}, 0.5f, Curve::Linear());
    for (int i = 0; i < 60; ++i)
        tweens.Update(1.0f / 60.0f);
    CHECK_NEAR(e.GetComponent<TransformComponent>().position.x, -50.0f, 1e-3);
}

TEST_CASE("character controller: lands, stops at walls, jumps to the right height")
{
    Scene scene("Controller");
    Ground(scene, {-500.0f, -20.0f}, {1000.0f, 20.0f});
    Ground(scene, {200.0f, 0.0f}, {20.0f, 400.0f}, "Wall");

    Entity player = scene.CreateEntity("Player");
    player.GetComponent<TransformComponent>().position = {0.0f, 200.0f};
    auto &c = player.AddComponent<CharacterControllerComponent>();
    c.playerInput = false;
    auto position = [&] { return player.GetComponent<TransformComponent>().position; };

    Step(scene, 90);
    CHECK(c.grounded);
    CHECK_NEAR(position().y, 24.0f, 0.2);

    c.scriptInput = {1.0f, 0.0f};
    Step(scene, 120);
    CHECK_NEAR(position().x, 184.0f, 0.3);
    CHECK(c.facing == 1);

    c.scriptInput = {0.0f, 0.0f};
    c.scriptJump = true;
    float peak = position().y;
    for (int i = 0; i < 120; ++i)
    {
        Step(scene, 1);
        peak = std::max(peak, position().y);
    }
    CHECK_NEAR(peak, 24.0f + c.jumpSpeed * c.jumpSpeed / (2.0f * c.gravity), 15.0);
    CHECK(c.grounded);
}

TEST_CASE("character controller: one-way platforms only block from above")
{
    Scene scene("OneWay");
    Ground(scene, {-500.0f, -20.0f}, {1000.0f, 20.0f});
    Ground(scene, {-300.0f, 100.0f}, {100.0f, 10.0f}, "OneWay");

    Entity player = scene.CreateEntity("Player");
    player.GetComponent<TransformComponent>().position = {-250.0f, 30.0f};
    auto &c = player.AddComponent<CharacterControllerComponent>();
    c.playerInput = false;
    Step(scene, 30);
    c.scriptJump = true;
    float top = 0.0f;
    for (int i = 0; i < 60; ++i)
    {
        Step(scene, 1);
        top = std::max(top, player.GetComponent<TransformComponent>().position.y);
    }
    Step(scene, 60);
    CHECK(top > 140.0f);
    CHECK_NEAR(player.GetComponent<TransformComponent>().position.y, 134.0f, 0.3);
}

TEST_CASE("project: create, load and save keep every setting")
{
    const fs::path dir = FreshDirectory("simp-test-project");
    ProjectSettings settings;
    settings.Name = "Test Game";
    settings.WindowWidth = 800;
    settings.WindowHeight = 600;
    std::string error;
    CHECK(Project::Create(dir, settings, ProjectTemplate::Platformer, &error));
    CHECK(error.empty());
    CHECK(fs::is_regular_file(dir / Project::FileName));
    CHECK(fs::is_regular_file(dir / "assets/scenes/Main.yaml"));
    CHECK(fs::is_regular_file(dir / "assets/scripts/player.lua"));

    ProjectSettings loaded;
    CHECK(Project::Load(dir, loaded, &error));
    CHECK(loaded.Name == "Test Game");
    CHECK(loaded.WindowWidth == 800);
    CHECK(loaded.EngineVersion == EngineVersion);
    CHECK(loaded.StartScene == "assets://scenes/Main.yaml");

    loaded.Fullscreen = true;
    loaded.WindowTitle = "Custom";
    CHECK(Project::Save(dir, loaded, &error));
    ProjectSettings again;
    CHECK(Project::Load(dir / Project::FileName, again, &error));
    CHECK(again.Fullscreen);
    CHECK(again.Title() == "Custom");

    CHECK(!Project::Create(dir, settings, ProjectTemplate::Empty, &error));
    CHECK(!error.empty());

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("project: template scenes load and contain a primary camera")
{
    for (ProjectTemplate kind : {ProjectTemplate::Empty, ProjectTemplate::Platformer})
    {
        const fs::path dir = FreshDirectory("simp-test-template");
        CHECK(Project::Create(dir, ProjectSettings{}, kind));
        Scene scene("Template");
        CHECK(SceneSerializer(&scene).Deserialize((dir / "assets/scenes/Main.yaml").string()));
        int cameras = 0;
        for (auto entity : scene.GetRegistry().view<CameraComponent>())
            cameras += scene.GetRegistry().get<CameraComponent>(entity).primary ? 1 : 0;
        CHECK(cameras == 1);
        if (kind == ProjectTemplate::Platformer)
        {
            int controllers = 0;
            for (auto entity : scene.GetRegistry().view<CharacterControllerComponent>())
            {
                (void)entity;
                ++controllers;
            }
            CHECK(controllers == 1);
        }
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
}

TEST_CASE("project: broken project files report an error")
{
    const fs::path dir = FreshDirectory("simp-test-broken");
    fs::create_directories(dir);
    std::string error;
    ProjectSettings settings;
    CHECK(!Project::Load(dir, settings, &error));
    CHECK(!error.empty());

    std::ofstream(dir / Project::FileName) << "Project: [unclosed";
    error.clear();
    CHECK(!Project::Load(dir, settings, &error));
    CHECK(!error.empty());

    std::ofstream(dir / Project::FileName) << "Something: else\n";
    error.clear();
    CHECK(!Project::Load(dir, settings, &error));

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST_CASE("trigger: detects character controllers without a collision box")
{
    Scene scene("Trigger");
    Entity zone = scene.CreateEntity("Zone");
    zone.GetComponent<TransformComponent>().position = {-50.0f, -50.0f};
    auto &trigger = zone.AddComponent<TriggerComponent>();
    trigger.width = 100.0f;
    trigger.height = 100.0f;

    Entity player = scene.CreateEntity("Player");
    player.GetComponent<TransformComponent>().position = {0.0f, 0.0f};
    auto &controller = player.AddComponent<CharacterControllerComponent>();
    controller.playerInput = false;
    controller.mode = ControllerMode::TopDown;

    Step(scene, 1);
    CHECK(trigger.inside.size() == 1);

    player.GetComponent<TransformComponent>().position = {500.0f, 0.0f};
    Step(scene, 1);
    CHECK(trigger.inside.empty());
}

TEST_CASE("damage: hurts on an interval, heals with negative amounts, can be consumed")
{
    Scene scene("Damage");
    Entity player = scene.CreateEntity("Player");
    auto &controller = player.AddComponent<CharacterControllerComponent>();
    controller.playerInput = false;
    controller.mode = ControllerMode::TopDown;
    auto &health = player.AddComponent<HealthComponent>();
    health.max = 100.0f;
    health.current = 100.0f;

    Entity spikes = scene.CreateEntity("Spikes");
    spikes.GetComponent<TransformComponent>().position = {-40.0f, -40.0f};
    auto &zone = spikes.AddComponent<TriggerComponent>();
    zone.width = 80.0f;
    zone.height = 80.0f;
    auto &damage = spikes.AddComponent<DamageComponent>();
    damage.amount = 10.0f;
    damage.interval = 0.5f;
    damage.filter = "Player";

    Step(scene, 1);
    CHECK_NEAR(health.current, 90.0f, 1e-3);
    Step(scene, 20);
    CHECK_NEAR(health.current, 90.0f, 1e-3);
    Step(scene, 15);
    CHECK_NEAR(health.current, 80.0f, 1e-3);

    damage.amount = -50.0f;
    damage.interval = 0.0f;
    damage.destroyOnHit = true;
    damage.cooldowns.clear();
    Step(scene, 3);
    CHECK_NEAR(health.current, 100.0f, 1e-3);
    CHECK(!scene.GetRegistry().valid(spikes.GetHandle()));
}

TEST_CASE("oscillator: moves around its start and returns to it every cycle")
{
    Scene scene("Oscillator");
    Entity coin = scene.CreateEntity("Coin");
    coin.GetComponent<TransformComponent>().position = {100.0f, 50.0f};
    auto &osc = coin.AddComponent<OscillatorComponent>();
    osc.move = {0.0f, 20.0f};
    osc.frequency = 1.0f;
    osc.pulse = 0.2f;

    Step(scene, 15);
    const auto &transform = coin.GetComponent<TransformComponent>();
    CHECK_NEAR(transform.position.y, 70.0f, 0.1);
    CHECK_NEAR(transform.scale.x, 1.2f, 0.01);

    Step(scene, 45);
    CHECK_NEAR(transform.position.x, 100.0f, 1e-3);
    CHECK_NEAR(transform.position.y, 50.0f, 0.1);
    CHECK_NEAR(transform.scale.x, 1.0f, 0.01);
}

TEST_CASE("spawner: copies its prototype on an interval up to the alive limit")
{
    Scene scene("Spawner");
    Entity bat = scene.CreateEntity("Bat");
    bat.AddComponent<HealthComponent>().max = 30.0f;
    Entity wing = scene.CreateEntity("Wing");
    scene.SetParent(wing.GetHandle(), bat.GetHandle());

    Entity cave = scene.CreateEntity("Cave");
    cave.GetComponent<TransformComponent>().position = {300.0f, 0.0f};
    auto &spawner = cave.AddComponent<SpawnerComponent>();
    spawner.prototype = "Bat";
    spawner.interval = 0.5f;
    spawner.maxAlive = 3;
    spawner.spawnOnStart = true;
    spawner.velocity = {0.0f, -60.0f};

    auto count = [&](const char *name)
    {
        int n = 0;
        for (auto e : scene.GetRegistry().view<TagComponent>())
            n += scene.GetRegistry().get<TagComponent>(e).Tag == name;
        return n;
    };

    Step(scene, 1);
    CHECK(count("Bat") == 1);
    CHECK(count("Wing") == 1);
    CHECK(spawner.alive.size() == 1);
    const entt::entity first = spawner.alive.front();
    CHECK_NEAR(scene.GetRegistry().get<TransformComponent>(first).position.x, 300.0f, 1e-3);
    CHECK(scene.GetRegistry().all_of<VelocityComponent>(first));
    CHECK_NEAR(scene.GetRegistry().get<HealthComponent>(first).max, 30.0f, 1e-3);

    Step(scene, 120);
    CHECK(spawner.alive.size() == 3);
    CHECK(count("Bat") == 3);

    scene.GetRegistry().destroy(first);
    Step(scene, 40);
    CHECK(spawner.alive.size() == 3);
    CHECK(spawner.spawned == 4);
}

TEST_CASE("scene: clone copies an entity with its children")
{
    Scene scene("Clone");
    Entity parent = scene.CreateEntity("Enemy");
    parent.GetComponent<TransformComponent>().position = {12.0f, 34.0f};
    parent.AddComponent<HealthComponent>().max = 7.0f;
    Entity child = scene.CreateEntity("Eye");
    scene.SetParent(child.GetHandle(), parent.GetHandle());
    scene.UpdateTransforms();

    const entt::entity copy = SpawnerSystem::Clone(scene, parent.GetHandle());
    CHECK(copy != entt::null && copy != parent.GetHandle());
    auto &registry = scene.GetRegistry();
    CHECK_NEAR(registry.get<TransformComponent>(copy).position.y, 34.0f, 1e-3);
    CHECK_NEAR(registry.get<HealthComponent>(copy).max, 7.0f, 1e-3);
    CHECK(registry.get<HierarchyComponent>(copy).children.size() == 1);
}

TEST_CASE("follow: eases toward its target, respects axes and stop distance")
{
    Scene scene("Follow");
    Entity hero = scene.CreateEntity("Hero");
    hero.GetComponent<TransformComponent>().position = {200.0f, 100.0f};
    Entity pet = scene.CreateEntity("Pet");
    auto &follow = pet.AddComponent<FollowComponent>();
    follow.target = "Hero";
    follow.followY = false;
    follow.smoothing = 10.0f;

    Step(scene, 120);
    const auto &position = pet.GetComponent<TransformComponent>().position;
    CHECK_NEAR(position.x, 200.0f, 0.5);
    CHECK_NEAR(position.y, 0.0f, 1e-4);

    follow.followY = true;
    follow.stopDistance = 50.0f;
    Step(scene, 240);
    const float distance = glm::length(glm::vec2(200.0f, 100.0f) - position);
    CHECK_NEAR(distance, 50.0f, 1.0);

    follow.enabled = false;
    hero.GetComponent<TransformComponent>().position = {-400.0f, 0.0f};
    const glm::vec2 before = position;
    Step(scene, 30);
    CHECK_NEAR(position.x, before.x, 1e-4);
}

TEST_CASE("pickup: adds to a game variable once, and shows in text")
{
    GameVariables::Clear();
    Scene scene("Pickup");
    Entity player = scene.CreateEntity("Player");
    player.AddComponent<CharacterControllerComponent>().playerInput = false;

    Entity coin = scene.CreateEntity("Coin");
    coin.GetComponent<TransformComponent>().position = {-20.0f, -20.0f};
    auto &zone = coin.AddComponent<TriggerComponent>();
    zone.width = zone.height = 40.0f;
    auto &pickup = coin.AddComponent<PickupComponent>();
    pickup.key = "coins";
    pickup.amount = 3.0f;
    pickup.destroyOnPickup = false;

    Step(scene, 30);
    CHECK_NEAR(GameVariables::Get("coins"), 3.0f, 1e-6);

    player.GetComponent<TransformComponent>().position = {500.0f, 0.0f};
    Step(scene, 2);
    player.GetComponent<TransformComponent>().position = {0.0f, 0.0f};
    Step(scene, 2);
    CHECK_NEAR(GameVariables::Get("coins"), 6.0f, 1e-6);

    CHECK(GameVariables::Format("Coins: {coins}/{missing}") == "Coins: 6/{missing}");
    GameVariables::Set("time", 1.5);
    CHECK(GameVariables::Format("{time}s") == "1.50s");

    pickup.destroyOnPickup = true;
    player.GetComponent<TransformComponent>().position = {500.0f, 0.0f};
    Step(scene, 2);
    player.GetComponent<TransformComponent>().position = {0.0f, 0.0f};
    Step(scene, 3);
    CHECK(!scene.GetRegistry().valid(coin.GetHandle()));
    GameVariables::Clear();
}

TEST_CASE("respawn: checkpoints move the respawn point, death and falling bring the entity back")
{
    Scene scene("Respawn");
    Entity player = scene.CreateEntity("Player");
    player.GetComponent<TransformComponent>().position = {10.0f, 20.0f};
    auto &controller = player.AddComponent<CharacterControllerComponent>();
    controller.playerInput = false;
    controller.mode = ControllerMode::TopDown;
    auto &health = player.AddComponent<HealthComponent>();
    auto &respawn = player.AddComponent<RespawnComponent>();
    respawn.delay = 0.5f;
    respawn.useKillY = true;
    respawn.killY = -300.0f;

    Step(scene, 2);
    CHECK_NEAR(respawn.point.x, 10.0f, 1e-4);

    Entity flag = scene.CreateEntity("Flag");
    flag.GetComponent<TransformComponent>().position = {400.0f, 0.0f};
    auto &zone = flag.AddComponent<TriggerComponent>();
    zone.width = zone.height = 200.0f;
    zone.offsetX = zone.offsetY = -100.0f;
    flag.AddComponent<CheckpointComponent>().offset = {0.0f, 50.0f};

    auto &position = player.GetComponent<TransformComponent>().position;
    position = {400.0f, 0.0f};
    Step(scene, 2);
    CHECK_NEAR(respawn.point.x, 400.0f, 1e-3);
    CHECK_NEAR(respawn.point.y, 50.0f, 1e-3);

    position = {5000.0f, 0.0f};
    scene.GetRegistry().get<HealthComponent>(player.GetHandle()).current = 0.0f;
    scene.GetRegistry().get<HealthComponent>(player.GetHandle()).dead = true;
    Step(scene, 10);
    CHECK_NEAR(position.x, 5000.0f, 1e-3);
    Step(scene, 30);
    CHECK_NEAR(position.x, 400.0f, 1e-3);
    CHECK_NEAR(position.y, 50.0f, 1e-3);
    CHECK(!health.dead);
    CHECK_NEAR(health.current, 100.0f, 1e-3);

    position = {400.0f, -900.0f};
    Step(scene, 40);
    CHECK_NEAR(position.y, 50.0f, 1e-3);
}

TEST_CASE("music: playlists advance, wrap and stop without an audio device")
{
    std::mt19937 random(1);
    MusicComponent music;
    music.tracks = {"a.ogg", "b.ogg", "c.ogg"};
    MusicSystem::Start(music, random);
    CHECK(music.index == 0);
    MusicSystem::Next(music, random);
    CHECK(music.index == 1);
    MusicSystem::Next(music, random);
    MusicSystem::Next(music, random);
    CHECK(music.index == 0);

    music.loopPlaylist = false;
    music.index = 2;
    music.active = true;
    MusicSystem::Next(music, random);
    CHECK(!music.active);
    CHECK(music.index == 2);

    music.loopPlaylist = true;
    music.shuffle = true;
    music.index = 1;
    for (int i = 0; i < 50; ++i)
    {
        const int before = music.index;
        MusicSystem::Next(music, random);
        CHECK(music.index != before);
    }

    MusicComponent empty;
    MusicSystem::Start(empty, random);
    CHECK(empty.index == -1);
}

TEST_CASE("pickable: grabs under the cursor, drags, throws and honours the top-most entity")
{
    Input::SetEnabled(true);
    Input::SetViewport(0.0f, 0.0f, 800.0f, 600.0f);
    Scene scene("Pickable");
    scene.GetActiveCamera().SetViewportSize(800.0f, 600.0f);

    auto make = [&](const char *name, glm::vec2 at, float z)
    {
        Entity e = scene.CreateEntity(name);
        e.GetComponent<TransformComponent>().position = at;
        e.GetComponent<TransformComponent>().zIndex = z;
        auto &shape = e.AddComponent<RenderComponent>();
        shape.width = shape.height = 100.0f;
        e.AddComponent<PickableComponent>().liftScale = 1.0f;
        return e;
    };
    Entity back = make("Back", {0.0f, 0.0f}, 0.0f);
    Entity front = make("Front", {0.0f, 0.0f}, 1.0f);
    front.AddComponent<VelocityComponent>();
    auto &position = front.GetComponent<TransformComponent>().position;

    const glm::vec2 centre = scene.GetActiveCamera().WorldToScreen({0.0f, 0.0f});
    auto frame = [&](float x, float y, bool down)
    {
        Input::BeginFrame();
        Input::OnMouseMoved(x, y);
        if (down)
            Input::OnMouseButtonPressed(1);
        scene.OnUpdate(1.0f / 60.0f);
    };

    frame(centre.x + 400.0f, centre.y, false);
    CHECK(!scene.GetRegistry().get<PickableComponent>(front.GetHandle()).held);

    frame(centre.x + 10.0f, centre.y, true);
    CHECK(scene.GetRegistry().get<PickableComponent>(front.GetHandle()).held);
    CHECK(!scene.GetRegistry().get<PickableComponent>(back.GetHandle()).held);

    const glm::vec2 start = position;
    for (int i = 1; i <= 10; ++i)
    {
        Input::BeginFrame();
        Input::OnMouseMoved(centre.x + 10.0f + 20.0f * static_cast<float>(i), centre.y);
        scene.OnUpdate(1.0f / 60.0f);
    }
    const float expected = scene.ScreenToWorld({centre.x + 210.0f, centre.y}).x - scene.ScreenToWorld({centre.x + 10.0f, centre.y}).x;
    CHECK(expected > 100.0f);
    CHECK_NEAR(position.x - start.x, expected, 1.0);

    Input::BeginFrame();
    Input::OnMouseButtonReleased(1);
    scene.OnUpdate(1.0f / 60.0f);
    CHECK(!scene.GetRegistry().get<PickableComponent>(front.GetHandle()).held);
    CHECK(front.GetComponent<VelocityComponent>().vx > 500.0f);

    Input::SetEnabled(false);
}

TEST_CASE("button: hover, press and click events with scale feedback")
{
    Input::SetEnabled(true);
    Input::SetViewport(0.0f, 0.0f, 800.0f, 600.0f);
    Scene scene("Button");
    scene.GetActiveCamera().SetViewportSize(800.0f, 600.0f);

    Entity play = scene.CreateEntity("Play");
    auto &shape = play.AddComponent<RenderComponent>();
    shape.width = shape.height = 100.0f;
    auto &button = play.AddComponent<ButtonComponent>();
    button.hoverScale = 1.5f;
    button.pressScale = 0.5f;
    const auto &scale = play.GetComponent<TransformComponent>().scale;

    const glm::vec2 centre = scene.GetActiveCamera().WorldToScreen({0.0f, 0.0f});
    auto frame = [&](float dx, bool press, bool release)
    {
        Input::BeginFrame();
        Input::OnMouseMoved(centre.x + dx, centre.y);
        if (press)
            Input::OnMouseButtonPressed(1);
        if (release)
            Input::OnMouseButtonReleased(1);
        scene.OnUpdate(1.0f / 60.0f);
    };

    frame(300.0f, false, false);
    CHECK(!button.hovering);
    CHECK_NEAR(scale.x, 1.0f, 1e-5);

    frame(5.0f, false, false);
    CHECK(button.hovering);
    CHECK_NEAR(scale.x, 1.5f, 1e-5);

    frame(5.0f, true, false);
    CHECK(button.pressed);
    CHECK_NEAR(scale.x, 0.5f, 1e-5);

    frame(5.0f, false, true);
    CHECK(!button.pressed);
    CHECK_NEAR(scale.x, 1.5f, 1e-5);

    frame(300.0f, false, false);
    CHECK(!button.hovering);
    CHECK_NEAR(scale.x, 1.0f, 1e-5);

    frame(5.0f, true, false);
    frame(300.0f, false, true);
    CHECK(!button.pressed);
    CHECK_NEAR(scale.x, 1.0f, 1e-5);
    Input::SetEnabled(false);
}

TEST_CASE("teleporter: moves the player, does not bounce back until they step out")
{
    Scene scene("Teleporter");
    Entity player = scene.CreateEntity("Player");
    auto &controller = player.AddComponent<CharacterControllerComponent>();
    controller.playerInput = false;
    controller.mode = ControllerMode::TopDown;
    auto &position = player.GetComponent<TransformComponent>().position;

    auto portal = [&](const char *name, glm::vec2 at, const char *target)
    {
        Entity e = scene.CreateEntity(name);
        e.GetComponent<TransformComponent>().position = at;
        auto &zone = e.AddComponent<TriggerComponent>();
        zone.width = zone.height = 100.0f;
        zone.offsetX = zone.offsetY = -50.0f;
        auto &t = e.AddComponent<TeleporterComponent>();
        t.target = target;
        return e;
    };
    portal("A", {0.0f, 0.0f}, "B");
    portal("B", {600.0f, 0.0f}, "A");

    Step(scene, 3);
    CHECK_NEAR(position.x, 600.0f, 1e-3);
    Step(scene, 60);
    CHECK_NEAR(position.x, 600.0f, 1e-3);

    position = {1000.0f, 0.0f};
    Step(scene, 3);
    position = {600.0f, 0.0f};
    Step(scene, 3);
    CHECK_NEAR(position.x, 0.0f, 1e-3);
}

TEST_CASE("camera zone: zooms while the player is inside and restores afterwards")
{
    Scene scene("CameraZone");
    Entity camera = scene.CreateEntity("Camera");
    auto &cam = camera.AddComponent<CameraComponent>(1.0f);
    cam.primary = true;

    Entity player = scene.CreateEntity("Player");
    auto &controller = player.AddComponent<CharacterControllerComponent>();
    controller.playerInput = false;
    controller.mode = ControllerMode::TopDown;
    auto &position = player.GetComponent<TransformComponent>().position;
    position = {5000.0f, 0.0f};

    Entity area = scene.CreateEntity("Area");
    auto &trigger = area.AddComponent<TriggerComponent>();
    trigger.width = trigger.height = 200.0f;
    trigger.offsetX = trigger.offsetY = -100.0f;
    auto &zone = area.AddComponent<CameraZoneComponent>();
    zone.zoom = 0.5f;
    zone.speed = 20.0f;
    zone.setBounds = true;
    zone.boundsMin = {-10.0f, -20.0f};
    zone.boundsMax = {30.0f, 40.0f};

    Step(scene, 5);
    CHECK_NEAR(cam.Camera.GetZoom(), 1.0f, 1e-4);

    position = {0.0f, 0.0f};
    Step(scene, 60);
    CHECK_NEAR(cam.Camera.GetZoom(), 0.5f, 1e-3);
    CHECK(cam.useBounds);
    CHECK_NEAR(cam.boundsMax.y, 40.0f, 1e-4);

    position = {5000.0f, 0.0f};
    Step(scene, 90);
    CHECK_NEAR(cam.Camera.GetZoom(), 1.0f, 1e-3);
    CHECK(!cam.useBounds);
}

TEST_CASE("world bounds: clamp, wrap, bounce and destroy")
{
    Scene scene("Bounds");
    auto make = [&](BoundsMode mode, glm::vec2 at)
    {
        Entity e = scene.CreateEntity("Thing");
        e.GetComponent<TransformComponent>().position = at;
        auto &bounds = e.AddComponent<WorldBoundsComponent>();
        bounds.min = {-640.0f, -360.0f};
        bounds.max = {640.0f, 360.0f};
        bounds.mode = mode;
        return e;
    };

    Entity clamped = make(BoundsMode::Clamp, {1000.0f, -900.0f});
    Entity wrapped = make(BoundsMode::Wrap, {700.0f, 0.0f});
    Entity bounced = make(BoundsMode::Bounce, {700.0f, 0.0f});
    bounced.AddComponent<VelocityComponent>().vx = 100.0f;
    Entity doomed = make(BoundsMode::Destroy, {2000.0f, 0.0f});
    clamped.GetComponent<WorldBoundsComponent>().margin = 10.0f;

    Step(scene, 3);
    CHECK_NEAR(clamped.GetComponent<TransformComponent>().position.x, 630.0f, 1e-3);
    CHECK_NEAR(clamped.GetComponent<TransformComponent>().position.y, -350.0f, 1e-3);
    CHECK_NEAR(wrapped.GetComponent<TransformComponent>().position.x, -580.0f, 1e-3);
    CHECK_NEAR(bounced.GetComponent<TransformComponent>().position.x, 640.0f, 5.0);
    CHECK(bounced.GetComponent<VelocityComponent>().vx < 0.0f);
    CHECK(!scene.GetRegistry().valid(doomed.GetHandle()));
}

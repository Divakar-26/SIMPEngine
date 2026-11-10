#include "TestFramework.h"

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/Tween.h>
#include <Engine/Core/Version.h>
#include <Engine/Project/Project.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>

#include <filesystem>
#include <fstream>

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

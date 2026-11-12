#include "TestFramework.h"

#include <Engine/Core/VFS.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Prefab.h>
#include <Engine/Scene/Scene.h>

#include <filesystem>

using namespace SIMPEngine;

namespace
{
    std::string MountTemp()
    {
        const std::filesystem::path dir = std::filesystem::temp_directory_path() / "simp_prefab_assets";
        std::filesystem::create_directories(dir / "prefabs");
        VFS::Mount("assets", dir.string());
        return dir.string();
    }

    size_t CountTagged(Scene &scene, const std::string &tag)
    {
        size_t count = 0;
        for (auto handle : scene.GetRegistry().view<TagComponent>())
            if (scene.GetRegistry().get<TagComponent>(handle).Tag == tag)
                ++count;
        return count;
    }
}

TEST_CASE("Prefabs save a subtree and instantiate copies")
{
    const std::string dir = MountTemp();
    Scene scene("prefabs");

    Entity root = scene.BuildEntity("Enemy").At(300.0f, 50.0f).Build();
    root.AddComponent<HealthComponent>().max = 77.0f;
    Entity child = scene.BuildEntity("Sword").Build();
    scene.SetParent(child.GetHandle(), root.GetHandle());

    CHECK(Prefab::Save(scene, root.GetHandle(), "assets://prefabs/Enemy.prefab"));
    CHECK(std::filesystem::exists(dir + "/prefabs/Enemy.prefab"));
    CHECK_NEAR(root.GetComponent<TransformComponent>().position.x, 300.0f, 1e-4);

    entt::entity a = Prefab::Instantiate(scene, "assets://prefabs/Enemy.prefab", glm::vec2(10.0f, 20.0f));
    entt::entity b = Prefab::Instantiate(scene, "assets://prefabs/Enemy.prefab", glm::vec2(-5.0f, 0.0f));
    CHECK(a != entt::null);
    CHECK(b != entt::null);
    CHECK(a != b);
    CHECK(CountTagged(scene, "Enemy") == 3);
    CHECK(CountTagged(scene, "Sword") == 3);

    auto &registry = scene.GetRegistry();
    CHECK_NEAR(registry.get<TransformComponent>(a).position.x, 10.0f, 1e-4);
    CHECK_NEAR(registry.get<TransformComponent>(b).position.x, -5.0f, 1e-4);
    CHECK_NEAR(registry.get<HealthComponent>(a).max, 77.0f, 1e-4);
    CHECK(registry.get<PrefabInstanceComponent>(a).path == "assets://prefabs/Enemy.prefab");
    CHECK(registry.get<HierarchyComponent>(a).children.size() == 1);

    root.GetComponent<HealthComponent>().max = 120.0f;
    CHECK(Prefab::Save(scene, root.GetHandle(), "assets://prefabs/Enemy.prefab"));
    registry.get<TransformComponent>(a).position = {42.0f, 7.0f};
    entt::entity refreshed = Prefab::Refresh(scene, a);
    CHECK(refreshed != entt::null);
    CHECK(!registry.valid(a));
    CHECK_NEAR(registry.get<HealthComponent>(refreshed).max, 120.0f, 1e-4);
    CHECK_NEAR(registry.get<TransformComponent>(refreshed).position.x, 42.0f, 1e-4);
    CHECK(CountTagged(scene, "Enemy") == 3);
    CHECK(CountTagged(scene, "Sword") == 3);

    CHECK(Prefab::Instantiate(scene, "assets://prefabs/Missing.prefab") == entt::null);
    std::filesystem::remove_all(dir);
}

TEST_CASE("Spawner accepts a prefab path")
{
    const std::string dir = MountTemp();
    Scene scene("spawner");

    Entity template_ = scene.BuildEntity("Coin").Build();
    CHECK(Prefab::Save(scene, template_.GetHandle(), "assets://prefabs/Coin.prefab"));

    Entity spawner = scene.BuildEntity("Spawner").At(100.0f, 100.0f).Build();
    auto &component = spawner.AddComponent<SpawnerComponent>();
    component.prototype = "assets://prefabs/Coin.prefab";
    component.spawnOnStart = true;
    component.interval = 0.1f;
    component.maxAlive = 2;

    for (int i = 0; i < 10; ++i)
        scene.OnUpdate(0.1f);

    CHECK(CountTagged(scene, "Coin") == 3);
    for (auto handle : scene.GetRegistry().view<PrefabInstanceComponent>())
        CHECK(scene.GetRegistry().get<PrefabInstanceComponent>(handle).path == "assets://prefabs/Coin.prefab");
    std::filesystem::remove_all(dir);
}

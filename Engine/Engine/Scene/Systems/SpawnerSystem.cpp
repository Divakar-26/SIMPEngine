#include "SpawnerSystem.h"

#include <Engine/Core/Log.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Prefab.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace SIMPEngine
{
    namespace
    {
        void CollectSubtree(entt::registry &registry, entt::entity root, std::vector<entt::entity> &out, int depth = 0)
        {
            if (!registry.valid(root) || depth > 64)
                return;
            out.push_back(root);
            if (const auto *hierarchy = registry.try_get<HierarchyComponent>(root))
                for (entt::entity child : hierarchy->children)
                    CollectSubtree(registry, child, out, depth + 1);
        }

        bool IsAncestorOrSelf(entt::registry &registry, entt::entity candidate, entt::entity entity)
        {
            for (int depth = 0; entity != entt::null && registry.valid(entity) && depth < 64; ++depth)
            {
                if (entity == candidate)
                    return true;
                const auto *hierarchy = registry.try_get<HierarchyComponent>(entity);
                entity = hierarchy ? hierarchy->parent : entt::null;
            }
            return false;
        }
    }

    entt::entity SpawnerSystem::Clone(Scene &scene, entt::entity source)
    {
        entt::registry &registry = scene.GetRegistry();
        if (!registry.valid(source))
            return entt::null;

        std::vector<entt::entity> subtree;
        CollectSubtree(registry, source, subtree);

        SceneSerializer serializer(&scene);
        std::vector<entt::entity> created = serializer.DeserializeEntities(serializer.SerializeEntities(subtree));
        if (created.empty())
            return entt::null;

        for (size_t i = 0; i < subtree.size() && i < created.size(); ++i)
        {
            const auto *script = registry.try_get<ScriptComponent>(subtree[i]);
            if (script && script->InstantiateScript && script->LuaPath.empty())
                registry.emplace_or_replace<ScriptComponent>(created[i]).InstantiateScript = script->InstantiateScript;
        }

        if (auto *transform = registry.try_get<TransformComponent>(created.front()); transform && registry.all_of<TransformComponent>(source))
        {
            const WorldTransform2D world = DecomposeWorld(registry.get<TransformComponent>(source));
            transform->position = world.Position;
            transform->dirty = true;
        }
        scene.MarkPhysicsDirty();
        return created.front();
    }

    void SpawnerSystem::Clear()
    {
        m_Prototypes.clear();
        m_Missing.clear();
        m_Requests.clear();
    }

    const SpawnerSystem::Prototype *SpawnerSystem::PrototypeFor(Scene &scene, entt::entity spawner, const std::string &name, bool hide)
    {
        if (auto it = m_Prototypes.find(name); it != m_Prototypes.end())
            return &it->second;

        entt::registry &registry = scene.GetRegistry();
        if (Prefab::IsPrefabPath(name))
        {
            const std::optional<std::string> yaml = Prefab::LoadYaml(name);
            if (!yaml)
            {
                if (m_Missing.insert(name).second)
                    CORE_WARN("Spawner: prefab '{}' could not be loaded", name);
                return nullptr;
            }
            Prototype prefab;
            prefab.Yaml = *yaml;
            return &m_Prototypes.emplace(name, std::move(prefab)).first->second;
        }
        const Entity found = scene.GetEntityByName(name);
        if (!found || found.GetHandle() == spawner)
        {
            if (m_Missing.insert(name).second)
                CORE_WARN("Spawner: no entity named '{}' to copy", name);
            return nullptr;
        }

        std::vector<entt::entity> subtree;
        CollectSubtree(registry, found.GetHandle(), subtree);

        Prototype prototype;
        prototype.Yaml = SceneSerializer(&scene).SerializeEntities(subtree);
        for (size_t i = 0; i < subtree.size(); ++i)
        {
            const auto *script = registry.try_get<ScriptComponent>(subtree[i]);
            if (script && script->InstantiateScript && script->LuaPath.empty())
                prototype.Scripts.emplace_back(i, script->InstantiateScript);
        }

        if (hide && !IsAncestorOrSelf(registry, found.GetHandle(), spawner))
        {
            for (auto it = subtree.rbegin(); it != subtree.rend(); ++it)
                if (registry.valid(*it))
                    registry.destroy(*it);
            scene.MarkPhysicsDirty();
        }

        return &m_Prototypes.emplace(name, std::move(prototype)).first->second;
    }

    void SpawnerSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();
        m_Requests.clear();

        auto view = registry.view<SpawnerComponent>();
        for (entt::entity handle : view)
        {
            SpawnerComponent &spawner = view.get<SpawnerComponent>(handle);
            if (spawner.prototype.empty())
                continue;

            spawner.alive.erase(std::remove_if(spawner.alive.begin(), spawner.alive.end(), [&](entt::entity e)
                                               { return !registry.valid(e); }),
                                spawner.alive.end());

            int count = std::exchange(spawner.pending, 0);
            if (!spawner.started)
            {
                spawner.started = true;
                if (spawner.spawnOnStart)
                    spawner.elapsed = spawner.interval;
                m_Requests.push_back({handle, 0});
            }

            if (spawner.playing)
            {
                const float interval = std::max(spawner.interval, 0.01f);
                spawner.elapsed += dt;
                if (spawner.elapsed >= interval)
                {
                    spawner.elapsed = std::fmod(spawner.elapsed, interval);
                    count += std::max(spawner.perSpawn, 1);
                }
            }

            if (spawner.maxAlive > 0)
                count = std::min(count, spawner.maxAlive - static_cast<int>(spawner.alive.size()));
            if (spawner.totalLimit > 0)
                count = std::min(count, spawner.totalLimit - spawner.spawned);
            if (count > 0)
                m_Requests.push_back({handle, count});
        }

        bool spawnedAny = false;
        for (const Request &request : m_Requests)
        {
            if (!registry.valid(request.Spawner) || !registry.all_of<SpawnerComponent>(request.Spawner))
                continue;

            const std::string name = registry.get<SpawnerComponent>(request.Spawner).prototype;
            const bool hide = registry.get<SpawnerComponent>(request.Spawner).hidePrototype;
            const Prototype *prototype = PrototypeFor(scene, request.Spawner, name, hide);
            if (!prototype || request.Count <= 0 || !registry.valid(request.Spawner))
                continue;

            glm::vec2 origin{0.0f, 0.0f};
            if (const auto *transform = registry.try_get<TransformComponent>(request.Spawner))
                origin = DecomposeWorld(*transform).Position;

            for (int i = 0; i < request.Count; ++i)
            {
                std::vector<entt::entity> created = SceneSerializer(&scene).DeserializeEntities(prototype->Yaml);
                if (created.empty())
                    break;
                for (const auto &[index, factory] : prototype->Scripts)
                    if (index < created.size())
                        registry.emplace_or_replace<ScriptComponent>(created[index]).InstantiateScript = factory;

                if (!registry.valid(request.Spawner) || !registry.all_of<SpawnerComponent>(request.Spawner))
                    break;
                SpawnerComponent &spawner = registry.get<SpawnerComponent>(request.Spawner);

                glm::vec2 jitter{0.0f, 0.0f};
                for (int axis = 0; axis < 2; ++axis)
                {
                    const float range = std::abs(spawner.spread[axis]);
                    if (range > 0.0f)
                        jitter[axis] = std::uniform_real_distribution<float>(-range, range)(m_Random);
                }

                const entt::entity root = created.front();
                if (Prefab::IsPrefabPath(name))
                    registry.emplace_or_replace<PrefabInstanceComponent>(root, PrefabInstanceComponent{name});
                if (auto *transform = registry.try_get<TransformComponent>(root))
                {
                    transform->position = origin + spawner.offset + jitter;
                    transform->dirty = true;
                }
                if (spawner.velocity.x != 0.0f || spawner.velocity.y != 0.0f)
                    registry.emplace_or_replace<VelocityComponent>(root, VelocityComponent{spawner.velocity.x, spawner.velocity.y});
                if (spawner.lifetime > 0.0f)
                    registry.emplace_or_replace<LifetimeComponent>(root, LifetimeComponent{spawner.lifetime});

                spawner.alive.push_back(root);
                ++spawner.spawned;
                spawnedAny = true;
            }
        }

        if (spawnedAny)
            scene.MarkPhysicsDirty();
    }
}

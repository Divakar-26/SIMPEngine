#include <Engine/Core/Log.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <entt/entt.hpp>
#include <iostream>
#include <algorithm>

namespace SIMPEngine
{
    Scene::Scene(const std::string &name) : m_Name(name)
    {
        physicsSystem.SetSubstep(20);

        m_Registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);
        m_Registry.on_destroy<PhysicsComponent>().connect<&Scene::OnPhysicsComponentDestroyed>(this);
        m_Registry.on_destroy<AnimatedSpriteComponent>().connect<&Scene::OnAnimatedSpriteComponentDestroyed>(this);
        m_Registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyComponentDestroyed>(this);
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!entity || !m_Registry.valid(entity.GetHandle()))
            return;
        m_Registry.destroy(entity.GetHandle());
    }

    Entity Scene::CreateEntity(const std::string &name)
    {
        entt::entity handle = m_Registry.create();
        Entity entity(handle, this);

        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(name.empty() ? "Unnamed Entity" : name);
        return entity;
    }

    Entity Scene::GetEntityByName(const std::string &name)
    {
        auto view = m_Registry.view<TagComponent>();
        for (auto e : view)
        {
            auto &tag = view.get<TagComponent>(e);
            if (tag.Tag == name)
                return Entity(e, this);
        }
        return Entity{};
    }

    void Scene::OnUpdate(float deltaTime)
    {
        // start phycsis frame
        physicsSystem.startFrame(deltaTime);
        physicsSystem.UpdateTransform(m_Registry);

        cameraSystem.OnUpdate(m_Registry, deltaTime);
        movementSystem.Update(m_Registry, deltaTime);
        collisionSystem.Update(m_Registry, deltaTime);
        hierarchySystem.Update(m_Registry);
        lifetimeSystem.Update(m_Registry, deltaTime);

        auto view = m_Registry.view<ScriptComponent>();
        for (auto entityHandle : view)
        {
            auto &sc = view.get<ScriptComponent>(entityHandle);

            if (!sc.InstantiateScript)
                continue;

            if (!sc.Instance)
            {
                sc.Instance = sc.InstantiateScript();
                sc.Instance->m_Entity = Entity(entityHandle, this);
                sc.Instance->OnCreate();
            }

            sc.Instance->OnUpdate(deltaTime);
        }

        // update physics
        physicsSystem.Update(m_Registry, deltaTime);

        // upgrading all animation
        animationSystem.Update(m_Registry, deltaTime);
    }

    // TODO -> Z INDEX SYSTEM -> DONE
    void Scene::OnRender()
    {
        cameraSystem.OnRender(m_Registry);

        tilemapSystem.Render(m_Registry); 

        renderSystem.RenderQuads(m_Registry);
        renderSystem.RenderSprites(m_Registry);
    }

    void Scene::Clear()
    {
        // Destroy physics bodies
        physicsSystem.DestroyBodies();
        physicsSystem.DestroyWorld();

        // Destroy all ECS entities
        m_Registry.clear();
    }

    void Scene::SetParent(entt::entity child,
                          entt::entity parent)
    {
        auto &ch =
            m_Registry.get_or_emplace<HierarchyComponent>(child);

        auto &ph =
            m_Registry.get_or_emplace<HierarchyComponent>(parent);

        ch.parent = parent;
        ph.children.push_back(child);
    }

    void Scene::OnScriptComponentDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto &scriptComponent = registry.get<ScriptComponent>(entity);
        if (scriptComponent.Instance)
        {
            scriptComponent.Instance->OnDestroy();

            if (scriptComponent.DestroyScript)
                scriptComponent.DestroyScript(&scriptComponent);
            else
            {
                delete scriptComponent.Instance;
                scriptComponent.Instance = nullptr;
            }
        }
    }

    void Scene::OnPhysicsComponentDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto &physicsComponent = registry.get<PhysicsComponent>(entity);
        if (!physicsComponent.body)
            return;

        auto &bodies = physicsSystem.bodies;
        bodies.erase(std::remove(bodies.begin(), bodies.end(), physicsComponent.body), bodies.end());

        delete physicsComponent.body;
        physicsComponent.body = nullptr;
    }

    void Scene::OnAnimatedSpriteComponentDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto &animatedSprite = registry.get<AnimatedSpriteComponent>(entity);
        delete animatedSprite.animation;
        animatedSprite.animation = nullptr;
    }

    void Scene::OnHierarchyComponentDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto &hierarchy = registry.get<HierarchyComponent>(entity);

        if (hierarchy.parent != entt::null && registry.valid(hierarchy.parent) &&
            registry.any_of<HierarchyComponent>(hierarchy.parent))
        {
            auto &parentHierarchy = registry.get<HierarchyComponent>(hierarchy.parent);
            auto &siblings = parentHierarchy.children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
        }

        for (auto child : hierarchy.children)
        {
            if (!registry.valid(child) || !registry.any_of<HierarchyComponent>(child))
                continue;

            auto &childHierarchy = registry.get<HierarchyComponent>(child);
            if (childHierarchy.parent == entity)
                childHierarchy.parent = entt::null;
        }
    }

} // namespace SIMPEngine

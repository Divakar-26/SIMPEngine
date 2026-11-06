#include <Engine/Core/Log.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Input/Input.h>

#include <entt/entt.hpp>
#include <iostream>
#include <algorithm>

namespace SIMPEngine
{
    glm::vec2 ScriptableEntity::GetMouseWorldPosition() const
    {
        Scene *scene = m_Entity.GetScene();
        if (!scene)
            return {0.0f, 0.0f};
        return scene->ScreenToWorld(Input::GetMouseViewportPosition());
    }

    bool ScriptableEntity::IsMouseInViewport() const
    {
        return Input::IsMouseInViewport();
    }

    Entity::operator bool() const
    {
        return m_Scene != nullptr && m_EntityHandle != entt::null &&
               m_Scene->GetRegistry().valid(m_EntityHandle);
    }

    Scene::Scene(const std::string &name) : m_Name(name)
    {
        physicsSystem.SetSubstep(20);

        m_Registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);
        m_Registry.on_destroy<PhysicsComponent>().connect<&Scene::OnPhysicsComponentDestroyed>(this);
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

    EntityBuilder Scene::BuildEntity(const std::string &name)
    {
        entt::entity handle = m_Registry.create();
        Entity entity(handle, this);

        entity.AddComponent<TransformComponent>();
        entity.AddComponent<TagComponent>(name.empty() ? "Unnamed Entity" : name);
        return EntityBuilder(m_Registry, entity);
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

    void Scene::RebuildPhysicsWorld()
    {
        physicsSystem.DestroyBodies();
        physicsSystem.DestroyWorld();

        for (auto entity : m_Registry.view<PhysicsComponent>())
        {
            auto &pc = m_Registry.get<PhysicsComponent>(entity);
            if (!pc.body)
                continue;
            pc.body->entityID = static_cast<uint32_t>(entity);
            physicsSystem.GetWorld().addBody(pc.body.get());
            physicsSystem.bodies.push_back(pc.body.get());
        }

        m_PhysicsDirty = false;
    }

    void Scene::SyncPhysicsFromTransforms()
    {
        auto view = m_Registry.view<TransformComponent, PhysicsComponent>();
        for (auto entity : view)
        {
            auto &tc = view.get<TransformComponent>(entity);
            auto &pc = view.get<PhysicsComponent>(entity);
            if (!pc.body)
                continue;
            pc.body->position = {tc.position.x, tc.position.y};
            pc.body->orientation = glm::radians(tc.rotation);
            pc.body->calculateDerivativeData();
        }
    }

    void Scene::OnUpdate(float deltaTime)
    {
        if (m_PhysicsDirty)
            RebuildPhysicsWorld();

        physicsSystem.startFrame(deltaTime);
        physicsSystem.UpdateTransform(m_Registry);

        cameraSystem.OnUpdate(m_Registry, deltaTime);
        movementSystem.Update(m_Registry, deltaTime);
        collisionSystem.Update(m_Registry, deltaTime);
        hierarchySystem.Update(m_Registry, deltaTime);
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

        physicsSystem.Update(m_Registry, deltaTime);

        animationSystem.Update(m_Registry, deltaTime);
        particleSystem.Update(m_Registry, deltaTime);
    }

    void Scene::UpdateParticles(float dt)
    {
        UpdateTransforms();
        particleSystem.Update(m_Registry, dt);
    }

    void Scene::UpdateTransforms()
    {
        hierarchySystem.Update(m_Registry, 0.0f);
    }

    glm::vec2 Scene::ScreenToWorld(const glm::vec2 &viewportPixel)
    {
        return GetActiveCamera().ScreenToWorld(viewportPixel);
    }

    void Scene::OnRender()
    {
        UpdateTransforms();
        cameraSystem.OnRender(m_Registry);
        RenderContent();
    }

    void Scene::OnRender(const glm::mat4 &view, const glm::mat4 &projection)
    {
        UpdateTransforms();
        Renderer::SetProjectionMatrix(projection);
        Renderer::SetViewMatrix(view);
        RenderContent();
    }

    void Scene::RenderContent()
    {
        tilemapSystem.Render(m_Registry);

        renderSystem.RenderQuads(m_Registry);
        renderSystem.RenderSprites(m_Registry);
        particleSystem.Render(m_Registry);
        renderSystem.RenderText(m_Registry);

        auto view = m_Registry.view<ScriptComponent>();
        for (auto entityHandle : view)
        {
            auto &sc = view.get<ScriptComponent>(entityHandle);
            if (sc.Instance)
                sc.Instance->OnRender();
        }
    }

    void Scene::Clear()
    {
        physicsSystem.DestroyBodies();
        physicsSystem.DestroyWorld();

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
        }
    }

    void Scene::OnPhysicsComponentDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto &physicsComponent = registry.get<PhysicsComponent>(entity);
        if (!physicsComponent.body)
            return;

        auto &bodies = physicsSystem.bodies;
        bodies.erase(std::remove(bodies.begin(), bodies.end(), physicsComponent.body.get()), bodies.end());
        m_PhysicsDirty = true;
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
}

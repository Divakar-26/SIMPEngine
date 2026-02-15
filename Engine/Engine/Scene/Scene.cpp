#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <entt/entt.hpp>
#include <iostream>

namespace SIMPEngine
{
    Scene::Scene(const std::string &name) : m_Name(name)
    {
        physicsSystem.SetSubstep(20);
    }

    void Scene::DestroyEntity(Entity entity)
    {
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

} // namespace SIMPEngine

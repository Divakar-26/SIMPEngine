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

    void ScriptableEntity::OnEvent(const ScriptMessage &message)
    {
        switch (message.Type)
        {
        case ScriptEvent::FixedUpdate:
            OnFixedUpdate(message.Value);
            break;
        case ScriptEvent::TriggerEnter:
            OnTriggerEnter(message.Other);
            break;
        case ScriptEvent::TriggerStay:
            OnTriggerStay(message.Other);
            break;
        case ScriptEvent::TriggerExit:
            OnTriggerExit(message.Other);
            break;
        case ScriptEvent::Collision:
            OnCollision(message.Other);
            break;
        case ScriptEvent::Timer:
            OnTimer();
            break;
        case ScriptEvent::Damaged:
            OnDamaged(message.Value, message.Other);
            break;
        case ScriptEvent::Death:
            OnDeath();
            break;
        case ScriptEvent::PathPoint:
            OnPathPoint(static_cast<int>(message.Value));
            break;
        case ScriptEvent::PathEnd:
            OnPathEnd();
            break;
        case ScriptEvent::Count:
            break;
        }
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
        m_Registry.on_destroy<AudioSourceComponent>().connect<&Scene::OnAudioSourceDestroyed>(this);
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

    void Scene::SetFixedTimeStep(float seconds)
    {
        m_FixedStep = std::clamp(seconds, 1.0f / 240.0f, 1.0f / 10.0f);
    }

    void Scene::SendEvent(entt::entity target, ScriptEvent type, entt::entity other, float value)
    {
        if (target == entt::null || !m_Registry.valid(target))
            return;

        ScriptComponent *script = m_Registry.try_get<ScriptComponent>(target);
        if (!script || !script->Instance)
            return;

        ScriptMessage message;
        message.Type = type;
        message.Value = value;
        if (other != entt::null && m_Registry.valid(other))
            message.Other = Entity(other, this);

        script->Instance->SendEvent(message);
    }

    void Scene::OnUpdate(float deltaTime)
    {
        if (m_PhysicsDirty)
            RebuildPhysicsWorld();

        m_Time += deltaTime;
        m_LastDelta = deltaTime;

        physicsSystem.startFrame(deltaTime);
        physicsSystem.UpdateTransform(m_Registry);

        cameraSystem.OnUpdate(m_Registry, deltaTime);
        movementSystem.Update(m_Registry, deltaTime);
        collisionSystem.Update(m_Registry, deltaTime);
        hierarchySystem.Update(m_Registry, deltaTime);
        lifetimeSystem.Update(m_Registry, deltaTime);

        m_ScriptOrder.clear();
        for (entt::entity handle : m_Registry.view<ScriptComponent>())
            m_ScriptOrder.push_back(handle);

        for (size_t i = 0; i < m_ScriptOrder.size(); ++i)
        {
            const entt::entity handle = m_ScriptOrder[i];
            if (!m_Registry.valid(handle))
                continue;

            ScriptComponent *sc = m_Registry.try_get<ScriptComponent>(handle);
            if (!sc || !sc->InstantiateScript)
                continue;

            if (!sc->Instance)
            {
                sc->Instance = sc->InstantiateScript();
                sc->Instance->m_Entity = Entity(handle, this);
                sc->Instance->OnCreate();

                if (!m_Registry.valid(handle))
                    continue;
                sc = m_Registry.try_get<ScriptComponent>(handle);
                if (!sc || !sc->Instance)
                    continue;
            }

            sc->Instance->OnUpdate(deltaTime);
        }

        m_FixedAccumulator = std::min(m_FixedAccumulator + deltaTime, m_FixedStep * 5.0f);
        while (m_FixedAccumulator >= m_FixedStep)
        {
            for (size_t i = 0; i < m_ScriptOrder.size(); ++i)
                SendEvent(m_ScriptOrder[i], ScriptEvent::FixedUpdate, entt::null, m_FixedStep);
            m_FixedAccumulator -= m_FixedStep;
        }

        m_Tweens.Update(deltaTime);
        pathFollowerSystem.Update(*this, deltaTime);
        triggerSystem.Update(*this);
        timerSystem.Update(*this, deltaTime);
        healthSystem.Update(*this, deltaTime);

        physicsSystem.Update(m_Registry, deltaTime);

        const std::vector<PhysicsSystem::ContactPair> contacts = physicsSystem.GetContacts();
        for (const PhysicsSystem::ContactPair &contact : contacts)
        {
            SendEvent(contact.a, ScriptEvent::Collision, contact.b);
            SendEvent(contact.b, ScriptEvent::Collision, contact.a);
        }

        animationSystem.Update(m_Registry, deltaTime);
        particleSystem.Update(m_Registry, deltaTime);
        audioSystem.Update(m_Registry);
    }

    void Scene::StopAudio()
    {
        for (auto entity : m_Registry.view<AudioSourceComponent>())
        {
            AudioSystem::StopSource(m_Registry, entity);
            m_Registry.get<AudioSourceComponent>(entity).started = false;
        }
    }

    void Scene::OnAudioSourceDestroyed(entt::registry &registry, entt::entity entity)
    {
        AudioSystem::StopSource(registry, entity);
    }

    void Scene::ApplyParallax()
    {
        auto view = m_Registry.view<ParallaxComponent, TransformComponent>();
        if (view.begin() == view.end())
            return;

        glm::vec2 camera = GetActiveCamera().GetPosition();
        auto shift = [this](auto &&self, entt::entity entity, const glm::vec2 &offset, int depth) -> void
        {
            auto &transform = m_Registry.get<TransformComponent>(entity);
            transform.worldTransform[3][0] += offset.x;
            transform.worldTransform[3][1] += offset.y;
            auto *hierarchy = m_Registry.try_get<HierarchyComponent>(entity);
            if (!hierarchy || depth > 64)
                return;
            for (auto child : hierarchy->children)
                if (m_Registry.valid(child) && m_Registry.all_of<TransformComponent>(child))
                    self(self, child, offset, depth + 1);
        };

        for (auto entity : view)
        {
            glm::vec2 offset = camera * (glm::vec2(1.0f) - view.get<ParallaxComponent>(entity).factor);
            shift(shift, entity, offset, 0);
        }
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
        ApplyParallax();
        cameraSystem.OnRender(m_Registry);
        RenderContent();
    }

    void Scene::OnRender(const glm::mat4 &view, const glm::mat4 &projection)
    {
        UpdateTransforms();
        ApplyParallax();
        Renderer::SetProjectionMatrix(projection);
        Renderer::SetViewMatrix(view);
        RenderContent();
    }

    void Scene::RenderContent()
    {
        tilemapSystem.Render(m_Registry);

        renderSystem.RenderQuads(m_Registry);
        renderSystem.RenderSprites(m_Registry);
        vectorSystem.Render(m_Registry);
        bool lit = m_Lighting.Enabled && m_LightingVisible;
        particleSystem.Render(m_Registry, lit ? 0 : -1);
        renderSystem.RenderText(m_Registry);

        auto view = m_Registry.view<ScriptComponent>();
        for (auto entityHandle : view)
        {
            auto &sc = view.get<ScriptComponent>(entityHandle);
            if (sc.Instance)
                sc.Instance->OnRender();
        }

        if (lit)
        {
            lightingSystem.Render(m_Registry, m_Lighting);
            particleSystem.Render(m_Registry, 1);
        }
        if (m_LightingVisible)
            glowSystem.Render(m_Registry);
        if (m_Bloom.Enabled && m_LightingVisible)
            bloomRenderer.Apply(m_Bloom);
    }

    void Scene::Clear()
    {
        physicsSystem.DestroyBodies();
        physicsSystem.DestroyWorld();

        m_Tweens.Clear();
        m_ScriptOrder.clear();
        m_Time = 0.0f;
        m_LastDelta = 0.0f;
        m_FixedAccumulator = 0.0f;
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

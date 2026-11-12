#include <Engine/Core/Log.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Input/Input.h>
#include <Engine/Audio/Music.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/ShaderEffect.h>

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
        case ScriptEvent::Landed:
            OnLand(message.Value);
            break;
        case ScriptEvent::Jumped:
            OnJump();
            break;
        case ScriptEvent::Grabbed:
            OnGrab();
            break;
        case ScriptEvent::Dropped:
            OnDrop();
            break;
        case ScriptEvent::Clicked:
            OnClick();
            break;
        case ScriptEvent::HoverEnter:
            OnHoverEnter();
            break;
        case ScriptEvent::HoverExit:
            OnHoverExit();
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

    void Scene::SyncPhysicsFromTransforms()
    {
        physicsSystem.MarkAllUnsynced(m_Registry);
    }

    PhysicsComponent &Scene::AddBody(entt::entity entity, std::unique_ptr<AccelEngine::RigidBody> body)
    {
        physicsSystem.AddBody(m_Registry, entity, std::move(body));
        return m_Registry.get_or_emplace<PhysicsComponent>(entity);
    }

    void Scene::RemoveBody(entt::entity entity)
    {
        physicsSystem.RemoveBody(m_Registry, entity);
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

    bool Scene::LoadSceneFile(const std::string &path)
    {
        SceneSerializer serializer(this);
        const std::string backup = serializer.SerializeToString();
        const std::string source = m_SourcePath;
        Clear();
        if (serializer.Deserialize(path))
        {
            m_SourcePath = path;
            return true;
        }
        CORE_ERROR("Could not load scene '{}'", path);
        Clear();
        serializer.DeserializeFromString(backup);
        m_SourcePath = source;
        return false;
    }

    void Scene::RequestLoad(const std::string &path, float fade, const std::string &shader)
    {
        if (path.empty())
        {
            CORE_WARN("Scene load requested with an empty path");
            return;
        }
        m_Transition.Requested = true;
        m_Transition.RequestedPath = path;
        m_Transition.RequestedFade = std::max(fade, 0.0f);
        m_Transition.RequestedShader = shader;
    }

    void Scene::RequestReload(float fade, const std::string &shader)
    {
        if (m_SourcePath.empty())
        {
            CORE_WARN("Scene reload requested but the scene was not loaded from a file");
            return;
        }
        RequestLoad(m_SourcePath, fade, shader);
    }

    float Scene::GetFadeAlpha() const
    {
        if (m_Transition.Duration <= 0.0f)
            return 0.0f;
        const float t = std::clamp(m_Transition.Time / m_Transition.Duration, 0.0f, 1.0f);
        if (m_Transition.Phase == TransitionPhase::Out)
            return t;
        if (m_Transition.Phase == TransitionPhase::In)
            return 1.0f - t;
        return 0.0f;
    }

    void Scene::UpdateTransition(float dt)
    {
        Transition &t = m_Transition;

        if (t.Phase == TransitionPhase::Out)
        {
            t.Time += dt;
            if (t.Time < t.Duration)
                return;
            const std::string path = t.ActivePath;
            const float duration = t.Duration;
            const Transition pending = t;
            LoadSceneFile(path);
            m_Transition = Transition{};
            m_Transition.Requested = pending.Requested;
            m_Transition.RequestedPath = pending.RequestedPath;
            m_Transition.RequestedFade = pending.RequestedFade;
            m_Transition.RequestedShader = pending.RequestedShader;
            m_Transition.Shader = pending.Shader;
            m_Transition.Phase = TransitionPhase::In;
            m_Transition.Duration = duration;
            return;
        }

        if (t.Phase == TransitionPhase::In)
        {
            t.Time += dt;
            if (t.Time >= t.Duration)
            {
                t.Phase = TransitionPhase::None;
                t.Time = 0.0f;
            }
        }

        if (!t.Requested || t.Phase == TransitionPhase::In)
            return;

        const std::string path = t.RequestedPath;
        const float fade = t.RequestedFade;
        const std::string shader = t.RequestedShader;
        t.Requested = false;
        if (fade <= 0.0f)
        {
            LoadSceneFile(path);
            m_Transition = Transition{};
            return;
        }
        t.Phase = TransitionPhase::Out;
        t.Shader = shader;
        t.ActivePath = path;
        t.Duration = fade;
        t.Time = 0.0f;
    }

    void Scene::RenderFade()
    {
        const float alpha = GetFadeAlpha();
        if (alpha <= 0.0f)
            return;
        if (!m_Transition.Shader.empty())
        {
            m_TransitionCanvas.shader = m_Transition.Shader;
            m_TransitionCanvas.effect = ShaderEffect::Load(m_Transition.Shader);
            m_TransitionCanvas.progress = alpha;
            if (m_TransitionCanvas.effect->IsValid())
            {
                for (const ShaderUniformDecl &decl : m_TransitionCanvas.effect->GetDeclared())
                    if (!ShaderCanvasSystem::FindUniform(m_TransitionCanvas, decl.Name))
                    {
                        ShaderUniformValue value;
                        value.name = decl.Name;
                        value.type = static_cast<int>(decl.Type);
                        for (int i = 0; i < 4; ++i)
                            value.value[i] = decl.Default[i];
                        m_TransitionCanvas.uniforms.push_back(std::move(value));
                    }
                shaderCanvasSystem.RenderOne(*this, m_TransitionCanvas);
                return;
            }
        }
        glm::vec2 min, max;
        Renderer::GetVisibleBounds(min, max);
        const float width = max.x - min.x;
        const float height = max.y - min.y;
        const uint8_t a = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
        Renderer::DrawQuad((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, width * 1.05f, height * 1.05f, 0.0f, {0, 0, 0, a}, true, 900.0f);
    }

    void Scene::OnUpdate(float deltaTime)
    {
        UpdateTransition(deltaTime);

        if (m_PhysicsDirty)
        {
            physicsSystem.RegisterMissing(m_Registry);
            m_PhysicsDirty = false;
        }

        m_Time += deltaTime;
        m_LastDelta = deltaTime;

        movementSystem.Update(m_Registry, deltaTime);
        oscillatorSystem.Update(m_Registry, deltaTime);
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

        characterControllerSystem.Update(*this, deltaTime);
        m_Tweens.Update(deltaTime);
        pathFollowerSystem.Update(*this, deltaTime);
        triggerSystem.Update(*this);
        damageSystem.Update(*this, deltaTime);
        pickupSystem.Update(*this);
        teleporterSystem.Update(*this);
        timerSystem.Update(*this, deltaTime);
        spawnerSystem.Update(*this, deltaTime);
        healthSystem.Update(*this, deltaTime);
        respawnSystem.Update(*this, deltaTime);
        uiSystem.Update(*this, deltaTime);
        pickableSystem.Update(*this, deltaTime);
        buttonSystem.Update(*this);

        physicsSystem.Update(m_Registry, deltaTime);

        const std::vector<PhysicsSystem::ContactPair> contacts = physicsSystem.GetContacts();
        const std::vector<PhysicsSystem::SensorEvent> sensorEvents = physicsSystem.GetSensorEvents();
        for (const PhysicsSystem::ContactPair &contact : contacts)
        {
            SendEvent(contact.a, ScriptEvent::Collision, contact.b);
            SendEvent(contact.b, ScriptEvent::Collision, contact.a);
        }
        for (const PhysicsSystem::SensorEvent &event : sensorEvents)
        {
            const ScriptEvent type = event.Type == PhysicsSystem::SensorEventType::Enter  ? ScriptEvent::TriggerEnter
                                     : event.Type == PhysicsSystem::SensorEventType::Stay ? ScriptEvent::TriggerStay
                                                                                          : ScriptEvent::TriggerExit;
            SendEvent(event.a, type, event.b);
            SendEvent(event.b, type, event.a);
        }

        followSystem.Update(*this, deltaTime);
        boundsSystem.Update(*this);
        cameraZoneSystem.Update(*this, deltaTime);
        cameraSystem.OnUpdate(m_Registry, deltaTime);

        animationSystem.Update(m_Registry, deltaTime);
        particleSystem.Update(m_Registry, deltaTime);
        audioSystem.Update(m_Registry);
        musicSystem.Update(*this);
    }

    void Scene::StopAudio()
    {
        for (auto entity : m_Registry.view<AudioSourceComponent>())
        {
            AudioSystem::StopSource(m_Registry, entity);
            m_Registry.get<AudioSourceComponent>(entity).started = false;
        }
        for (auto entity : m_Registry.view<MusicComponent>())
        {
            auto &music = m_Registry.get<MusicComponent>(entity);
            music.started = music.active = music.wasInside = false;
            music.index = -1;
        }
        Music::Stop(0.0f);
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
        Camera2D &camera = GetActiveCamera();
        const auto [cameraWidth, cameraHeight] = camera.GetViewportSize();
        const glm::vec2 input = Input::GetViewportSize();
        glm::vec2 pixel = viewportPixel;
        if (cameraWidth > 0 && cameraHeight > 0 && input.x > 0.0f && input.y > 0.0f)
            pixel = {viewportPixel.x * static_cast<float>(cameraWidth) / input.x, viewportPixel.y * static_cast<float>(cameraHeight) / input.y};
        return camera.ScreenToWorld(pixel);
    }

    void Scene::OnRender()
    {
        UpdateTransforms();
        ApplyParallax();
        cameraSystem.OnRender(m_Registry);
        RenderContent(true);
    }

    void Scene::OnRender(const glm::mat4 &view, const glm::mat4 &projection)
    {
        UpdateTransforms();
        ApplyParallax();
        Renderer::SetProjectionMatrix(projection);
        Renderer::SetViewMatrix(view);
        RenderContent(false);
    }

    void Scene::RenderContent(bool screenSpaceUI)
    {
        shaderCanvasSystem.Prepare(*this, m_LightingVisible);
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

        shaderCanvasSystem.RenderGroups(*this);

        if (lit)
        {
            lightingSystem.Render(m_Registry, m_Lighting);
            particleSystem.Render(m_Registry, 1);
        }
        if (m_LightingVisible)
            glowSystem.Render(m_Registry);
        if (m_Bloom.Enabled && m_LightingVisible)
            bloomRenderer.Apply(m_Bloom);
        if (m_LightingVisible)
            shaderCanvasSystem.RenderPost(*this);
        uiSystem.Render(*this, screenSpaceUI);
        RenderFade();
    }

    void Scene::Clear()
    {
        physicsSystem.Clear();
        physicsSystem.ResetSettings();

        m_Tweens.Clear();
        spawnerSystem.Clear();
        teleporterSystem.Clear();
        cameraZoneSystem.Clear();
        uiSystem.Clear();
        shaderCanvasSystem.Clear();
        m_ScriptOrder.clear();
        m_Time = 0.0f;
        m_LastDelta = 0.0f;
        m_FixedAccumulator = 0.0f;
        m_Transition = Transition{};
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
        if (physicsComponent.body)
            physicsSystem.OnBodyDestroyed(physicsComponent.body.get());
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

#pragma once

#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Scene/Systems/MovementSystem.h>
#include <Engine/Scene/Systems/CollisionSystem.h>
#include <Engine/Scene/Systems/CameraSystem.h>
#include <Engine/Scene/Systems/HierarchySystem.h>
#include <Engine/Scene/Systems/LifetimeSystem.h>
#include <Engine/Scene/Systems/PhysicsSystem.h>
#include <Engine/Scene/Systems/AnimationSystem.h>
#include <Engine/Scene/Systems/RenderSystem.h>
#include <Engine/Scene/Systems/TilemapSystem.h>
#include <Engine/Scene/Systems/ParticleSystem.h>
#include <Engine/Scene/Systems/LightingSystem.h>
#include <Engine/Scene/Systems/AudioSystem.h>
#include <Engine/Rendering/Bloom.h>
#include <Engine/Scene/Systems/GlowSystem.h>
#include <Engine/Scene/Systems/TriggerSystem.h>
#include <Engine/Scene/Systems/TimerSystem.h>
#include <Engine/Scene/Systems/PathFollowerSystem.h>
#include <Engine/Scene/Systems/HealthSystem.h>
#include <Engine/Scene/Systems/VectorSystem.h>
#include <Engine/Scene/Systems/CharacterControllerSystem.h>
#include <Engine/Scene/Systems/DamageSystem.h>
#include <Engine/Scene/Systems/OscillatorSystem.h>
#include <Engine/Scene/Systems/SpawnerSystem.h>
#include <Engine/Scene/Systems/FollowSystem.h>
#include <Engine/Scene/Systems/PickupSystem.h>
#include <Engine/Scene/Systems/RespawnSystem.h>
#include <Engine/Scene/Systems/MusicSystem.h>
#include <Engine/Scene/Systems/PickableSystem.h>
#include <Engine/Scene/Systems/ButtonSystem.h>
#include <Engine/Scene/Systems/UISystem.h>
#include <Engine/Scene/Systems/ShaderCanvasSystem.h>
#include <Engine/Scene/Systems/TeleporterSystem.h>
#include <Engine/Scene/Systems/CameraZoneSystem.h>
#include <Engine/Scene/Systems/BoundsSystem.h>
#include <Engine/Scene/ScriptEvent.h>
#include <Engine/Animation/Tween.h>

#include <AccelEngine/world.h>
#include <AccelEngine/ForceRegistry.h>

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace SIMPEngine
{
    class Entity;

    class EntityBuilder
    {
    public:
        EntityBuilder(entt::registry& registry, Entity entity)
            : m_Registry(registry), m_Entity(entity) {}

        template<typename T, typename... Args>
        EntityBuilder& With(Args&&... args)
        {
            m_Registry.emplace<T>(m_Entity.GetHandle(), std::forward<Args>(args)...);
            return *this;
        }

        EntityBuilder& At(glm::vec2 pos)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).position = pos;
            return *this;
        }

        EntityBuilder& At(float x, float y)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).position = {x, y};
            return *this;
        }

        EntityBuilder& Scale(glm::vec2 scale)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).scale = scale;
            return *this;
        }

        EntityBuilder& Scale(float x, float y)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).scale = {x, y};
            return *this;
        }

        EntityBuilder& Rotation(float degrees)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).rotation = degrees;
            return *this;
        }

        EntityBuilder& ZIndex(float z)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).zIndex = z;
            return *this;
        }

        Entity& Build()
        {
            return m_Entity;
        }

        operator Entity&()
        {
            return m_Entity;
        }

    private:
        entt::registry& m_Registry;
        Entity m_Entity;
    };

    class Scene
    {
    public:
        Scene(const std::string &name = "untitled");
        ~Scene() = default;

        void DestroyEntity(Entity entity);

        void OnUpdate(float dt);
        void OnRender();
        void OnRender(const glm::mat4 &view, const glm::mat4 &projection);
        void OnEvent();

        void UpdateTransforms();
        void UpdateParticles(float dt);

        bool LoadSceneFile(const std::string &path);
        void RequestLoad(const std::string &path, float fade = 0.0f, const std::string &shader = std::string());
        void RequestReload(float fade = 0.0f, const std::string &shader = std::string());
        bool IsTransitioning() const { return m_Transition.Phase != TransitionPhase::None || m_Transition.Requested; }
        float GetFadeAlpha() const;
        bool IsMouseOverUI() const { return uiSystem.IsMouseOverUI(); }
        UISystem &GetUI() { return uiSystem; }
        const std::string &GetSourcePath() const { return m_SourcePath; }
        void SetSourcePath(const std::string &path) { m_SourcePath = path; }
        glm::vec2 ScreenToWorld(const glm::vec2 &viewportPixel);

        const glm::vec4 &GetBackgroundColor() const { return m_BackgroundColor; }
        void SetBackgroundColor(const glm::vec4 &color) { m_BackgroundColor = color; }

        SceneLighting &GetLighting() { return m_Lighting; }
        const SceneLighting &GetLighting() const { return m_Lighting; }
        BloomSettings &GetBloom() { return m_Bloom; }
        const BloomSettings &GetBloom() const { return m_Bloom; }
        void SetLightingVisible(bool visible) { m_LightingVisible = visible; }
        void StopAudio();
        MusicSystem &GetMusicSystem() { return musicSystem; }

        void SendEvent(entt::entity target, ScriptEvent type, entt::entity other = entt::null, float value = 0.0f);

        float GetTime() const { return m_Time; }
        float GetDeltaTime() const { return m_LastDelta; }
        float GetFixedTimeStep() const { return m_FixedStep; }
        void SetFixedTimeStep(float seconds);

        void Clear();

        TweenSystem &GetTweens() { return m_Tweens; }

        Entity CreateEntity(const std::string &name = "");
        EntityBuilder BuildEntity(const std::string &name = "");
        Entity GetEntityByName(const std::string &name);

        entt::registry &GetRegistry()
        {
            return m_Registry;
        }
        const std::string &GetName() const { return m_Name; }
        void SetName(std::string name) { m_Name = name; }

        Camera2D &GetMainCamera() { return cameraSystem.GetMainCamera(); }

        Camera2D &GetActiveCamera()
        {
            return cameraSystem.GetActiveCamera(m_Registry);
        }

        AccelEngine::World &GetPhysicsWorld()
        {
            return physicsSystem.GetWorld();
        }
        PhysicsSystem &GetPhysicsSystem()
        {
            return physicsSystem;
        }

        void SetParent(entt::entity child, entt::entity parent);

        void MarkPhysicsDirty() { m_PhysicsDirty = true; }
        void SyncPhysicsFromTransforms();
        PhysicsComponent &AddBody(entt::entity entity, std::unique_ptr<AccelEngine::RigidBody> body);
        void RemoveBody(entt::entity entity);

    private:
        void RenderContent(bool screenSpaceUI);
        void ApplyParallax();

        std::string m_Name;
        glm::vec4 m_BackgroundColor{0.329f, 0.329f, 0.329f, 1.0f};
        bool m_PhysicsDirty = false;
        SceneLighting m_Lighting;
        bool m_LightingVisible = true;
        BloomSettings m_Bloom;
        entt::registry m_Registry;

        MovementSystem movementSystem;
        CollisionSystem collisionSystem;
        CameraSystem cameraSystem;
        HierarchySystem hierarchySystem;
        LifetimeSystem lifetimeSystem;
        PhysicsSystem physicsSystem;
        AnimationSystem animationSystem;
        RenderSystem renderSystem;
        TilemapSystem tilemapSystem;
        ParticleSystem particleSystem;
        LightingSystem lightingSystem;
        AudioSystem audioSystem;
        BloomRenderer bloomRenderer;
        GlowSystem glowSystem;
        TriggerSystem triggerSystem;
        TimerSystem timerSystem;
        PathFollowerSystem pathFollowerSystem;
        HealthSystem healthSystem;
        VectorSystem vectorSystem;
        CharacterControllerSystem characterControllerSystem;
        DamageSystem damageSystem;
        OscillatorSystem oscillatorSystem;
        SpawnerSystem spawnerSystem;
        FollowSystem followSystem;
        PickupSystem pickupSystem;
        RespawnSystem respawnSystem;
        MusicSystem musicSystem;
        PickableSystem pickableSystem;
        ButtonSystem buttonSystem;
        UISystem uiSystem;
        ShaderCanvasSystem shaderCanvasSystem;
        TeleporterSystem teleporterSystem;
        CameraZoneSystem cameraZoneSystem;
        BoundsSystem boundsSystem;
        TweenSystem m_Tweens{*this};

        enum class TransitionPhase
        {
            None,
            Out,
            In
        };

        struct Transition
        {
            TransitionPhase Phase = TransitionPhase::None;
            bool Requested = false;
            std::string RequestedPath;
            std::string RequestedShader;
            float RequestedFade = 0.0f;
            std::string Shader;
            std::string ActivePath;
            float Duration = 0.0f;
            float Time = 0.0f;
        };

        void UpdateTransition(float dt);
        void RenderFade();

        Transition m_Transition;
        ShaderCanvasComponent m_TransitionCanvas;
        std::string m_SourcePath;

        std::vector<entt::entity> m_ScriptOrder;
        float m_Time = 0.0f;
        float m_LastDelta = 0.0f;
        float m_FixedStep = 1.0f / 60.0f;
        float m_FixedAccumulator = 0.0f;

        void OnPhysicsComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnHierarchyComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnScriptComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnAudioSourceDestroyed(entt::registry &registry, entt::entity entity);
    };
}

#include <Engine/Scene/Entity-inl.h>

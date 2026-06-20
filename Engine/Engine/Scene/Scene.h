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

#include <AccelEngine/world.h>
#include <AccelEngine/ForceRegistry.h>

#include <entt/entt.hpp>
#include <string>

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

        EntityBuilder& Rotation(float radians)
        {
            m_Registry.get<TransformComponent>(m_Entity.GetHandle()).rotation = radians;
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
        void OnEvent();

        void Clear();

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


        
        // temp

    private:
        std::string m_Name;
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

        // 
        void OnPhysicsComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnAnimatedSpriteComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnHierarchyComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnScriptComponentDestroyed(entt::registry &registry, entt::entity entity);
    };
}

#pragma once

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

#include <AccelEngine/world.h>
#include <AccelEngine/ForceRegistry.h>

#include <entt/entt.hpp>
#include <string>

namespace SIMPEngine
{
    class Entity;
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

        // 
        void OnPhysicsComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnAnimatedSpriteComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnHierarchyComponentDestroyed(entt::registry &registry, entt::entity entity);
        void OnScriptComponentDestroyed(entt::registry &registry, entt::entity entity);
    };
}

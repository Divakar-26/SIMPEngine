#pragma once

#include <Engine/Scene/Component.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Scene/Systems/MovementSystem.h>
#include <Engine/Scene/Systems/CollisionSystem.h>
#include <Engine/Scene/Systems/CameraSystem.h>

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

        Entity CreatePlayer();
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

        // helper functions
        void RenderQuad();
        void RenderSprites();
        void RenderColliders();

        // temp

        AccelEngine::World physicsWorld;
        std::vector<AccelEngine::RigidBody *> bodies;
        AccelEngine::ForceRegistry physicsRegistry;
    private:
        std::string m_Name;
        entt::registry m_Registry;

        MovementSystem movementSystem;
        CollisionSystem collisionSystem;
        CameraSystem cameraSystem;
    };
}
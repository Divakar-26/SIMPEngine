#pragma once

#include <entt/entt.hpp>
#include "Component.h"
#include "Math/Camera2D.h"
#include <string>
#include "Systems/MovementSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/CameraSystem.h"

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

    private:
        std::string m_Name;
        entt::registry m_Registry;

        MovementSystem movementSystem;
        CollisionSystem collisionSystem;
        CameraSystem cameraSystem;
    };
}
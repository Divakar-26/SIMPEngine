#pragma once

#include <entt/entt.hpp>
#include "Component.h"
#include "Math/Camera2D.h"
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

        Camera2D &GetMainCamera() { return m_MainCamera; }
        void SetMainCamera(const Camera2D &camera) { m_MainCamera = camera; }

        bool HasActiveCamera();
        Camera2D &GetActiveCamera();

    private:
        std::string m_Name;
        entt::registry m_Registry;

        Camera2D m_MainCamera;
        bool useMainCamera = true;

    };
}
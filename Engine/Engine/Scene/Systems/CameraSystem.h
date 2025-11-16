#pragma once
#include <Engine/Math/Camera2D.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class CameraSystem
    {
    private:
        Camera2D m_MainCamera;
        bool useMainCamera = true;

    public:
        void OnUpdate(entt::registry &registry, float dt);
        void OnRender(entt::registry &registry);

        bool HasActiveCamera(entt::registry &registry);
        Camera2D &GetActiveCamera(entt::registry &registry);
        Camera2D *GetActiveCameraPtr(entt::registry &registry);

        Camera2D &GetMainCamera() { return m_MainCamera; }
    };
}


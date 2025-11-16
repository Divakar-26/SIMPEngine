#include <Engine/Scene/Systems/CameraSystem.h>
#include <Engine/Core/Log.h>

namespace SIMPEngine
{

    void CameraSystem::OnUpdate(entt::registry &registry, float dt)
    {
        auto cameraView = registry.view<CameraComponent, TransformComponent>();
        for (auto entity : cameraView)
        {
            auto &camComp = cameraView.get<CameraComponent>(entity);
            auto &transform = cameraView.get<TransformComponent>(entity);

            if (camComp.primary)
            {      
            
                auto w = camComp.Camera.GetViewportSize();
                camComp.Camera.SetPosition({transform.position.x , transform.position.y});
                camComp.Camera.Update(dt);
                break;
            }
        }
        
        m_MainCamera.Update(dt);
    }

    void CameraSystem::OnRender(entt::registry &registry)
    {
        Camera2D *activeCamera = GetActiveCameraPtr(registry);

        if (activeCamera)
        {
            Renderer::SetViewMatrix(activeCamera->GetViewMatrix());
        }
    }

    bool CameraSystem::HasActiveCamera(entt::registry &registry)
    {
        for (auto entity : registry.view<CameraComponent, TransformComponent>())
        {
            auto &cameraComp = registry.get<CameraComponent>(entity);
            if (cameraComp.primary)
                return true;
        }
        return false;
    }

    Camera2D &CameraSystem::GetActiveCamera(entt::registry &registry)
    {
        auto view = registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto &camComp = view.get<CameraComponent>(entity);
            if (camComp.primary)
            {
                return camComp.Camera;
            }
        }

        return m_MainCamera;
    }

    Camera2D *CameraSystem::GetActiveCameraPtr(entt::registry &registry)
    {
        Camera2D *activeCamera = nullptr;
        auto view = registry.view<CameraComponent>();
        for (auto entity : view)
        {
            auto &camComp = view.get<CameraComponent>(entity);
            if (camComp.primary)
            {
                activeCamera = &camComp.Camera;
                break;
            }
        }
        return activeCamera ? activeCamera : &m_MainCamera;
    }
}
#include <Engine/Scene/Systems/CameraSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Math/EntityUtility.h>

#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        glm::vec2 CurrentWorldPosition(entt::registry &registry, entt::entity entity)
        {
            glm::mat4 world = registry.get<TransformComponent>(entity).GetLocalTransform();
            entt::entity parent = entt::null;
            if (const auto *hierarchy = registry.try_get<HierarchyComponent>(entity))
                parent = hierarchy->parent;
            for (int depth = 0; depth < 64 && parent != entt::null && registry.valid(parent); ++depth)
            {
                const auto *transform = registry.try_get<TransformComponent>(parent);
                if (!transform)
                    break;
                world = transform->GetLocalTransform() * world;
                const auto *hierarchy = registry.try_get<HierarchyComponent>(parent);
                parent = hierarchy ? hierarchy->parent : entt::null;
            }
            return {world[3][0], world[3][1]};
        }

        void Follow(entt::registry &registry, CameraComponent &cam, TransformComponent &transform, float dt)
        {
            if (cam.followTarget.empty())
                return;

            entt::entity target = cam.followHandle;
            bool valid = target != entt::null && registry.valid(target) &&
                         registry.all_of<TagComponent, TransformComponent>(target) &&
                         registry.get<TagComponent>(target).Tag == cam.followTarget;
            if (!valid)
            {
                target = entt::null;
                for (auto e : registry.view<TagComponent, TransformComponent>())
                {
                    if (registry.get<TagComponent>(e).Tag == cam.followTarget)
                    {
                        target = e;
                        break;
                    }
                }
                cam.followHandle = target;
            }
            if (target == entt::null)
                return;

            glm::vec2 goal = CurrentWorldPosition(registry, target) + cam.followOffset;
            glm::vec2 current = transform.position;
            glm::vec2 delta = goal - current;
            glm::vec2 half = glm::max(cam.deadZone, glm::vec2(0.0f)) * 0.5f;
            for (int i = 0; i < 2; ++i)
            {
                if (std::abs(delta[i]) <= half[i])
                    delta[i] = 0.0f;
                else
                    delta[i] -= delta[i] > 0.0f ? half[i] : -half[i];
            }

            if (cam.followSmoothing > 0.0f)
                current += delta * (1.0f - std::exp(-cam.followSmoothing * dt));
            else
                current += delta;

            if (cam.useBounds)
            {
                float left, right, top, bottom;
                cam.Camera.GetVisibleWorldBounds(left, right, top, bottom);
                glm::vec2 extent(std::abs(right - left) * 0.5f, std::abs(top - bottom) * 0.5f);
                for (int i = 0; i < 2; ++i)
                {
                    float lo = cam.boundsMin[i] + extent[i];
                    float hi = cam.boundsMax[i] - extent[i];
                    current[i] = lo > hi ? (cam.boundsMin[i] + cam.boundsMax[i]) * 0.5f : std::clamp(current[i], lo, hi);
                }
            }

            transform.position = current;
            transform.dirty = true;
        }
    }

    void CameraSystem::OnUpdate(entt::registry &registry, float dt)
    {
        m_Time += dt;
        auto cameraView = registry.view<CameraComponent, TransformComponent>();
        for (auto entity : cameraView)
        {
            auto &camComp = cameraView.get<CameraComponent>(entity);
            auto &transform = cameraView.get<TransformComponent>(entity);

            if (camComp.primary)
            {
                Follow(registry, camComp, transform, dt);

                glm::vec2 shake(0.0f);
                if (camComp.trauma > 0.0f)
                {
                    float amount = camComp.shakeStrength * camComp.trauma * camComp.trauma;
                    shake = amount * glm::vec2(std::sin(m_Time * 47.0f) * 0.6f + std::sin(m_Time * 83.0f) * 0.4f,
                                               std::cos(m_Time * 53.0f) * 0.6f + std::sin(m_Time * 71.0f) * 0.4f);
                    camComp.trauma = std::max(0.0f, camComp.trauma - dt * 1.5f);
                }

                camComp.Camera.SetPosition(transform.position + shake);
                camComp.Camera.Update(dt);
                break;
            }
        }

        m_MainCamera.Update(dt);
    }

    void CameraSystem::OnRender(entt::registry &registry)
    {
        Camera2D *activeCamera = GetActiveCameraPtr(registry);

        Renderer::GetAPI()->SetProjectionMatrix(activeCamera->GetProjectionMatrix());

        Renderer::SetViewMatrix(
            activeCamera->GetViewMatrix());
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

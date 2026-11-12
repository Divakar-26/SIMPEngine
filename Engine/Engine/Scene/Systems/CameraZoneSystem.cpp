#include "CameraZoneSystem.h"

#include <Engine/Scene/Scene.h>

#include <cmath>

namespace SIMPEngine
{
    void CameraZoneSystem::Clear()
    {
        *this = CameraZoneSystem();
    }

    void CameraZoneSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();

        CameraComponent *camera = nullptr;
        for (entt::entity handle : registry.view<CameraComponent>())
        {
            CameraComponent &candidate = registry.get<CameraComponent>(handle);
            if (candidate.primary)
            {
                camera = &candidate;
                break;
            }
        }
        if (!camera)
            return;

        entt::entity zone = entt::null;
        auto zones = registry.view<CameraZoneComponent, TriggerComponent>();
        auto occupied = [&](entt::entity handle)
        {
            return registry.valid(handle) && registry.all_of<CameraZoneComponent, TriggerComponent>(handle) &&
                   registry.get<CameraZoneComponent>(handle).enabled && !registry.get<TriggerComponent>(handle).inside.empty();
        };
        if (occupied(m_Zone))
            zone = m_Zone;
        else
            for (entt::entity handle : zones)
                if (occupied(handle))
                {
                    zone = handle;
                    break;
                }

        if (zone != entt::null)
        {
            const CameraZoneComponent &settings = registry.get<CameraZoneComponent>(zone);
            if (!m_Active)
            {
                m_Active = true;
                m_BaseZoom = camera->Camera.GetZoom();
                m_BaseUseBounds = camera->useBounds;
                m_BaseMin = camera->boundsMin;
                m_BaseMax = camera->boundsMax;
            }
            m_Zone = zone;
            m_Restore = settings.restoreOnExit;
            m_TargetZoom = settings.zoom;
            m_Speed = settings.speed;
            if (settings.setBounds)
            {
                camera->useBounds = true;
                camera->boundsMin = settings.boundsMin;
                camera->boundsMax = settings.boundsMax;
            }
        }
        else if (m_Active)
        {
            m_Zone = entt::null;
            if (!m_Restore)
            {
                m_Active = false;
                return;
            }
            m_TargetZoom = m_BaseZoom;
            camera->useBounds = m_BaseUseBounds;
            camera->boundsMin = m_BaseMin;
            camera->boundsMax = m_BaseMax;
        }
        else
            return;

        const float current = camera->Camera.GetZoom();
        const float blend = m_Speed <= 0.0f ? 1.0f : 1.0f - std::exp(-m_Speed * dt);
        float next = current + (m_TargetZoom - current) * blend;
        if (std::abs(next - m_TargetZoom) < 0.0005f)
            next = m_TargetZoom;
        if (next != current)
            camera->Camera.SetZoom(next);
        if (m_Zone == entt::null && next == m_TargetZoom)
            m_Active = false;
    }
}

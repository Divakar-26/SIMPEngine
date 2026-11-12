#pragma once
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class Scene;

    class CameraZoneSystem
    {
    public:
        void Update(Scene &scene, float dt);
        void Clear();

    private:
        bool m_Active = false;
        bool m_Restore = true;
        float m_BaseZoom = 1.0f;
        bool m_BaseUseBounds = false;
        glm::vec2 m_BaseMin{0.0f};
        glm::vec2 m_BaseMax{0.0f};
        entt::entity m_Zone = entt::null;
        float m_TargetZoom = 1.0f;
        float m_Speed = 4.0f;
    };
}

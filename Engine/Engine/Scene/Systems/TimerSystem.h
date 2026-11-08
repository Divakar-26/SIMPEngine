#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>
#include <vector>

namespace SIMPEngine
{
    class Scene;

    class TimerSystem
    {
    public:
        void Update(Scene &scene, float dt);

    private:
        std::vector<entt::entity> m_Fired;
    };
}

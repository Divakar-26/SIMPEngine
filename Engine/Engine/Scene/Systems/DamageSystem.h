#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>
#include <vector>

namespace SIMPEngine
{
    class Scene;

    class DamageSystem
    {
    public:
        void Update(Scene &scene, float dt);

    private:
        struct Hit
        {
            entt::entity Source = entt::null;
            entt::entity Target = entt::null;
        };

        std::vector<Hit> m_Hits;
    };
}

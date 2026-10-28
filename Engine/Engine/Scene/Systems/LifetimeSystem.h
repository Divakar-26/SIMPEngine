#pragma once
#include <entt/entt.hpp>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class LifetimeSystem
    {
    public:
        void DestroyRecursive(entt::registry &reg, entt::entity e);

        void Update(entt::registry &reg, float dt);
    };
}
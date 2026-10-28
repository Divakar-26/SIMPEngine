#pragma once
#include <Engine/Scene/Component.h>
#include <entt/entt.hpp>
#include <cmath>

namespace SIMPEngine
{
    class MovementSystem
    {
    public:
        void Update(entt::registry &r, float dt);
    };

}
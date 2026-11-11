#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class OscillatorSystem
    {
    public:
        void Update(entt::registry &registry, float dt);
    };
}

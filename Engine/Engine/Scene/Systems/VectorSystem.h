#pragma once

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class VectorSystem
    {
    public:
        void Render(entt::registry &registry);

        static void DrawVector(entt::registry &registry, entt::entity entity);
    };
}

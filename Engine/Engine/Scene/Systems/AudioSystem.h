#pragma once

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class AudioSystem
    {
    public:
        void Update(entt::registry &registry);
        static void StopSource(entt::registry &registry, entt::entity entity);
    };
}

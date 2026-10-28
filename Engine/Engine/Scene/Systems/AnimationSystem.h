#pragma once

#include <entt/entt.hpp>
#include <Engine/Scene/Component.h>
#include <Engine/Core/Profiler/EngineProfiler.h>

namespace SIMPEngine
{
    class AnimationSystem
    {
    public:
        void Update(entt::registry &m_Registry, float deltaTime);

    private:
    };
}
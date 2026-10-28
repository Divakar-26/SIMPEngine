#pragma once
#include <Engine/Scene/Component.h>
#include <entt/entt.hpp>
#include <Engine/Core/Profiler/EngineProfiler.h>

namespace SIMPEngine
{

    class HierarchySystem
    {

    public:
        void Update(entt::registry &registry, float deltaTime = 0.0f);
    };
}
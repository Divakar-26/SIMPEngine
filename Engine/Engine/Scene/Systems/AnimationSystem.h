#pragma once

#include <entt/entt.hpp>
#include <Engine/Scene/Component.h>
#include <Engine/Core/Profiler/EngineProfiler.h>

namespace SIMPEngine
{
    class AnimationSystem
    {
    public:
        void Update(entt::registry &m_Registry, float deltaTime)
        {
            PROFILE_FUNCTION();

            auto animView = m_Registry.view<AnimatedSpriteComponent>();
            for (auto entity : animView)
            {
                auto &anim = animView.get<AnimatedSpriteComponent>(entity);
                if (anim.controller)
                    anim.controller->Update(deltaTime);
            }
        }

    private:
    };
}
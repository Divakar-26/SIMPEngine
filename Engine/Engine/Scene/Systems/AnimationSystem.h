#pragma once

#include <entt/entt.hpp>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class AnimationSystem
    {
    public:
        void Update(entt::registry &m_Registry, float deltaTime)
        {
            auto animView = m_Registry.view<AnimatedSpriteComponent>();
            for (auto entity : animView)
            {
                auto &anim = animView.get<AnimatedSpriteComponent>(entity);
                if (anim.animation)
                    anim.animation->Update(deltaTime);
            }
        }

    private:
    };
}
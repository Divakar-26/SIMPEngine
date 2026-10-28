//
// Created by divakar on 9/14/26.
//

#include "AnimationSystem.h"

void SIMPEngine::AnimationSystem::Update(entt::registry &m_Registry, float deltaTime)
{
    PROFILE_FUNCTION();

    auto animView = m_Registry.view<AnimatedSpriteComponent>();
    for (auto entity : animView)
    {
        auto &anim = animView.get<AnimatedSpriteComponent>(entity);
        if (anim.animation)
            anim.animation->Update(deltaTime);
        if (anim.controller)
            anim.controller->Update(deltaTime);
    }
}
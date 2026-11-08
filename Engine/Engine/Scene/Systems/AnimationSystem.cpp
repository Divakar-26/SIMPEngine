#include "AnimationSystem.h"

void SIMPEngine::AnimationSystem::Update(entt::registry &m_Registry, float deltaTime)
{
    PROFILE_FUNCTION();

    auto animView = m_Registry.view<AnimatedSpriteComponent>();
    for (auto entity : animView)
    {
        auto &anim = animView.get<AnimatedSpriteComponent>(entity);
        if (!anim.playing || anim.speed <= 0.0f)
            continue;
        if (anim.animation)
            anim.animation->Update(deltaTime * anim.speed);
        if (anim.controller)
            anim.controller->Update(deltaTime * anim.speed);
    }
}

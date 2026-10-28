// Scene/Systems/CollisionSystem.h
#pragma once
#include <Engine/Scene/Component.h>

#include <SDL3/SDL_rect.h>
#include <entt/entt.hpp>
#include <cmath>

namespace SIMPEngine
{

    class CollisionSystem
    {
    public:
        void Update(entt::registry &r, float dt);
    };

}
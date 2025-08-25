#pragma once
#include <entt/entt.hpp>
#include "Scene/Component.h"

class MovementSystem
{
public:
    void Update(entt::registry &r, float dt)
    {
        auto view = r.view<TransformComponent, VelocityComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &velocity = view.get<VelocityComponent>(entity);

            transform.position.x += velocity.vx * dt;
            transform.position.y += velocity.vy * dt;
        }
    }
};
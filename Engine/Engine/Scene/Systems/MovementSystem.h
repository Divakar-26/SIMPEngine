#pragma once
#include <Engine/Scene/Component.h>
#include <entt/entt.hpp>
#include <cmath>

namespace SIMPEngine
{
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

                transform.rotation = std::fmod(transform.rotation, 360.0f);

                if (transform.rotation < 0.0f)
                    transform.rotation += 360.0f;
            }
        }
    };

}
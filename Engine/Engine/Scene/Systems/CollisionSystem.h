// Scene/Systems/CollisionSystem.h
#pragma once
#include <Engine/Scene/Component.h>

#include <SDL3/SDL_rect.h>
#include <entt/entt.hpp>
#include <cmath>

class CollisionSystem
{
public:
    void Update(entt::registry &r, float dt)
    {
        auto collidable = r.view<TransformComponent, CollisionComponent>();

        for (auto entityA : collidable)
        {
            auto &aTransform = collidable.get<TransformComponent>(entityA);
            auto &aCollision = collidable.get<CollisionComponent>(entityA);


            SDL_FRect aRect = aCollision.GetBoundsWorld(aTransform);

            for (auto entityB : collidable)
            {
                if (entityA == entityB)
                    continue;

                auto &bTransform = collidable.get<TransformComponent>(entityB);
                auto &bCollision = collidable.get<CollisionComponent>(entityB);


                SDL_FRect bRect = bCollision.GetBoundsWorld(bTransform);

                if (SDL_HasRectIntersectionFloat(&aRect, &bRect))
                {
                    
                }
            }
        }
    }
};
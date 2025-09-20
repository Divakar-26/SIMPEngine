// Scene/Systems/CollisionSystem.h
#pragma once
#include <entt/entt.hpp>
#include <SDL3/SDL_rect.h>
#include "Scene/Component.h"
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
            
            if (!aCollision.enabled) continue;
            
            if (aCollision.isTrigger) continue;
            
            SDL_FRect aRect = aCollision.GetBoundsWorld(aTransform);

            for (auto entityB : collidable)
            {
                if (entityA == entityB)
                    continue;

                auto &bTransform = collidable.get<TransformComponent>(entityB);
                auto &bCollision = collidable.get<CollisionComponent>(entityB);
                
                if (!bCollision.enabled) continue;
                
                SDL_FRect bRect = bCollision.GetBoundsWorld(bTransform);

                if (SDL_HasRectIntersectionFloat(&aRect, &bRect))
                {
                    float dx = (aRect.x + aRect.w / 2.0f) - (bRect.x + bRect.w / 2.0f);
                    float px = (aRect.w / 2.0f + bRect.w / 2.0f) - std::abs(dx);

                    float dy = (aRect.y + aRect.h / 2.0f) - (bRect.y + bRect.h / 2.0f);
                    float py = (aRect.h / 2.0f + bRect.h / 2.0f) - std::abs(dy);

                    if (px < py)
                    {
                        if (dx > 0) aTransform.position.x += px;
                        else        aTransform.position.x -= px;

                        if (r.any_of<VelocityComponent>(entityA))
                            r.get<VelocityComponent>(entityA).vx = 0.0f;
                    }
                    else
                    {
                        if (dy > 0) aTransform.position.y += py;
                        else        aTransform.position.y -= py;

                        if (r.any_of<VelocityComponent>(entityA))
                            r.get<VelocityComponent>(entityA).vy = 0.0f;
                    }

                    aRect = aCollision.GetBoundsWorld(aTransform);
                    
                    // TODO: Add collision events/triggers here
                }
            }
        }
    }
};
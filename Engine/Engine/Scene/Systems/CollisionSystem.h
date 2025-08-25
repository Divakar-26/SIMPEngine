#pragma once
#include <entt/entt.hpp>
#include "Scene/Component.h"

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
            SDL_FRect aRect = aCollision.GetBounds(aTransform.position.x, aTransform.position.y);
            
            for (auto entityB : collidable)
            {
                if (entityA == entityB)
                continue;
                
                auto &bTransform = collidable.get<TransformComponent>(entityB);
                auto &bCollision = collidable.get<CollisionComponent>(entityB);
                SDL_FRect bRect = bCollision.GetBounds(bTransform.position.x, bTransform.position.y);
                
                if (SDL_HasRectIntersectionFloat(&aRect, &bRect))
                {
                    
                }
            }
        }
    }
};
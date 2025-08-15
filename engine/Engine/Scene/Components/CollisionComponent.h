#pragma once
#include <SDL3/SDL_rect.h>

struct CollisionComponent
{
    float width, height;
    SDL_FRect GetBounds(float x, float y) const
    {
        return SDL_FRect{x, y, width, height};
    }
    SDL_FRect GetBoundsWorld(const TransformComponent &transform) const
    {
        return GetBounds(transform.x, transform.y);
    }
};

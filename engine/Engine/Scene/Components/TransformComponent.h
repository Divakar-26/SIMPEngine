#pragma once
#include<SDL3/SDL_pixels.h>

struct TransformComponent
{
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct RenderComponent
{
    float width, height;
    SDL_Color color;
};

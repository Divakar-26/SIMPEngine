#pragma once
#include <SDL3/SDL_rect.h>
#include "Rendering/Texture.h"
#include "Rendering/Sprite.h"
#include "Math/Camera2D.h"
#include <memory>
#include <string>

struct TransformComponent
{
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

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

struct SpriteComponent
{
    std::shared_ptr<SIMPEngine::Texture> texture = nullptr;
    float width;
    float height;
};

struct TagComponent
{
    std::string Tag;
};

struct RenderComponent
{
    float width, height;
    SDL_Color color = {255,255,255,255};
};

struct VelocityComponent
{
    float vx = 0.0f;
    float vy = 0.0f;
};

struct CameraComponent
{
    SIMPEngine::Camera2D Camera;
    bool primary = false;

    CameraComponent(float zoom = 1.0f, glm::vec2 position = {0.0f, 0.0f})
        : Camera(zoom, position), primary(false) {}
};

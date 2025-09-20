#pragma once
#include <SDL3/SDL_rect.h>
#include "Rendering/Texture.h"
#include "Rendering/Sprite.h"
#include "Math/Camera2D.h"
#include <memory>
#include <string>

namespace SIMPEngine
{
    class ScriptableEntity;
}

struct TransformComponent
{
    glm::vec2 position{0.0f, 0.0f};
    float rotation = 0.0f;
    glm::vec2 scale{1.0f, 1.0f};
    float zIndex = 0.0f;

    glm::mat4 GetTransform() const
    {
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(position, zIndex));
        transform = glm::rotate(transform, glm::radians(rotation), {0, 0, 1});
        transform = glm::scale(transform, glm::vec3(scale, 1.0f));
        return transform;
    }
};

struct CollisionComponent
{
    float width = 0.0f, height = 0.0f;
    float offsetX = 0.0f, offsetY = 0.0f;
    SDL_FRect GetBounds(float x, float y) const
    {
        return SDL_FRect{x + offsetX, y + offsetY, width, height};
    }
    SDL_FRect GetBoundsWorld(const TransformComponent &transform) const
    {
        return GetBounds(transform.position.x, transform.position.y);
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
    SDL_Color color = {255, 255, 255, 255};
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

struct ScriptComponent
{
    SIMPEngine::ScriptableEntity *Instance = nullptr;

    std::function<SIMPEngine::ScriptableEntity *()> InstantiateScript;
    std::function<void(ScriptComponent *)> DestroyScript;

    template <typename T>
    void Bind()
    {
        InstantiateScript = []()
        { return new T(); };
        DestroyScript = [](ScriptComponent *sc)
        { delete sc->Instance; sc->Instance = nullptr; };
    }
};


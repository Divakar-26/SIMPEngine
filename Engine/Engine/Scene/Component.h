#pragma once
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Sprite.h>
#include <Engine/Rendering/Animation.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Math/Camera2D.h>

#include <AccelEngine/world.h>

#include <SDL3/SDL_rect.h>
#include <memory>
#include <vector>
#include <string>

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class ScriptableEntity;
}

struct TransformComponent
{
    glm::vec2 position{0, 0};
    float rotation = 0;
    glm::vec2 scale{1, 1};
    float zIndex = 0;

    glm::mat4 worldTransform;

    bool dirty = true;

    glm::mat4 GetLocalTransform() const
    {
        glm::mat4 t = glm::translate(glm::mat4(1), {position, zIndex});
        t = glm::rotate(t, glm::radians(rotation), {0, 0, 1});
        t = glm::scale(t, {scale, 1});
        return t;
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
    float width = 1.0f;
    float height = 1.0f;
};

struct TagComponent
{
    std::string Tag;
};

struct RenderComponent
{
    float width = 1.0f, height = 1.0f;
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

struct PhysicsComponent
{
    AccelEngine::RigidBody *body = nullptr;
};

struct AnimatedSpriteComponent
{
    SIMPEngine::Animation *animation = nullptr;
};

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct LifetimeComponent
{
    float remaining = 1.0f;
};

struct TilemapComponent
{
    std::shared_ptr<SIMPEngine::Tileset> tileset;

    int width = 0;
    int height = 0;

    float tileSize = 32.0f; // world size

    std::vector<int> tiles; // tileIDs

    int Get(int x, int y) const
    {
        return tiles[y * width + x]; // our grid
    }
};
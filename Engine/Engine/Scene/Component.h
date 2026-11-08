#pragma once
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Sprite.h>
#include <Engine/Rendering/Animation.h>
#include <Engine/Rendering/AnimationController.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Particles/ParticleEffect.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Vector/VectorPath.h>
#include <Engine/Vector/VectorTessellator.h>

#include <AccelEngine/world.h>

#include <SDL3/SDL_rect.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cstdint>

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class ScriptableEntity;
    struct VectorShapeCache;
}

struct TransformComponent
{
    glm::vec2 position{0, 0};
    float rotation = 0;
    glm::vec2 scale{1, 1};
    float zIndex = 0;

    glm::mat4 worldTransform{1.0f};

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
    SDL_Color tint{255, 255, 255, 255};
    bool flipX = false;
    bool flipY = false;
    bool useRegion = false;
    SDL_FRect region{0.0f, 0.0f, 0.0f, 0.0f};
};

struct TextComponent
{
    std::string Text = "Text";
    float Size = 32.0f;
    SDL_Color Color{255, 255, 255, 255};
    int Align = 1;
    std::string FontPath;
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

    std::string followTarget;
    glm::vec2 followOffset{0.0f, 0.0f};
    float followSmoothing = 6.0f;
    glm::vec2 deadZone{0.0f, 0.0f};
    bool useBounds = false;
    glm::vec2 boundsMin{-1000.0f, -1000.0f};
    glm::vec2 boundsMax{1000.0f, 1000.0f};
    float shakeStrength = 16.0f;

    float trauma = 0.0f;
    entt::entity followHandle = entt::null;

    CameraComponent(float zoom = 1.0f, glm::vec2 position = {0.0f, 0.0f})
        : Camera(zoom, position), primary(false) {}

    void Shake(float amount) { trauma = std::min(1.0f, trauma + amount); }
};

struct ScriptComponent
{
    std::unique_ptr<SIMPEngine::ScriptableEntity> Instance;

    std::function<std::unique_ptr<SIMPEngine::ScriptableEntity>()> InstantiateScript;

    template <typename T>
    void Bind()
    {
        InstantiateScript = []() -> std::unique_ptr<SIMPEngine::ScriptableEntity>
        { return std::make_unique<T>(); };
    }

    std::string LuaPath;

    ScriptComponent();
    ~ScriptComponent();
    ScriptComponent(ScriptComponent &&) noexcept;
    ScriptComponent &operator=(ScriptComponent &&) noexcept;

    void BindLua(const std::string &path);
    void ClearScript();
};

struct PhysicsComponent
{
    std::unique_ptr<AccelEngine::RigidBody> body;
};

struct AnimatedSpriteComponent
{
    std::unique_ptr<SIMPEngine::Animation> animation;

    std::unique_ptr<SIMPEngine::AnimationController> controller;

    bool playing = true;
    float speed = 1.0f;
};

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct LifetimeComponent
{
    float remaining = 10000.0f;
};

enum TileFlags : uint8_t
{
    TileFlipX = 1,
    TileFlipY = 2
};

struct Tile
{
    int id = -1;
    uint8_t flags = 0;
};

static constexpr int CHUNK_SIZE = 16;

struct TileChunk
{
    std::array<Tile, CHUNK_SIZE * CHUNK_SIZE> tiles;
};

struct TilemapComponent
{
    std::shared_ptr<SIMPEngine::Tileset> tileset;
    float tileSize = 32.0f;

    std::unordered_map<int64_t, TileChunk> chunks;

    SDL_Color tint{255, 255, 255, 255};
    bool visible = true;
    bool solid = false;
};

struct ParticleEmitterComponent
{
    SIMPEngine::ParticleEffect effect;
    bool playing = true;
    SIMPEngine::ParticleState state;
};

enum class LightType : uint8_t
{
    Point,
    Spot
};

struct LightComponent
{
    LightType type = LightType::Point;
    SDL_Color color{255, 214, 160, 255};
    float intensity = 1.0f;
    float radius = 240.0f;
    float falloff = 1.0f;
    float innerAngle = 20.0f;
    float outerAngle = 35.0f;
    float flicker = 0.0f;
    float flickerSpeed = 8.0f;
    bool enabled = true;
};

struct ParallaxComponent
{
    glm::vec2 factor{0.5f, 0.5f};
};

struct AudioSourceComponent
{
    std::string clip;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool playOnStart = true;

    uint32_t voice = 0;
    bool started = false;
};

struct GlowComponent
{
    SDL_Color color{255, 255, 255, 255};
    float intensity = 1.5f;
    float spread = 1.0f;
    bool enabled = true;
};

struct TriggerComponent
{
    float width = 64.0f;
    float height = 64.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    std::string filter;
    bool enabled = true;

    std::vector<entt::entity> inside;
};

struct TimerComponent
{
    float interval = 1.0f;
    bool repeat = true;
    bool running = true;

    float elapsed = 0.0f;
};

enum class PathMode : uint8_t
{
    Once,
    Loop,
    PingPong
};

struct PathFollowerComponent
{
    std::vector<glm::vec2> points;
    float speed = 100.0f;
    float waitTime = 0.0f;
    PathMode mode = PathMode::Loop;
    bool relative = true;
    bool playing = true;
    bool faceDirection = false;

    glm::vec2 origin{0.0f, 0.0f};
    bool originSet = false;
    int target = 0;
    int direction = 1;
    float waiting = 0.0f;
};

struct HealthComponent
{
    float max = 100.0f;
    float current = 100.0f;
    float invulnerability = 0.0f;
    bool destroyOnDeath = false;

    float invulnerableRemaining = 0.0f;
    bool dead = false;
};

struct VectorCacheHandle
{
    std::shared_ptr<SIMPEngine::VectorShapeCache> Ptr;

    VectorCacheHandle() = default;
    VectorCacheHandle(const VectorCacheHandle &) {}
    VectorCacheHandle &operator=(const VectorCacheHandle &) { return *this; }
    VectorCacheHandle(VectorCacheHandle &&) noexcept = default;
    VectorCacheHandle &operator=(VectorCacheHandle &&) noexcept = default;
};

struct VectorShapeComponent
{
    SIMPEngine::VectorPath path = SIMPEngine::VectorPath::Circle(50.0f);
    SIMPEngine::VectorPath morphTarget;
    float morph = 0.0f;

    bool fill = true;
    SDL_Color fillColor{90, 170, 255, 255};
    bool stroke = false;
    SDL_Color strokeColor{255, 255, 255, 255};
    float strokeWidth = 4.0f;
    SIMPEngine::StrokeCap cap = SIMPEngine::StrokeCap::Round;
    SIMPEngine::StrokeJoin join = SIMPEngine::StrokeJoin::Round;
    float miterLimit = 4.0f;

    float trimStart = 0.0f;
    float trimEnd = 1.0f;
    float trimOffset = 0.0f;

    float dashLength = 0.0f;
    float dashGap = 0.0f;
    float dashOffset = 0.0f;

    bool gradient = false;
    glm::vec2 gradientFrom{0.0f, 50.0f};
    glm::vec2 gradientTo{0.0f, -50.0f};
    SDL_Color gradientFromColor{255, 120, 200, 255};
    SDL_Color gradientToColor{90, 170, 255, 255};

    VectorCacheHandle cache;
};

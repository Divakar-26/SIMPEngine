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
#include <utility>
#include <functional>
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
    class ShaderEffect;
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

    glm::vec2 lastPosition{0.0f, 0.0f};
    float lastAngle = 0.0f;
    bool synced = false;
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
    int bus = 0;

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

enum class ControllerMode : uint8_t
{
    Platformer,
    TopDown
};

struct CharacterControllerComponent
{
    ControllerMode mode = ControllerMode::Platformer;
    bool playerInput = true;
    glm::vec2 size{32.0f, 48.0f};
    glm::vec2 offset{0.0f, 0.0f};

    float moveSpeed = 280.0f;
    float acceleration = 2600.0f;
    float deceleration = 3200.0f;
    float airControl = 0.65f;

    float gravity = 1900.0f;
    float maxFallSpeed = 1100.0f;
    float jumpSpeed = 680.0f;
    float jumpCut = 0.45f;
    float coyoteTime = 0.1f;
    float jumpBuffer = 0.12f;
    int maxJumps = 1;

    std::string solidTag;
    std::string oneWayTag = "OneWay";
    bool collideTilemaps = true;
    bool flipSprite = true;

    glm::vec2 velocity{0.0f, 0.0f};
    glm::vec2 scriptInput{0.0f, 0.0f};
    bool scriptJump = false;
    bool scriptRelease = false;
    bool grounded = false;
    bool justLanded = false;
    bool justJumped = false;
    bool hitWall = false;
    bool hitCeiling = false;
    float coyoteLeft = 0.0f;
    float bufferLeft = 0.0f;
    int jumpsUsed = 0;
    int facing = 1;
};

struct DamageComponent
{
    float amount = 10.0f;
    float interval = 0.5f;
    std::string filter;
    float knockback = 0.0f;
    bool destroyOnHit = false;
    bool enabled = true;

    std::vector<std::pair<entt::entity, float>> cooldowns;
};

struct OscillatorComponent
{
    glm::vec2 move{0.0f, 16.0f};
    float frequency = 0.5f;
    float phase = 0.0f;
    float swing = 0.0f;
    float spin = 0.0f;
    float pulse = 0.0f;
    bool playing = true;

    float time = 0.0f;
    glm::vec2 appliedMove{0.0f, 0.0f};
    float appliedSwing = 0.0f;
    float appliedPulse = 0.0f;
};

struct SpawnerComponent
{
    std::string prototype;
    float interval = 1.0f;
    int perSpawn = 1;
    int maxAlive = 10;
    int totalLimit = 0;
    glm::vec2 offset{0.0f, 0.0f};
    glm::vec2 spread{0.0f, 0.0f};
    glm::vec2 velocity{0.0f, 0.0f};
    float lifetime = 0.0f;
    bool hidePrototype = true;
    bool spawnOnStart = false;
    bool playing = true;

    float elapsed = 0.0f;
    int spawned = 0;
    int pending = 0;
    bool started = false;
    std::vector<entt::entity> alive;
};

struct FollowComponent
{
    std::string target;
    glm::vec2 offset{0.0f, 0.0f};
    float smoothing = 8.0f;
    float stopDistance = 0.0f;
    bool followX = true;
    bool followY = true;
    bool enabled = true;

    entt::entity handle = entt::null;
};

struct PickupComponent
{
    std::string key = "score";
    float amount = 1.0f;
    std::string sound;
    float volume = 1.0f;
    bool destroyOnPickup = true;
    bool enabled = true;

    bool collected = false;
};

struct CheckpointComponent
{
    glm::vec2 offset{0.0f, 0.0f};
    std::string sound;
    bool enabled = true;
};

struct RespawnComponent
{
    float delay = 1.0f;
    bool useKillY = false;
    float killY = -1000.0f;
    bool restoreHealth = true;

    glm::vec2 point{0.0f, 0.0f};
    bool pointSet = false;
    float timer = -1.0f;
};

struct MusicComponent
{
    std::vector<std::string> tracks;
    float volume = 1.0f;
    float fadeIn = 1.5f;
    float fadeOut = 1.5f;
    bool shuffle = false;
    bool loopPlaylist = true;
    bool playOnStart = true;
    bool onTrigger = false;
    bool stopOnExit = false;

    bool started = false;
    bool active = false;
    bool wasInside = false;
    int index = -1;
};

struct PickableComponent
{
    bool enabled = true;
    int button = 0;
    float smoothing = 0.0f;
    float throwStrength = 1.0f;
    float liftScale = 1.1f;
    bool bringToFront = true;
    bool keepOffset = true;
    float padding = 0.0f;

    bool held = false;
    glm::vec2 grabOffset{0.0f, 0.0f};
    glm::vec2 velocity{0.0f, 0.0f};
    glm::vec2 baseScale{1.0f, 1.0f};
    float baseZ = 0.0f;
};

struct ButtonComponent
{
    bool enabled = true;
    int button = 0;
    float hoverScale = 1.06f;
    float pressScale = 0.94f;
    std::string hoverSound;
    std::string clickSound;
    float volume = 1.0f;
    float padding = 0.0f;

    int state = 0;
    bool pressed = false;
    bool hovering = false;
    glm::vec2 baseScale{1.0f, 1.0f};
};

struct ShaderUniformValue
{
    std::string name;
    int type = 0;
    float value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float min = 0.0f;
    float max = 1.0f;
    std::string texturePath;
    std::shared_ptr<SIMPEngine::Texture> texture;
};

struct ShaderGroupMemberTag
{
};

struct ShaderCanvasComponent
{
    std::string shader;
    int scope = 0;
    float zMin = 0.0f;
    float zMax = 1.0f;
    int order = 0;
    float resolutionScale = 1.0f;
    float progress = 0.0f;
    bool enabled = true;
    bool limitArea = false;
    glm::vec2 areaSize{400.0f, 300.0f};
    std::vector<ShaderUniformValue> uniforms;

    std::string error;
    std::shared_ptr<SIMPEngine::ShaderEffect> effect;
    uint32_t effectVersion = 0;
};

struct PrefabInstanceComponent
{
    std::string path;
};

struct CanvasComponent
{
    glm::vec2 referenceSize{1280.0f, 720.0f};
    int scaleMode = 1;
    float match = 0.5f;
    float safeArea = 0.0f;
    int sortOrder = 0;
    bool visible = true;

    glm::vec2 size{1280.0f, 720.0f};
    float scale = 1.0f;
};

struct UIRectComponent
{
    glm::vec2 anchorMin{0.5f, 0.5f};
    glm::vec2 anchorMax{0.5f, 0.5f};
    glm::vec2 pivot{0.5f, 0.5f};
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{160.0f, 48.0f};
    glm::vec4 margin{0.0f, 0.0f, 0.0f, 0.0f};
    bool visible = true;
    bool blocksInput = true;

    glm::vec4 rect{0.0f, 0.0f, 0.0f, 0.0f};
};

struct UIPanelComponent
{
    SDL_Color color{28, 30, 40, 220};
    float cornerRadius = 8.0f;
    SDL_Color borderColor{255, 255, 255, 70};
    float borderWidth = 0.0f;
};

struct UILabelComponent
{
    std::string text = "Label";
    float size = 24.0f;
    SDL_Color color{255, 255, 255, 255};
    int align = 1;
    int verticalAlign = 1;
    std::string fontPath;
    bool shadow = false;
};

struct UIImageComponent
{
    std::shared_ptr<SIMPEngine::Texture> texture = nullptr;
    SDL_Color tint{255, 255, 255, 255};
    bool preserveAspect = true;
    bool useRegion = false;
    SDL_FRect region{0.0f, 0.0f, 0.0f, 0.0f};
};

struct UIButtonComponent
{
    bool enabled = true;
    bool focusable = true;
    SDL_Color hoverTint{235, 240, 255, 255};
    SDL_Color pressTint{175, 180, 200, 255};
    SDL_Color disabledTint{130, 130, 130, 200};
    float hoverScale = 1.03f;
    float pressScale = 0.97f;
    std::string hoverSound;
    std::string clickSound;
    float volume = 1.0f;

    bool hovering = false;
    bool pressed = false;
    bool focused = false;
};

struct UILayoutComponent
{
    int direction = 1;
    float spacing = 8.0f;
    glm::vec4 padding{8.0f, 8.0f, 8.0f, 8.0f};
    int justify = 0;
    int align = 1;
    bool stretch = false;
    bool enabled = true;
};

struct TeleporterComponent
{
    std::string target;
    glm::vec2 offset{0.0f, 0.0f};
    std::string sound;
    bool keepVelocity = true;
    bool enabled = true;
};

struct CameraZoneComponent
{
    float zoom = 1.0f;
    float speed = 4.0f;
    bool setBounds = false;
    glm::vec2 boundsMin{-1000.0f, -1000.0f};
    glm::vec2 boundsMax{1000.0f, 1000.0f};
    bool restoreOnExit = true;
    bool enabled = true;
};

enum class BoundsMode : uint8_t
{
    Clamp,
    Wrap,
    Bounce,
    Destroy
};

struct WorldBoundsComponent
{
    glm::vec2 min{-640.0f, -360.0f};
    glm::vec2 max{640.0f, 360.0f};
    BoundsMode mode = BoundsMode::Clamp;
    float margin = 0.0f;
    bool enabled = true;
};

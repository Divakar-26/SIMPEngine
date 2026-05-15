# SIMPEngine Components Reference

Comprehensive guide to all built-in components and their usage.

## Component System Overview

Components are data containers attached to entities. They hold the state of game objects while systems contain the logic that operates on them.

```cpp
// Add component to entity
auto& transform = entity.AddComponent<TransformComponent>();
transform.position = {100, 200};

// Get component reference
auto& sprite = entity.GetComponent<SpriteComponent>();

// Check if entity has component
if (entity.HasComponent<PhysicsComponent>()) {
    auto& physics = entity.GetComponent<PhysicsComponent>();
}

// Remove component
entity.RemoveComponent<SpriteComponent>();
```

## Transform Component

**Location**: `Engine/Scene/Component.h`

Defines entity position, rotation, scale, and depth.

```cpp
struct TransformComponent {
    glm::vec2 position{0, 0};      // World position
    float rotation = 0;             // Rotation in degrees
    glm::vec2 scale{1, 1};          // Scale multiplier
    float zIndex = 0;               // Depth for layering
    
    bool dirty = true;              // Recalculate transform matrix?
    glm::mat4 worldTransform;       // Cached world matrix
    
    // Get local transformation matrix
    glm::mat4 GetLocalTransform() const {
        glm::mat4 t = glm::translate(glm::mat4(1), {position, zIndex});
        t = glm::rotate(t, glm::radians(rotation), {0, 0, 1});
        t = glm::scale(t, {scale, 1});
        return t;
    }
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Player");
auto& transform = entity.AddComponent<TransformComponent>();

// Set properties
transform.position = {512, 320};
transform.scale = {2, 2};
transform.rotation = 45.0f;
transform.zIndex = 10;

// Movement
transform.position.x += velocity.x * deltaTime;
transform.position.y += velocity.y * deltaTime;

// Check position
if (transform.position.x > 1920) {
    LOG_INFO("Entity off-screen!");
}

// Mark for update
transform.dirty = true;  // Forces transform recalculation
```

## Sprite Component

**Location**: `Engine/Scene/Component.h`

Holds visual representation data for an entity.

```cpp
struct SpriteComponent {
    std::shared_ptr<SIMPEngine::Texture> texture = nullptr;
    float width = 0;
    float height = 0;
    SDL_Color color = {255, 255, 255, 255};
    float opacity = 1.0f;
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Enemy");
auto& transform = entity.AddComponent<TransformComponent>();
transform.position = {256, 128};

// Load texture
auto& textureManager = SIMPEngine::TextureManager::Get();
auto texture = textureManager.LoadTexture("enemy", "assets://textures/enemy.png");

// Add sprite
auto& sprite = entity.AddComponent<SpriteComponent>();
sprite.texture = texture;
sprite.width = texture->GetWidth();
sprite.height = texture->GetHeight();
sprite.color = {255, 0, 0, 255};  // Red tint
sprite.opacity = 0.8f;

// Fade out
sprite.opacity -= 0.01f * deltaTime;

// Color changes
sprite.color.g = 128;  // Reduce green channel
```

### Important Notes

- Sprite width/height should match texture dimensions (or part for atlasing)
- Color is applied as a tint to the texture
- Opacity affects rendering transparency (0.0 = invisible, 1.0 = opaque)
- If texture is null, RenderSystem skips rendering

## Collision Component

**Location**: `Engine/Scene/Component.h`

Defines collision bounds for entities (separate from render bounds).

```cpp
struct CollisionComponent {
    float width = 0.0f;
    float height = 0.0f;
    float offsetX = 0.0f;           // Relative to position
    float offsetY = 0.0f;
    
    SDL_FRect GetBounds(float x, float y) const {
        return SDL_FRect{x + offsetX, y + offsetY, width, height};
    }
    
    SDL_FRect GetBoundsWorld(const TransformComponent& transform) const {
        return GetBounds(transform.position.x, transform.position.y);
    }
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Player");
auto& transform = entity.AddComponent<TransformComponent>();
transform.position = {100, 100};

// Add collision box smaller than sprite
auto& collision = entity.AddComponent<CollisionComponent>();
collision.width = 32;
collision.height = 32;
collision.offsetX = 16;  // Center on sprite
collision.offsetY = 16;

// Get collision bounds
SDL_FRect bounds = collision.GetBoundsWorld(transform);

// Manual collision check
auto& Other = otherEntity.GetComponent<CollisionComponent>();
auto& otherTransform = otherEntity.GetComponent<TransformComponent>();
SDL_FRect otherBounds = other.GetBoundsWorld(otherTransform);

if (SDL_HasIntersectionF(&bounds, &otherBounds)) {
    LOG_INFO("Collision!");
}
```

### Use Cases

- Hitboxes for attacks
- Pickup ranges
- Enemy detection areas
- Platform collision
- Projectile hit detection

## Animated Sprite Component

**Location**: `Engine/Scene/Component.h`

Manages frame-based sprite sheet animations.

```cpp
struct AnimatedSpriteComponent {
    Animation* animation = nullptr;
    float time = 0;
    bool playing = true;
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Character");
auto& transform = entity.AddComponent<TransformComponent>();

// Load spritesheet (8 frames, each 64x64)
auto spritesheet = textureManager.LoadTexture(
    "walk_sheet", 
    "assets://textures/walk_animation.png"
);

// Create animation
auto animation = new SIMPEngine::Animation(
    spritesheet.get(),
    64,      // frame width
    64,      // frame height
    8,       // frame count
    0.1f,    // frame duration (100ms per frame)
    true     // loop
);

// Add to entity
auto& animSprite = entity.AddComponent<AnimatedSpriteComponent>();
animSprite.animation = animation;
animSprite.playing = true;

// Control animation
animSprite.playing = false;  // Pause
animSprite.time = 0;         // Reset to first frame

// AnimationSystem handles time updates automatically
```

### Animation Class

```cpp
class Animation {
public:
    Animation(Texture* spritesheet, int frameWidth, int frameHeight,
              int frameCount, float frameDuration, bool loop);
    
    int GetFrameWidth() const;
    int GetFrameHeight() const;
    int GetFrameCount() const;
    float GetFrameDuration() const;
    bool IsLooping() const;
    
    // Get current frame's uv coordinates
    SDL_FRect GetCurrentFrameUV() const;
};
```

## Physics Component

**Location**: `Engine/Scene/Component.h`

Connects entity to physics simulation via AccelEngine.

```cpp
struct PhysicsComponent {
    AccelEngine::RigidBody* body = nullptr;
    bool useGravity = true;
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Ball");
auto& transform = entity.AddComponent<TransformComponent>();
transform.position = {500, 300};

// Create physics body
auto& physics = entity.AddComponent<PhysicsComponent>();
physics.body = new AccelEngine::RigidBody();
physics.body->setMass(1.0f);
physics.body->setVelocity({10, 0});
physics.useGravity = true;

// Physics simulation runs automatically via PhysicsSystem

// Access physics properties
float mass = physics.body->getMass();
glm::vec3 velocity = physics.body->getVelocity();
glm::vec3 acceleration = physics.body->getAcceleration();

// Apply forces
physics.body->addForce({100, 0});
```

### AccelEngine RigidBody Methods

```cpp
void setMass(float mass);
float getMass() const;

void setVelocity(glm::vec3 vel);
glm::vec3 getVelocity() const;

void setAcceleration(glm::vec3 acc);
glm::vec3 getAcceleration() const;

void addForce(glm::vec3 force);
void clearForces();

void integrate(float dt);  // Called by PhysicsSystem
```

### Important Notes

- Physics bodies should have collision components for collision detection
- Mass must be > 0 for dynamic bodies
- Gravity is applied by PhysicsSystem
- Forces are cleared each frame

## Camera Component

**Location**: `Engine/Scene/Component.h`

Defines an entity as a camera for rendering.

```cpp
struct CameraComponent {
    Camera2D camera;
    bool isPrimary = true;  // Active camera if true
};
```

### Usage

```cpp
auto cameraEntity = scene->CreateEntity("MainCamera");
auto& transform = cameraEntity.AddComponent<TransformComponent>();
transform.position = {512, 320};

auto& cameraComp = cameraEntity.AddComponent<CameraComponent>();
cameraComp.isPrimary = true;  // Use this camera for rendering
cameraComp.camera.SetZoom(1.5f);

// Follow player
auto& playerTransform = player.GetComponent<TransformComponent>();
cameraComp.camera.SetPosition(playerTransform.position);

// Get active camera
Camera2D& activeCamera = scene->GetActiveCamera();
```

### Camera2D Methods

```cpp
void SetPosition(float x, float y);
glm::vec2 GetPosition() const;

void SetZoom(float zoom);
float GetZoom() const;

void SetRotation(float angle);
float GetRotation() const;

glm::mat4 GetViewMatrix() const;

glm::vec2 ScreenToWorld(float screenX, float screenY) const;
glm::vec2 WorldToScreen(float worldX, float worldY) const;
```

## Hierarchy Component

**Location**: `Engine/Scene/Component.h`

Manages parent-child entity relationships.

```cpp
struct HierarchyComponent {
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};
```

### Usage

```cpp
auto parent = scene->CreateEntity("Parent");
auto child = scene->CreateEntity("Child");

// Set parent-child relationship
scene->SetParent(child, parent);

// Hierarchy system handles transform propagation
// Child's world transform = parent's world transform * child's local transform

// Access hierarchy
auto& hierarchy = parent.GetComponent<HierarchyComponent>();
for (auto childEntity : hierarchy.children) {
    auto& childTransform = scene->GetRegistry()
        .get<TransformComponent>(childEntity);
    // Process child
}
```

### Use Cases

- Player with equipped weapons
- UI panels with child elements
- Skeletal animation bones
- Scene hierarchies

## Tag Component

**Location**: `Engine/Scene/Component.h`

Simple string tag for identifying entities.

```cpp
struct TagComponent {
    std::string tag;
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Player");
auto& tag = entity.AddComponent<TagComponent>();
tag.tag = "Player";

// Find by tag
auto found = scene->GetEntityByName("Player");

// Or iterate and check tag
auto& registry = scene->GetRegistry();
auto view = registry.view<TagComponent>();
for (auto e : view) {
    auto& t = registry.get<TagComponent>(e);
    if (t.tag == "Enemy") {
        // Process enemy
    }
}
```

## Script Component

**Location**: `Engine/Scene/Component.h`

Attach custom C++ scripts/behavior to entities.

```cpp
struct ScriptComponent {
    ScriptableEntity* Instance = nullptr;
};
```

### Usage

```cpp
// Define custom entity script
class PlayerScript : public SIMPEngine::ScriptableEntity {
public:
    virtual void OnCreate() override {
        LOG_INFO("Player created");
    }
    
    virtual void OnUpdate(float dt) override {
        if (SIMPEngine::Input::IsKeyPressed(SIMP_KEY_W)) {
            auto& transform = GetComponent<TransformComponent>();
            transform.position.y -= 200 * dt;
        }
    }
    
    virtual void OnDestroy() override {
        LOG_INFO("Player destroyed");
    }
};

// Attach to entity
auto entity = scene->CreateEntity("Player");
auto& script = entity.AddComponent<ScriptComponent>();
script.Instance = new PlayerScript();
script.Instance->m_Entity = entity;
```

### ScriptableEntity Base Class

```cpp
class ScriptableEntity {
public:
    virtual ~ScriptableEntity() = default;
    
    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}
    
    template<typename T>
    T& GetComponent() {
        return m_Entity.GetComponent<T>();
    }
    
protected:
    Entity m_Entity;
};
```

## Tileset Component

**Location**: `Engine/Scene/Component.h`

Defines tileset data for tilemap rendering.

```cpp
struct TilesetComponent {
    std::shared_ptr<SIMPEngine::Tileset> tileset;
    int width = 0;
    int height = 0;
};
```

### Usage

```cpp
auto entity = scene->CreateEntity("Tileset");
auto& tileset = entity.AddComponent<TilesetComponent>();

auto tilesetData = std::make_shared<SIMPEngine::Tileset>();
// Load tileset data...
tileset.tileset = tilesetData;
```

## Tilemap Component

**Location**: `Engine/Scene/Component.h`

Contains tilemap layer data.

```cpp
struct TilemapComponent {
    std::vector<std::vector<int>> tiles;  // 2D tile ID array
    int tileWidth = 0;
    int tileHeight = 0;
    int width = 0;   // Map width in tiles
    int height = 0;  // Map height in tiles
};
```

### Usage

```cpp
auto mapEntity = scene->CreateEntity("TileLayer");
auto& transform = mapEntity.AddComponent<TransformComponent>();

auto& tilemap = mapEntity.AddComponent<TilemapComponent>();
tilemap.width = 50;
tilemap.height = 50;
tilemap.tileWidth = 32;
tilemap.tileHeight = 32;
tilemap.tiles.resize(50, std::vector<int>(50, 0));

// Set tile IDs
tilemap.tiles[0][0] = 5;  // Tile ID 5 at position (0, 0)

// TilemapSystem handles rendering
```

## Creating Custom Components

```cpp
// Define new component
struct HealthComponent {
    int currentHP = 100;
    int maxHP = 100;
    
    bool IsAlive() const { return currentHP > 0; }
    void TakeDamage(int amount) { currentHP = std::max(0, currentHP - amount); }
    void Heal(int amount) { currentHP = std::min(maxHP, currentHP + amount); }
};

// Use like any other component
auto entity = scene->CreateEntity("Enemy");
auto& health = entity.AddComponent<HealthComponent>();
health.maxHP = 50;
health.currentHP = 50;

// Query components with custom component
auto& registry = scene->GetRegistry();
auto view = registry.view<HealthComponent, TransformComponent>();
for (auto e : view) {
    auto& health = registry.get<HealthComponent>(e);
    if (!health.IsAlive()) {
        scene->DestroyEntity(Entity{e, &registry});
    }
}
```

## Component Best Practices

### DO ✓

- Keep components as pure data structures
- Use `std::shared_ptr` for shared resources (textures, animations)
- Initialize all member variables
- Document complex logic outside components
- Use meaningful component names

### DON'T ✗

- Don't put game logic in components (use systems instead)
- Don't have components own other entities
- Don't have circular dependencies between components
- Don't directly manipulate other entity's components
- Don't create uninitialized pointers

---

For system details, see [ARCHITECTURE.md](docs/ARCHITECTURE.md#5-systems)

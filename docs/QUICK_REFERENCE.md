# SIMPEngine Quick Reference

Quick lookup guide for common tasks.

## Creating Your Game

```cpp
#include <Engine/SIMPEngine.h>

class MyGame : public SIMPEngine::Application {
public:
    MyGame() {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("MainLevel");
        
        sceneManager->AddScene("MainLevel", scene);
        sceneManager->SetActiveScene("MainLevel");
        
        auto renderingLayer = new SIMPEngine::RenderingLayer(sceneManager);
        PushOverlay(renderingLayer);
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication() {
    return new MyGame();
}

int main() {
    SIMPEngine::Log::Init();
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
```

## Scene & Entities

```cpp
// Access active scene
auto scene = sceneManager->GetActiveScene();

// Create entity
auto entity = scene->CreateEntity("PlayerName");
auto unnamed = scene->CreateEntity();  // Auto-named

// Find entity by name
auto player = scene->GetEntityByName("PlayerName");

// Destroy entity
scene->DestroyEntity(entity);

// Switch scenes
sceneManager->SetActiveScene("Level2");
```

## Components - Common Pattern

```cpp
// Add component
auto& sprite = entity.AddComponent<SpriteComponent>();

// Get component
auto& transform = entity.GetComponent<TransformComponent>();

// Check if has component
if (entity.HasComponent<PhysicsComponent>()) { }

// Remove component
entity.RemoveComponent<SpriteComponent>();
```

## Transform

```cpp
auto& transform = entity.GetComponent<TransformComponent>();

transform.position = {100, 200};
transform.rotation = 45.0f;           // Degrees
transform.scale = {2, 2};
transform.zIndex = 10;                // Depth layer

// Move entity
transform.position += velocity * deltaTime;
```

## Rendering

```cpp
// Load texture
auto texture = SIMPEngine::TextureManager::Get()
    .LoadTexture("id", "assets://textures/player.png");

// Add sprite
auto& sprite = entity.AddComponent<SpriteComponent>();
sprite.texture = texture;
sprite.width = 64;
sprite.height = 64;
sprite.color = {255, 0, 0, 255};      // Red tint
sprite.opacity = 0.8f;

// Draw primitives directly
SIMPEngine::Renderer::DrawQuad(x, y, w, h, rot, color);
SIMPEngine::Renderer::DrawLine(x1, y1, x2, y2, color);
```

## Input

```cpp
// Check key pressed
if (SIMPEngine::Input::IsKeyPressed(SIMP_KEY_W)) {
    // W key is held
}

// Check mouse button
if (SIMPEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
    // Left click
}

// Get mouse position
auto [x, y] = SIMPEngine::Input::GetMousePosition();

// Mouse scroll
int = SIMPEngine::Input::GetMouseWheel();  // -1, 0, or 1
```

## Key Codes

```cpp
SIMP_KEY_W, SIMP_KEY_A, SIMP_KEY_S, SIMP_KEY_D
SIMP_KEY_SPACE, SIMP_KEY_ENTER, SIMP_KEY_ESCAPE
SIMP_KEY_UP, SIMP_KEY_DOWN, SIMP_KEY_LEFT, SIMP_KEY_RIGHT
SIMP_KEY_LCTRL, SIMP_KEY_RCTRL
SIMP_KEY_LSHIFT, SIMP_KEY_RSHIFT
SIMP_KEY_LALT, SIMP_KEY_RALT
// ... and A-Z, 0-9
```

## Collision

```cpp
auto& collision = entity.AddComponent<CollisionComponent>();
collision.width = 32;
collision.height = 32;
collision.offsetX = 0;
collision.offsetY = 0;

// Manual collision check
SDL_FRect rect1 = collision1.GetBoundsWorld(transform1);
SDL_FRect rect2 = collision2.GetBoundsWorld(transform2);
if (SDL_HasIntersectionF(&rect1, &rect2)) {
    LOG_INFO("Collision!");
}
```

## Physics

```cpp
auto& physics = entity.AddComponent<PhysicsComponent>();
physics.body = new AccelEngine::RigidBody();
physics.body->setMass(1.0f);
physics.body->setVelocity({10, 0});
physics.useGravity = true;

// Apply force
physics.body->addForce({100, 0});

// Get properties
float mass = physics.body->getMass();
glm::vec3 vel = physics.body->getVelocity();
```

## Animation

```cpp
// Create animation from spritesheet
auto animation = new SIMPEngine::Animation(
    spritesheet,      // Texture*
    64,               // frame width
    64,               // frame height
    8,                // frame count
    0.1f,             // frame duration (seconds)
    true              // loop
);

// Add to entity
auto& animSprite = entity.AddComponent<AnimatedSpriteComponent>();
animSprite.animation = animation;
animSprite.playing = true;

// Control
animSprite.playing = false;   // Pause
animSprite.time = 0;          // Reset
```

## Camera

```cpp
// Get active camera
Camera2D& camera = scene->GetActiveCamera();

// Control camera
camera.SetPosition(playerX, playerY);
camera.SetZoom(2.0f);
camera.SetRotation(45.0f);

// Screen to world (mouse picking)
auto worldPos = camera.ScreenToWorld(screenX, screenY);
```

## Logging

```cpp
// Engine logs
CORE_INFO("Message");
CORE_WARN("Warning");
CORE_ERROR("Error");

// Game logs
LOG_INFO("Message");
LOG_WARN("Warning");
LOG_ERROR("Error");

// With formatting
LOG_INFO("Player at ({}, {})", x, y);
```

## Scene Serialization

```cpp
SIMPEngine::SceneSerializer serializer(scene.get());

// Load scene
serializer.Deserialize("assets://scenes/level1.yaml");

// Save scene
serializer.Serialize("assets://scenes/mylevel.yaml");
```

## Asset Management

```cpp
// Load texture
auto texture = SIMPEngine::TextureManager::Get()
    .LoadTexture("player", "assets://textures/player.png");

// Get loaded texture
auto tex = SIMPEngine::TextureManager::Get()
    .GetTexture("player");

// Get properties
int width = texture->GetWidth();
int height = texture->GetHeight();
std::string path = texture->GetPath();
```

## Custom Script Component

```cpp
class PlayerScript : public SIMPEngine::ScriptableEntity {
public:
    void OnCreate() override {
        LOG_INFO("Player initialized");
    }
    
    void OnUpdate(float dt) override {
        if (SIMPEngine::Input::IsKeyPressed(SIMP_KEY_W)) {
            auto& transform = GetComponent<TransformComponent>();
            transform.position.y -= 200 * dt;
        }
    }
    
    void OnDestroy() override {
        LOG_INFO("Player destroyed");
    }
};

// Attach
auto entity = scene->CreateEntity("Player");
auto& script = entity.AddComponent<ScriptComponent>();
script.Instance = new PlayerScript();
script.Instance->m_Entity = entity;
```

## Event Handling

```cpp
class MyLayer : public SIMPEngine::Layer {
    void OnEvent(SIMPEngine::Event& e) override {
        auto dispatcher = SIMPEngine::EventDispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>([this](auto& e) {
            return OnKeyPressed(e);
        });
    }
    
    bool OnKeyPressed(KeyPressedEvent& e) {
        if (e.GetKeyCode() == SIMP_KEY_W) {
            e.Handled = true;  // Stop propagation
            return true;
        }
        return false;
    }
};
```

## Entity Queries

```cpp
auto& registry = scene->GetRegistry();

// Get all entities with component
auto view = registry.view<TransformComponent>();
for (auto entity : view) {
    auto& transform = registry.get<TransformComponent>(entity);
    LOG_INFO("Entity at {}", transform.position);
}

// Get entities with multiple components
auto view2 = registry.view<TransformComponent, SpriteComponent>();
for (auto entity : view2) {
    auto& [transform, sprite] = view2.get<TransformComponent, SpriteComponent>(entity);
    // Process
}
```

## Hierarchy (Parent-Child)

```cpp
// Set parent
scene->SetParent(childEntity, parentEntity);

// Access hierarchy
auto& hierarchy = parentEntity.GetComponent<HierarchyComponent>();
for (auto child : hierarchy.children) {
    // Process child
}
```

## Performance Tips

```cpp
// Profile code sections
{
    ProfileTimer timer("MySystem");
    // Code to profile - automatically logged on scope exit
}

// Mark transform dirty if changed
transform.dirty = true;

// Check components before access
if (entity.HasComponent<SpriteComponent>()) {
    auto& sprite = entity.GetComponent<SpriteComponent>();
}

// Use VFS paths consistently
"assets://textures/...";
"assets://scenes/...";
```

## Common Mistakes

```cpp
// ❌ Don't do this - stores dangling pointer
auto* ptr = &entity.GetComponent<TransformComponent>();
entity.RemoveComponent<TransformComponent>();  // Dangerous!

// ✅ Do this - get fresh reference
auto& transform = entity.GetComponent<TransformComponent>();

// ❌ Don't create scenes inside systems
// ✅ Create all scenes before loop starts

// ❌ Don't delete entities during iteration
for (auto e : view) {
    scene->DestroyEntity(Entity);  // ❌ Undefined behavior
}

// ✅ Collect entities first, then delete
std::vector<Entity> toDelete;
for (auto e : view) {
    if (shouldDelete) toDelete.push_back(e);
}
for (auto e : toDelete) {
    scene->DestroyEntity(e);
}

// ❌ Don't assume texture loaded
auto tex = textureManager.LoadTexture(...);
sprite.width = tex->GetWidth();  // Might crash!

// ✅ Check null
auto tex = textureManager.LoadTexture(...);
if (tex) {
    sprite.width = tex->GetWidth();
}
```

## File Paths

```cpp
// Asset Virtual File System
"assets://textures/player.png"
"assets://scenes/level1.yaml"
"assets://shaders/sprite.vert"

// Mounted automatically by engine
// Mount additional directories:
SIMPEngine::VFS::Mount("myassets", "../other/assets");
"myassets://texture.png"
```

## Delta Time

```cpp
// Use in all frame-dependent code
transform.position += velocity * deltaTime;

// Animation
sprite.opacity -= 0.5f * deltaTime;  // Fade over time

// Physics
displacement = initialVelocity * deltaTime + 0.5 * acceleration * deltaTime * deltaTime;
```

---

**For full documentation:** [docs/API_GUIDE.md](docs/API_GUIDE.md)

**Architecture details:** [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

**Component reference:** [docs/COMPONENTS.md](docs/COMPONENTS.md)

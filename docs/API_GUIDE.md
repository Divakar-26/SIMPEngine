# SIMPEngine API Reference

Complete guide to using the SIMPEngine public API.

## Table of Contents

1. [Application](#application)
2. [Scene Management](#scene-management)
3. [Entity & Components](#entity--components)
4. [Rendering](#rendering)
5. [Input](#input)
6. [Physics](#physics)
7. [Animation](#animation)
8. [Assets](#assets)
9. [Logging](#logging)
10. [Events](#events)

---

## Application

### Creating Your Game Application

**Header**: `<Engine/SIMPEngine.h>`

```cpp
class MyGame : public SIMPEngine::Application
{
public:
    MyGame() {
        // Initialize your game here
        // Create scenes, load assets, setup layers
    }
};

SIMPEngine::Application* SIMPEngine::CreateApplication()
{
    return new MyGame();
}

int main() {
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
```

### Application Methods

```cpp
void Run();
// Starts the main game loop
// Returns when m_Running becomes false

void OnEvent(Event& e);
// Called for internal engine events
// Used for window resize, close, etc.

void PushLayer(Layer* layer);
// Add a layer to the layer stack
// Returns control to caller immediately

void PushOverlay(Layer* overlay);
// Push a layer as overlay (top of stack)
// Overlays receive events first

Window& GetWindow();
// Get the main window object
// Used for window properties

ImGuiLayer* GetImGuiLayer();
// Get ImGui layer for UI rendering

static Application& Get();
// Get singleton instance from anywhere
// Example: auto& app = SIMPEngine::Application::Get();
```

---

## Scene Management

### SceneManager

**Header**: `<Engine/Scene/SceneManager.h>`

```cpp
auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();

// Add scenes
auto scene1 = std::make_shared<SIMPEngine::Scene>("Level1");
sceneManager->AddScene("Level1", scene1);

// Set active scene (updated each frame)
sceneManager->SetActiveScene("Level1");

// Get active scene
auto activeScene = sceneManager->GetActiveScene();

// Remove scene
sceneManager->RemoveScene("Level1");

// Check if scene exists
if (sceneManager->HasScene("Level1")) { /*...*/ }

// Get all scenes
const auto& allScenes = sceneManager->GetAllScenes();
for (const auto& [name, scene] : allScenes) {
    LOG_INFO("Scene: {}", name);
}
```

### Scene

**Header**: `<Engine/Scene/Scene.h>`

```cpp
auto scene = std::make_shared<SIMPEngine::Scene>("MyScene");

// Entity creation
auto entity = scene->CreateEntity("Player");
auto unnamed = scene->CreateEntity();  // Auto-named

// Entity lookup
auto player = scene->GetEntityByName("Player");

// Destroy entity
scene->DestroyEntity(entity);

// Get/Update scene
scene->OnUpdate(deltaTime);  // Called automatically
scene->OnRender();           // Called automatically
scene->Clear();              // Destroy all entities

// Access ECS registry directly
entt::registry& registry = scene->GetRegistry();

// Camera access
Camera2D& camera = scene->GetMainCamera();
Camera2D& activeCamera = scene->GetActiveCamera();

// Physics access
AccelEngine::World& world = scene->GetPhysicsWorld();
PhysicsSystem& physicsSystem = scene->GetPhysicsSystem();

// Hierarchy
scene->SetParent(childEntity, parentEntity);
```

---

## Entity & Components

### Creating & Managing Entities

**Header**: `<Engine/Scene/Entity.h>`

```cpp
// Create entity
auto entity = scene->CreateEntity("MyEntity");

// Get entity by name
auto found = scene->GetEntityByName("MyEntity");

// Destroy entity
scene->DestroyEntity(entity);
```

### Adding Components

```cpp
// Add component with default values
auto& transform = entity.AddComponent<TransformComponent>();

// Add component with initialization
auto& sprite = entity.AddComponent<SpriteComponent>(texture, width, height);

// Get component
auto& transform = entity.GetComponent<TransformComponent>();

// Check if has component
if (entity.HasComponent<SpriteComponent>()) {
    auto& sprite = entity.GetComponent<SpriteComponent>();
}

// Remove component
entity.RemoveComponent<SpriteComponent>();
```

### Built-in Components

**TransformComponent**
```cpp
struct TransformComponent {
    glm::vec2 position{0, 0};
    float rotation = 0;           // In degrees
    glm::vec2 scale{1, 1};
    float zIndex = 0;             // For depth sorting
    
    bool dirty = true;            // Recalculate transform?
    glm::mat4 worldTransform;
    
    glm::mat4 GetLocalTransform() const;
};
```

**SpriteComponent**
```cpp
struct SpriteComponent {
    std::shared_ptr<Texture> texture;
    float width, height;
    SDL_Color color = {255, 255, 255, 255};
    float opacity = 1.0f;
};
```

**CollisionComponent**
```cpp
struct CollisionComponent {
    float width, height;
    float offsetX = 0, offsetY = 0;  // Relative to position
    
    SDL_FRect GetBounds(float x, float y) const;
    SDL_FRect GetBoundsWorld(const TransformComponent& t) const;
};
```

**AnimatedSpriteComponent**
```cpp
struct AnimatedSpriteComponent {
    Animation* animation;
    float time = 0;
    bool playing = true;
};
```

**PhysicsComponent**
```cpp
struct PhysicsComponent {
    AccelEngine::RigidBody* body;
    bool useGravity = true;
};
```

**CameraComponent**
```cpp
struct CameraComponent {
    Camera2D camera;
    bool isPrimary = true;    // Active camera if true
};
```

**HierarchyComponent**
```cpp
struct HierarchyComponent {
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};
```

**ScriptComponent**
```cpp
struct ScriptComponent {
    ScriptableEntity* Instance = nullptr;
};
```

**TagComponent**
```cpp
struct TagComponent {
    std::string tag;
};
```

See [COMPONENTS.md](COMPONENTS.md) for detailed component documentation.

---

## Rendering

### Renderer

**Header**: `<Engine/Rendering/Renderer.h>`

```cpp
// Initialization (done by engine)
Renderer::Init(std::make_unique<GLRenderingAPI>(), width, height);

// Clear and color
Renderer::SetClearColor(r, g, b, a);
Renderer::Clear();

// Draw primitives
Renderer::DrawQuad(x, y, width, height, rotation, color, fill, zIndex);
Renderer::DrawLine(x1, y1, x2, y2, color);

// Draw textures
auto texture = TextureManager::Get().GetTexture("player");
Renderer::DrawTexture(
    texture,
    x, y, width, height,
    color,
    rotation,
    zIndex,
    nullptr  // srcRect for sprite atlasing
);

// Frame management
Renderer::BeginFrame();
Renderer::EndFrame();
Renderer::Flush();
Renderer::Present();

// View/Projection
Renderer::SetViewMatrix(viewMatrix);
const auto& viewMatrix = Renderer::GetViewMatrix();
auto viewportSize = Renderer::GetViewportSize();

// Backend access
RenderingAPI* api = Renderer::GetAPI();
```

### Texture Management

**Header**: `<Engine/Rendering/TextureManager.h>`

```cpp
auto& textureManager = SIMPEngine::TextureManager::Get();

// Load texture
auto texture = textureManager.LoadTexture("player", "assets://textures/player.png");

// Get already loaded texture
auto loaded = textureManager.GetTexture("player");

// Unload specific texture
textureManager.UnloadTexture("player");

// Clear all textures
textureManager.UnloadAll();

// Texture methods
int width = texture->GetWidth();
int height = texture->GetHeight();
std::string path = texture->GetPath();
std::string vfsPath = texture->GetVFSPath();
```

### Shader Management

**Header**: `<Engine/Rendering/Shader.h>`

```cpp
auto shader = std::make_shared<SIMPEngine::Shader>();

// Load shader files
shader->LoadFromFile("shaders/sprite.vert", "shaders/sprite.frag");

// Bind/Unbind
shader->Bind();
shader->Unbind();

// Set uniforms
shader->SetUniform1i("texture", 0);
shader->SetUniform1f("opacity", 0.8f);
shader->SetUniform2f("position", 100, 200);
shader->SetUniform3f("color", 1.0f, 1.0f, 1.0f);
shader->SetUniformMat4("transform", matrix);
```

### Camera System

**Header**: `<Engine/Math/Camera2D.h>`

```cpp
Camera2D camera;

// Set camera properties
camera.SetPosition(x, y);
camera.SetZoom(2.0f);
camera.SetRotation(45.0f);

// Get camera properties
glm::vec2 pos = camera.GetPosition();
float zoom = camera.GetZoom();

// Camera matrix (used by renderer)
glm::mat4 viewMatrix = camera.GetViewMatrix();

// Screen to world conversion (for mouse picking)
glm::vec2 worldPos = camera.ScreenToWorld(screenX, screenY);
glm::vec2 screenPos = camera.WorldToScreen(worldX, worldY);
```

---

## Input

### Input System

**Header**: `<Engine/Input/Input.h>`

```cpp
// Keyboard
bool pressed = SIMPEngine::Input::IsKeyPressed(SIMP_KEY_W);

// Mouse buttons
bool clicked = SIMPEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT);

// Mouse position
auto [x, y] = SIMPEngine::Input::GetMousePosition();

// Mouse wheel
int scroll = SIMPEngine::Input::GetMouseWheel();

// Reset state (called automatically each frame)
SIMPEngine::Input::ResetAllKeys();

// Event callbacks (called by event system automatically)
SIMPEngine::Input::OnKeyPressed(keyCode);
SIMPEngine::Input::OnKeyReleased(keyCode);
SIMPEngine::Input::OnMouseButtonPressed(button);
SIMPEngine::Input::OnMouseButtonReleased(button);
SIMPEngine::Input::OnMouseMoved(x, y);
SIMPEngine::Input::OnMouseWheel(scrollY);
```

### Key Codes

**Header**: `<Engine/Input/SIMP_Keys.h>`

```cpp
// Letter keys (SIMP_KEY_A to SIMP_KEY_Z)
// Number keys (SIMP_KEY_0 to SIMP_KEY_9)
// Arrow keys (SIMP_KEY_UP, SIMP_KEY_DOWN, SIMP_KEY_LEFT, SIMP_KEY_RIGHT)
// Special keys:
SIMP_KEY_SPACE
SIMP_KEY_ENTER
SIMP_KEY_ESCAPE
SIMP_KEY_TAB
SIMP_KEY_LCTRL, SIMP_KEY_RCTRL
SIMP_KEY_LSHIFT, SIMP_KEY_RSHIFT
SIMP_KEY_LALT, SIMP_KEY_RALT
// ... and more

// Example
if (SIMPEngine::Input::IsKeyPressed(SIMP_KEY_W)) {
    // Move forward
}
```

### Event Handling

**Header**: `<Engine/Events/Event.h>`

Custom event handling in layers:

```cpp
class MyLayer : public SIMPEngine::Layer {
    void OnEvent(SIMPEngine::Event& e) override {
        if (e.GetEventType() == SIMPEngine::EventType::KeyPressed) {
            auto* keyEvent = static_cast<KeyPressedEvent*>(&e);
            if (keyEvent->GetKeyCode() == SIMP_KEY_W) {
                e.Handled = true;  // Prevent other layers
                // Handle key
            }
        }
    }
};
```

---

## Physics

### Physics System Integration

**Header**: `<Engine/Scene/Systems/PhysicsSystem.h>`

```cpp
// Access physics from scene
PhysicsSystem& physicsSystem = scene->GetPhysicsSystem();
AccelEngine::World& world = scene->GetPhysicsWorld();

// Create physics entity
auto entity = scene->CreateEntity("Ball");
auto& physics = entity.AddComponent<PhysicsComponent>();
physics.body = new AccelEngine::RigidBody();
physics.body->setMass(1.0f);
physics.body->setVelocity({10, 0});
physics.useGravity = true;

// Add collision
auto& collision = entity.AddComponent<CollisionComponent>();
collision.width = 10;
collision.height = 10;

// Access body properties
float mass = physics.body->getMass();
glm::vec3 velocity = physics.body->getVelocity();
```

### Collision Detection

**Header**: `<Engine/Scene/Systems/CollisionSystem.h>`

```cpp
// Systems handle collision detection automatically
// Query collision results:
CollisionSystem& collisionSystem = scene->GetCollisionSystem();

// Or manually check collision between entities
auto& transform1 = entity1.GetComponent<TransformComponent>();
auto& collision1 = entity1.GetComponent<CollisionComponent>();
auto& transform2 = entity2.GetComponent<TransformComponent>();
auto& collision2 = entity2.GetComponent<CollisionComponent>();

SDL_FRect rect1 = collision1.GetBoundsWorld(transform1);
SDL_FRect rect2 = collision2.GetBoundsWorld(transform2);

if (SDL_HasIntersectionF(&rect1, &rect2)) {
    LOG_INFO("Collision detected!");
}
```

---

## Animation

### Animation System

**Header**: `<Engine/Rendering/Animation.h>`

```cpp
// Create animation
auto animation = new SIMPEngine::Animation(
    spritesheet,  // Texture*
    64,           // frame width
    64,           // frame height
    8,            // frame count
    0.1f,         // frame duration (seconds)
    true          // loop
);

// Add to entity
auto& animSprite = entity.AddComponent<AnimatedSpriteComponent>();
animSprite.animation = animation;
animSprite.playing = true;

// Control animation
animSprite.playing = false;  // Pause
animSprite.time = 0;         // Reset
// Playback is automatic in AnimationSystem
```

### Sprite Classes

**Header**: `<Engine/Rendering/Sprite.h>`

```cpp
// Sprite represents a drawable entity
auto sprite = std::make_shared<SIMPEngine::Sprite>(texture);

sprite->SetPosition(x, y);
sprite->SetRotation(angle);
sprite->SetScale(scaleX, scaleY);
sprite->SetZIndex(0);
sprite->SetColor(r, g, b, a);

// Draw the sprite
sprite->Draw(camera);
```

---

## Assets

### Asset Manager

**Header**: `<Engine/Assets/AssetManager.h>`

```cpp
auto& assetManager = SIMPEngine::AssetManager::Get();

// Initialize (done by engine)
assetManager.Init("rootDir", "registry.yaml");

// Import asset if needed
auto handle = assetManager.ImportIfNeeded(
    "assets/textures/player.png",
    SIMPEngine::AssetType::Texture
);

// Get asset (as void*)
void* rawPtr = assetManager.Get(handle);

// Get asset (typed)
auto texture = assetManager.GetAs<SIMPEngine::Texture>(handle);

// Access registry
const auto& registry = assetManager.Registry();
auto& mutableRegistry = assetManager.Registry();
```

### Asset Registry

**Header**: `<Engine/Assets/AssetRegistry.h>`

```cpp
// Registry stores asset metadata
auto& registry = assetManager.Registry();

// Query assets
const auto& allAssets = registry.GetAssets();
for (const auto& asset : allAssets) {
    LOG_INFO("Asset: {}", asset.FilePath);
}

// Get metadata
auto metadata = registry.GetMetadata(handle);
if (metadata) {
    LOG_INFO("Type: {}", (int)metadata->Type);
    LOG_INFO("Path: {}", metadata->FilePath);
}
```

### Virtual File System (VFS)

**Header**: `<Engine/Core/VFS.h>`

```cpp
// Mount asset directory (done by engine)
SIMPEngine::VFS::Mount("assets", "../assets");

// Access assets with VFS paths
auto texture = textureManager.LoadTexture("player", "assets://textures/player.png");
auto scene = sceneManager->...;  // Scene paths also support VFS
```

---

## Logging

### Logging System

**Header**: `<Engine/Core/Log.h>`

```cpp
// Initialize (done by engine)
SIMPEngine::Log::Init();

// Engine/Core logs
CORE_TRACE("Trace message");
CORE_INFO("Info message");
CORE_WARN("Warning message");
CORE_ERROR("Error message");
CORE_CRITICAL("Critical message");

// Game/Client logs
LOG_TRACE("Trace message");
LOG_INFO("Info message");
LOG_WARN("Warning message");
LOG_ERROR("Error message");
LOG_CRITICAL("Critical message");

// With formatting
LOG_INFO("Player at ({}, {})", x, y);
LOG_ERROR("Failed to load: {}", filename);

// Access loggers directly
auto& coreLogger = SIMPEngine::Log::GetCoreLogger();
auto& clientLogger = SIMPEngine::Log::GetClientLogger();
```

### ImGui Sink

```cpp
// Logs are displayed in editor ImGui console
auto sink = SIMPEngine::Log::GetImGuiSink();
// Editor shows all logs in real-time
```

---

## Events

### Event System

**Header**: `<Engine/Events/Event.h>`

```cpp
// Event types
enum class EventType {
    None = 0,
    WindowClose,
    WindowResize,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

// Handle events in layers
class MyLayer : public Layer {
    void OnEvent(Event& e) override {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
            return OnKeyPressed(e);
        });
    }
    
    bool OnKeyPressed(KeyPressedEvent& e) {
        LOG_INFO("Key pressed: {}", e.GetKeyCode());
        e.Handled = true;
        return true;
    }
};
```

### Event Types

**KeyPressedEvent**
```cpp
int GetKeyCode() const;
int GetRepeatCount() const;
```

**KeyReleasedEvent**
```cpp
int GetKeyCode() const;
```

**MouseButtonPressedEvent**
```cpp
int GetButton() const;
```

**MouseButtonReleasedEvent**
```cpp
int GetButton() const;
```

**MouseMovedEvent**
```cpp
float GetX() const;
float GetY() const;
std::pair<float, float> GetPosition() const;
```

**MouseScrolledEvent**
```cpp
float GetXOffset() const;
float GetYOffset() const;
```

**WindowCloseEvent**
```cpp
// No data members
```

**WindowResizeEvent**
```cpp
unsigned int GetWidth() const;
unsigned int GetHeight() const;
```

---

## Scene Serialization

### SceneSerializer

**Header**: `<Engine/Scene/SceneSerializer.h>`

```cpp
auto scene = std::make_shared<SIMPEngine::Scene>("Level1");
SIMPEngine::SceneSerializer serializer(scene.get());

// Deserialize (load from file)
serializer.Deserialize("assets://scenes/level1.yaml");

// Serialize (save to file)
serializer.Serialize("assets://scenes/level1_backup.yaml");

// Scene format (YAML)
// Scene:
//   Name: Level1
//   Entities:
//     - Entity:
//         ID: 12345
//         Name: Player
//         Components:
//           - TagComponent:
//               Tag: Player
//           - TransformComponent:
//               Position: [100, 200]
//               Rotation: 0
//               Scale: [1, 1]
//           - SpriteComponent:
//               Texture: assets://textures/player.png
//               ... and more
```

---

## Tips & Best Practices

### DO ✓

- Use smart pointers (`unique_ptr`, `shared_ptr`) for resources
- Check if components exist before accessing: `if (entity.HasComponent<T>())`
- Use VFS paths for assets: `assets://textures/...`
- Handle events by setting `event.Handled = true`
- Profile performance using `ProfileTimer`
- Log important events using CORE_INFO/LOG_INFO

### DON'T ✗

- Don't store raw pointers from GetComponent (use references)
- Don't call renderer methods from entity scripts directly
- Don't create new scenes inside systems
- Don't delete entities during iteration
- Don't assume textures loaded successfully
- Don't block the main thread with expensive operations

---

For architectural details, see [ARCHITECTURE.md](ARCHITECTURE.md)

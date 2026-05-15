# Getting Started with SIMPEngine

## Installation

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16+
- OpenGL 3.3+

### Building

```bash
# Clone or navigate to project directory
cd SDLGameEngine

# Create build directory
mkdir -p build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run the game
./Game/GameApp
```

## Your First Game Entity

### 1. Create a Scene

```cpp
auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
auto scene = std::make_shared<SIMPEngine::Scene>("MainLevel");

sceneManager->AddScene("MainLevel", scene);
sceneManager->SetActiveScene("MainLevel");
```

### 2. Create an Entity

```cpp
auto entity = scene->CreateEntity("Player");
```

### 3. Add Components

Add a transform component:
```cpp
auto& transform = entity.AddComponent<TransformComponent>();
transform.position = {100, 100};
transform.scale = {1, 1};
```

Add a sprite component:
```cpp
auto texture = SIMPEngine::TextureManager::Get()
    .LoadTexture("player", "assets://textures/player.png");

auto& sprite = entity.AddComponent<SpriteComponent>();
sprite.texture = texture;
sprite.width = texture->GetWidth();
sprite.height = texture->GetHeight();
```

Add a collision component:
```cpp
auto& collision = entity.AddComponent<CollisionComponent>();
collision.width = 32;
collision.height = 32;
```

### 4. Complete Example

```cpp
#include <Engine/SIMPEngine.h>

class TutorialGame : public SIMPEngine::Application
{
public:
    TutorialGame()
    {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("MainLevel");
        
        sceneManager->AddScene("MainLevel", scene);
        sceneManager->SetActiveScene("MainLevel");
        
        // Create player entity
        auto player = scene->CreateEntity("Player");
        
        auto& transform = player.AddComponent<TransformComponent>();
        transform.position = {512, 320};
        
        auto texture = SIMPEngine::TextureManager::Get()
            .LoadTexture("player", "assets://textures/player.png");
        
        auto& sprite = player.AddComponent<SpriteComponent>();
        sprite.texture = texture;
        sprite.width = 64;
        sprite.height = 64;
        
        auto& collision = player.AddComponent<CollisionComponent>();
        collision.width = 64;
        collision.height = 64;
        
        auto renderingLayer = new SIMPEngine::RenderingLayer(sceneManager);
        PushOverlay(renderingLayer);
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new TutorialGame();
}

int main()
{
    SIMPEngine::Log::Init();
    CORE_INFO("Starting game...");
    
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    
    delete app;
    return 0;
}
```

## Input Handling

### Keyboard Input

```cpp
if (SIMPEngine::Input::IsKeyPressed(SIMP_KEY_W))
{
    LOG_INFO("W key pressed");
}
```

### Mouse Input

```cpp
if (SIMPEngine::Input::IsMouseButtonPressed(SDL_BUTTON_LEFT))
{
    auto [x, y] = SIMPEngine::Input::GetMousePosition();
    LOG_INFO("Mouse clicked at ({}, {})", x, y);
}
```

## Scene Serialization

### Loading a Scene

```cpp
SIMPEngine::SceneSerializer serializer(scene.get());
serializer.Deserialize("assets://scenes/level1.yaml");
```

### Saving a Scene

```cpp
serializer.Serialize("assets://scenes/mylevel.yaml");
```

## Using the Physics System

### Add Physics to an Entity

```cpp
auto& physics = entity.AddComponent<PhysicsComponent>();
physics.body = new AccelEngine::RigidBody();
physics.body->setMass(1.0f);
physics.body->setVelocity({10, 0});

// Get physics system from scene
auto& physicsSystem = scene->GetPhysicsSystem();
auto& world = scene->GetPhysicsWorld();

// Bodies are automatically added to the world via systems
```

## Animation

### Create an Animated Sprite

```cpp
auto& animSprite = entity.AddComponent<AnimatedSpriteComponent>();
animSprite.animation = new SIMPEngine::Animation(
    texture,           // spritesheet texture
    64,                // frame width
    64,                // frame height
    4,                 // frame count
    0.1f,              // frame duration
    true               // loop
);
```

## Logging

### Engine Logging

```cpp
CORE_INFO("Engine message");
CORE_WARN("Warning: something");
CORE_ERROR("Error occurred");

LOG_INFO("Game message");
LOG_DEBUG("Debug info");
```

## Using the Editor

### Running with Editor

```bash
./Game/GameApp  # Built with editor by default
```

### Editor Controls

- **Viewport**: Scroll to zoom, middle-click to pan
- **Hierarchy**: Click entities to select
- **Inspector**: Modify component properties
- **Gizmos**: Use transform tools to move/rotate entities

## Asset Management

### Mounting Asset Paths

The VFS is automatically mounted at startup:

```cpp
// Access assets using VFS paths
auto texture = TextureManager::Get().LoadTexture("id", "assets://textures/file.png");
```

### Supported Asset Types

- **Textures**: PNG, JPG, BMP
- **Scenes**: YAML format
- **Tilesets**: Tiled editor (.tsx)
- **Tilemaps**: Tiled editor (.tmx)

## Common Patterns

### Camera Control

```cpp
auto& camera = scene->GetActiveCamera();
camera.SetPosition({player_x, player_y});
camera.SetZoom(2.0f);
```

### Querying Entities

```cpp
auto& registry = scene->GetRegistry();

// Get all entities with specific components
auto view = registry.view<TransformComponent, SpriteComponent>();
for (auto entity : view)
{
    auto& transform = registry.get<TransformComponent>(entity);
    // Process entity
}
```

### Hierarchical Transforms

```cpp
// Set parent-child relationship
scene->SetParent(child_entity, parent_entity);
```

## Troubleshooting

### Assets not loading

- Check asset path uses `assets://` prefix
- Verify VFS is mounted correctly (check logs)
- Ensure asset file exists in correct location

### Scene not rendering

- Verify RenderingLayer is pushed as overlay
- Check entities have TransformComponent and SpriteComponent
- Ensure textures are loaded successfully

### Physics not working

- Verify PhysicsComponent is added to entity
- Check PhysicsSystem is running (included in Scene)
- Ensure mass is > 0

## Next Steps

- Read the [Architecture Overview](ARCHITECTURE.md)
- Explore the [API Reference](API_GUIDE.md)
- Check out [Component Documentation](COMPONENTS.md)
- Look at example scripts in `Game/` directory

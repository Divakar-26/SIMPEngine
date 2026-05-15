# SIMPEngine - A Simple 2D Game Engine

A modern, modular 2D game engine built with C++20, SDL3, OpenGL, and EnTT ECS. Designed for rapid game prototyping and development with integrated editor support.

## Features

- **Entity Component System (ECS)** - Built on EnTT for fast, scalable game logic
- **Scene Management** - Load, switch, and manage multiple game scenes
- **Rendering Pipeline** - Hardware-accelerated 2D rendering with Camera2D support
- **Physics Integration** - Rigid body physics via AccelEngine library
- **Animation System** - Sprite animation with frame-based timing
- **Tilemap Support** - Tiled map format support for level design
- **Input Handling** - Keyboard, mouse, and gamepad input management
- **Asset Management** - Centralized texture, shader, and resource loading
- **Layer System** - Modular layer-based architecture for scene organization
- **Integrated Editor** - Real-time game editor with viewport, hierarchy, and inspector panels
- **Virtual File System (VFS)** - Asset mounting and path abstraction

## Quick Start

### Building the Engine

```bash
cd build
cmake ..
make -j$(nproc)
```

### Creating a Simple Game

```cpp
#include <Engine/SIMPEngine.h>

class MyGame : public SIMPEngine::Application
{
public:
    MyGame()
    {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>("Level1");
        
        sceneManager->AddScene("Level1", scene);
        sceneManager->SetActiveScene("Level1");
        
        auto renderingLayer = new SIMPEngine::RenderingLayer(sceneManager);
        PushOverlay(renderingLayer);
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new MyGame();
}

int main()
{
    SIMPEngine::Log::Init();
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
```

## Project Structure

```
├── Engine/              # Core engine library
│   ├── Application/     # Application lifecycle management
│   ├── Core/            # Logging, windowing, file system
│   ├── Scene/           # Scene management and ECS systems
│   ├── Rendering/       # Rendering API and graphics
│   ├── Input/           # Input handling (keyboard, mouse)
│   ├── Events/          # Event system
│   ├── Layer/           # Layer management
│   ├── Math/            # Camera and math utilities
│   ├── Assets/          # Asset management
│   └── UI/              # ImGui integration
├── Editor/              # Integrated game editor
│   └── Panels/          # Editor UI panels
├── Game/                # Example game implementation
└── vendor/              # Third-party libraries
```

## Core Concepts

### Application Loop

The engine manages a main game loop that:
1. Polls SDL events
2. Updates all active layers
3. Renders ImGui and game content
4. Swaps buffers

### Scene & Entity Component System

Scenes are the primary organizational unit, containing entities with components:

```cpp
auto scene = sceneManager->GetActiveScene();
auto entity = scene->CreateEntity("MyEntity");
entity.AddComponent<SpriteComponent>(...);
entity.AddComponent<TransformComponent>(...);
```

### Systems

Systems process entities with specific component combinations:
- **RenderSystem** - Draws sprites and geometry
- **PhysicsSystem** - Handles rigid body dynamics
- **CameraSystem** - Manages camera positioning
- **AnimationSystem** - Updates sprite animations
- **TilemapSystem** - Renders tilemap layers
- **CollisionSystem** - Detects entity collisions

### Layers

Layers provide modular scene organization:
- **RenderingLayer** - Game rendering
- **EditorLayer** - Editor UI and tools
- **ImGuiLayer** - ImGui rendering

## Documentation

- [Getting Started](docs/GETTING_STARTED.md) - Step-by-step tutorials
- [Architecture Overview](docs/ARCHITECTURE.md) - Deep dive into engine design
- [API Reference](docs/API_GUIDE.md) - Complete API documentation
- [Component System](docs/COMPONENTS.md) - Built-in components guide

## Dependencies

- SDL3 - Window management and events
- OpenGL 3.3+ - Graphics rendering
- EnTT - Entity Component System
- GLM - Math library
- spdlog - Logging
- yaml-cpp - Scene serialization
- ImGui - Editor UI
- AccelEngine - Physics simulation

## Configuration

### Build Options

```cmake
# Debug build with optimizations
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Editor Features

The integrated editor provides:
- **Viewport** - Real-time game view with camera control
- **Hierarchy** - Entity tree view and scene structure
- **Inspector** - Component property editing
- **Content Browser** - Asset management and preview
- **Gizmo System** - Transform manipulation tools

## License

[Your License Here]

## Contributing

[Contributing Guidelines]

---

For detailed implementation guides, see [docs/API_GUIDE.md](docs/API_GUIDE.md).


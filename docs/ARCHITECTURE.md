# SIMPEngine Architecture

## Overview

SIMPEngine follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────┐
│      Application Layer              │
│  (Main Game Loop & Event Loop)      │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Layer Stack                    │
│  ┌──────────────────────────────┐   │
│  │  Editor/UI Layer (ImGui)     │   │
│  ├──────────────────────────────┤   │
│  │  Rendering Layer (Game)      │   │
│  ├──────────────────────────────┤   │
│  │  ImGui Layer (Rendering)     │   │
│  └──────────────────────────────┘   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Scene & ECS System             │
│  ┌──────────────────────────────┐   │
│  │  Scene (Entity Container)    │   │
│  │  ├─ EnTT Registry            │   │
│  │  ├─ Systems                  │   │
│  │  │  ├─ RenderSystem          │   │
│  │  │  ├─ PhysicsSystem         │   │
│  │  │  ├─ AnimationSystem       │   │
│  │  │  ├─ CameraSystem          │   │
│  │  │  └─ Others...             │   │
│  │  └─ Components               │   │
│  └──────────────────────────────┘   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      Core Systems                   │
│  ├─ Renderer (GPU abstraction)      │
│  ├─ Input Handling                  │
│  ├─ Asset Management                │
│  ├─ Logging                         │
│  ├─ Virtual File System             │
│  └─ Window Management               │
└─────────────────────────────────────┘
```

## Core Components

### 1. Application

**File**: `Engine/Application/Application.h`

Entry point for the engine. Manages:
- Window lifecycle
- Main game loop
- Event distribution
- Layer stack management
- Renderer initialization

**Key Methods**:
- `Run()` - Start the main game loop
- `PushLayer()` / `PushOverlay()` - Add layers to the stack
- `OnEvent()` - Route events to layers
- `SDLEventToEngine()` - Convert SDL events to engine events

### 2. Layer System

**File**: `Engine/Layer/Layer.h`

Hierarchical layer organization for modular scene management:

```cpp
class Layer {
    virtual void OnAttach() {}     // Layer attached to stack
    virtual void OnDetach() {}     // Layer detached
    virtual void OnUpdate(TimeStep) {}  // Per-frame update
    virtual void OnRender() {}     // Per-frame render
    virtual void OnEvent(Event&) {}     // Handle events
};
```

**Layer Stack**: LIFO ordering means top layer processes events first. Preventing propagation with `event.Handled = true` blocks lower layers.

### 3. Scene & ECS

**File**: `Engine/Scene/Scene.h`

The scene is the primary organizational container:

```cpp
class Scene {
    entt::registry m_Registry;  // ECS registry
    
    // Systems
    RenderSystem renderSystem;
    PhysicsSystem physicsSystem;
    CameraSystem cameraSystem;
    // ... other systems
    
    void OnUpdate(float dt);    // Update all systems
    void OnRender();            // Render all entities
};
```

**Entity Component System (ECS)**:
- **Entities**: Unique identifiers for game objects
- **Components**: Data containers (transform, sprite, physics, etc.)
- **Systems**: Logic that processes entities with specific components

**Advantages**:
- Cache-friendly iteration
- Data-oriented design
- Easy composition
- No deep inheritance hierarchies

### 4. Component Architecture

Components are data structures attached to entities:

```cpp
struct TransformComponent {
    glm::vec2 position;
    float rotation;
    glm::vec2 scale;
    glm::mat4 worldTransform;
    bool dirty;  // For optimization
};

struct SpriteComponent {
    std::shared_ptr<Texture> texture;
    float width, height;
    SDL_Color color;
};

entity.AddComponent<SpriteComponent>(texture, width, height);
auto& sprite = entity.GetComponent<SpriteComponent>();
```

See [COMPONENTS.md](COMPONENTS.md) for complete component list.

### 5. Systems

Systems contain game logic:

```cpp
class RenderSystem {
    void OnUpdate(Scene& scene) {
        auto view = scene.GetRegistry()
            .view<TransformComponent, SpriteComponent>();
        
        for (auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            auto& sprite = view.get<SpriteComponent>(entity);
            // Render sprite at transform position
        }
    }
};
```

**System Categories**:

#### Transform Systems
- **HierarchySystem** - Parent-child relationships
- **CameraSystem** - Camera positioning and culling

#### Physics Systems
- **PhysicsSystem** - Rigid body simulation
- **CollisionSystem** - Collision detection

#### Rendering Systems
- **RenderSystem** - Sprite rendering
- **TilemapSystem** - Tilemap rendering
- **AnimationSystem** - Animation updates

#### Lifecycle Systems
- **LifetimeSystem** - Entity lifetime management

### 6. Rendering Pipeline

**File**: `Engine/Rendering/Renderer.h`

Abstract rendering API with OpenGL backend:

```
Game Code
    ↓
Renderer (Static Interface)
    ↓
RenderingAPI (Backend Interface)
    ↓
GLRenderingAPI (OpenGL Implementation)
    ↓
GPU
```

**Key Classes**:
- `Renderer` - High-level API (static interface)
- `RenderingAPI` - Low-level rendering contract
- `GLRenderingAPI` - OpenGL implementation
- `Shader` - Shader program management
- `Texture` - Texture loading and management

### 7. Input System

**File**: `Engine/Input/Input.h`

Centralized input handling with static interface:

```cpp
static bool IsKeyPressed(int keyCode);
static bool IsMouseButtonPressed(int button);
static std::pair<int,int> GetMousePosition();
```

Input events flow:
```
SDL_Event → Application::SDLEventToEngine()
    ↓
Engine Event
    ↓
Layer::OnEvent()
    ↓
Input::OnKeyPressed/Released/etc.
    ↓
Input static state updated
```

### 8. Asset Management

**Files**: `Engine/Assets/AssetManager.h`, `Engine/Assets/AssetRegistry.h`

Centralized resource loading and caching:

```cpp
auto texture = AssetManager::Get().ImportIfNeeded("path/to/texture.png", AssetType::Texture);
auto texturePtr = AssetManager::Get().GetAs<Texture>(handle);
```

**Features**:
- Reference counting
- Lazy loading
- Caching
- Metadata tracking
- Extensible importer system

### 9. Virtual File System (VFS)

**File**: `Engine/Core/VFS.h`

Asset path abstraction:

```cpp
VFS::Mount("assets", "../assets");
// Now use: "assets://textures/player.png"
```

Enables:
- Asset relocation without code changes
- Multiple mount points
- Consistent asset access

### 10. Event System

**File**: `Engine/Events/Event.h`

Hierarchical event routing:

```cpp
class Event {
    virtual EventType GetEventType() const = 0;
    bool Handled = false;
};

// Event types: WindowClose, KeyPressed, MouseMoved, etc.
```

**Flow**:
```
SDL_Event
    ↓
Application::Run() polls SDL_PollEvent()
    ↓
Application::SDLEventToEngine() converts to Engine::Event
    ↓
Layer::OnEvent() on each layer (top → bottom)
    ↓
If Handled=true, stop propagation
```

## Data Flow

### Per-Frame Update Cycle

```cpp
while (m_Running) {
    // 1. Input Collection
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        SDLEventToEngine(sdlEvent);
    }
    
    // 2. Layer Updates
    for (Layer* layer : m_LayerStack)
        layer->OnUpdate(deltaTime);
    
    // 3. Rendering
    Renderer::Clear();
    m_ImGuiLayer->Begin();
    
    for (Layer* layer : m_LayerStack)
        layer->OnRender();
    
    m_ImGuiLayer->End();
    m_Window.OnUpdate();  // Swap buffers
}
```

### Scene Update Cycle

```cpp
Scene::OnUpdate(float dt) {
    // Update all systems in sequence
    movementSystem.OnUpdate(*this);
    physicsSystem.OnUpdate(*this);
    collisionSystem.OnUpdate(*this);
    cameraSystem.OnUpdate(*this);
    animationSystem.OnUpdate(*this);
    hierarchySystem.OnUpdate(*this);
    lifeTimeSystem.OnUpdate(*this);
}
```

## Memory Management

### Ownership Model

- **Application** owns Layer pointers (manual delete in destructor)
- **Scene** owns Systems (member variables)
- **Scene** owns Entities via EnTT Registry
- **TextureManager** owns Textures via shared_ptr
- **AssetManager** owns Assets via shared_ptr

**Current Issues**:
- Some raw `new`/`delete` without proper cleanup
- Layers created with `new` should use `unique_ptr`

**Recommendations**:
- Replace raw pointers with `unique_ptr` for ownership
- Use `shared_ptr` for shared resources
- Implement RAII for all resources

## Thread Safety

**Current**: Single-threaded design. All operations on main thread.

**Considerations for future**:
- ImGui is NOT thread-safe
- Renderer calls must be from main thread
- ECS operations should be from single thread
- Consider thread pool for physics/collision

## Extension Points

### Custom Components

```cpp
struct MyCustomComponent {
    int customData;
};

entity.AddComponent<MyCustomComponent>(42);
```

### Custom Systems

```cpp
class CustomSystem {
    void OnUpdate(Scene& scene) {
        auto view = scene.GetRegistry()
            .view<CustomComponent, OtherComponent>();
        // Custom logic
    }
};

// Add to Scene constructor
class MyScene : public Scene {
    CustomSystem customSystem;
};
```

### Custom Events

```cpp
class CustomEvent : public Event {
    EventType GetEventType() const override { return EventType::Custom; }
    const char* GetName() const override { return "CustomEvent"; }
};
```

## Performance Considerations

### Optimization Techniques Used

1. **Component Grouping** - EnTT caches component views
2. **Z-Index Optimization** - Render sorting for depth
3. **Transform Caching** - Dirty flags for matrix updates
4. **Spatial Partitioning** - Collision detection optimization
5. **Asset Caching** - Texture reference counting

### Profiling

Built-in profiling macros:

```cpp
ProfileTimer timer("SystemName");
// Code to profile
// Logged on destruction
```

## Dependencies

```
SIMPEngine
├── SDL3 (window, events, input)
├── OpenGL 3.3+ (rendering)
├── EnTT (ECS)
├── GLM (math)
├── spdlog (logging)
├── yaml-cpp (serialization)
├── imgui (UI)
├── AccelEngine (physics)
└── stb (image loading)
```

## Known Limitations

1. Single-threaded execution
2. No async asset loading
3. Tilemap rendering performance (no LOD/culling optimizations)
4. Limited shader customization
5. No spatial partitioning for broad-phase collision

## Future Improvements

1. Multi-threaded rendering
2. Async asset pipeline
3. Advanced culling and LOD systems
4. Custom shader support
5. Particle system integration
6. Scripting language support (Lua/Python)

---

For API details, see [API_GUIDE.md](API_GUIDE.md)

# SIMPEngine Documentation Index

Complete documentation for the SIMPEngine game engine.

## Quick Links

- **Just Starting?** → [Getting Started](GETTING_STARTED.md)
- **Need Quick Answers?** → [Quick Reference](QUICK_REFERENCE.md)
- **Understanding Architecture?** → [Architecture Overview](ARCHITECTURE.md)
- **Using the API?** → [API Reference](API_GUIDE.md)
- **All Components?** → [Components Guide](COMPONENTS.md)
- **Custom Rendering?** → [Rendering & Graphics](RENDERING_GUIDE.md)

## Documentation Structure

### Beginner Path

Start here if you're new to SIMPEngine:

1. **[Getting Started](GETTING_STARTED.md)** (30 min)
   - Installation & setup
   - Creating your first entity
   - Basic rendering
   - Input handling
   - Common patterns

2. **[Quick Reference](QUICK_REFERENCE.md)** (5 min)
   - Copy-paste code snippets
   - Common tasks
   - Key codes
   - Common mistakes to avoid

### Intermediate Path

For understanding how things work:

3. **[Architecture Overview](ARCHITECTURE.md)** (1 hour)
   - System design
   - Data flow
   - Component architecture
   - Layer system
   - Memory management

4. **[API Reference](API_GUIDE.md)** (2 hours)
   - Complete method documentation
   - All public APIs
   - Usage examples
   - Best practices

### Advanced Path

For extending and customizing:

5. **[Components Guide](COMPONENTS.md)** (1 hour)
   - All built-in components
   - Creating custom components
   - Component patterns
   - Data layout

6. **[Rendering & Graphics](RENDERING_GUIDE.md)** (1.5 hours)
   - Rendering pipeline
   - Shader system
   - Camera control
   - Custom backends
   - Performance optimization

## By Topic

### Getting Started

- [Installation & Build](GETTING_STARTED.md#installation)
- [Your First Entity](GETTING_STARTED.md#your-first-game-entity)
- [Input Handling](GETTING_STARTED.md#input-handling)
- [Physics Basics](GETTING_STARTED.md#using-the-physics-system)

### Core Concepts

- [Application Loop](ARCHITECTURE.md#application-loop)
- [Scene & ECS](ARCHITECTURE.md#3-scene--ecs)
- [Component System](ARCHITECTURE.md#4-component-architecture)
- [Systems](ARCHITECTURE.md#5-systems)
- [Layer System](ARCHITECTURE.md#2-layer-system)

### Game Development

- [Creating Entities](API_GUIDE.md#creating--managing-entities)
- [Working with Components](API_GUIDE.md#adding-components)
- [Transform & Position](COMPONENTS.md#transform-component)
- [Sprites & Rendering](COMPONENTS.md#sprite-component)
- [Physics Integration](API_GUIDE.md#physics)
- [Animation System](API_GUIDE.md#animation)
- [Input System](API_GUIDE.md#input)
- [Collision Detection](COMPONENTS.md#collision-component)

### Resource Management

- [Asset Loading](API_GUIDE.md#assets)
- [Texture Management](RENDERING_GUIDE.md#texture-system)
- [Scene Serialization](API_GUIDE.md#scene-serialization)
- [Virtual File System](API_GUIDE.md#virtual-file-system-vfs)

### Graphics & Rendering

- [Rendering Pipeline](RENDERING_GUIDE.md#overview)
- [Renderer API](RENDERING_GUIDE.md#renderer-api)
- [Camera System](RENDERING_GUIDE.md#camera-system)
- [Shaders](RENDERING_GUIDE.md#shader-system)
- [Sprite Atlasing](RENDERING_GUIDE.md#sprite-atlasing)
- [Performance Tips](RENDERING_GUIDE.md#performance-optimization)

### Advanced Topics

- [Custom Systems](ARCHITECTURE.md#extension-points)
- [Custom Components](COMPONENTS.md#creating-custom-components)
- [Custom Rendering Backend](RENDERING_GUIDE.md#creating-a-custom-rendering-backend)
- [Event System](API_GUIDE.md#events)
- [Custom Scripts](COMPONENTS.md#script-component)

### Troubleshooting

- [Common Issues](GETTING_STARTED.md#troubleshooting)
- [Common Mistakes](QUICK_REFERENCE.md#common-mistakes)
- [Do's and Don'ts](API_GUIDE.md#tips--best-practices)

## API Quick Reference

### Most Used APIs

```cpp
// Scene & Entities
scene->CreateEntity(name);
entity.AddComponent<ComponentType>();
entity.GetComponent<ComponentType>();

// Rendering
Renderer::DrawQuad(x, y, w, h, rot, color);
Renderer::DrawTexture(texture, x, y, w, h, color, rot, z);

// Input
Input::IsKeyPressed(keyCode);
Input::GetMousePosition();

// Logging
LOG_INFO("message");
CORE_INFO("engine message");

// Assets
TextureManager::Get().LoadTexture(id, path);
```

## File Organization

```
docs/
├── README.md                    ← This file
├── GETTING_STARTED.md           ← Start here!
├── QUICK_REFERENCE.md           ← Cheat sheet
├── ARCHITECTURE.md              ← How it works
├── API_GUIDE.md                 ← Complete API
├── COMPONENTS.md                ← All components
└── RENDERING_GUIDE.md           ← Graphics system
```

## Engine Structure

```
Engine/
├── Application/                 ← Game loop & window
├── Core/                        ← Logging, filesystem
├── Scene/                       ← ECS & scene management
│   └── Systems/                 ← Game logic systems
├── Rendering/                   ← Graphics & rendering
├── Input/                       ← Keyboard, mouse
├── Events/                      ← Event system
├── Layer/                       ← Layer management
├── Math/                        ← Camera & math
├── Assets/                      ← Resource management
└── UI/                          ← ImGui integration
```

## Common Tasks

### I want to...

**...load a texture**
- See: [Texture Management](RENDERING_GUIDE.md#texture-system) and [Quick Reference](QUICK_REFERENCE.md#rendering)

**...move an entity**
- See: [Transform](QUICK_REFERENCE.md#transform) and [Components - Transform](COMPONENTS.md#transform-component)

**...handle input**
- See: [Input System](API_GUIDE.md#input) and [Quick Reference](QUICK_REFERENCE.md#input)

**...create animations**
- See: [Animation System](API_GUIDE.md#animation) and [Animated Sprite Component](COMPONENTS.md#animated-sprite-component)

**...add physics**
- See: [Physics Integration](API_GUIDE.md#physics) and [Physics Component](COMPONENTS.md#physics-component)

**...detect collisions**
- See: [Collision Detection](API_GUIDE.md#collision-detection) and [Collision Component](COMPONENTS.md#collision-component)

**...serialize scenes**
- See: [Scene Serialization](API_GUIDE.md#scene-serialization) and [Getting Started](GETTING_STARTED.md#scene-serialization)

**...extend the engine**
- For custom components: [Creating Custom Components](COMPONENTS.md#creating-custom-components)
- For custom systems: [Architecture - Extension Points](ARCHITECTURE.md#extension-points)
- For custom rendering: [Custom Rendering Backend](RENDERING_GUIDE.md#creating-a-custom-rendering-backend)

**...debug my game**
- See: [Logging](API_GUIDE.md#logging) and [Performance Tips](QUICK_REFERENCE.md#performance-tips)

**...optimize performance**
- See: [Performance Optimization](RENDERING_GUIDE.md#performance-optimization) and [Architecture Performance](ARCHITECTURE.md#performance-considerations)

## FAQ

**Q: How do I create a simple game?**
A: Follow [Getting Started](GETTING_STARTED.md#your-first-game-entity) for a complete example.

**Q: What's the difference between layers and scenes?**
A: Scenes contain game logic (ECS entities). Layers organize rendering and UI on top of scenes. See [Architecture](ARCHITECTURE.md#2-layer-system).

**Q: How do systems work?**
A: Systems are functions that process entities with matching components. See [Architecture - Systems](ARCHITECTURE.md#5-systems).

**Q: Can I use my own rendering backend?**
A: Yes! See [Creating a Custom Rendering Backend](RENDERING_GUIDE.md#creating-a-custom-rendering-backend).

**Q: How do I save/load games?**
A: Use [Scene Serialization](API_GUIDE.md#scene-serialization) to save/load scene state.

**Q: What key codes are available?**
A: See [Quick Reference - Key Codes](QUICK_REFERENCE.md#key-codes).

**Q: Is the engine thread-safe?**
A: No, currently single-threaded. See [Architecture - Thread Safety](ARCHITECTURE.md#thread-safety).

**Q: How do I profile my game?**
A: Use `ProfileTimer` and logging. See [Performance Tips](QUICK_REFERENCE.md#performance-tips).

## Examples

Example code snippets are integrated throughout the documentation. Key examples:

- **Basic Entity Setup** → [Quick Reference](QUICK_REFERENCE.md#creating-your-game)
- **Game Loop** → [Quick Reference](QUICK_REFERENCE.md#creating-your-game)
- **Input Handling** → [Quick Reference](QUICK_REFERENCE.md#input)
- **Physics** → [Quick Reference](QUICK_REFERENCE.md#physics)
- **Custom Scripts** → [Quick Reference](QUICK_REFERENCE.md#custom-script-component)
- **ECS Queries** → [Quick Reference](QUICK_REFERENCE.md#entity-queries)

## Getting Help

1. **Check the docs** - Start with [Quick Reference](QUICK_REFERENCE.md)
2. **Look at examples** - [Getting Started](GETTING_STARTED.md) has full examples
3. **Check Common Mistakes** - [Quick Reference - Common Mistakes](QUICK_REFERENCE.md#common-mistakes)
4. **Review Architecture** - [Architecture](ARCHITECTURE.md) explains how systems work
5. **Check specific component docs** - [Components](COMPONENTS.md) for each component

## Documentation Quality Checklist

All documentation includes:

- ✅ Clear examples with copy-paste code
- ✅ Method signatures and parameters
- ✅ Return values and error cases
- ✅ Common use cases
- ✅ Do's and don'ts
- ✅ Links to related topics

## Keeping Documentation Updated

Documentation is maintained alongside code. When adding features:

1. Update relevant documentation sections
2. Add code examples
3. Update Quick Reference for commonly used features
4. Update Architecture if design changes
5. Keep links up-to-date

---

**Last Updated:** May 15, 2026

**Engine Version:** 1.0

**Questions?** Check the relevant guide above or review the code examples!

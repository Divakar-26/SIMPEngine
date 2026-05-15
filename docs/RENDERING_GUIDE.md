# Rendering & Graphics Guide

Complete guide to SIMPEngine's rendering system and how to extend it.

## Overview

The rendering system follows an abstraction layer pattern:

```
High-Level API (Renderer static interface)
    ↓
Rendering API (Backend interface)
    ↓
GL Rendering API (OpenGL implementation)
    ↓
GPU/Graphics Card
```

This design allows multiple rendering backends while keeping game code independent of the implementation.

## Renderer API

**Header**: `Engine/Rendering/Renderer.h`

### Initialization

```cpp
Renderer::Init(std::make_unique<GLRenderingAPI>(), windowWidth, windowHeight);
```

### Clearing & Colors

```cpp
// Set background color
Renderer::SetClearColor(r, g, b, a);

// Clear frame buffer
Renderer::Clear();

// Swap buffers (called by engine)
Renderer::Present();
```

### Drawing Primitives

```cpp
// Draw filled or outlined quad
Renderer::DrawQuad(
    x, y,              // Position
    width, height,     // Size
    rotation = 0.0f,   // Rotation in degrees
    color = {255,255,255,255},
    fill = true,       // true = filled, false = outline
    zIndex = 0.0f      // Depth layer
);

// Draw line
Renderer::DrawLine(
    x1, y1,            // Start point
    x2, y2,            // End point
    color
);
```

### Drawing Textures

```cpp
auto texture = TextureManager::Get().GetTexture("id");

Renderer::DrawTexture(
    texture,           // Texture pointer
    x, y,              // Position
    width, height,     // Size
    color = {255,255,255,255},
    rotation = 0.0f,   // Rotation
    zIndex = 0.0f,     // Depth
    srcRect = nullptr  // Source rectangle for sprite atlasing
);
```

### View & Projection

```cpp
// Set camera view matrix
Renderer::SetViewMatrix(glm::mat4 viewMatrix);

// Get current view matrix
const auto& view = Renderer::GetViewMatrix();

// Get viewport size
auto size = Renderer::GetViewportSize();
```

### Frame Management

```cpp
// Begin new frame
Renderer::BeginFrame();

// End frame and submit
Renderer::EndFrame();

// Force flush pending draw calls
Renderer::Flush();

// Swap buffers
Renderer::Present();
```

## Texture System

### Loading Textures

```cpp
auto& tm = SIMPEngine::TextureManager::Get();

// Load from file
auto texture = tm.LoadTexture("id", "assets://path/texture.png");

// Get already loaded
auto loaded = tm.GetTexture("id");

// Unload
tm.UnloadTexture("id");

// Clear all
tm.UnloadAll();
```

### Texture Class

**Header**: `Engine/Rendering/Texture.h`

```cpp
class Texture {
public:
    bool LoadFromFile(const char* path);
    void Destroy();
    
    GLuint GetID() const;
    int GetWidth() const;
    int GetHeight() const;
    
    std::string GetPath() const;
    std::string GetVFSPath() const;
    void SetVFSPath(std::string vfsPath);
    
    bool LoadFromMemory(unsigned char* data, size_t size);
    void UploadToGPU(unsigned char* pixels, int width, int height, int channels);
};
```

### Supported Formats

- PNG (8/16/32-bit)
- JPG
- BMP
- TGA

## Shader System

### Creating & Using Shaders

```cpp
auto shader = std::make_shared<SIMPEngine::Shader>();

// Load from files
shader->LoadFromFile("shaders/sprite.vert", "shaders/sprite.frag");

// Or load from strings
shader->LoadFromString(vertexSource, fragmentSource);

// Bind for rendering
shader->Bind();

// Set uniforms
shader->SetUniform1i("texture", 0);
shader->SetUniform1f("opacity", 1.0f);
shader->SetUniform2f("offset", 10, 20);
shader->SetUniform3f("color", 1.0f, 0.0f, 0.0f);
shader->SetUniform4f("mix", 1.0f, 0.0f, 0.0f, 0.5f);
shader->SetUniformMat4("transform", matrix);

// Unbind
shader->Unbind();
```

### Shader File Format

**Vertex Shader** (`sprite.vert`):
```glsl
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    gl_Position = uProjection * uView * vec4(aPosition, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
```

**Fragment Shader** (`sprite.frag`):
```glsl
#version 330 core

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;

layout(location = 0) out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    FragColor = texColor * vColor;
}
```

## Camera System

### Camera2D Class

**Header**: `Engine/Math/Camera2D.h`

```cpp
class Camera2D {
public:
    void SetPosition(float x, float y);
    glm::vec2 GetPosition() const;
    
    void SetZoom(float zoom);
    float GetZoom() const;
    
    void SetRotation(float angle);
    float GetRotation() const;
    
    glm::mat4 GetViewMatrix() const;
    
    // Coordinate conversion
    glm::vec2 ScreenToWorld(float screenX, float screenY) const;
    glm::vec2 WorldToScreen(float worldX, float worldY) const;
};
```

### Using Camera in Game

```cpp
// Create camera entity
auto cameraEntity = scene->CreateEntity("MainCamera");
auto& transform = cameraEntity.AddComponent<TransformComponent>();

auto& cameraComp = cameraEntity.AddComponent<CameraComponent>();
cameraComp.isPrimary = true;  // Use this camera
cameraComp.camera.SetZoom(2.0f);

// Follow player (in update loop)
auto& playerTransform = player.GetComponent<TransformComponent>();
cameraComp.camera.SetPosition(playerTransform.position);

// Screen to world (for mouse picking)
Camera2D& camera = scene->GetActiveCamera();
auto [screenX, screenY] = Input::GetMousePosition();
glm::vec2 worldPos = camera.ScreenToWorld(screenX, screenY);

// Check if world position is on screen
glm::vec2 screenPos = camera.WorldToScreen(worldX, worldY);
bool onScreen = (screenPos.x >= 0 && screenPos.x < 1920 &&
                 screenPos.y >= 0 && screenPos.y < 1080);
```

## RenderingAPI (Backend Interface)

**Header**: `Engine/Rendering/RenderingAPI.h`

For extending with new rendering backends:

```cpp
class RenderingAPI {
public:
    virtual ~RenderingAPI() = default;
    
    virtual void Init(int width, int height) = 0;
    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void Clear() = 0;
    virtual void Present() = 0;
    
    virtual void DrawQuad(float x, float y, float w, float h,
                         float rot, const SDL_Color& color,
                         bool fill, float zIndex) = 0;
    
    virtual void DrawTexture(std::shared_ptr<Texture> texture,
                            float x, float y, float w, float h,
                            const SDL_Color& color, float rot,
                            float zIndex, const SDL_FRect* srcRect) = 0;
    
    virtual void DrawLine(float x1, float y1, float x2, float y2,
                         const SDL_Color& color) = 0;
    
    virtual void SetViewMatrix(const glm::mat4& view) = 0;
    
    virtual glm::mat4 GetViewMatrix() const = 0;
    virtual glm::vec2 GetViewportSize() const = 0;
    virtual void Flush() = 0;
};
```

## GL Rendering API (Implementation)

**Header**: `Engine/Rendering/GLRenderingAPI.h`

Current implementation using OpenGL 3.3+.

### Key Methods

```cpp
class GLRenderingAPI : public RenderingAPI {
    void Init(int width, int height) override;
    void SetClearColor(float r, float g, float b, float a) override;
    void Clear() override;
    void DrawQuad(...) override;
    void DrawTexture(...) override;
    void DrawLine(...) override;
    void SetViewMatrix(const glm::mat4& view) override;
    glm::vec2 GetViewportSize() const override;
    void Flush() override;
};
```

## Creating a Custom Rendering Backend

To add a new rendering backend (e.g., DirectX, Vulkan):

### 1. Implement RenderingAPI

```cpp
// MyRenderer.h
#pragma once
#include <Engine/Rendering/RenderingAPI.h>

class MyCustomRenderingAPI : public SIMPEngine::RenderingAPI {
public:
    void Init(int width, int height) override {
        // Initialize your graphics API
    }
    
    void SetClearColor(float r, float g, float b, float a) override {
        // Set clear color in your API
    }
    
    void Clear() override {
        // Clear buffers
    }
    
    void DrawQuad(float x, float y, float w, float h,
                 float rot, const SDL_Color& color,
                 bool fill, float zIndex) override {
        // Implement quad rendering
    }
    
    void DrawTexture(std::shared_ptr<Texture> texture,
                    float x, float y, float w, float h,
                    const SDL_Color& color, float rot,
                    float zIndex, const SDL_FRect* srcRect) override {
        // Implement texture rendering
    }
    
    void DrawLine(float x1, float y1, float x2, float y2,
                 const SDL_Color& color) override {
        // Implement line rendering
    }
    
    void SetViewMatrix(const glm::mat4& view) override {
        m_ViewMatrix = view;
    }
    
    glm::mat4 GetViewMatrix() const override {
        return m_ViewMatrix;
    }
    
    glm::vec2 GetViewportSize() const override {
        return {m_Width, m_Height};
    }
    
    void Flush() override {
        // Submit batched draw calls
    }
    
    void Present() override {
        // Present to screen
    }
    
private:
    glm::mat4 m_ViewMatrix;
    int m_Width, m_Height;
};
```

### 2. Use in Application

```cpp
// Update Application::Application() to use your API
Renderer::Init(std::make_unique<MyCustomRenderingAPI>(), w, h);
```

## Sprite Atlasing

Use sprite atlases to reduce draw calls:

```cpp
// Load spritesheet
auto spritesheet = TextureManager::Get()
    .LoadTexture("sprites", "assets://sprites/sheet.png");

// Define sprite regions
SDL_FRect frame1 = {0, 0, 64, 64};      // Top-left sprite
SDL_FRect frame2 = {64, 0, 64, 64};     // Next sprite

// Draw from spritesheet
Renderer::DrawTexture(
    spritesheet,
    x, y,
    64, 64,
    {255, 255, 255, 255},
    0.0f,
    0.0f,
    &frame1  // Source rectangle
);
```

## Post-Processing

For full-screen effects, render to texture then apply shader:

```cpp
// 1. Create framebuffer texture
// 2. Render scene to framebuffer
// 3. Apply post-process shader
// 4. Render result to screen

// Note: Current engine doesn't have built-in post-processing
// This would require custom implementation
```

## Performance Optimization

### Batching

The rendering system automatically batches similar draw calls. To optimize:

1. **Sort by texture** - Minimize texture switches
2. **Sort by shader** - Keep shader constant between draws
3. **Sort by zIndex** - Natural order maintained by zIndex

### Tips

```cpp
// ❌ Inefficient - many texture switches
for (auto entity : entities) {
    Renderer::DrawTexture(entity.texture, ...);
}

// ✅ Efficient - batch by texture
std::sort(entities.begin(), entities.end(),
    [](const Entity& a, const Entity& b) {
        return a.texture.get() < b.texture.get();
    });
for (auto entity : entities) {
    Renderer::DrawTexture(entity.texture, ...);
}
```

### Z-Index Optimization

Use z-index efficiently:

```cpp
// ❌ Many unique z-indices cause sorting overhead
for (int i = 0; i < 1000; i++) {
    entity.zIndex = i * 0.001f;  // Too granular
}

// ✅ Use meaningful layers
constexpr float BACKGROUND = 0.0f;
constexpr float MIDGROUND = 50.0f;
constexpr float FOREGROUND = 100.0f;
```

## Color & Blending

### Color Tinting

```cpp
auto& sprite = entity.GetComponent<SpriteComponent>();
sprite.color = {255, 0, 0, 255};    // Red tint
sprite.color = {128, 128, 128, 255}; // Gray tint
sprite.color = {255, 255, 255, 255}; // No tint (white)
```

### Transparency

```cpp
sprite.opacity = 1.0f;    // Fully opaque
sprite.opacity = 0.5f;    // 50% transparent
sprite.opacity = 0.0f;    // Invisible

// Fade effect
sprite.opacity = glm::clamp(sprite.opacity - 0.01f * deltaTime, 0.0f, 1.0f);
```

## Viewport & Window Resizing

```cpp
// Get viewport dimensions
auto size = Renderer::GetViewportSize();
float width = size.x;
float height = size.y;

// Handle window resize (automatic via engine)
// Camera aspect ratio should be updated on resize
```

---

**For component guide:** [COMPONENTS.md](COMPONENTS.md)

**For architecture details:** [ARCHITECTURE.md](ARCHITECTURE.md)

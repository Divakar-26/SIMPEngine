#pragma once
#include <Engine/Rendering/Texture.h>

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <cstdint>
#include <memory>
#include <iostream>

namespace SIMPEngine
{
    class Framebuffer;

    struct RenderStats
    {
        int drawCalls = 0;
        int primitives = 0;
        int quadCount = 0;
        int lineSegmentCount = 0;
        int shapeCount = 0;
        int glyphCount = 0;
        int textureSwitches = 0;
    };

    enum class BlendMode : uint8_t
    {
        Alpha,
        Additive,
        Multiply
    };

    enum class PrimitiveMode : uint8_t
    {
        Sprite = 0,
        Shape = 1,
        SdfText = 2,
        Light = 3
    };

    struct Primitive
    {
        glm::vec2 Center{0.0f};
        glm::vec2 HalfSize{0.5f};
        float Rotation = 0.0f;
        float Z = 0.0f;
        glm::vec4 Color{1.0f};
        glm::vec4 UV{0.0f, 1.0f, 1.0f, 0.0f};
        unsigned int Texture = 0;
        PrimitiveMode Mode = PrimitiveMode::Sprite;
        BlendMode Blend = BlendMode::Alpha;
        float CornerRadius = 0.0f;
        float Stroke = 0.0f;
        float Softness = 1.0f;
        float Padding = 0.0f;
    };

    class RenderingAPI
    {
    public:
        virtual ~RenderingAPI();

        virtual void Init() = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear() = 0;

        virtual void SetViewMatrix(const glm::mat4 &view) = 0;
        virtual void SetProjectionMatrix(const glm::mat4 &proj) = 0;
        virtual const glm::mat4 &GetViewMatrix() const = 0;
        virtual const glm::mat4 &GetProjectionMatrix() const = 0;

        virtual void Submit(const Primitive &primitive) = 0;
        virtual void Flush() = 0;

        virtual void BindRenderTarget(Framebuffer *target) = 0;
        virtual Framebuffer *GetRenderTarget() const = 0;

        virtual void ResizeViewport(int width, int height) = 0;
        virtual glm::vec2 GetViewportSize() = 0;

        virtual const RenderStats &GetStats() const = 0;
        virtual void ResetStats() = 0;
    };
}

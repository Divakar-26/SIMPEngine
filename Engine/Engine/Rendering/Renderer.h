#pragma once
#include <Engine/Rendering/RenderingAPI.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Font.h>

#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace SIMPEngine
{
    class SvgDocument;
    class Framebuffer;

    struct SpriteDesc
    {
        std::shared_ptr<Texture> TextureRef;
        glm::vec2 Position{0.0f};
        glm::vec2 Size{1.0f};
        float Rotation = 0.0f;
        float Z = 0.0f;
        SDL_Color Tint{255, 255, 255, 255};
        const SDL_FRect *SourceRect = nullptr;
        bool FlipX = false;
        bool FlipY = false;
    };

    class Renderer
    {
    public:
        static void Init(std::unique_ptr<RenderingAPI> api, int w, int h);
        static void SetWindowSize(int w, int h);
        static void Shutdown();

        static RenderingAPI *GetAPI();

        static void SetClearColor(float r, float g, float b, float a);
        static void Clear();

        static void BeginScene(const glm::mat4 &view, const glm::mat4 &projection);
        static void EndScene();

        static void BeginRenderTarget(Framebuffer &target);
        static void EndRenderTarget();
        static void Recover();

        static void SetBlendMode(BlendMode mode);
        static BlendMode GetBlendMode();

        static void SetColorMultiplier(const glm::vec4 &multiplier);
        static const glm::vec4 &GetColorMultiplier();

        static void DrawQuad(float x, float y, float width, float height, float rotation = 0.0f, SDL_Color color = {255, 255, 255, 255}, bool fill = true, float zIndex = 0.0f);
        static void DrawRect(float x, float y, float width, float height, float thickness, SDL_Color color, float rotation = 0.0f, float zIndex = 0.0f);
        static void DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect *srcRect = nullptr);
        static void DrawSprite(const SpriteDesc &desc);
        static void DrawTriangles(const glm::vec2 *positions, const glm::vec4 *colors, size_t vertexCount, const uint32_t *indices, size_t indexCount, float zIndex = 0.0f);

        static void DrawLine(float x1, float y1, float x2, float y2, SDL_Color color, float zIndex = 0.0f);
        static void DrawLine(float x1, float y1, float x2, float y2, float width, SDL_Color color, float zIndex = 0.0f);

        static void DrawCircle(float cx, float cy, float radius,
                               SDL_Color color, float aa = 1.0f, float zIndex = 0.0f);
        static void DrawRing(float cx, float cy, float radius, float thickness,
                             SDL_Color color, float aa = 1.0f, float zIndex = 0.0f);

        static void DrawRoundedRect(float cx, float cy, float halfW, float halfH,
                                    float cornerRadius, SDL_Color color,
                                    float aa = 1.0f, float zIndex = 0.0f);

        static void DrawCapsuleLine(float x1, float y1, float x2, float y2,
                                    float width, SDL_Color color,
                                    float aa = 1.0f, float zIndex = 0.0f);

        static void DrawText(const std::string &text, float x, float y, float size, SDL_Color color,
                             TextAlign align = TextAlign::Left, float rotation = 0.0f, float zIndex = 0.0f,
                             const Font *font = nullptr);

        static void DrawLight(const glm::vec2 &center, float radius, float rotationDegrees, const glm::vec4 &color,
                              float falloff, float innerAngleDegrees = 0.0f, float outerAngleDegrees = 0.0f, float zIndex = 0.0f);

        static void DrawSvg(SvgDocument &doc, float x, float y, float width, float height,
                            SDL_Color tint = {255, 255, 255, 255}, float rotation = 0.0f, float zIndex = 0.0f);

        static void Present();

        static void SetViewMatrix(const glm::mat4 &view);
        static void SetProjectionMatrix(const glm::mat4 &projection);
        static const glm::mat4 &GetViewMatrix();
        static glm::vec2 GetViewportSize();
        static float GetWorldPerPixel();
        static void GetVisibleBounds(glm::vec2 &min, glm::vec2 &max);
        static bool IsVisible(const glm::vec2 &center, const glm::vec2 &halfExtent);

        static const RenderStats &GetStats();
        static void ResetStats();

        static void Flush();
        static void BeginFrame() { s_RenderingAPI->BeginFrame(); }
        static void EndFrame() { s_RenderingAPI->EndFrame(); }
        static int m_WindowWidth, m_WindowHeight;

    private:
        static void SubmitPrimitive(Primitive &primitive);

        static std::unique_ptr<RenderingAPI> s_RenderingAPI;
        static glm::mat4 s_ViewMatrix;
        static BlendMode s_BlendMode;
        static glm::vec4 s_ColorMultiplier;
        static glm::mat4 s_CullViewProjection;
        static glm::vec2 s_CullMin;
        static glm::vec2 s_CullMax;
    };
}

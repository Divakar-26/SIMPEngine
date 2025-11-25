#pragma once
#include <Engine/Rendering/RenderingAPI.h>
#include <Engine/Rendering/Shader.h>
#include <Engine/Rendering/Texture.h>

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <memory>

namespace SIMPEngine
{
    class GLRenderingAPI : public RenderingAPI
    {
    public:
        GLRenderingAPI();
        virtual ~GLRenderingAPI();

        void Init() override;
        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;
        void Present() override;

        void DrawQuad(float x, float y, float width, float height, float rotation = 0.0f, SDL_Color color = {255, 255, 255, 255}, bool fill = true, float zIndex = 0.0f) override;
        void DrawLine(float, float, float, float, SDL_Color) override;
        void DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect *srcRect = nullptr) override;
        std::shared_ptr<Texture> CreateTexture(const char *) override { return nullptr; }

        void Flush() override {}

        void ResizeViewport(int, int) override;
        void SetProjection(float width, float height);
        void SetViewMatrix(const glm::mat4 &view) override;

        unsigned int GetViewportTexture() override;
        glm::vec2 GetViewportSize() {return {m_ViewportWidth, m_ViewportHeight};}

        void BeginFrame() override;
        void EndFrame() override;
        void InitFramebuffer(int width, int height);

        // -- helper functions----

        void ApplyCommonSpriteState(
            float x, float y, float width, float height,
            float rotation, float zIndex,
            SDL_Color color, bool useTexture);

        int m_ViewportWidth = 1920;
        int m_ViewportHeight = 1080;

    private:
        float m_ClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

        unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
        unsigned int m_LineVAO = 0, m_LineVBO = 0;
        std::unique_ptr<Shader> m_Shader;

        glm::mat4 m_Projection = glm::mat4(1.0f);
        glm::mat4 m_ViewMatrix = glm::mat4(1.0f);

        glm::vec2 TransformPosition(float x, float y) const;
        glm::vec2 TransformSize(float w, float h) const;

        unsigned int m_Framebuffer = 0;
        unsigned int m_ColorAttachment = 0;
        unsigned int m_RBO = 0;
    };
}

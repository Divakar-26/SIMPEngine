// GLRenderingAPI.h
#pragma once
#include "RenderingAPI.h"
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

        void DrawQuad(float x, float y, float width, float height, SDL_Color color, bool fill = true) override;

        // Empty stubs
        void DrawCircle(float, float, float, SDL_Color) override {}
        void DrawLine(float, float, float, float, SDL_Color) override {}
        void SetViewMatrix(const glm::mat4 &) override {}
        void DrawTexture(SDL_Texture *, float, float, float, float, SDL_Color, float, const SDL_FRect *) override {}
        std::shared_ptr<Texture> CreateTexture(const char *) override { return nullptr; }
        void Flush() override {}
        SDL_Renderer *GetSDLRenderer() override { return nullptr; }
        void ResizeViewport(int, int) override {}

        // In GLRenderingAPI.h
        unsigned int GetViewportTexture() override { return 0; }
        void BeginFrame() override {}
        void EndFrame() override {}

    private:
        float m_ClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

        unsigned int m_ShaderProgram = 0;
        unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;

        void CreateShaders();
    };
}

// Renderer.h
#pragma once
#include <Engine/Rendering/RenderingAPI.h>
#include <Engine/Rendering/Texture.h>

#include <glm/glm.hpp>
#include <memory>

namespace SIMPEngine
{

    class Renderer
    {
    public:
        static void Init(std::unique_ptr<RenderingAPI> api, int w, int h);

        // Renderer.h
        static RenderingAPI *GetAPI(); // add this

        static void SetClearColor(float r, float g, float b, float a);
        static void Clear();
        static void DrawQuad(float x, float y, float width, float height, float rotation = 0.0f ,SDL_Color color = {255,255,255,255}, bool fill = true, float zIndex = 0.0f);
        static void DrawCircle(float x, float y, float r, SDL_Color color);
        static void DrawLine(float x1, float y1, float x2, float y2, SDL_Color color);
        static void Present();

        static void SetViewMatrix(const glm::mat4 &view);
        static const glm::mat4 &GetViewMatrix();

        static void DrawTexture(GLuint texture, float x, float y, float w, float h, SDL_Color tint, float rotation, float zIndex);
        static std::shared_ptr<Texture> CreateTexture(const char *path);

        static void Flush();
        static void BeginFrame();
        static void EndFrame();

        static SDL_Renderer *GetSDLRenderer();

        static int m_WindowWidth, m_WindowHeight;

    private:
        static std::unique_ptr<RenderingAPI> s_RenderingAPI;
        static glm::mat4 s_ViewMatrix;
    };

}

// Renderer.h
#pragma once
#include "RenderingAPI.h"
#include <glm/glm.hpp>
#include <memory>
#include "Texture.h"

namespace SIMPEngine
{

    class Renderer
    {
    public:
        static void Init(std::unique_ptr<RenderingAPI> api, SDL_Renderer *sdlRenderer);
        static void SetClearColor(float r, float g, float b, float a);
        static void Clear();
        static void DrawQuad(float x, float y, float width, float height, SDL_Color color);
        static void DrawCircle(float x, float y, float r, SDL_Color color);
        static void Present();

        static void SetViewMatrix(const glm::mat4 &view);
        static const glm::mat4 &GetViewMatrix();

        static void DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, const SDL_FRect *srcRect = nullptr);
        static std::shared_ptr<Texture> CreateTexture(const char *path);

        static SDL_Renderer *GetSDLRenderer();

    private:
        static std::unique_ptr<RenderingAPI> s_RenderingAPI;
        static glm::mat4 s_ViewMatrix;
    };

}

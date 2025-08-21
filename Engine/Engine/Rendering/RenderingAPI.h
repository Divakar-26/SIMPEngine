#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <memory>
#include "Texture.h"

namespace SIMPEngine
{
    class RenderingAPI
    {
    public:
        virtual ~RenderingAPI();

        virtual void Init(SDL_Renderer *sdlRenderer) = 0;
        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear() = 0;
        virtual void DrawQuad(float x, float y, float width, float height, SDL_Color color) = 0;
        virtual void DrawCircle(float x, float y, float r, SDL_Color color) = 0;
        virtual void Present() = 0;
        virtual void SetViewMatrix(const glm::mat4 &view) = 0;

        virtual void DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, const SDL_FRect *srcRect = nullptr) = 0;

        virtual std::shared_ptr<Texture> CreateTexture(const char *path) = 0;

        virtual void ResizeViewport(int width, int height) = 0;

        virtual SDL_Texture *GetViewportTexture()
        {
            return nullptr;
        }

        virtual void Flush() = 0;

        virtual SDL_Renderer *GetSDLRenderer() = 0;
    };
}
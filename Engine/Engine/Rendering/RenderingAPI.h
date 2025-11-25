#pragma once
#include <Engine/Rendering/Texture.h>

#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <memory>
#include <iostream>

namespace SIMPEngine
{

    class RenderingAPI
    {
    public:
        virtual ~RenderingAPI();

        virtual void Init() = 0;
        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear() = 0;
        virtual void DrawQuad(float x, float y, float width, float height,float rotation, SDL_Color color, bool fill, float zIndex) = 0;
        virtual void DrawLine(float x1, float y1, float x2, float y2, SDL_Color color) = 0;
        virtual void Present() = 0;
        virtual void SetViewMatrix(const glm::mat4 &view) = 0;

        virtual void DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect* srcRect = nullptr) = 0;

        virtual std::shared_ptr<Texture> CreateTexture(const char *path) = 0;

        virtual void ResizeViewport(int width, int height) = 0;
        virtual glm::vec2 GetViewportSize() = 0;

        virtual unsigned int GetViewportTexture() = 0;

        virtual void Flush() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
    };
}
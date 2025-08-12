#pragma once
#include "RenderingAPI.h"
#include <glm/glm.hpp>

#include"Texture.h"
#include<memory>

namespace SIMPEngine
{
    class SDLRenderingAPI : public RenderingAPI
    {
    public:
        SDLRenderingAPI();
        virtual ~SDLRenderingAPI() = default;

        void Init(SDL_Renderer *sdlRenderer) override;
        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;
        void DrawQuad(float x, float y, float width, float height, SDL_Color color) override;
        void DrawLine(float x1, float y1, float x2, float y2, SDL_Color color) override;
        void DrawCircle(float x, float y, float r, SDL_Color color) override;
        void Present() override;
        void SetViewMatrix(const glm::mat4 &view) override; // <-- override here

        void DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, SDL_FRect * srcRect = nullptr);
        std::shared_ptr<Texture> CreateTexture(const char* path);


        virtual void SetViewport(int x, int y, int width, int height) override;
        virtual void ResetViewport() override;
        virtual SDL_Renderer * GetSDLRenderer() override;

    private:
        SDL_Renderer *m_Renderer = nullptr;
        SDL_Color m_ClearColor{0, 0, 0, 255};
        glm::mat4 s_ViewMatrix = glm::mat4(1.0f);

        Texture circleTexture;
    };
}
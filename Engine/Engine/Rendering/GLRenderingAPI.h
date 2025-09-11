#pragma once
#include "RenderingAPI.h"
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
namespace SIMPEngine
{

    class GLRenderingAPI : public RenderingAPI
    {
    public:
        GLRenderingAPI();
        virtual ~GLRenderingAPI() = default;

        void Init() override;
        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;
        void Present() override;

        void DrawQuad(float x, float y, float width, float height, SDL_Color color, bool fill = true) override;
        void DrawCircle(float x, float y, float r, SDL_Color color) override;
        void DrawLine(float x1, float y1, float x2, float y2, SDL_Color color) override;
        void SetViewMatrix(const glm::mat4 &view) override;
        void DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, const SDL_FRect *srcRect = nullptr) override;
        std::shared_ptr<Texture> CreateTexture(const char *path) override;

        void Flush() override;

        SDL_Renderer *GetSDLRenderer() override;
        void ResizeViewport(int width, int height) override;

    private:
        float m_ClearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    };

};
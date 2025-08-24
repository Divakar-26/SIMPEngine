#pragma once
#include "RenderingAPI.h"
#include <glm/glm.hpp>
#include <vector>
#include "Texture.h"
#include <memory>

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
        void DrawQuad(float x, float y, float width, float height,  SDL_Color color, bool fill = true) override;
        void DrawCircle(float x, float y, float r, SDL_Color color) override;
        void DrawLine(float x1, float y1, float x2, float y2, SDL_Color color) override;
        void Present() override;
        void SetViewMatrix(const glm::mat4 &view) override;

        void DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, const SDL_FRect *srcRect = nullptr);
        std::shared_ptr<Texture> CreateTexture(const char *path);

        void Flush();

        virtual SDL_Renderer *GetSDLRenderer() override;

        SDL_Texture *GetViewportTexture() { return m_ViewportTexture; }

        void ResizeViewport(int width, int height);

    private:
        struct ViewState
        {
            glm::mat4 matrix;
            float zoomX;
            float zoomY;
            bool isUniformZoom;
        };

        struct BatchedTexture
        {
            SDL_Texture *texture;
            SDL_FRect dest;
            SDL_Color tint;
            float rotation;
            const SDL_FRect *srcRect;
        };

        struct BatchedQuad
        {
            SDL_FRect rect;
            SDL_Color color;
            bool isFill;
        };

        inline SDL_FRect WorldToScreen(float x, float y, float w, float h) const;

        SDL_Renderer *m_Renderer = nullptr;
        SDL_Color m_ClearColor{0, 0, 0, 255};
        ViewState m_ViewState;
        glm::vec2 TransformPosition(float x, float y) const;
        glm::vec2 TransformSize(float w, float h) const;

        Texture circleTexture;

        std::vector<BatchedTexture> m_TextureBatch;
        SDL_Texture *m_CurrentTexture = nullptr;

        std::vector<BatchedQuad> m_QuadBatch;

        void FlushTextureBatch();
        void FlushQuadBatch();

        SDL_Texture *m_ViewportTexture = nullptr;
        int m_ViewportWidth = 0, m_ViewportHeight = 0;
    };
}
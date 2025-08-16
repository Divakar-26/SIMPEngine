#include "Rendering/SDLRenderingAPI.h"
#include <SDL3/SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>
#include "TextureManager.h"

namespace SIMPEngine
{

    SDLRenderingAPI::SDLRenderingAPI() : m_ViewState{glm::mat4(1.0f), 1.0f, 1.0f, true} {}

    void SDLRenderingAPI::Init(SDL_Renderer *sdlRenderer)
    {
        m_Renderer = sdlRenderer;
        TextureManager::Get().LoadTexture("circle", "../assets/circle.png", m_Renderer);
        TextureManager::Get().LoadTexture("coin", "../assets/coin.png", m_Renderer);
    }

    void SDLRenderingAPI::SetClearColor(float r, float g, float b, float a)
    {
        m_ClearColor = {
            static_cast<Uint8>(r * 255),
            static_cast<Uint8>(g * 255),
            static_cast<Uint8>(b * 255),
            static_cast<Uint8>(a * 255)};
    }

    void SDLRenderingAPI::Clear()
    {
        SDL_SetRenderDrawColor(m_Renderer, m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
        SDL_RenderClear(m_Renderer);
    }

    void SDLRenderingAPI::DrawQuad(float x, float y, float w, float h, SDL_Color color)
    {
        glm::vec2 pos = TransformPosition(x, y);
        float tx = pos.x;
        float ty = pos.y;

        glm::vec2 size = TransformSize(w, h);
        float tw = size.x;
        float th = size.y;

        m_QuadBatch.push_back({SDL_FRect{tx, ty, tw, th},
                               color});

        if (m_QuadBatch.size() >= 1000)
            FlushQuadBatch();
    }

    void SDLRenderingAPI::FlushQuadBatch()
    {
        if (m_QuadBatch.empty())
            return;

        std::vector<SDL_Vertex> vertices;
        std::vector<int> indices;
        vertices.reserve(m_QuadBatch.size() * 4);
        indices.reserve(m_QuadBatch.size() * 6);

        for (size_t i = 0; i < m_QuadBatch.size(); ++i)
        {
            const auto &quad = m_QuadBatch[i];
            size_t base_index = i * 4;

            vertices.insert(vertices.end(), {{{quad.rect.x, quad.rect.y}, {quad.color.r, quad.color.g, quad.color.b, quad.color.a}, {0, 0}},
                                             {{quad.rect.x + quad.rect.w, quad.rect.y}, {quad.color.r, quad.color.g, quad.color.b, quad.color.a}, {0, 0}},
                                             {{quad.rect.x + quad.rect.w, quad.rect.y + quad.rect.h}, {quad.color.r, quad.color.g, quad.color.b, quad.color.a}, {0, 0}},
                                             {{quad.rect.x, quad.rect.y + quad.rect.h}, {quad.color.r, quad.color.g, quad.color.b, quad.color.a}, {0, 0}}});

            indices.insert(indices.end(), {(int)base_index, (int)base_index + 1, (int)base_index + 2,
                                           (int)base_index, (int)base_index + 2, (int)base_index + 3});
        }

        SDL_RenderGeometry(m_Renderer, nullptr,
                           vertices.data(), (int)vertices.size(),
                           indices.data(), (int)indices.size());

        m_QuadBatch.clear();
    }

    void SDLRenderingAPI::Present()
    {
        SDL_RenderPresent(m_Renderer);
    }

    void SDLRenderingAPI::Flush()
    {
        FlushTextureBatch();
        FlushQuadBatch();
    }

    void SDLRenderingAPI::SetViewMatrix(const glm::mat4 &view)
    {
        m_ViewState.matrix = view;
        m_ViewState.zoomX = glm::length(glm::vec3(view[0]));
        m_ViewState.zoomY = glm::length(glm::vec3(view[1]));
        m_ViewState.isUniformZoom = (std::abs(m_ViewState.zoomX - m_ViewState.zoomY) < 0.001f);
    }

    void SDLRenderingAPI::DrawCircle(float cx, float cy, float radius, SDL_Color color)
    {
        auto circleTexture = TextureManager::Get().GetTexture("circle");
        DrawTexture(circleTexture->GetSDLTexture(), cx, cy, radius, radius, {0, 255, 255, 255}, 0);
    }

    void SDLRenderingAPI::DrawTexture(SDL_Texture *texture, float x, float y, float w, float h,
                                      SDL_Color tint, float rotation, const SDL_FRect *srcRect)
    {
        if (texture != m_CurrentTexture || m_TextureBatch.size() >= 1000)
        {
            FlushTextureBatch();
            m_CurrentTexture = texture;
        }

        glm::vec2 pos = TransformPosition(x, y);
        float tx = pos.x;
        float ty = pos.y;

        glm::vec2 size = TransformSize(w, h);
        float tw = size.x;
        float th = size.y;

        m_TextureBatch.push_back(BatchedTexture{
            texture,
            SDL_FRect{tx, ty, tw, th},
            tint,
            rotation,
            srcRect});
    }

    void SDLRenderingAPI::FlushTextureBatch()
    {
        if (m_TextureBatch.empty())
            return;

        for (const auto &batch : m_TextureBatch)
        {
            SDL_SetTextureColorMod(batch.texture, batch.tint.r, batch.tint.g, batch.tint.b);
            SDL_SetTextureAlphaMod(batch.texture, batch.tint.a);

            SDL_FPoint center = {batch.dest.w / 2, batch.dest.h / 2};
            SDL_RenderTextureRotated(m_Renderer, batch.texture, batch.srcRect,
                                     &batch.dest, batch.rotation, &center, SDL_FLIP_NONE);
        }

        m_TextureBatch.clear();
    }

    std::shared_ptr<Texture> SDLRenderingAPI::CreateTexture(const char *path)
    {
        auto tex = std::make_shared<Texture>();
        if (!tex->LoadFromFile(m_Renderer, path))
        {
            return nullptr;
        }
        return tex;
    }

    SDL_Renderer *SDLRenderingAPI::GetSDLRenderer()
    {
        return m_Renderer;
    }

    glm::vec2 SDLRenderingAPI::TransformPosition(float x, float y) const
    {
        glm::vec4 pos = m_ViewState.matrix * glm::vec4(x, y, 0.0f, 1.0f);
        return {pos.x, pos.y};
    }

    glm::vec2 SDLRenderingAPI::TransformSize(float w, float h) const
    {
        return {
            w * m_ViewState.zoomX,
            h * m_ViewState.zoomY};
    }

    void SDLRenderingAPI::ResizeViewport(int width, int height)
    {
        if (m_ViewportWidth == width && m_ViewportHeight == height)
            return;

        if (m_ViewportTexture)
            SDL_DestroyTexture(m_ViewportTexture);

        m_ViewportTexture = SDL_CreateTexture(
            m_Renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            width, height);
        m_ViewportWidth = width;
        m_ViewportHeight = height;
    }

}

#include "Rendering/SDLRenderingAPI.h"
#include <SDL3/SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>

namespace SIMPEngine
{
    SDLRenderingAPI::SDLRenderingAPI() {}

    void SDLRenderingAPI::Init(SDL_Renderer *sdlRenderer)
    {
        m_Renderer = sdlRenderer;
        circleTexture.LoadFromFile(m_Renderer, "circle.png");
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

    void SDLRenderingAPI::DrawQuad(float x, float y, float width, float height, SDL_Color color)
    {
        glm::vec4 posWorld(x, y, 0.0f, 1.0f);
        glm::vec4 posCamera = s_ViewMatrix * posWorld;
        SDL_FRect rect;

        float zoomX = glm::length(glm::vec3(s_ViewMatrix[0])); // length of first column vector
        float zoomY = glm::length(glm::vec3(s_ViewMatrix[1])); // length of second column vector

        rect.w = width * zoomX;
        rect.h = height * zoomY;

        rect.x = static_cast<float>(posCamera.x);
        rect.y = static_cast<float>(posCamera.y);
        // rect.w = static_cast<float>(width); // Optionally multiply by zoom scale if you want
        // rect.h = static_cast<float>(height);

        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(m_Renderer, &rect);
    }

    void SDLRenderingAPI::Present()
    {
        SDL_RenderPresent(m_Renderer);
    }

    void SDLRenderingAPI::SetViewMatrix(const glm::mat4 &view)
    {
        s_ViewMatrix = view;
    }

    void SDLRenderingAPI::SetViewport(int x, int y, int width, int height)
    {
        SDL_Rect viewport{x, y, width, height};
        SDL_SetRenderViewport(m_Renderer, &viewport);
    }

    void SDLRenderingAPI::ResetViewport()
    {
        SDL_SetRenderViewport(m_Renderer, nullptr); // resets to full window
    }

    void SDLRenderingAPI::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.b, color.a);
        SDL_RenderLine(m_Renderer, x1, y1, x2, y2);
    }

    void SDLRenderingAPI::DrawCircle(float cx, float cy, float radius, SDL_Color color)
    {
        DrawTexture(circleTexture.GetSDLTexture(), cx, cy, radius, radius, {0, 255, 255, 255}, 0);
    }

    void SDLRenderingAPI::DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation)
    {
        glm::vec4 posWorld(x, y, 0.0f, 1.0f);
        glm::vec4 posCamera = s_ViewMatrix * posWorld;

        SDL_FRect dest;
        dest.x = posCamera.x;
        dest.y = posCamera.y;

        float zoomX = glm::length(glm::vec3(s_ViewMatrix[0]));
        float zoomY = glm::length(glm::vec3(s_ViewMatrix[1]));

        dest.w = w * zoomX;
        dest.h = h * zoomY;

        SDL_SetTextureColorMod(texture, tint.r, tint.g, tint.b);
        SDL_SetTextureAlphaMod(texture, tint.a);

        SDL_FPoint center;
        center.x = dest.w / 2;
        center.y = dest.h / 2;

        SDL_RenderTextureRotated(m_Renderer, texture, nullptr, &dest, rotation, &center, SDL_FLIP_NONE);
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

}
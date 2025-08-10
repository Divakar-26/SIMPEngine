#include "Engine/SDLRenderingAPI.h"
#include <SDL3/SDL.h>

namespace SIMPEngine
{
    SDLRenderingAPI::SDLRenderingAPI() {}

    void SDLRenderingAPI::Init(SDL_Renderer *sdlRenderer)
    {
        m_Renderer = sdlRenderer;
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
        SDL_SetRenderDrawColor(m_Renderer, color.r, color.g, color.g, color.a);
        SDL_FRect rect{x, y, width, height};
        SDL_RenderFillRect(m_Renderer, &rect);
    }

    void SDLRenderingAPI::Present()
    {
        SDL_RenderPresent(m_Renderer);
    }
}
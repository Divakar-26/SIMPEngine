// Renderer.cpp
#include "Rendering/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SIMPEngine
{

    RenderingAPI *Renderer::s_RenderingAPI = nullptr;
    glm::mat4 Renderer::s_ViewMatrix = glm::mat4(1.0f);

    void Renderer::Init(RenderingAPI *api, SDL_Renderer *sdlRenderer)
    {
        s_RenderingAPI = api;
        s_RenderingAPI->Init(sdlRenderer);

    }

    void Renderer::SetClearColor(float r, float g, float b, float a)
    {
        s_RenderingAPI->SetClearColor(r, g, b, a);
    }

    void Renderer::Clear()
    {
        s_RenderingAPI->Clear();
    }

    void Renderer::DrawQuad(float x, float y, float width, float height, SDL_Color color)
    {
        s_RenderingAPI->DrawQuad(x, y, width, height, color);
    }

    void Renderer::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        s_RenderingAPI->DrawLine(x1, y1, x2, y2, color);
    }

    void Renderer::DrawCircle(float x, float y, float r, SDL_Color color)
    {
        s_RenderingAPI->DrawCircle(x, y, r, color);
    }

    void Renderer::Present()
    {
        s_RenderingAPI->Present();
    }

    void Renderer::SetViewMatrix(const glm::mat4 &view)
    {
        s_ViewMatrix = view;
        if (s_RenderingAPI)
            s_RenderingAPI->SetViewMatrix(view);
    }

    const glm::mat4 &Renderer::GetViewMatrix()
    {
        return s_ViewMatrix;
    }

    void Renderer::SetViewport(int x, int y, int width, int height)
    {
        s_RenderingAPI->SetViewport(x, y, width, height);
    }

    void Renderer::ResetViewport()
    {
        s_RenderingAPI->ResetViewport();
    }

    void Renderer::DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, SDL_FRect * srcRect)
    {
        s_RenderingAPI->DrawTexture(texture, x, y, w, h, tint, rotation, srcRect);
    }

    std::shared_ptr<Texture> Renderer::CreateTexture(const char *path)
    {
        return s_RenderingAPI->CreateTexture(path);
    }

    SDL_Renderer * Renderer::GetSDLRenderer(){
        return s_RenderingAPI->GetSDLRenderer();
    }
}

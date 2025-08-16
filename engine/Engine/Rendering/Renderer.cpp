// Renderer.cpp
#include "Rendering/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SIMPEngine
{

    std::unique_ptr<RenderingAPI> Renderer::s_RenderingAPI = nullptr;
    glm::mat4 Renderer::s_ViewMatrix = glm::mat4(1.0f);

    void Renderer::Init(std::unique_ptr<RenderingAPI> api, SDL_Renderer *sdlRenderer)
    {
        s_RenderingAPI = std::move(api);
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

    void Renderer::DrawCircle(float x, float y, float r, SDL_Color color)
    {
        s_RenderingAPI->DrawCircle(x, y, r, color);
    }

    void Renderer::Present()
    {
        s_RenderingAPI->Present();
    }

    void Renderer::Flush(){
        s_RenderingAPI->Flush();
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

    void Renderer::DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, const SDL_FRect * srcRect)
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

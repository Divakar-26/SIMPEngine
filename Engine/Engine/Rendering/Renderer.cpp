// Renderer.cpp
#include "Rendering/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SIMPEngine
{   
    int Renderer::m_WindowHeight = 0;
    int Renderer::m_WindowWidth = 0;

    std::unique_ptr<RenderingAPI> Renderer::s_RenderingAPI = nullptr;
    glm::mat4 Renderer::s_ViewMatrix = glm::mat4(1.0f);

    RenderingAPI *Renderer::GetAPI()
    {
        return s_RenderingAPI.get();
    }

    void Renderer::Init(std::unique_ptr<RenderingAPI> api, int w, int h)
    {   
        s_RenderingAPI = std::move(api);
        s_RenderingAPI->Init();
        m_WindowWidth = w;
        m_WindowHeight = h;
    }

    void Renderer::SetClearColor(float r, float g, float b, float a)
    {
        s_RenderingAPI->SetClearColor(r, g, b, a);
    }

    void Renderer::Clear()
    {
        s_RenderingAPI->Clear();
    }

    void Renderer::DrawQuad(float x, float y, float width, float height,  SDL_Color color, bool fill)
    {
        s_RenderingAPI->DrawQuad(x, y, width, height, color, fill);
    }

    void Renderer::DrawCircle(float x, float y, float r, SDL_Color color)
    {
        // s_RenderingAPI->DrawCircle(x, y, r, color);
    }

    void Renderer::Present()
    {
        s_RenderingAPI->Present();
    }

    void Renderer::Flush()
    {
        // s_RenderingAPI->Flush();
    }

    void Renderer::SetViewMatrix(const glm::mat4 &view)
    {
        s_ViewMatrix = view;
        if (s_RenderingAPI)
            s_RenderingAPI->SetViewMatrix(view);
    }

    const glm::mat4 &Renderer::GetViewMatrix()
    {
        // return s_ViewMatrix;
    }

    void Renderer::DrawTexture(GLuint texture, float x, float y, float w, float h, SDL_Color tint, float rotation)
    {
        s_RenderingAPI->DrawTexture(texture, x, y, w, h, tint, rotation);
    }

    std::shared_ptr<Texture> Renderer::CreateTexture(const char *path)
    {
        // return s_RenderingAPI->CreateTexture(path);
    }

    SDL_Renderer *Renderer::GetSDLRenderer()
    {
        // return s_RenderingAPI->GetSDLRenderer();
    }

    void Renderer::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color){
        // s_RenderingAPI->DrawLine(x1, y1, x2, y2, color);
    }
}

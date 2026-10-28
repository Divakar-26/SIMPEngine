#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/SvgTextureCache.h>
#include <Engine/Rendering/SVGDocument.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

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
        m_WindowWidth = w;
        m_WindowHeight = h;
        s_RenderingAPI->ResizeViewport(w, h);
        s_RenderingAPI->Init();
    }

    void Renderer::SetClearColor(float r, float g, float b, float a)
    {
        s_RenderingAPI->SetClearColor(r, g, b, a);
    }

    void Renderer::Clear()
    {
        s_RenderingAPI->Clear();
    }

    void Renderer::DrawQuad(float x, float y, float width, float height, float rotation, SDL_Color color, bool fill, float zIndex)
    {
        s_RenderingAPI->DrawQuad(x, y, width, height, rotation, color, fill, zIndex);
    }

    void Renderer::Present()
    {
        s_RenderingAPI->Present();
    }

    void Renderer::Flush()
    {
        s_RenderingAPI->Flush();
    }

    void Renderer::SetViewMatrix(const glm::mat4 &view)
    {
        s_ViewMatrix = view;
        if (s_RenderingAPI)
            s_RenderingAPI->SetViewMatrix(view);
    }

    void Renderer::DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float w, float h, SDL_Color tint, float rotation, float zIndex, const SDL_FRect *srcRect)
    {
        s_RenderingAPI->DrawTexture(texture, x, y, w, h, tint, rotation, zIndex, srcRect);
    }

    void Renderer::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        s_RenderingAPI->DrawLine(x1, y1, x2, y2, color);
    }

    void Renderer::DrawCircle(float cx, float cy, float radius,
                              SDL_Color color, float aa, float zIndex)
    {
        s_RenderingAPI->DrawCircle(cx, cy, radius, color, aa, zIndex);
    }

    void Renderer::DrawRoundedRect(float cx, float cy, float halfW, float halfH,
                                   float cornerRadius, SDL_Color color,
                                   float aa, float zIndex)
    {
        s_RenderingAPI->DrawRoundedRect(cx, cy, halfW, halfH, cornerRadius, color, aa, zIndex);
    }

    void Renderer::DrawCapsuleLine(float x1, float y1, float x2, float y2,
                                   float width, SDL_Color color,
                                   float aa, float zIndex)
    {
        s_RenderingAPI->DrawCapsuleLine(x1, y1, x2, y2, width, color, aa, zIndex);
    }

    void Renderer::DrawSvg(SvgDocument &doc, float x, float y, float width, float height,
                           SDL_Color tint, float rotation, float zIndex)
    {
        int texW = std::max(1, (int)std::ceil(width));
        int texH = std::max(1, (int)std::ceil(height));

        auto tex = SvgTextureCache::GetOrCreate(doc, texW, texH);
        if (!tex)
            return;

        DrawTexture(tex, x, y, width, height, tint, rotation, zIndex);
    }

    glm::vec2 Renderer::GetViewportSize()
    {
        return s_RenderingAPI->GetViewportSize();
    }

    const RenderStats &Renderer::GetStats()
    {
        return s_RenderingAPI->GetStats();
    }
}
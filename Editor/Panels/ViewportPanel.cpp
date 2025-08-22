#include "ViewportPanel.h"
#include "Log.h"
#include "Input/Input.h"
#include "Rendering/Renderer.h"

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer)
{
    m_RenderingLayer = renderingLayer;
}

void ViewportPanel::OnAttach()
{
    auto& cam = m_RenderingLayer->GetCamera();
    auto it = cam.GetViewportSize();
    int w = SIMPEngine::Renderer::m_WindowWidth;
    int h = SIMPEngine::Renderer::m_WindowHeight;
    cam.SetPosition(glm::vec2{w / 2, h / 2});
    cam.SetZoom(0.60);
    
}

void ViewportPanel::OnRender()
{
    ImGui::Begin("Scene");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();



    bool prevActive = (m_ViewportFocused && m_ViewportHovered);
    bool nowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool nowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    SIMPEngine::Application::Get().GetImGuiLayer()->SetBlockEvent(!(nowFocused));

    if (!nowFocused)
    {
        SIMPEngine::Input::ResetAllKeys();
    }

    m_ViewportFocused = nowFocused;
    m_ViewportHovered = nowHovered;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int vw = (int)std::round(avail.x);
    int vh = (int)std::round(avail.y);

    static int lastW = -1, lastH = -1;
    if (vw != lastW || vh != lastH)
    {
        SIMPEngine::Renderer::GetAPI()->ResizeViewport(vw, vh);
        m_RenderingLayer->GetCamera().SetViewportSize((float)vw, (float)vh);
        lastW = vw;
        lastH = vh;
    }

    SIMPEngine::Renderer::GetAPI()->ResizeViewport((int)viewportSize.x, (int)viewportSize.y);
    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), SIMPEngine::Renderer::GetAPI()->GetViewportTexture());

    // Update camera viewport
    m_RenderingLayer->GetCamera().SetViewportSize(viewportSize.x, viewportSize.y);

    // Render scene
    SIMPEngine::Renderer::Clear();

    // rendering lines
    float rightEdge = 100000.0f;
    float bottomEdge = 100000.0f;
    SDL_Color axisColorX = {255, 0, 0, 255};
    SDL_Color axisColorY = {0, 255, 0, 255};

    SIMPEngine::Renderer::DrawLine(SIMPEngine::Renderer::m_WindowWidth, 0.0f, SIMPEngine::Renderer::m_WindowWidth, SIMPEngine::Renderer::m_WindowHeight, SDL_Color{0, 0, 255, 255});
    SIMPEngine::Renderer::DrawLine(0.0f, SIMPEngine::Renderer::m_WindowHeight, SIMPEngine::Renderer::m_WindowWidth, SIMPEngine::Renderer::m_WindowHeight, SDL_Color{0, 0, 255, 255});

    SIMPEngine::Renderer::DrawLine(-rightEdge, 0.0f, rightEdge, 0.0f, axisColorX);   // X axis
    SIMPEngine::Renderer::DrawLine(0.0f, -bottomEdge, 0.0f, bottomEdge, axisColorY); // Y axis

    m_RenderingLayer->OnRender();
    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), nullptr);

    // Show framebuffer in ImGui
    SDL_Texture *tex = SIMPEngine::Renderer::GetAPI()->GetViewportTexture();
    ImGui::Image((void *)tex, viewportSize, ImVec2(0, 0), ImVec2(1, 1));
    
    auto c = m_RenderingLayer->GetCamera();

    CORE_ERROR("{} {} {}", c.GetPosition().x, c.GetPosition().y, c.GetZoom());

    ImGui::End();
}

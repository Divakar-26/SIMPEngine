#include "ViewportPanel.h"
#include "Log.h"
#include "Input/Input.h"

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer)
{
    m_RenderingLayer = renderingLayer;
}

void ViewportPanel::OnRender()
{
    ImGui::Begin("Scene");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    bool prevActive = (m_ViewportFocused && m_ViewportHovered);
    bool nowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool nowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    SIMPEngine::Application::Get().GetImGuiLayer()->SetBlockEvent(!(nowFocused));

    if(!nowFocused){
        SIMPEngine::Input::ResetAllKeys();
        CORE_ERROR("OUT OF VIEWPORT");
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
    m_RenderingLayer->OnRender();
    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), nullptr);

    // Show framebuffer in ImGui
    SDL_Texture *tex = SIMPEngine::Renderer::GetAPI()->GetViewportTexture();
    ImGui::Image((void *)tex, viewportSize, ImVec2(0, 0), ImVec2(1, 1));

    ImGui::End();
}

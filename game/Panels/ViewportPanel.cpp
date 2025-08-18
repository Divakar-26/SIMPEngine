#include "ViewportPanel.h"

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer)
{
    m_RenderingLayer = renderingLayer;
}

void ViewportPanel::OnRender()
{
    ImGui::Begin("Scene");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    m_ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // resize
    SIMPEngine::Renderer::GetAPI()->ResizeViewport((int)viewportSize.x, (int)viewportSize.y);
    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), SIMPEngine::Renderer::GetAPI()->GetViewportTexture());

    // update camera viewport
    m_RenderingLayer->GetCamera().SetViewportSize(viewportSize.x, viewportSize.y);

    // render scene
    SIMPEngine::Renderer::Clear();
    m_RenderingLayer->OnRender();
    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), nullptr);

    // show framebuffer in ImGui
    SDL_Texture *tex = SIMPEngine::Renderer::GetAPI()->GetViewportTexture();
    ImGui::Image((void *)tex, viewportSize, ImVec2(0, 0), ImVec2(1, 1));

    ImGui::End();
}
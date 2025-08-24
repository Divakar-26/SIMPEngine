#include "ViewportPanel.h"
#include "Log.h"
#include "Input/Input.h"
#include "Rendering/Renderer.h"

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer* renderingLayer)
    : m_RenderingLayer(renderingLayer)
{}

void ViewportPanel::OnAttach()
{
    auto& cam = m_RenderingLayer->GetCamera();
    int w = SIMPEngine::Renderer::m_WindowWidth;
    int h = SIMPEngine::Renderer::m_WindowHeight;
    cam.SetPosition({ w / 2.0f, h / 2.0f });
    cam.SetZoom(0.60f);
}

void ViewportPanel::OnRender()
{
    ImGui::Begin("Scene");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    UpdateFocusState();

    ResizeViewportIfNeeded(viewportSize);


    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), SIMPEngine::Renderer::GetAPI()->GetViewportTexture());
    m_RenderingLayer->GetCamera().SetViewportSize(viewportSize.x, viewportSize.y);
    SIMPEngine::Renderer::Clear();

    RenderViewportBorder();
    OriginLines();
    m_RenderingLayer->OnRender();

    SDL_SetRenderTarget(SIMPEngine::Renderer::GetSDLRenderer(), nullptr);

    // Display framebuffer
    SDL_Texture* tex = SIMPEngine::Renderer::GetAPI()->GetViewportTexture();
    ImGui::Image((void*)tex, viewportSize, ImVec2(0, 0), ImVec2(1, 1));

    DrawMouseWorldPosition();
    ImGui::End();
}

void ViewportPanel::UpdateFocusState()
{
    bool nowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool nowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    SIMPEngine::Application::Get().GetImGuiLayer()->SetBlockEvent(!nowFocused);

    if (!nowFocused)
        SIMPEngine::Input::ResetAllKeys();

    m_ViewportFocused = nowFocused;
    m_ViewportHovered = nowHovered;
}

void ViewportPanel::ResizeViewportIfNeeded(const ImVec2& viewportSize)
{
    static int lastW = -1, lastH = -1;
    int vw = (int)std::round(viewportSize.x);
    int vh = (int)std::round(viewportSize.y);

    if (vw != lastW || vh != lastH)
    {
        SIMPEngine::Renderer::GetAPI()->ResizeViewport(vw, vh);
        m_RenderingLayer->GetCamera().SetViewportSize((float)vw, (float)vh);
        lastW = vw;
        lastH = vh;
    }
}

void ViewportPanel::DrawMouseWorldPosition()
{
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

    glm::vec2 worldPos = m_RenderingLayer->GetCamera().ScreenToWorld(
        { mousePos.x - (windowPos.x + contentMin.x), mousePos.y - (windowPos.y + contentMin.y) });

    char posText[64];
    snprintf(posText, sizeof(posText), "X: %.2f Y: %.2f", worldPos.x, worldPos.y);

    ImVec2 textSize = ImGui::CalcTextSize(posText);
    ImVec2 textPos = { windowPos.x + contentMax.x - textSize.x - 10,
                       windowPos.y + contentMax.y - textSize.y - 10 };

    ImGui::GetForegroundDrawList()->AddText(textPos, IM_COL32(255, 255, 255, 255), posText);
}

void ViewportPanel::RenderViewportBorder()
{
    SIMPEngine::Renderer::DrawQuad(0, 0, SIMPEngine::Renderer::m_WindowWidth, SIMPEngine::Renderer::m_WindowHeight, {255, 255, 255, 255}, false);
}

void ViewportPanel::OriginLines()
{
    constexpr float kEdge = 100000.0f;
    SIMPEngine::Renderer::DrawLine(-kEdge, 0.0f, kEdge, 0.0f, {255, 0, 0, 200});   // X axis
    SIMPEngine::Renderer::DrawLine(0.0f, -kEdge, 0.0f, kEdge, {0, 255, 0, 200});   // Y axis
}

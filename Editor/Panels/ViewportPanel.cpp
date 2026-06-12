#include "ViewportPanel.h"
#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Scene/Component.h>

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext * context)
{
    m_RenderingLayer = renderingLayer;
    m_Context = context;
}

void ViewportPanel::OnAttach()
{

}

void ViewportPanel::OnImGuiRender()
{
    m_ViewportPos = ImGui::GetCursorScreenPos();
    m_ViewportSize = ImGui::GetContentRegionAvail();

    ImGui::Begin("Viewport");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    UpdateFocusState();
    ResizeViewportIfNeeded(viewportSize);

    // Render scene
    auto api = SIMPEngine::Renderer::GetAPI();
    api->BeginFrame();
    m_RenderingLayer->GetCamera().SetViewportSize(viewportSize.x, viewportSize.y);

    SIMPEngine::Renderer::Clear();
    m_RenderingLayer->OnRender();

    api->EndFrame();

    ImTextureID texID = (ImTextureID)(intptr_t)api->GetViewportTexture();
    ImGui::Image(texID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

    m_ViewportPos = ImGui::GetItemRectMin();
    m_ViewportSize = ImGui::GetItemRectSize();
    m_ViewportHovered = ImGui::IsItemHovered();

    // if (m_ViewportHovered && SIMPEngine::Input::IsMouseButtonPressed(1))
    // {
    //     auto scene = m_Context->Scene;
    //     auto &camera = scene->GetActiveCamera();

    //     glm::vec2 world = GetMouseWorldPosition();

    //     SIMPEngine::Entity e = scene->BuildEntity("Box").At(world.x, world.y).With<RenderComponent>();

    //     auto &rc = e.GetComponent<RenderComponent>();
    //     rc.width = 64;
    //     rc.height = 64;
    //     rc.color = {255, 0, 0, 255};
    // }

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

void ViewportPanel::ResizeViewportIfNeeded(const ImVec2 &viewportSize)
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

glm::vec2 ViewportPanel::GetMouseWorldPosition() const
{
    if (!m_ViewportHovered)
        return {0.0f, 0.0f};

    auto [mx, my] = SIMPEngine::Input::GetMousePosition();

    glm::vec2 viewportMouse =
    {
        mx - m_ViewportPos.x,
        my - m_ViewportPos.y
    };

    return m_RenderingLayer
        ->GetCamera()
        .ScreenToWorld(viewportMouse);
}

bool ViewportPanel::IsHovered() const
{
    return m_ViewportHovered;
}

bool ViewportPanel::IsFocused() const
{
    return m_ViewportFocused;
}

const ImVec2& ViewportPanel::GetViewportPosition() const
{
    return m_ViewportPos;
}

const ImVec2& ViewportPanel::GetViewportSize() const
{
    return m_ViewportSize;
}
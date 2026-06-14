#include "ViewportPanel.h"
#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Scene/Component.h>

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext *context)
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
    m_Context->ViewportHovered = ImGui::IsItemHovered();

    if (m_Context->ViewportHovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_Context->SelectedEntity = GetEntityUnderMouse();
    }

    if(m_Context->ViewportFocused && ImGui::IsKeyDown(ImGuiKey_F)){
        FocusOnSelectedEntity(); // inside ViewportUtility.cpp
    }

    ShowDetailsInViewport();
    DrawSelectionOutline();

    ImGui::End();
}

void ViewportPanel::UpdateFocusState()
{
    bool nowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool nowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    SIMPEngine::Application::Get().GetImGuiLayer()->SetBlockEvent(!nowFocused);

    if (!nowFocused)
        SIMPEngine::Input::ResetAllKeys();

    m_Context->ViewportFocused = nowFocused;
    m_Context->ViewportHovered = nowHovered;
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
    if (!m_Context->ViewportHovered)
        return {0.0f, 0.0f};

    auto [mx, my] = SIMPEngine::Input::GetMousePosition();

    glm::vec2 viewportMouse =
        {
            mx - m_ViewportPos.x,
            my - m_ViewportPos.y};

    return m_RenderingLayer
        ->GetCamera()
        .ScreenToWorld(viewportMouse);
}

bool ViewportPanel::IsHovered() const
{
    return m_Context->ViewportHovered;
}

bool ViewportPanel::IsFocused() const
{
    return m_Context->ViewportFocused;
}

const ImVec2 &ViewportPanel::GetViewportPosition() const
{
    return m_ViewportPos;
}

const ImVec2 &ViewportPanel::GetViewportSize() const
{
    return m_ViewportSize;
}

SIMPEngine::Entity ViewportPanel::GetEntityUnderMouse()
{
    if (!m_Context->ViewportHovered || !m_Context->Scene)
        return {};

    glm::vec2 worldPos = GetMouseWorldPosition();

    auto &registry = m_Context->Scene->GetRegistry();
    auto view = registry.view<TransformComponent>();

    SIMPEngine::Entity best{};
    float highestZ = -FLT_MAX;

    for (auto entityHandle : view)
    {
        SIMPEngine::Entity entity(entityHandle, m_Context->Scene);

        auto bounds = SIMPEngine::GetEntityBounds2D(entity);
        if (!bounds)
            continue;

        if (!bounds->Contains(worldPos))
            continue;

        auto &tc = entity.GetComponent<TransformComponent>();
        if (tc.zIndex >= highestZ)
        {
            highestZ = tc.zIndex;
            best = entity;
        }
    }

    return best;
}

void ViewportPanel::ShowDetailsInViewport()
{
    auto hovered = GetEntityUnderMouse();
    m_Context->HoveredEntity = hovered;
    std::string hoveredEntityName = "None";
    if (hovered)
    {
        if (hovered.HasComponent<TagComponent>())
            hoveredEntityName =
                hovered.GetComponent<TagComponent>().Tag;
        else
            hoveredEntityName = "Unnamed";
    }

    const char *status = "None";

    if (m_Context->ViewportFocused && m_Context->ViewportHovered)
        status = "Focused & Hovered";
    else if (m_Context->ViewportFocused)
        status = "Focused";
    else if (m_Context->ViewportHovered)
        status = "Hovered";

    glm::vec2 worldPos = GetMouseWorldPosition();

    char buffer[512];
    snprintf(
        buffer,
        sizeof(buffer),
        "Status: %s\n"
        "Coords: (%.2f, %.2f)\n"
        "Entity: %s",
        status,
        worldPos.x,
        worldPos.y,
        hoveredEntityName.c_str());

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    ImVec2 textPos =
        {
            m_ViewportPos.x + 10.0f,
            m_ViewportPos.y + m_ViewportSize.y - 65.0f};

    ImVec2 textSize = ImGui::CalcTextSize(buffer);

    constexpr float padding = 6.0f;

    ImVec2 rectMin =
        {
            textPos.x - padding,
            textPos.y - padding};

    ImVec2 rectMax =
        {
            textPos.x + textSize.x + padding,
            textPos.y + textSize.y + padding};

    drawList->AddRectFilled(
        rectMin,
        rectMax,
        IM_COL32(60, 60, 60, 180),
        4.0f);

    drawList->AddRect(
        rectMin,
        rectMax,
        IM_COL32(120, 120, 120, 220),
        4.0f);

    drawList->AddText(
        textPos,
        IM_COL32(255, 255, 255, 255),
        buffer);
}


void ViewportPanel::DrawSelectionOutline()
{
    auto entity = m_Context->SelectedEntity;

    if (!entity)
        return;

    auto bounds =
        SIMPEngine::GetEntityBounds2D(entity);

    if (!bounds)
        return;

    ImDrawList* drawList =
        ImGui::GetWindowDrawList();

    glm::vec2 screenMin =
        m_RenderingLayer
            ->GetCamera()
            .WorldToScreen(bounds->min);

    glm::vec2 screenMax =
        m_RenderingLayer
            ->GetCamera()
            .WorldToScreen(bounds->max);

    ImVec2 min =
    {
        m_ViewportPos.x + screenMin.x,
        m_ViewportPos.y + screenMin.y
    };

    ImVec2 max =
    {
        m_ViewportPos.x + screenMax.x,
        m_ViewportPos.y + screenMax.y
    };

    drawList->AddRect(
        min,
        max,
        IM_COL32(255, 0, 0, 255), // yellow
        0.0f,                       // rounding
        0,
        2.0f                        // thickness
    );
}
#pragma once
#include <imgui.h>
#include "Rendering/Renderer.h"
#include "Layer/RenderingLayer.h"
#include "Application/Application.h"
#include <ImGuizmo.h>
#include "Scene/Scene.h"


class ViewportPanel
{
public:
    ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer);

    void OnAttach();
    void OnRender(SIMPEngine::Entity &m_SelectedEntity);

    void RenderViewportBorder();
    void OriginLines();
    bool iSFocusedAndHovered() { return m_ViewportFocused; }
    void DrawMouseWorldPosition();
    void ResizeViewportIfNeeded(const ImVec2& viewportSize);
    void UpdateFocusState();
    void RenderGizmos(SIMPEngine::Entity &selectedEntity);
    void SelectEntites(SIMPEngine::Entity &m_SelectedEntity);


    bool DrawCircleHandle(const glm::vec2 &center, float radius, glm::vec2 &outDelta);
private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    SIMPEngine::Scene * m_Context= nullptr;
    SIMPEngine::Entity m_SelectedEntites;
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;
};
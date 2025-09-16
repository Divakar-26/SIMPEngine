#pragma once
#include <imgui.h>
#include "Rendering/Renderer.h"
#include "Layer/RenderingLayer.h"
#include "Application/Application.h"
#include <ImGuizmo.h>
#include "Scene/Scene.h"

enum class GizmoHandle
{
    None,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    TopCenter,
    BottomCenter,
    LeftCenter,
    RightCenter
};

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
    void ResizeViewportIfNeeded(const ImVec2 &viewportSize);
    void UpdateFocusState();
    void RenderGizmos(SIMPEngine::Entity &selectedEntity);
    void SelectEntites(SIMPEngine::Entity &m_SelectedEntity);

    bool DrawCircleHandle(const glm::vec2 &center, float radius, glm::vec2 &outDelta);

private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    SIMPEngine::Scene *m_Context = nullptr;
    SIMPEngine::Entity m_SelectedEntites;
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;

    GizmoHandle m_ActiveGizmo;
    bool m_IsDraggingGizmo;
    glm::vec2 m_DragStartWorldPos;
    glm::vec2 m_DragStartEntityPos;
    glm::vec2 m_DragStartEntityScale;

    SIMPEngine::Entity m_SelectedEntity;

    void CalculateEntityCorners(const TransformComponent &transform,
                                const RenderComponent &render,
                                glm::vec2 corners[4]);
    GizmoHandle DrawGizmoHandles(const glm::vec2 corners[4]);
    void DrawSingleGizmo(const glm::vec2 &worldPos, GizmoHandle handleType,
                         GizmoHandle &hoveredGizmo, float &minDist);
    void HandleGizmoDragging(SIMPEngine::Entity &entity,
                             TransformComponent &transform,
                             RenderComponent &render,
                             const glm::vec2 corners[4],
                             GizmoHandle hoveredGizmo);
    glm::vec2 WorldToScreen(const glm::vec2 &worldPos);
    void RenderImGuizmo(SIMPEngine::Entity &selectedEntity, TransformComponent &transform);
    bool IsClickingOnGizmo();
};
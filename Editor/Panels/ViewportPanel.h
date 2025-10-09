#pragma once
#include <imgui.h>
#include "Rendering/Renderer.h"
#include "Layer/RenderingLayer.h"
#include "Application/Application.h"
#include <ImGuizmo.h>
#include "Scene/Scene.h"
#include "GizmoSystem.h"


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
    void SelectEntities(SIMPEngine::Entity &selectedEntity);

private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    SIMPEngine::Scene *m_Context = nullptr;
    SIMPEngine::Entity m_SelectedEntites;
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;

    GizmoSystem m_GizmoSystem;
    
    glm::vec2 m_DragStartWorldPos;
    glm::vec2 m_DragStartEntityPos;
    glm::vec2 m_DragStartEntityScale;

    float  m_DragStartColliderWidth;
    float m_DragStartColliderHeight;
    float m_DragStartColliderOffsetX;
    float m_DragStartColliderOffsetY;

    SIMPEngine::Entity m_SelectedEntity;

    void CalculateEntityCorners(const TransformComponent &transform,
                                const RenderComponent &render,
                                glm::vec2 corners[4]);
    glm::vec2 WorldToScreen(const glm::vec2 &worldPos);
    void RenderImGuizmo(SIMPEngine::Entity &selectedEntity, TransformComponent &transform);
    bool IsClickingOnGizmo();
    glm::vec2 GetMouseWorldPos() const; 

};
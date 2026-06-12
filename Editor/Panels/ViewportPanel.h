#pragma once
#include <imgui.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Scene/Scene.h>
#include <../EditorContext.h>


class ViewportPanel
{
public:
    ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext * context);

    void OnAttach();
    void OnRender(SIMPEngine::Entity &m_SelectedEntity);

    bool iSFocusedAndHovered() { return m_ViewportFocused; }
    glm::vec2 GetMouseWorldPosition() const;

private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    EditorContext *m_Context = nullptr;
    SIMPEngine::Entity *m_SelectedEntity = nullptr;
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;

    ImVec2 m_ViewportPos;
    ImVec2 m_ViewportSize;

    void ResizeViewportIfNeeded(const ImVec2 &viewportSize);
    void UpdateFocusState();
};
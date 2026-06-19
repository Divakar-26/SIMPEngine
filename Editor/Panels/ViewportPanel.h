#pragma once
#include <imgui.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Math/EntityUtility.h>
#include <../EditorContext.h>
#include <EditorPanel.h>

class ViewportPanel : public EditorPanel
{
public:
    ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext *context);

    void OnAttach();
    void OnImGuiRender() override;

    bool iSFocusedAndHovered() { return m_Context->ViewportFocused && m_Context->ViewportHovered; }
    glm::vec2 GetMouseWorldPosition() const;

    bool IsHovered() const;
    bool IsFocused() const;

    const ImVec2 &GetViewportPosition() const;
    const ImVec2 &GetViewportSize() const;

    void ShowDetailsInViewport();
    void DrawSelectionOutline();
    void DrawGridDots();
    void FocusOnSelectedEntity();



private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    EditorContext *m_Context = nullptr;
    SIMPEngine::Entity *m_SelectedEntity = nullptr;

    ImVec2 m_ViewportPos;
    ImVec2 m_ViewportSize;

    void ResizeViewportIfNeeded(const ImVec2 &viewportSize);
    void UpdateFocusState();

    SIMPEngine::Entity GetEntityUnderMouse();
};
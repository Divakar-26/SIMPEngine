#pragma once
#include <imgui.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Math/EntityUtility.h>
#include <../EditorContext.h>
#include <../EditorCamera.h>
#include <EditorPanel.h>

#include <memory>

class ViewportPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Viewport";

    ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext *context);

    void OnAttach();
    void OnImGuiRender() override;

    bool IsInteracting() const { return m_Drag.Active || m_Panning; }
    void FocusOnSelectedEntity();
    void FrameScene();

    glm::vec2 GetMouseWorldPosition() const;

private:
    enum class Handle
    {
        None,
        Body,
        Center,
        AxisX,
        AxisY,
        Rotate,
        ScaleX,
        ScaleY,
        ScaleXY
    };

    struct DragState
    {
        bool Active = false;
        bool Recorded = false;
        Handle Grab = Handle::None;
        glm::vec2 StartMouseWorld{0.0f};
        glm::vec2 StartMouseScreen{0.0f};
        glm::vec2 StartWorldPos{0.0f};
        glm::vec2 StartScale{1.0f};
        float StartRotation = 0.0f;
        float StartAngle = 0.0f;
    };

    void DrawToolbar();
    void RenderScene();
    void DrawGrid();
    void DrawCameraFrame();
    void HandleCameraInput();
    void HandleSelectionAndGizmo();
    void HandleShortcuts();
    void HandleDrop();
    void DrawOverlays();
    void DrawGizmo(SIMPEngine::Entity entity);
    void DrawOutline(SIMPEngine::Entity entity, ImU32 color, float thickness);

    Handle HitTestGizmo(SIMPEngine::Entity entity, const ImVec2 &mouse) const;
    void ApplyDrag(SIMPEngine::Entity entity);
    SIMPEngine::Entity PickEntity(const glm::vec2 &world) const;
    ImVec2 ToScreen(const glm::vec2 &world) const;
    bool SnapActive() const;

    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    EditorContext *m_Context = nullptr;

    std::unique_ptr<SIMPEngine::Framebuffer> m_Framebuffer;
    EditorCamera m_Camera;
    bool m_CameraInitialized = false;

    ImVec2 m_CanvasPos{0, 0};
    ImVec2 m_CanvasSize{0, 0};
    bool m_CanvasHovered = false;
    bool m_Panning = false;

    DragState m_Drag;
    Handle m_HoverHandle = Handle::None;
    SIMPEngine::RenderStats m_Stats;
};

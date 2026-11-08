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
#include <../TileEditor.h>
#include <EditorPanel.h>

#include <memory>
#include <utility>
#include <vector>

class ViewportPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Viewport";

    ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext *context);

    void OnAttach();
    void OnImGuiRender() override;

    bool IsInteracting() const { return m_Drag.Active || m_Panning || m_Marquee.Active || m_TileEditor.IsInteracting(); }
    void FocusOnSelectedEntity();
    void FrameScene();

    glm::vec2 GetMouseWorldPosition() const;
    glm::vec2 GetCameraCenter() const { return m_Camera.GetPosition(); }

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
        std::vector<std::pair<entt::entity, glm::vec2>> Others;
    };

    struct MarqueeState
    {
        bool Active = false;
        bool Additive = false;
        ImVec2 Start{0, 0};
    };

    void DrawToolbar();
    void DrawTileToolbar();
    bool SelectedHasTilemap() const;
    void ViewBounds(glm::vec2 &min, glm::vec2 &max) const;
    void RenderScene();
    void DrawGrid();
    void DrawCameraFrame();
    void DrawEmitterShapes();
    void DrawLightShapes();
    void DrawColliders();
    void DrawLogicShapes();
    void HandleCameraInput();
    void HandleSelectionAndGizmo();
    void HandleShortcuts();
    void HandleDrop();
    void DrawOverlays();
    void DrawGizmo(SIMPEngine::Entity entity);
    void DrawOutline(SIMPEngine::Entity entity, ImU32 color, float thickness);
    void FinishMarquee();
    std::vector<entt::entity> EntitiesInMarquee() const;

    Handle HitTestGizmo(SIMPEngine::Entity entity, const ImVec2 &mouse) const;
    void ApplyDrag(SIMPEngine::Entity entity);
    SIMPEngine::Entity PickEntity(const glm::vec2 &world) const;
    ImVec2 ToScreen(const glm::vec2 &world) const;
    bool SnapActive() const;

    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    EditorContext *m_Context = nullptr;
    TileEditor m_TileEditor;

    std::unique_ptr<SIMPEngine::Framebuffer> m_Framebuffer;
    EditorCamera m_Camera;
    bool m_CameraInitialized = false;

    ImVec2 m_CanvasPos{0, 0};
    ImVec2 m_CanvasSize{0, 0};
    bool m_CanvasHovered = false;
    bool m_Panning = false;

    DragState m_Drag;
    MarqueeState m_Marquee;
    Handle m_HoverHandle = Handle::None;
    SIMPEngine::RenderStats m_Stats;
};

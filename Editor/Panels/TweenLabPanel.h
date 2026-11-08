#pragma once

#include <EditorPanel.h>

#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Vector/VectorPath.h>

#include <SDL3/SDL_pixels.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SIMPEngine
{
    class Scene;
    class Curve;
}

class TweenLabPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###TweenLab";

    TweenLabPanel();
    ~TweenLabPanel() override;

    void OnImGuiRender() override;

    bool IsVisible() const { return m_Visible; }
    bool *VisibleFlag() { return &m_Visible; }

private:
    struct Track
    {
        bool Enabled = false;
        float From[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float To[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    enum TrackId
    {
        MoveX,
        MoveY,
        Scale,
        Rotation,
        Alpha,
        FillColor,
        Draw,
        Erase,
        Morph,
        TrackCount
    };

    void EnsureScene();
    void Restart();
    void ApplyStyle();
    void UpdatePreview(float dt);
    void RenderPreview(int width, int height);
    bool CurveAt(SIMPEngine::Curve &curve) const;
    std::string CurveCode() const;
    std::string GenerateCode() const;
    float CycleProgress() const;
    bool AllFinished() const;
    std::vector<uint32_t> ActiveIds() const;

    void DrawControls();
    void DrawTracks();
    void DrawTiming();
    void DrawTransport();
    void DrawCurveGraph(float width, float height);
    void DrawCode();

    static SIMPEngine::VectorPath ShapePath(int index);
    static const char *ShapeCode(int index);
    static SIMPEngine::VectorPath MotionPath(int index);
    static const char *MotionCode(int index);

    bool m_Visible = false;
    bool m_Dirty = true;

    std::unique_ptr<SIMPEngine::Scene> m_Scene;
    SIMPEngine::Entity m_Object;
    std::unique_ptr<SIMPEngine::Framebuffer> m_Target;

    int m_Shape = 3;
    int m_MorphShape = 4;
    bool m_Fill = true;
    SDL_Color m_FillColor{104, 136, 255, 255};
    bool m_Stroke = true;
    SDL_Color m_StrokeColor{235, 240, 255, 255};
    float m_StrokeWidth = 5.0f;

    Track m_Tracks[TrackCount];
    bool m_Along = false;
    int m_MotionPath = 0;
    bool m_Orient = false;

    int m_Curve = 0;
    int m_PreviewCurve = -1;
    float m_Cubic[4] = {0.25f, 0.1f, 0.25f, 1.0f};
    float m_Duration = 0.8f;
    float m_Delay = 0.0f;
    int m_LoopMode = 0;
    int m_LoopCount = 1;
    bool m_Yoyo = false;
    float m_Speed = 1.0f;

    bool m_Spring = false;
    float m_Stiffness = 180.0f;
    float m_Damping = 14.0f;
    float m_Mass = 1.0f;

    bool m_AutoReplay = true;
    bool m_Paused = false;
    bool m_ShowTrail = true;
    bool m_ShowPath = true;
    float m_Scrub = 0.0f;
    float m_Idle = 0.0f;

    uint32_t m_PropertyId = 0;
    uint32_t m_MotionId = 0;
    uint32_t m_SpringId = 0;

    std::vector<glm::vec2> m_Trail;
    std::vector<std::string> m_CurveNames;
};

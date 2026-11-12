#pragma once

#include <EditorPanel.h>

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/EffectPresets.h>
#include <Engine/Scene/Entity.h>

#include <imgui.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SIMPEngine
{
    class Scene;
}

class EffectsPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Effects";

    EffectsPanel();
    ~EffectsPanel() override;

    void OnImGuiRender() override;

    bool IsVisible() const { return m_Visible; }
    bool *VisibleFlag() { return &m_Visible; }

private:
    struct Preview
    {
        SIMPEngine::Entity Subject;
        uint32_t Tween = 0;
        float Wait = 0.0f;
    };

    struct CurveEntry
    {
        std::string Name;
        SIMPEngine::Curve Curve;
    };

    void EnsureScene();
    void Tick(float dt);
    void Restart(Preview &preview, const SIMPEngine::EffectPreset &preset, const SIMPEngine::EffectOptions &options);
    void SelectEffect(int index);
    void RefreshCode();
    void Copy(const std::string &text, const char *what);

    void DrawEffects();
    void DrawEffectGrid(float width);
    void DrawEffectDetail();
    bool DrawEffectCard(int index, bool selected);

    void DrawEasing();
    void DrawEasingGrid(float width);
    void DrawEasingDetail();
    bool DrawCurveCard(int index, bool selected);

    void CodeBlock(const char *title, const char *id, std::string &code, float height);

    bool m_Visible = false;
    std::unique_ptr<SIMPEngine::Scene> m_Scene;
    std::vector<Preview> m_Previews;
    Preview m_Detail;

    int m_Tab = 0;
    int m_Category = 0;
    int m_Selected = 0;
    char m_Search[64] = {};
    float m_Clock = 0.0f;

    SIMPEngine::EffectOptions m_Options;
    float m_Duration = 0.5f;
    int m_LoopMode = 0;
    std::string m_OneLiner;
    std::string m_PlayCall;
    std::string m_Expanded;
    bool m_Restart = true;

    std::vector<CurveEntry> m_Curves;
    int m_SelectedCurve = 0;
    char m_CurveSearch[64] = {};
    float m_CurveSeconds = 0.6f;

    std::string m_Toast;
    float m_ToastTime = 0.0f;
};

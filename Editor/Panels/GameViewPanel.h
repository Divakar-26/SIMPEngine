#pragma once

#include <EditorPanel.h>
#include <../EditorContext.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/GameView.h>
#include <Engine/Rendering/RenderingAPI.h>

#include <imgui.h>
#include <memory>
#include <string>

class GameViewPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Game";

    explicit GameViewPanel(EditorContext *context) : m_Context(context) {}

    void OnImGuiRender() override;
    void OnImGuiRenderFullscreen();
    void ShowFullscreenHint() { m_HintUntil = ImGui::GetTime() + 3.0; }

    bool IsFocused() const { return m_Focused; }

private:
    void DrawCanvas(bool fullscreen);
    void DrawToolbar();
    SIMPEngine::ViewLayout RenderScene(int width, int height);

    EditorContext *m_Context;
    std::unique_ptr<SIMPEngine::Framebuffer> m_Framebuffer;
    SIMPEngine::GameView m_GameView;
    SIMPEngine::RenderStats m_Stats;
    bool m_Focused = false;
    double m_HintUntil = 0.0;
    int m_SizePreset = 0;
    int m_CustomWidth = 1280;
    int m_CustomHeight = 720;
    bool m_ShowGuides = false;
};

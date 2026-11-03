#define IMGUI_DEFINE_MATH_OPERATORS
#include "GameViewPanel.h"
#include "UI/Widgets.h"

#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Scene.h>

#include <cstdio>

void GameViewPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (m_Context->FocusGameView)
    {
        ImGui::SetNextWindowFocus();
        m_Context->FocusGameView = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool visible = ImGui::Begin("Game###Game");
    ImGui::PopStyleVar();

    m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool playing = m_Context->State != SceneState::Edit;

    if (!visible)
    {
        SIMPEngine::Input::SetEnabled(false);
        ImGui::End();
        return;
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    int w = (int)size.x;
    int h = (int)size.y;
    if (w < 2 || h < 2)
    {
        ImGui::End();
        return;
    }

    if (!m_Framebuffer)
        m_Framebuffer = std::make_unique<SIMPEngine::Framebuffer>(w, h);
    else
        m_Framebuffer->Resize(w, h);

    SIMPEngine::Scene &scene = *m_Context->Scene;
    auto &camera = scene.GetActiveCamera();
    camera.SetViewportSize((float)w, (float)h);

    SIMPEngine::Renderer::ResetStats();
    SIMPEngine::Renderer::BeginRenderTarget(*m_Framebuffer);
    const glm::vec4 &bg = scene.GetBackgroundColor();
    SIMPEngine::Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
    SIMPEngine::Renderer::Clear();
    scene.OnRender();
    SIMPEngine::Renderer::EndScene();
    m_Stats = SIMPEngine::Renderer::GetStats();
    SIMPEngine::Renderer::EndRenderTarget();

    ImGui::Image((ImTextureID)(intptr_t)m_Framebuffer->GetColorAttachment(), size, ImVec2(0, 1), ImVec2(1, 0));
    bool hovered = ImGui::IsItemHovered();

    SIMPEngine::Input::SetViewport(pos.x, pos.y, size.x, size.y);
    bool typing = ImGui::GetIO().WantTextInput;
    SIMPEngine::Input::SetEnabled(m_Context->State == SceneState::Play && (m_Focused || (hovered && !typing)));

    ImDrawList *dl = ImGui::GetWindowDrawList();
    float fs = ImGui::GetFontSize();
    ImVec2 tl(pos.x + 8.0f, pos.y + 6.0f);

    if (!playing)
    {
        const char *hint = "Preview. Press F5 to play.";
        dl->AddText(tl + ImVec2(1, 1), IM_COL32(0, 0, 0, 120), hint);
        dl->AddText(tl, IM_COL32(255, 255, 255, 215), hint);
    }
    else if (m_Context->State == SceneState::Pause)
    {
        const char *hint = "Paused";
        dl->AddText(tl + ImVec2(1, 1), IM_COL32(0, 0, 0, 120), hint);
        dl->AddText(tl, U32(Colors::Warning), hint);
    }

    if (Settings().ShowStats)
    {
        char stats[96];
        std::snprintf(stats, sizeof(stats), "%dx%d  %d draw calls  %d primitives", w, h, m_Stats.drawCalls, m_Stats.primitives);
        ImVec2 ts = ImGui::CalcTextSize(stats);
        ImVec2 p(pos.x + size.x - ts.x - 8.0f, pos.y + size.y - fs - 6.0f);
        dl->AddText(p + ImVec2(1, 1), IM_COL32(0, 0, 0, 120), stats);
        dl->AddText(p, IM_COL32(255, 255, 255, 215), stats);
    }

    ImGui::End();
}

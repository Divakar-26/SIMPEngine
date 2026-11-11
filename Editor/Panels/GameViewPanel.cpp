#define IMGUI_DEFINE_MATH_OPERATORS
#include "GameViewPanel.h"
#include "UI/Widgets.h"

#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>

namespace
{
    void ShadowText(ImDrawList *dl, const ImVec2 &at, ImU32 color, const char *text)
    {
        dl->AddText(at + ImVec2(1, 1), IM_COL32(0, 0, 0, 140), text);
        dl->AddText(at, color, text);
    }
}

void GameViewPanel::OnImGuiRender()
{
    if (m_Context->FocusGameView)
    {
        ImGui::SetNextWindowFocus();
        m_Context->FocusGameView = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool visible = ImGui::Begin("Game###Game");
    ImGui::PopStyleVar();

    m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (!visible)
    {
        SIMPEngine::Input::SetEnabled(false);
        ImGui::End();
        return;
    }

    DrawCanvas(false);
    ImGui::End();
}

void GameViewPanel::OnImGuiRenderFullscreen()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    if (m_Context->FocusGameView)
    {
        ImGui::SetNextWindowFocus();
        m_Context->FocusGameView = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoNav;
    ImGui::Begin("##GameFullscreen", nullptr, flags);
    ImGui::PopStyleVar(3);

    if (ImGui::IsWindowAppearing())
        ImGui::SetWindowFocus();
    m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    DrawCanvas(true);
    ImGui::End();
}

void GameViewPanel::RenderScene(int width, int height)
{
    if (!m_Framebuffer)
        m_Framebuffer = std::make_unique<SIMPEngine::Framebuffer>(width, height);
    else
        m_Framebuffer->Resize(width, height);

    SIMPEngine::Scene &scene = *m_Context->Scene;
    auto &camera = scene.GetActiveCamera();
    camera.SetViewportSize(static_cast<float>(width), static_cast<float>(height));

    SIMPEngine::Renderer::ResetStats();
    SIMPEngine::Renderer::BeginRenderTarget(*m_Framebuffer);
    const glm::vec4 &bg = scene.GetBackgroundColor();
    SIMPEngine::Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
    SIMPEngine::Renderer::Clear();
    try
    {
        scene.OnRender();
        SIMPEngine::Renderer::EndScene();
    }
    catch (const std::exception &e)
    {
        SIMPEngine::Renderer::Recover();
        if (m_Context->RuntimeError.empty())
            m_Context->RuntimeError = std::string("render: ") + e.what();
        CORE_ERROR("Game view render failed: {}", e.what());
    }
    m_Stats = SIMPEngine::Renderer::GetStats();
    SIMPEngine::Renderer::EndRenderTarget();
}

void GameViewPanel::DrawCanvas(bool fullscreen)
{
    using namespace EditorUI;

    const bool playing = m_Context->State != SceneState::Edit;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos = ImVec2(std::round(pos.x), std::round(pos.y));
    ImGui::SetCursorScreenPos(pos);
    ImVec2 size = ImGui::GetContentRegionAvail();
    size = ImVec2(std::floor(size.x), std::floor(size.y));
    int w = static_cast<int>(size.x);
    int h = static_cast<int>(size.y);
    if (w < 2 || h < 2)
        return;

    RenderScene(w, h);

    ImGui::Image(EditorUI::TextureId(m_Framebuffer->GetColorAttachment()), size, ImVec2(0, 1), ImVec2(1, 0));
    bool hovered = ImGui::IsItemHovered();

    SIMPEngine::Input::SetViewport(pos.x, pos.y, size.x, size.y);
    bool typing = ImGui::GetIO().WantTextInput;
    SIMPEngine::Input::SetEnabled(m_Context->State == SceneState::Play && (m_Focused || (hovered && !typing)));

    ImDrawList *dl = ImGui::GetWindowDrawList();
    float fs = ImGui::GetFontSize();
    ImVec2 tl(pos.x + 8.0f, pos.y + 6.0f);

    if (!playing)
        ShadowText(dl, tl, IM_COL32(255, 255, 255, 215), "Preview. Press F5 to play.");
    else if (m_Context->State == SceneState::Pause)
        ShadowText(dl, tl, U32(Colors::Warning), fullscreen ? "Paused   F6 resume   F11 exit fullscreen   F5 stop" : "Paused");

    if (fullscreen && m_Context->State == SceneState::Play)
    {
        const double remaining = m_HintUntil - ImGui::GetTime();
        if (remaining > 0.0)
        {
            const float alpha = static_cast<float>(std::min(1.0, remaining / 0.6));
            const char *hint = "F11 exit fullscreen   F6 pause   F5 stop";
            const ImVec2 ts = ImGui::CalcTextSize(hint);
            const ImVec2 at(std::floor(pos.x + (size.x - ts.x) * 0.5f), pos.y + 18.0f);
            dl->AddRectFilled(at - ImVec2(12, 7), at + ts + ImVec2(12, 7), IM_COL32(0, 0, 0, static_cast<int>(150 * alpha)), 6.0f);
            dl->AddText(at, IM_COL32(255, 255, 255, static_cast<int>(235 * alpha)), hint);
        }
    }

    if (Settings().ShowStats)
    {
        char stats[96];
        std::snprintf(stats, sizeof(stats), "%dx%d  %d draw calls  %d primitives", w, h, m_Stats.drawCalls, m_Stats.primitives);
        ImVec2 ts = ImGui::CalcTextSize(stats);
        ImVec2 p(pos.x + size.x - ts.x - 8.0f, pos.y + size.y - fs - 6.0f);
        ShadowText(dl, p, IM_COL32(255, 255, 255, 215), stats);
    }
}

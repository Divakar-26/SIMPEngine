#define IMGUI_DEFINE_MATH_OPERATORS
#include "GameViewPanel.h"
#include "UI/Widgets.h"

#include <Engine/Core/Display.h>
#include <Engine/Core/Log.h>
#include <Engine/Project/Project.h>
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

SIMPEngine::ViewLayout GameViewPanel::RenderScene(int width, int height)
{
    if (!m_Framebuffer)
        m_Framebuffer = std::make_unique<SIMPEngine::Framebuffer>(width, height);
    else if (m_Framebuffer->GetWidth() != width || m_Framebuffer->GetHeight() != height)
        m_Framebuffer->Resize(width, height);

    SIMPEngine::Scene &scene = *m_Context->Scene;
    SIMPEngine::ViewLayout layout = SIMPEngine::Display::Compute(width, height);

    SIMPEngine::Renderer::ResetStats();
    try
    {
        layout = m_GameView.Render(scene, width, height, m_Framebuffer.get());
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
    return layout;
}

namespace
{
    struct SizePreset
    {
        const char *Name;
        int Width;
        int Height;
    };

    const SizePreset SizePresets[] = {
        {"Fit panel", 0, 0},
        {"Game window", -1, -1},
        {"Game resolution", -2, -2},
        {"1920 x 1080  (16:9)", 1920, 1080},
        {"1280 x 720  (16:9)", 1280, 720},
        {"2560 x 1440  (16:9)", 2560, 1440},
        {"1366 x 768  (laptop)", 1366, 768},
        {"1280 x 800  (16:10)", 1280, 800},
        {"1024 x 768  (4:3)", 1024, 768},
        {"2400 x 1080  (phone landscape)", 2400, 1080},
        {"1080 x 2400  (phone portrait)", 1080, 2400},
        {"1080 x 1920  (portrait 16:9)", 1080, 1920},
        {"2560 x 1080  (ultrawide)", 2560, 1080},
        {"Custom", -3, -3},
    };
    constexpr int PresetCount = static_cast<int>(sizeof(SizePresets) / sizeof(SizePresets[0]));
}

void GameViewPanel::DrawToolbar()
{
    using namespace EditorUI;
    ImGui::SetCursorPos(ImVec2(8.0f, 6.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::TextUnformatted("Window");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("##gameSize", SizePresets[m_SizePreset].Name))
    {
        for (int i = 0; i < PresetCount; ++i)
            if (ImGui::Selectable(SizePresets[i].Name, i == m_SizePreset))
                m_SizePreset = i;
        ImGui::EndCombo();
    }
    if (SizePresets[m_SizePreset].Width == -3)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f);
        ImGui::DragInt("##customW", &m_CustomWidth, 4.0f, 64, 8192);
        ImGui::SameLine();
        ImGui::TextUnformatted("x");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f);
        ImGui::DragInt("##customH", &m_CustomHeight, 4.0f, 64, 8192);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Safe frame", &m_ShowGuides);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        ImGui::SetTooltip("Outline the game resolution inside the window");
}

void GameViewPanel::DrawCanvas(bool fullscreen)
{
    using namespace EditorUI;

    const bool playing = m_Context->State != SceneState::Edit;

    if (!fullscreen)
        DrawToolbar();

    ImVec2 pos = fullscreen ? ImGui::GetCursorScreenPos() : ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + 36.0f);
    pos = ImVec2(std::round(pos.x), std::round(pos.y));
    ImGui::SetCursorScreenPos(pos);
    ImVec2 area = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - pos.x, ImGui::GetWindowPos().y + ImGui::GetWindowHeight() - pos.y);
    area = ImVec2(std::floor(area.x), std::floor(area.y));
    if (area.x < 2 || area.y < 2)
        return;

    int w = static_cast<int>(area.x);
    int h = static_cast<int>(area.y);
    ImVec2 size = area;
    if (!fullscreen)
    {
        const SizePreset &preset = SizePresets[m_SizePreset];
        int target[2] = {0, 0};
        if (preset.Width > 0)
        {
            target[0] = preset.Width;
            target[1] = preset.Height;
        }
        else if (preset.Width == -1)
        {
            target[0] = SIMPEngine::Project::IsOpen() ? SIMPEngine::Project::Settings().WindowWidth : 1280;
            target[1] = SIMPEngine::Project::IsOpen() ? SIMPEngine::Project::Settings().WindowHeight : 720;
        }
        else if (preset.Width == -2)
        {
            target[0] = SIMPEngine::Display::Get().Width;
            target[1] = SIMPEngine::Display::Get().Height;
        }
        else if (preset.Width == -3)
        {
            target[0] = m_CustomWidth;
            target[1] = m_CustomHeight;
        }
        if (target[0] > 0 && target[1] > 0)
        {
            w = target[0];
            h = target[1];
            const float scale = std::min(1.0f, std::min(area.x / static_cast<float>(w), area.y / static_cast<float>(h)));
            size = ImVec2(std::floor(w * scale), std::floor(h * scale));
            pos = ImVec2(std::round(pos.x + (area.x - size.x) * 0.5f), std::round(pos.y + (area.y - size.y) * 0.5f));
            ImGui::SetCursorScreenPos(pos);
        }
    }

    const SIMPEngine::ViewLayout layout = RenderScene(w, h);

    ImGui::Image(EditorUI::TextureId(m_Framebuffer->GetColorAttachment()), size, ImVec2(0, 1), ImVec2(1, 0));
    bool hovered = ImGui::IsItemHovered();

    const float scaleX = size.x / static_cast<float>(layout.WindowWidth);
    const float scaleY = size.y / static_cast<float>(layout.WindowHeight);
    SIMPEngine::Input::SetViewport(pos.x + layout.X * scaleX, pos.y + (layout.WindowHeight - layout.Y - layout.Height) * scaleY,
                                   layout.Width * scaleX, layout.Height * scaleY);
    bool typing = ImGui::GetIO().WantTextInput;
    SIMPEngine::Input::SetEnabled(m_Context->State == SceneState::Play && (m_Focused || (hovered && !typing)));

    ImDrawList *dl = ImGui::GetWindowDrawList();
    float fs = ImGui::GetFontSize();
    ImVec2 tl(pos.x + 8.0f, pos.y + 6.0f);

    if (m_ShowGuides && !fullscreen)
    {
        const SIMPEngine::DisplaySettings &display = SIMPEngine::Display::Get();
        const float designAspect = static_cast<float>(display.Width) / static_cast<float>(display.Height);
        float gw = static_cast<float>(layout.Width), gh = gw / designAspect;
        if (gh > layout.Height)
        {
            gh = static_cast<float>(layout.Height);
            gw = gh * designAspect;
        }
        const ImVec2 center(pos.x + (layout.X + layout.Width * 0.5f) * scaleX, pos.y + (layout.WindowHeight - layout.Y - layout.Height * 0.5f) * scaleY);
        dl->AddRect(ImVec2(center.x - gw * 0.5f * scaleX, center.y - gh * 0.5f * scaleY), ImVec2(center.x + gw * 0.5f * scaleX, center.y + gh * 0.5f * scaleY), IM_COL32(255, 220, 90, 160), 0.0f, 0, 1.5f);
    }

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
        std::snprintf(stats, sizeof(stats), "%dx%d  view %dx%d  %d draw calls  %d primitives", w, h, layout.RenderWidth, layout.RenderHeight, m_Stats.drawCalls, m_Stats.primitives);
        ImVec2 ts = ImGui::CalcTextSize(stats);
        ImVec2 p(pos.x + size.x - ts.x - 8.0f, pos.y + size.y - fs - 6.0f);
        ShadowText(dl, p, IM_COL32(255, 255, 255, 215), stats);
    }
}

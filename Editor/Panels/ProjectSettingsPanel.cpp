#include "ProjectSettingsPanel.h"
#include "UI/IconsLucide.h"
#include "UI/Widgets.h"

#include <Engine/Core/VFS.h>
#include <Engine/Project/Project.h>

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <utility>
#include <filesystem>

namespace
{
    bool IsSceneFile(const std::filesystem::path &path)
    {
        const std::string ext = path.extension().string();
        return ext == ".yaml" || ext == ".yml" || ext == ".simpscene";
    }
}

void ProjectSettingsPanel::Open()
{
    m_Open = true;
    m_Error.clear();
    m_Listening = -1;
    m_InputActions = SIMPEngine::InputActions::GetDefaults();
    RefreshScenes();
}

void ProjectSettingsPanel::DrawInputActions()
{
    using namespace EditorUI;
    using SIMPEngine::InputActions;

    const float icon = ImGui::GetFrameHeight();
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    const bool nameTaken = std::any_of(m_InputActions.begin(), m_InputActions.end(), [&](const SIMPEngine::InputAction &action)
                                       { return action.Name == m_NewAction; });
    const float addW = ImGui::CalcTextSize("Add").x + 28.0f;
    const float resetW = ImGui::CalcTextSize("Reset to Defaults").x + 28.0f;
    ImGui::SetNextItemWidth(std::max(120.0f, ImGui::GetContentRegionAvail().x - addW - resetW - spacing * 2.0f));
    const bool submit = ImGui::InputTextWithHint("##newAction", "Add new action", &m_NewAction, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    ImGui::BeginDisabled(m_NewAction.empty() || nameTaken);
    if (Button("Add", ButtonKind::Primary, ImVec2(addW, 0)) || (submit && !m_NewAction.empty() && !nameTaken))
    {
        SIMPEngine::InputAction action;
        action.Name = m_NewAction;
        m_InputActions.push_back(action);
        m_NewAction.clear();
        m_InputDirty = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (Button("Reset to Defaults", ButtonKind::Secondary, ImVec2(resetW, 0)))
    {
        InputActions::InstallBuiltinDefaults();
        m_InputActions = InputActions::GetDefaults();
        m_Listening = -1;
        m_InputDirty = true;
    }
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    int removeAction = -1;
    for (size_t a = 0; a < m_InputActions.size(); ++a)
    {
        SIMPEngine::InputAction &action = m_InputActions[a];
        const bool listening = m_Listening == static_cast<int>(a);
        ImGui::PushID(static_cast<int>(a));

        const ImVec2 rowMin = ImGui::GetCursorScreenPos();
        const float rowW = ImGui::GetContentRegionAvail().x;
        ImGui::GetWindowDrawList()->AddRectFilled(rowMin, ImVec2(rowMin.x + rowW, rowMin.y + icon), U32(Colors::Header), Metrics::FrameRounding);
        ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 8.0f, rowMin.y));
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(GetFont(FontRole::Medium), 0.0f);
        ImGui::TextUnformatted(action.Name.c_str());
        ImGui::PopFont();
        ImGui::SameLine(rowW - icon * 2.0f - 4.0f);
        if (IconButton("##add", ICON_LC_PLUS, listening ? "Cancel" : "Add binding", listening, icon))
            m_Listening = listening ? -1 : static_cast<int>(a);
        ImGui::SameLine(0.0f, 4.0f);
        if (IconButton("##delete", ICON_LC_TRASH_2, "Delete action", false, icon))
            removeAction = static_cast<int>(a);

        int removeBinding = -1;
        for (size_t b = 0; b < action.Bindings.size(); ++b)
        {
            SIMPEngine::InputBinding &binding = action.Bindings[b];
            ImGui::PushID(static_cast<int>(b));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 22.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(InputActions::DisplayText(binding).c_str());
            ImGui::SameLine(rowW - icon * 2.0f - 4.0f);
            if (IconButton("##flip", ICON_LC_ARROW_LEFT_RIGHT, binding.Scale < 0.0f ? "Make positive" : "Make negative", binding.Scale < 0.0f, icon))
            {
                binding.Scale = -binding.Scale;
                m_InputDirty = true;
            }
            ImGui::SameLine(0.0f, 4.0f);
            if (IconButton("##remove", ICON_LC_X, "Remove binding", false, icon))
                removeBinding = static_cast<int>(b);
            ImGui::PopID();
        }
        if (removeBinding >= 0)
        {
            action.Bindings.erase(action.Bindings.begin() + removeBinding);
            m_InputDirty = true;
        }

        if (listening)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 22.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Accent);
            ImGui::TextWrapped("Press a key, mouse button, pad button or move a stick (Esc cancels)");
            ImGui::PopStyleColor();
            if (auto pressed = InputActions::PollPressedBinding())
            {
                if (*pressed == "Key.Escape")
                    m_Listening = -1;
                else if (*pressed != "Mouse.Left")
                {
                    if (auto binding = InputActions::FromText(*pressed))
                    {
                        if (std::find(action.Bindings.begin(), action.Bindings.end(), *binding) == action.Bindings.end())
                            action.Bindings.push_back(*binding);
                        m_InputDirty = true;
                    }
                    m_Listening = -1;
                }
            }
        }
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::PopID();
    }
    if (removeAction >= 0)
    {
        m_InputActions.erase(m_InputActions.begin() + removeAction);
        m_Listening = -1;
        m_InputDirty = true;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("A binding marked (-) pushes the action negative (A on MoveX). Saved to input.yaml.");
    ImGui::PopStyleColor();
}

void ProjectSettingsPanel::RefreshScenes()
{
    namespace fs = std::filesystem;
    m_Scenes.clear();
    const fs::path assets = SIMPEngine::Project::AssetDirectory();
    std::error_code ec;
    if (!fs::is_directory(assets, ec))
        return;
    for (auto it = fs::recursive_directory_iterator(assets, ec); it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec)
            break;
        if (it->is_regular_file() && IsSceneFile(it->path()))
            m_Scenes.push_back("assets://" + fs::relative(it->path(), assets, ec).generic_string());
    }
    std::sort(m_Scenes.begin(), m_Scenes.end());
}

void ProjectSettingsPanel::DrawGeneralPage()
{
    using namespace EditorUI;
    SIMPEngine::ProjectSettings &s = SIMPEngine::Project::Settings();
    switch (m_Category)
    {
    case 0:
    {
    m_Dirty |= PropertyText("Name", s.Name);
    m_Dirty |= PropertyText("Version", s.Version);
    PropertyValue("Folder", SIMPEngine::Project::Directory().string().c_str());
        break;
    }
    case 1:
    {
    PropertyLabel("Start Scene");
    if (ImGui::BeginCombo("##start", s.StartScene.c_str()))
    {
        for (const std::string &scene : m_Scenes)
        {
            if (ImGui::Selectable(scene.c_str(), scene == s.StartScene))
            {
                s.StartScene = scene;
                m_Dirty = true;
            }
        }
        if (m_Scenes.empty())
            ImGui::TextDisabled("No scenes under assets://");
        ImGui::EndCombo();
    }
    PropertyLabel("");
    ImGui::BeginDisabled(m_CurrentScene.empty() || m_CurrentScene == s.StartScene);
    if (Button("Use Current Scene", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
    {
        s.StartScene = m_CurrentScene;
        m_Dirty = true;
    }
    ImGui::EndDisabled();
        break;
    }
    case 2:
    {
    static const char *orientations[] = {"Landscape", "Portrait", "Auto"};
    int orientation = s.Orientation == "portrait" ? 1 : s.Orientation == "auto" ? 2 : 0;
    if (PropertySegmented("Orientation", &orientation, orientations, 3))
    {
        static const char *values[] = {"landscape", "portrait", "auto"};
        s.Orientation = values[orientation];
        const bool wantPortrait = orientation == 1;
        const bool wantLandscape = orientation == 0;
        if ((wantPortrait && s.ViewWidth > s.ViewHeight) || (wantLandscape && s.ViewWidth < s.ViewHeight))
            std::swap(s.ViewWidth, s.ViewHeight);
        if ((wantPortrait && s.WindowWidth > s.WindowHeight) || (wantLandscape && s.WindowWidth < s.WindowHeight))
            std::swap(s.WindowWidth, s.WindowHeight);
        m_Dirty = true;
        SIMPEngine::Project::ApplyDisplay();
    }
    PropertyLabel("");
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("%s", orientation == 0 ? "The game is locked to landscape on phones and tablets. Resolution and window size are set to landscape."
                          : orientation == 1 ? "The game is locked to portrait on phones and tablets. Resolution and window size are set to portrait."
                                             : "The device decides: the game turns with the phone. Use an aspect mode that fits both, like Expand.");
    ImGui::PopStyleColor();
    static const struct
    {
        const char *Name;
        int W, H;
    } resolutions[] = {{"1920 x 1080  (default)", 1920, 1080}, {"1280 x 720", 1280, 720}, {"1152 x 648", 1152, 648},
                       {"960 x 540", 960, 540}, {"640 x 360  (pixel art)", 640, 360}, {"480 x 270  (pixel art)", 480, 270},
                       {"1080 x 1920  (portrait)", 1080, 1920}, {"720 x 1280  (portrait)", 720, 1280}, {"540 x 960  (portrait)", 540, 960}, {"1024 x 768  (4:3)", 1024, 768}};
    const bool portraitProject = s.Orientation == "portrait";
    int view[2] = {s.ViewWidth, s.ViewHeight};
    if (PropertyInt2("Game resolution", view, 1.0f, 64, 16384))
    {
        s.ViewWidth = view[0];
        s.ViewHeight = view[1];
        m_Dirty = true;
        SIMPEngine::Project::ApplyDisplay();
    }
    PropertyLabel("Presets");
    if (ImGui::BeginCombo("##viewPreset", "Choose..."))
    {
        for (const auto &r : resolutions)
        {
            const bool tall = r.H > r.W;
            if (tall != portraitProject && s.Orientation != "auto")
                continue;
            if (ImGui::Selectable(r.Name))
            {
                s.ViewWidth = r.W;
                s.ViewHeight = r.H;
                m_Dirty = true;
                SIMPEngine::Project::ApplyDisplay();
            }
        }
        ImGui::EndCombo();
    }

    static const char *aspects[] = {"Keep Height", "Keep Width", "Expand", "Keep (black bars)"};
    PropertyLabel("Aspect");
    if (ImGui::BeginCombo("##aspect", aspects[std::clamp(s.Aspect, 0, 3)]))
    {
        for (int i = 0; i < 4; ++i)
            if (ImGui::Selectable(aspects[i], i == s.Aspect))
            {
                s.Aspect = i;
                m_Dirty = true;
                SIMPEngine::Project::ApplyDisplay();
            }
        ImGui::EndCombo();
    }
    static const char *aspectHelp[] = {
        "The view always shows the game height; wider windows show more to the sides.",
        "The view always shows the game width; taller windows show more above and below.",
        "The whole game area is always visible; extra space shows more of the world.",
        "The game is drawn at its own aspect ratio with black bars around it."};
    PropertyLabel("");
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("%s", aspectHelp[std::clamp(s.Aspect, 0, 3)]);
    ImGui::PopStyleColor();

    PropertyLabel("Render scale");
    if (ImGui::SliderFloat("##renderScale", &s.RenderScale, 0.25f, 1.0f, "%.2fx"))
    {
        m_Dirty = true;
        SIMPEngine::Project::ApplyDisplay();
    }
    PropertyLabel("");
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("Draws the game at a fraction of the window size and scales it up. Lower it for phones and slow GPUs; 0.6 to 0.75 usually looks fine and runs much faster.");
    ImGui::PopStyleColor();

    static const char *stretches[] = {"Window resolution", "Game resolution"};
    int stretch = s.Stretch;
    if (PropertySegmented("Render at", &stretch, stretches, 2))
    {
        s.Stretch = stretch;
        m_Dirty = true;
        SIMPEngine::Project::ApplyDisplay();
    }
    PropertyLabel("");
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped(s.Stretch == 0 ? "Sharp text and UI at any window size." : "The game is drawn at the game resolution and scaled up, the pixel-art look.");
    ImGui::PopStyleColor();
        break;
    }
    default:
    {
    m_Dirty |= PropertyText("Title", s.WindowTitle);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        ImGui::SetTooltip("Empty = the project name");
    int size[2] = {s.WindowWidth, s.WindowHeight};
    if (PropertyInt2("Size", size, 1.0f, 160, 7680))
    {
        s.WindowWidth = size[0];
        s.WindowHeight = size[1];
        m_Dirty = true;
    }
    m_Dirty |= PropertyBool("Fullscreen", &s.Fullscreen);
    m_Dirty |= PropertyBool("Start maximized", &s.Maximized);
    m_Dirty |= PropertyBool("Borderless", &s.Borderless);
    m_Dirty |= PropertyBool("Resizable", &s.Resizable);
    m_Dirty |= PropertyBool("VSync", &s.VSync);
        break;
    }
    }
}

void ProjectSettingsPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Open || !SIMPEngine::Project::IsOpen())
        return;

    const float k = ImGui::GetFontSize() / Metrics::FontSize;
    ImGui::SetNextWindowSize(ImVec2(720.0f * k, 470.0f * k), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(560.0f, 380.0f), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::Header);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, Colors::Header);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, Colors::Header);
    const bool visible = ImGui::Begin("Project Settings###ProjectSettings", &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleColor(3);
    if (!visible)
    {
        ImGui::End();
        return;
    }

    PushEditSuppression();
    const float footerH = ImGui::GetFrameHeightWithSpacing() + 10.0f;

    if (ImGui::BeginTabBar("##projectSettingsTabs"))
    {
        if (ImGui::BeginTabItem("General"))
        {
            static const char *categories[] = {"Project", "Startup", "Display", "Game Window"};
            const float height = ImGui::GetContentRegionAvail().y - footerH;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Window);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 10.0f));
            ImGui::BeginChild("##projectNav", ImVec2(170.0f * k, height), ImGuiChildFlags_AlwaysUseWindowPadding);
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
            ImGui::TextUnformatted("Application");
            ImGui::PopStyleColor();
            for (int i = 0; i < 4; ++i)
            {
                if (NavItem(categories[i], m_Category == i))
                    m_Category = i;
            }
            ImGui::EndChild();
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
            ImGui::BeginChild("##projectBody", ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding);
            ImGui::PushFont(GetFont(FontRole::Strong), 0.0f);
            ImGui::TextUnformatted(categories[std::clamp(m_Category, 0, 3)]);
            ImGui::PopFont();
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            DrawGeneralPage();
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Input Map"))
        {
            const float height = ImGui::GetContentRegionAvail().y - footerH;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Window);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
            ImGui::BeginChild("##inputBody", ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding);
            DrawInputActions();
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (m_InputDirty)
    {
        m_InputDirty = false;
        SIMPEngine::InputActions::SetDefaults(m_InputActions);
        if (!SIMPEngine::InputActions::SaveProject(SIMPEngine::Project::Directory().string()))
            m_Error = "Could not write input.yaml";
    }

    if (m_Dirty && !ImGui::IsAnyItemActive())
    {
        m_Dirty = false;
        m_Error.clear();
        SIMPEngine::Project::SaveActive(&m_Error);
    }

    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, m_Error.empty() ? Colors::TextFaint : Colors::Error);
    ImGui::TextUnformatted(m_Error.empty() ? "Changes are saved to project.simp" : m_Error.c_str());
    ImGui::PopStyleColor();
    const float closeW = 96.0f * k;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(), (ImGui::GetWindowWidth() - closeW) * 0.5f));
    if (Button("Close", ButtonKind::Secondary, ImVec2(closeW, 0.0f)))
        m_Open = false;

    PopEditSuppression();
    ImGui::End();
}

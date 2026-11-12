#include "ProjectSettingsPanel.h"
#include "UI/Widgets.h"

#include <Engine/Core/VFS.h>
#include <Engine/Project/Project.h>

#include <imgui_internal.h>

#include <algorithm>
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

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("Gameplay actions with keyboard, mouse and gamepad bindings. A leading minus flips the sign of the contribution (A on MoveX). Saved to input.yaml.");
    ImGui::PopStyleColor();

    int removeAction = -1;
    for (size_t a = 0; a < m_InputActions.size(); ++a)
    {
        SIMPEngine::InputAction &action = m_InputActions[a];
        ImGui::PushID(static_cast<int>(a));
        ImGui::Separator();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(action.Name.c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 150.0f);
        if (Button(m_Listening == static_cast<int>(a) ? "Cancel" : "Add Binding", ButtonKind::Secondary, ImVec2(100, 0)))
            m_Listening = m_Listening == static_cast<int>(a) ? -1 : static_cast<int>(a);
        ImGui::SameLine();
        if (Button("Delete", ButtonKind::Secondary, ImVec2(0, 0)))
            removeAction = static_cast<int>(a);

        int removeBinding = -1;
        for (size_t b = 0; b < action.Bindings.size(); ++b)
        {
            SIMPEngine::InputBinding &binding = action.Bindings[b];
            ImGui::PushID(static_cast<int>(b));
            ImGui::Indent();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(InputActions::DisplayText(binding).c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 110.0f);
            if (Button(binding.Scale < 0.0f ? "Positive" : "Negative", ButtonKind::Secondary, ImVec2(70, 0)))
            {
                binding.Scale = -binding.Scale;
                m_InputDirty = true;
            }
            ImGui::SameLine();
            if (Button("X", ButtonKind::Secondary, ImVec2(0, 0)))
                removeBinding = static_cast<int>(b);
            ImGui::Unindent();
            ImGui::PopID();
        }
        if (removeBinding >= 0)
        {
            action.Bindings.erase(action.Bindings.begin() + removeBinding);
            m_InputDirty = true;
        }

        if (m_Listening == static_cast<int>(a))
        {
            ImGui::Indent();
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Accent);
            ImGui::TextUnformatted("Press a key, mouse button, pad button or move a stick (Esc cancels)");
            ImGui::PopStyleColor();
            ImGui::Unindent();
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
        ImGui::PopID();
    }
    if (removeAction >= 0)
    {
        m_InputActions.erase(m_InputActions.begin() + removeAction);
        m_Listening = -1;
        m_InputDirty = true;
    }

    ImGui::Separator();
    PropertyText("New Action", m_NewAction);
    PropertyLabel("");
    const bool nameTaken = std::any_of(m_InputActions.begin(), m_InputActions.end(), [&](const SIMPEngine::InputAction &action)
                                       { return action.Name == m_NewAction; });
    ImGui::BeginDisabled(m_NewAction.empty() || nameTaken);
    if (Button("Add Action", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
    {
        SIMPEngine::InputAction action;
        action.Name = m_NewAction;
        m_InputActions.push_back(action);
        m_NewAction.clear();
        m_InputDirty = true;
    }
    ImGui::EndDisabled();
    PropertyLabel("");
    if (Button("Reset to Built-in Actions", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
    {
        InputActions::InstallBuiltinDefaults();
        m_InputActions = InputActions::GetDefaults();
        m_Listening = -1;
        m_InputDirty = true;
    }
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

void ProjectSettingsPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Open || !SIMPEngine::Project::IsOpen())
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(460, 0), ImVec2(460, FLT_MAX));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin("Project Settings###ProjectSettings", &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }

    PushEditSuppression();
    SIMPEngine::ProjectSettings &s = SIMPEngine::Project::Settings();

    if (BeginSection("project", "Project"))
    {
        m_Dirty |= PropertyText("Name", s.Name);
        m_Dirty |= PropertyText("Version", s.Version);
        PropertyValue("Folder", SIMPEngine::Project::Directory().string().c_str());
    }
    EndSection();

    if (BeginSection("startup", "Startup"))
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
    }
    EndSection();

    if (BeginSection("display", "Display"))
    {
        static const struct
        {
            const char *Name;
            int W, H;
        } resolutions[] = {{"1920 x 1080  (default)", 1920, 1080}, {"1280 x 720", 1280, 720}, {"1152 x 648", 1152, 648},
                           {"960 x 540", 960, 540}, {"640 x 360  (pixel art)", 640, 360}, {"480 x 270  (pixel art)", 480, 270},
                           {"1080 x 1920  (portrait)", 1080, 1920}, {"720 x 1280  (portrait)", 720, 1280}, {"1024 x 768  (4:3)", 1024, 768}};
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
                if (ImGui::Selectable(r.Name))
                {
                    s.ViewWidth = r.W;
                    s.ViewHeight = r.H;
                    m_Dirty = true;
                    SIMPEngine::Project::ApplyDisplay();
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
    }
    EndSection();

    if (BeginSection("window", "Game Window"))
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
        static const char *orientations[] = {"landscape", "portrait", "auto"};
        int orientation = s.Orientation == "portrait" ? 1 : s.Orientation == "auto" ? 2 : 0;
        if (PropertySegmented("Phone orientation", &orientation, orientations, 3))
        {
            s.Orientation = orientations[orientation];
            m_Dirty = true;
        }
    }
    EndSection();

    if (BeginSection("input", "Input Actions", nullptr, false))
        DrawInputActions();
    EndSection();

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
    if (!m_Error.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::Error);
        ImGui::TextWrapped("%s", m_Error.c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextUnformatted("Changes are saved to project.simp");
        ImGui::PopStyleColor();
    }

    PopEditSuppression();
    ImGui::End();
}

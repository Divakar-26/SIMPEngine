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
    RefreshScenes();
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
        m_Dirty |= PropertyBool("Resizable", &s.Resizable);
        m_Dirty |= PropertyBool("VSync", &s.VSync);
    }
    EndSection();

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

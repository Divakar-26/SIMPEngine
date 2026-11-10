#include "BuildPanel.h"
#include "UI/FileDialogs.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <Engine/Project/Project.h>

#include <imgui_internal.h>

#include <filesystem>

namespace
{
    constexpr const char *FolderDialogId = "build-output";
}

void BuildPanel::Open()
{
    m_Open = true;
    if (m_Output.empty() && SIMPEngine::Project::IsOpen())
        m_Output = (SIMPEngine::Project::Directory() / "builds" / GameBuilder::PlatformName()).string();
}

void BuildPanel::StartBuild(bool runAfter)
{
    m_Message.clear();
    if (m_BeforeBuild && !m_BeforeBuild())
    {
        m_Message = "Save the scene first, then build again.";
        return;
    }

    GameBuilder::Options options;
    options.ProjectDirectory = SIMPEngine::Project::Directory();
    options.OutputDirectory = m_Output;
    options.Settings = SIMPEngine::Project::Settings();
    options.EngineResources = SIMPEngine::Project::EngineResourceDirectory();
    m_RunAfter = runAfter;
    m_Launched = false;
    m_Builder.Start(options);
}

void BuildPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (auto folder = TakePickedFolder(FolderDialogId))
        m_Output = *folder;

    const GameBuilder::State state = m_Builder.GetState();
    if (state == GameBuilder::State::Succeeded && m_RunAfter && !m_Launched)
    {
        m_Launched = true;
        LaunchProcess(m_Builder.GetExecutable().string());
    }

    if (!m_Open || !SIMPEngine::Project::IsOpen())
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(520, 0), ImVec2(520, FLT_MAX));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin("Build Game###BuildGame", &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }

    PushEditSuppression();
    const SIMPEngine::ProjectSettings &settings = SIMPEngine::Project::Settings();
    const bool running = state == GameBuilder::State::Running;

    if (BeginSection("build-project", "Project"))
    {
        PropertyValue("Name", (settings.Name + "  " + settings.Version).c_str());
        PropertyValue("Start Scene", settings.StartScene.c_str());
        PropertyValue("Window", (std::to_string(settings.WindowWidth) + " x " + std::to_string(settings.WindowHeight) + (settings.Fullscreen ? ", fullscreen" : "")).c_str());
        PropertyLabel("");
        if (Button("Project Settings...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_OpenSettings)
            m_OpenSettings();
    }
    EndSection();

    if (BeginSection("build-target", "Target"))
    {
        PropertyValue("Platform", (GameBuilder::PlatformName() + " (this computer)").c_str());
        PropertyValue("Executable", GameBuilder::ExecutableName(settings.Name).c_str());
        ImGui::BeginDisabled(running);
        PropertyText("Output Folder", m_Output);
        PropertyLabel("");
        if (Button("Browse...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            PickFolder(FolderDialogId, m_Output);
        ImGui::EndDisabled();
    }
    EndSection();

    if (state != GameBuilder::State::Idle)
    {
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, state == GameBuilder::State::Failed ? Colors::Error : (state == GameBuilder::State::Succeeded ? Colors::Success : Colors::Accent));
        ImGui::ProgressBar(m_Builder.GetProgress(), ImVec2(-FLT_MIN, 4.0f), "");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Frame);
        ImGui::BeginChild("##buildlog", ImVec2(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 7.0f + 8.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(GetFont(FontRole::Mono), 0.0f);
        for (const std::string &line : m_Builder.GetLog())
        {
            const bool error = line.rfind("Error", 0) == 0;
            const bool warning = line.rfind("Warning", 0) == 0;
            ImGui::PushStyleColor(ImGuiCol_Text, error ? Colors::Error : (warning ? Colors::Warning : Colors::TextDim));
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();
        }
        if (running)
            ImGui::SetScrollHereY(1.0f);
        ImGui::PopFont();
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    if (!m_Message.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
        ImGui::TextWrapped("%s", m_Message.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0, 4));
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float bw = 120.0f;
    if (state == GameBuilder::State::Succeeded)
    {
        if (Button("Open Folder", ButtonKind::Secondary, ImVec2(bw, 0)))
            OpenInFileManager(m_Builder.GetOutputDirectory().string());
        ImGui::SameLine();
        if (Button("Run", ButtonKind::Secondary, ImVec2(bw, 0)))
            LaunchProcess(m_Builder.GetExecutable().string());
        ImGui::SameLine();
    }
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - bw * 2.0f - spacing));
    ImGui::BeginDisabled(running || m_Output.empty());
    if (Button("Build and Run", ButtonKind::Secondary, ImVec2(bw, 0)))
        StartBuild(true);
    ImGui::SameLine();
    if (Button(running ? "Building..." : "Build", ButtonKind::Primary, ImVec2(bw, 0)))
        StartBuild(false);
    ImGui::EndDisabled();

    PopEditSuppression();
    ImGui::End();
}

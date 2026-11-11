#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Version.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Project/Project.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>

#include "EditorLayer.h"
#include "ProjectHub/ProjectHubLayer.h"
#include "ProjectHub/RecentProjects.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <filesystem>
#include <string>

namespace
{
    SIMPEngine::ApplicationSpecification HubSpecification()
    {
        SIMPEngine::ApplicationSpecification spec;
        spec.Title = std::string(SIMPEngine::EngineName) + " " + SIMPEngine::EngineVersion;
        spec.Name = "Editor";
        spec.Width = 1180;
        spec.Height = 720;
        return spec;
    }
}

class SIMPEditor : public SIMPEngine::Application
{
public:
    SIMPEditor()
        : Application(HubSpecification())
    {
        static std::string iniPath = (RecentProjects::PreferenceDirectory() / "editor.ini").string();
        ImGui::GetIO().IniFilename = iniPath.c_str();
        ShowHub();
    }

    bool OpenProject(const std::filesystem::path &directory, std::string &error)
    {
        if (!SIMPEngine::Project::Open(directory, &error))
            return false;

        Queue([this]
              {
            if (m_Hub)
            {
                PopOverlay(m_Hub);
                m_Hub = nullptr;
            }

            const SIMPEngine::ProjectSettings &settings = SIMPEngine::Project::Settings();
            m_SceneManager = std::make_shared<SIMPEngine::SceneManager>();
            auto scene = std::make_shared<SIMPEngine::Scene>(settings.Name);
            m_SceneManager->AddScene(settings.Name, scene);
            m_SceneManager->SetActiveScene(settings.Name);

            auto rendering = std::make_unique<SIMPEngine::RenderingLayer>(m_SceneManager);
            m_Rendering = rendering.get();
            PushOverlay(std::move(rendering));

            auto editor = std::make_unique<EditorLayer>(m_Rendering);
            m_Editor = editor.get();
            m_Editor->SetCloseProjectCallback([this] { CloseProject(); });
            PushOverlay(std::move(editor));
            m_Editor->OpenStartScene(settings.StartScene);

            SDL_MaximizeWindow(GetWindow().GetNativeWindow()); });
        return true;
    }

private:
    void ShowHub()
    {
        auto hub = std::make_unique<ProjectHubLayer>([this](const std::filesystem::path &directory, std::string &error)
                                                     { return OpenProject(directory, error); });
        m_Hub = hub.get();
        PushOverlay(std::move(hub));
    }

    void CloseProject()
    {
        Queue([this]
              {
            if (m_Editor)
            {
                PopOverlay(m_Editor);
                m_Editor = nullptr;
            }
            if (m_Rendering)
            {
                m_Rendering->GetScene().StopAudio();
                PopOverlay(m_Rendering);
                m_Rendering = nullptr;
            }
            m_SceneManager.reset();
            SIMPEngine::Project::Close();
            SDL_RestoreWindow(GetWindow().GetNativeWindow());
            ShowHub(); });
    }

    ProjectHubLayer *m_Hub = nullptr;
    EditorLayer *m_Editor = nullptr;
    SIMPEngine::RenderingLayer *m_Rendering = nullptr;
    std::shared_ptr<SIMPEngine::SceneManager> m_SceneManager;
};

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new SIMPEditor();
}

int main(int argc, char **argv)
{
    SIMPEngine::Log::Init();
    SIMPEngine::Application *app = SIMPEngine::CreateApplication();
    if (argc > 1)
    {
        std::string error;
        if (!static_cast<SIMPEditor *>(app)->OpenProject(argv[1], error))
            CORE_ERROR("{}", error);
    }
    app->Run();
    delete app;
    return 0;
}

#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Project/Project.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

namespace
{
    SIMPEngine::ApplicationSpecification Specification(const SIMPEngine::ProjectSettings &settings)
    {
        SIMPEngine::ApplicationSpecification spec;
        spec.Title = settings.Title();
        spec.Width = settings.WindowWidth;
        spec.Height = settings.WindowHeight;
        spec.Fullscreen = settings.Fullscreen;
        spec.Resizable = settings.Resizable;
        spec.VSync = settings.VSync;
        return spec;
    }
}

class Player : public SIMPEngine::Application
{
public:
    explicit Player(const SIMPEngine::ProjectSettings &settings)
        : Application(Specification(settings))
    {
        auto sceneManager = std::make_shared<SIMPEngine::SceneManager>();
        auto scene = std::make_shared<SIMPEngine::Scene>(settings.Name);
        SIMPEngine::SceneSerializer serializer(scene.get());
        if (!serializer.Deserialize(settings.StartScene))
            CORE_ERROR("Could not load the start scene {}", settings.StartScene);
        sceneManager->AddScene(settings.Name, scene);
        sceneManager->SetActiveScene(settings.Name);
        PushOverlay(std::make_unique<SIMPEngine::RenderingLayer>(sceneManager));
    }
};

SIMPEngine::Application *SIMPEngine::CreateApplication()
{
    return new Player(SIMPEngine::Project::Settings());
}

int main(int argc, char **argv)
{
    SIMPEngine::Log::Init();

    std::filesystem::path location;
    if (argc > 1)
        location = argv[1];
    else if (const char *base = SDL_GetBasePath())
        location = base;
    else
        location = std::filesystem::current_path();

    std::string error;
    if (!SIMPEngine::Project::Open(location, &error))
    {
        CORE_ERROR("{}", error);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SIMPEngine", ("Could not start the game.\n\n" + error).c_str(), nullptr);
        return 1;
    }

    SIMPEngine::Application *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}

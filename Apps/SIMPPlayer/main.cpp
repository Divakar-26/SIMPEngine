#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Project/Project.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include <SDL3/SDL.h>
#if defined(__ANDROID__)
#include <SDL3/SDL_main.h>
#endif

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    SIMPEngine::ApplicationSpecification Specification(const SIMPEngine::ProjectSettings &settings)
    {
        SIMPEngine::ApplicationSpecification spec;
        spec.Title = settings.Title();
        spec.Width = settings.WindowWidth;
        spec.Height = settings.WindowHeight;
#if defined(__ANDROID__)
        spec.Fullscreen = true;
        spec.Resizable = false;
#else
        spec.Fullscreen = settings.Fullscreen;
#endif
#if !defined(__ANDROID__)
        spec.Resizable = settings.Resizable;
#endif
        spec.VSync = settings.VSync;
        spec.Maximized = settings.Maximized;
        spec.Borderless = settings.Borderless;
        return spec;
    }
}

#if defined(__ANDROID__)
namespace
{
    std::string ReadAsset(const std::string &name)
    {
        size_t size = 0;
        void *data = SDL_LoadFile(name.c_str(), &size);
        if (!data)
            return {};
        std::string text(static_cast<const char *>(data), size);
        SDL_free(data);
        return text;
    }

    std::filesystem::path PrepareAndroidFiles()
    {
        const char *storage = SDL_GetAndroidInternalStoragePath();
        if (!storage)
            return {};
        const std::filesystem::path root = std::filesystem::path(storage) / "game";
        const std::string manifest = ReadAsset("simp_files.txt");
        if (manifest.empty())
        {
            CORE_ERROR("The APK has no simp_files.txt; pack a project with Android/tools/pack_project.py");
            return {};
        }

        std::error_code ec;
        const std::filesystem::path stamp = root / ".simp_manifest";
        std::ifstream existing(stamp, std::ios::binary);
        std::stringstream previous;
        previous << existing.rdbuf();
        if (existing.is_open() && previous.str() == manifest && std::filesystem::exists(root / "project.simp", ec))
            return root;

        std::filesystem::remove_all(root, ec);
        std::filesystem::create_directories(root, ec);
        std::istringstream lines(manifest);
        std::string rel;
        int copied = 0;
        while (std::getline(lines, rel))
        {
            while (!rel.empty() && (rel.back() == '\r' || rel.back() == ' '))
                rel.pop_back();
            if (rel.empty())
                continue;
            size_t size = 0;
            void *data = SDL_LoadFile(("files/" + rel).c_str(), &size);
            if (!data)
            {
                CORE_WARN("Could not read asset {}", rel);
                continue;
            }
            const std::filesystem::path out = root / rel;
            std::filesystem::create_directories(out.parent_path(), ec);
            std::ofstream file(out, std::ios::binary | std::ios::trunc);
            file.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
            SDL_free(data);
            ++copied;
        }
        std::ofstream(stamp, std::ios::binary | std::ios::trunc) << manifest;
        CORE_INFO("Unpacked {} files into {}", copied, root.string());
        return root;
    }
}
#endif

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
#if defined(__ANDROID__)
    (void)argc;
    (void)argv;
    location = PrepareAndroidFiles();
    if (!location.empty() && std::filesystem::is_directory(location / "engine"))
        SIMPEngine::VFS::Mount("engine", (location / "engine").string());
#else
    if (argc > 1)
        location = argv[1];
    else if (const char *base = SDL_GetBasePath())
        location = base;
    else
        location = std::filesystem::current_path();
#endif

    std::string error;
    if (!SIMPEngine::Project::Open(location, &error))
    {
        CORE_ERROR("{}", error);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SIMPEngine", ("Could not start the game.\n\n" + error).c_str(), nullptr);
        return 1;
    }
    SIMPEngine::InputActions::EnableUserBindings(SIMPEngine::Project::Settings().Name);

    SIMPEngine::Application *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}

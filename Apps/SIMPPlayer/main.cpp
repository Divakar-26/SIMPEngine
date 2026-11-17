#include <Engine/PCH.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/GameView.h>
#include <Engine/Rendering/ImageWriter.h>
#include <Engine/Project/Project.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include <SDL3/SDL.h>
#include <spdlog/sinks/callback_sink.h>
#if defined(__ANDROID__)
#include <SDL3/SDL_main.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    struct CheckOptions
    {
        bool Enabled = false;
        float Seconds = 3.0f;
        std::string Screenshot;
        int Width = 0;
        int Height = 0;
        bool KeepSaves = false;
        struct Click
        {
            float Time = 0.0f;
            float X = 0.0f;
            float Y = 0.0f;
            int Phase = 0;
        };
        std::vector<Click> Clicks;
        struct Shot
        {
            float Time = 0.0f;
            std::string Path;
            bool Done = false;
        };
        std::vector<Shot> Shots;
    };

    CheckOptions s_Check;
    int s_ExitCode = 0;

    struct CheckLog
    {
        std::mutex Mutex;
        std::vector<std::string> Errors;
        std::vector<std::string> Warnings;
    };

    CheckLog s_Log;

    void ListenToLog()
    {
        auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>([](const spdlog::details::log_msg &msg)
                                                                      {
                                                                          std::lock_guard<std::mutex> lock(s_Log.Mutex);
                                                                          if (msg.level >= spdlog::level::err)
                                                                              s_Log.Errors.emplace_back(msg.payload.data(), msg.payload.size());
                                                                          else if (msg.level == spdlog::level::warn)
                                                                              s_Log.Warnings.emplace_back(msg.payload.data(), msg.payload.size());
                                                                      });
        SIMPEngine::Log::GetCoreLogger()->sinks().push_back(sink);
        SIMPEngine::Log::GetClientLogger()->sinks().push_back(sink);
    }

    SIMPEngine::ApplicationSpecification Specification(const SIMPEngine::ProjectSettings &settings)
    {
        SIMPEngine::ApplicationSpecification spec;
        spec.Hidden = s_Check.Enabled;
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

class CheckLayer : public SIMPEngine::Layer
{
public:
    explicit CheckLayer(std::shared_ptr<SIMPEngine::SceneManager> scenes)
        : Layer("Check"), m_Scenes(std::move(scenes)) {}

    void OnUpdate(SIMPEngine::TimeStep ts) override
    {
        m_Time += ts.GetSeconds();
        ++m_Frames;
        for (CheckOptions::Click &click : s_Check.Clicks)
        {
            if (click.Phase == 0 && m_Time >= click.Time)
            {
                SIMPEngine::Input::OnMouseMoved(click.X, click.Y);
                SIMPEngine::Input::OnMouseButtonPressed(SDL_BUTTON_LEFT);
                click.Phase = 1;
            }
            else if (click.Phase == 1 && m_Time >= click.Time + 0.12f)
            {
                SIMPEngine::Input::OnMouseButtonReleased(SDL_BUTTON_LEFT);
                click.Phase = 2;
            }
        }
    }

    static bool Capture(SIMPEngine::Scene &scene, const std::string &path)
    {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(SIMPEngine::Application::Get().GetWindow().GetNativeWindow(), &w, &h);
        SIMPEngine::Framebuffer target(std::max(w, 16), std::max(h, 16));
        SIMPEngine::GameView view;
        view.Render(scene, target.GetWidth(), target.GetHeight(), &target);
        std::vector<uint8_t> pixels;
        return target.ReadPixels(pixels) && SIMPEngine::ImageWriter::WritePng(path, target.GetWidth(), target.GetHeight(), pixels);
    }

    void OnRender() override
    {
        if (auto active = m_Scenes->GetActiveScene())
            for (CheckOptions::Shot &shot : s_Check.Shots)
                if (!shot.Done && m_Time >= shot.Time)
                {
                    shot.Done = true;
                    if (Capture(*active, shot.Path))
                        std::printf("  screenshot at %.1f s: %s\n", shot.Time, shot.Path.c_str());
                    else
                        CORE_ERROR("could not write the screenshot {}", shot.Path);
                }
        if (m_Done || m_Time < s_Check.Seconds)
            return;
        m_Done = true;
        auto scene = m_Scenes->GetActiveScene();
        std::string shot;
        if (scene && !s_Check.Screenshot.empty())
        {
            if (Capture(*scene, s_Check.Screenshot))
                shot = s_Check.Screenshot;
            else
                CORE_ERROR("could not write the screenshot {}", s_Check.Screenshot);
        }
        std::lock_guard<std::mutex> lock(s_Log.Mutex);
        const size_t entities = scene ? scene->GetRegistry().view<TransformComponent>().size() : 0;
        std::printf("SIMP check: ran %.1f s, %d frames, %zu entities, %zu errors, %zu warnings\n", m_Time, m_Frames, entities, s_Log.Errors.size(), s_Log.Warnings.size());
        for (const std::string &line : s_Log.Errors)
            std::printf("  error: %s\n", line.c_str());
        for (const std::string &line : s_Log.Warnings)
            std::printf("  warning: %s\n", line.c_str());
        if (!shot.empty())
            std::printf("  screenshot: %s\n", shot.c_str());
        std::printf(s_Log.Errors.empty() ? "PASS\n" : "FAIL\n");
        std::fflush(stdout);
        s_ExitCode = s_Log.Errors.empty() ? 0 : 2;
        SIMPEngine::Application::Get().Close();
    }

private:
    std::shared_ptr<SIMPEngine::SceneManager> m_Scenes;
    float m_Time = 0.0f;
    int m_Frames = 0;
    bool m_Done = false;
};

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
        if (s_Check.Enabled)
            PushOverlay(std::make_unique<CheckLayer>(sceneManager));
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
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--check")
        {
            s_Check.Enabled = true;
            if (i + 1 < argc && std::isdigit(static_cast<unsigned char>(argv[i + 1][0])))
                s_Check.Seconds = std::stof(argv[++i]);
        }
        else if (arg == "--size" && i + 1 < argc)
        {
            int w = 0, h = 0;
            if (std::sscanf(argv[++i], "%dx%d", &w, &h) == 2 && w > 0 && h > 0)
            {
                s_Check.Width = w;
                s_Check.Height = h;
            }
        }
        else if (arg == "--click" && i + 1 < argc)
        {
            CheckOptions::Click click;
            if (std::sscanf(argv[++i], "%f,%f@%f", &click.X, &click.Y, &click.Time) == 3)
            {
                s_Check.Enabled = true;
                s_Check.Clicks.push_back(click);
            }
        }
        else if (arg == "--shot" && i + 2 < argc)
        {
            CheckOptions::Shot shot;
            shot.Time = std::stof(argv[++i]);
            shot.Path = argv[++i];
            s_Check.Enabled = true;
            s_Check.Shots.push_back(shot);
        }
        else if (arg == "--keep-saves")
            s_Check.KeepSaves = true;
        else if (arg == "--screenshot" && i + 1 < argc)
        {
            s_Check.Enabled = true;
            s_Check.Screenshot = argv[++i];
        }
        else if (arg == "--help")
        {
            std::printf("SIMPPlayer [project folder] [--check [seconds]] [--screenshot file.png] [--shot seconds file.png]\n"
                        "           [--click x,y@seconds] [--size WxH] [--keep-saves]\n"
                        "  --check        run hidden for a few seconds (default 3), print errors and warnings, exit 0 on PASS, 2 on FAIL\n"
                        "  --screenshot   save the last frame as a PNG (implies --check)\n"
                        "  --shot t f.png save a frame at t seconds (repeatable)\n"
                        "  --click x,y@t  left click at window point x,y at t seconds (repeatable)\n"
                        "  --size WxH     window size, like 480x1040 for a phone in portrait\n"
                        "  --keep-saves   check runs use a throwaway save folder unless this is given\n");
            return 0;
        }
        else
            location = arg;
    }
    if (location.empty())
    {
        if (const char *base = SDL_GetBasePath())
            location = base;
        else
            location = std::filesystem::current_path();
    }
#endif

    if (s_Check.Enabled)
    {
        ListenToLog();
        if (!s_Check.KeepSaves && !std::getenv("SIMP_SAVE_DIR"))
        {
            std::error_code ec;
            const std::filesystem::path scratch = std::filesystem::temp_directory_path(ec) / ("simp-check-" + std::to_string(SDL_GetTicks()));
            SDL_setenv_unsafe("SIMP_SAVE_DIR", scratch.string().c_str(), 1);
        }
    }
    std::string error;
    if (!SIMPEngine::Project::Open(location, &error))
    {
        CORE_ERROR("{}", error);
        if (s_Check.Enabled)
        {
            std::printf("SIMP check: %s\nFAIL\n", error.c_str());
            return 2;
        }
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SIMPEngine", ("Could not start the game.\n\n" + error).c_str(), nullptr);
        return 1;
    }
    if (s_Check.Width > 0)
    {
        SIMPEngine::Project::Settings().WindowWidth = s_Check.Width;
        SIMPEngine::Project::Settings().WindowHeight = s_Check.Height;
        SIMPEngine::Project::Settings().Fullscreen = false;
        SIMPEngine::Project::Settings().Maximized = false;
    }
    SIMPEngine::InputActions::EnableUserBindings(SIMPEngine::Project::Settings().Name);

    SIMPEngine::Application *app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
    return s_ExitCode;
}

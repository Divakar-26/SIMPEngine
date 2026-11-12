#include <Engine/Input/InputActions.h>
#include <Engine/Core/Display.h>
#include <algorithm>
#include "Project.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Version.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>

#include <SDL3/SDL.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <system_error>

namespace SIMPEngine
{
    namespace fs = std::filesystem;

    bool Project::s_Open = false;
    ProjectSettings Project::s_Settings;
    fs::path Project::s_Directory;

    namespace
    {
        void SetError(std::string *error, const std::string &message)
        {
            if (error)
                *error = message;
        }

        bool WriteText(const fs::path &path, const std::string &text)
        {
            std::ofstream out(path, std::ios::binary);
            if (!out)
                return false;
            out << text;
            return static_cast<bool>(out);
        }

        constexpr const char *PlayerScript =
            "local Player = {}\n"
            "\n"
            "function Player:OnCreate()\n"
            "    self.body = self.entity:GetCharacterController()\n"
            "end\n"
            "\n"
            "function Player:OnJump()\n"
            "    Tween.FromTo(self.entity, {scale = {0.8, 1.25}}, {scale = {1, 1}}, 0.3, \"easeOutBack\")\n"
            "end\n"
            "\n"
            "function Player:OnLand(speed)\n"
            "    if speed > 400 then\n"
            "        Tween.FromTo(self.entity, {scale = {1.25, 0.8}}, {scale = {1, 1}}, 0.3, \"easeOutBack\")\n"
            "    end\n"
            "end\n"
            "\n"
            "return Player\n";

        Entity AddBlock(Scene &scene, const char *name, glm::vec2 center, glm::vec2 size, SDL_Color color, bool solid)
        {
            Entity e = scene.CreateEntity(name);
            e.GetComponent<TransformComponent>().position = center;
            auto &render = e.AddComponent<RenderComponent>();
            render.width = size.x;
            render.height = size.y;
            render.color = color;
            if (solid)
            {
                auto &collision = e.AddComponent<CollisionComponent>();
                collision.width = size.x;
                collision.height = size.y;
                collision.offsetX = -size.x * 0.5f;
                collision.offsetY = -size.y * 0.5f;
            }
            return e;
        }

        bool WriteStartScene(const fs::path &file, const ProjectSettings &settings, ProjectTemplate kind)
        {
            Scene scene(settings.Name);
            scene.SetBackgroundColor({0.11f, 0.12f, 0.16f, 1.0f});

            Entity camera = scene.CreateEntity("Camera");
            auto &cc = camera.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f, 0.0f));
            cc.primary = true;
            cc.Camera.SetManualControl(false);

            if (kind == ProjectTemplate::Empty)
            {
                Entity title = scene.CreateEntity("Hello");
                auto &text = title.AddComponent<TextComponent>();
                text.Text = settings.Name;
                text.Size = 72.0f;
            }
            else
            {
                cc.followTarget = "Player";
                cc.followOffset = {0.0f, 120.0f};
                cc.followSmoothing = 5.0f;

                const SDL_Color ground{58, 66, 92, 255};
                AddBlock(scene, "Ground", {0.0f, -300.0f}, {2400.0f, 80.0f}, ground, true);
                AddBlock(scene, "Wall Left", {-1220.0f, 100.0f}, {40.0f, 880.0f}, ground, true);
                AddBlock(scene, "Wall Right", {1220.0f, 100.0f}, {40.0f, 880.0f}, ground, true);
                AddBlock(scene, "Step", {420.0f, -180.0f}, {320.0f, 160.0f}, ground, true);
                AddBlock(scene, "OneWay", {-380.0f, -60.0f}, {360.0f, 16.0f}, SDL_Color{120, 132, 170, 255}, true);
                AddBlock(scene, "OneWay", {-20.0f, 120.0f}, {300.0f, 16.0f}, SDL_Color{120, 132, 170, 255}, true);

                Entity player = AddBlock(scene, "Player", {-600.0f, -200.0f}, {44.0f, 60.0f}, SDL_Color{255, 196, 92, 255}, false);
                auto &controller = player.AddComponent<CharacterControllerComponent>();
                controller.size = {44.0f, 60.0f};
                controller.maxJumps = 2;
                player.AddComponent<ScriptComponent>().BindLua("assets://scripts/player.lua");

                Entity hint = scene.CreateEntity("Hint");
                hint.GetComponent<TransformComponent>().position = {0.0f, 380.0f};
                auto &text = hint.AddComponent<TextComponent>();
                text.Text = "A/D to move, Space to jump (twice for a double jump)";
                text.Size = 32.0f;
                text.Color = SDL_Color{200, 208, 230, 255};
            }

            SceneSerializer serializer(&scene);
            return serializer.Serialize(file.string());
        }
    }

    fs::path Project::FindProjectFile(const fs::path &fileOrDirectory)
    {
        std::error_code ec;
        if (fs::is_regular_file(fileOrDirectory, ec))
            return fileOrDirectory;
        fs::path candidate = fileOrDirectory / FileName;
        if (fs::is_regular_file(candidate, ec))
            return candidate;
        return {};
    }

    bool Project::Load(const fs::path &fileOrDirectory, ProjectSettings &settings, std::string *error)
    {
        const fs::path file = FindProjectFile(fileOrDirectory);
        if (file.empty())
        {
            SetError(error, "No " + std::string(FileName) + " in " + fileOrDirectory.string());
            return false;
        }

        try
        {
            YAML::Node root = YAML::LoadFile(file.string());
            YAML::Node node = root["Project"];
            if (!node || !node.IsMap())
            {
                SetError(error, file.string() + " has no Project section");
                return false;
            }

            ProjectSettings loaded;
            if (node["name"])
                loaded.Name = node["name"].as<std::string>();
            if (node["version"])
                loaded.Version = node["version"].as<std::string>();
            if (node["engine"])
                loaded.EngineVersion = node["engine"].as<std::string>();
            if (node["start_scene"])
                loaded.StartScene = node["start_scene"].as<std::string>();
            if (YAML::Node window = node["window"]; window && window.IsMap())
            {
                if (window["title"])
                    loaded.WindowTitle = window["title"].as<std::string>();
                if (window["width"])
                    loaded.WindowWidth = std::max(160, window["width"].as<int>());
                if (window["height"])
                    loaded.WindowHeight = std::max(120, window["height"].as<int>());
                if (window["fullscreen"])
                    loaded.Fullscreen = window["fullscreen"].as<bool>();
                if (window["resizable"])
                    loaded.Resizable = window["resizable"].as<bool>();
                if (window["vsync"])
                    loaded.VSync = window["vsync"].as<bool>();
                if (window["maximized"])
                    loaded.Maximized = window["maximized"].as<bool>();
                if (window["borderless"])
                    loaded.Borderless = window["borderless"].as<bool>();
                if (window["orientation"])
                    loaded.Orientation = window["orientation"].as<std::string>();
            }
            if (YAML::Node view = node["display"]; view && view.IsMap())
            {
                if (view["width"])
                    loaded.ViewWidth = std::clamp(view["width"].as<int>(), 64, 16384);
                if (view["height"])
                    loaded.ViewHeight = std::clamp(view["height"].as<int>(), 64, 16384);
                const std::string aspect = view["aspect"] ? view["aspect"].as<std::string>() : std::string("keep_height");
                loaded.Aspect = aspect == "keep_width" ? 1 : aspect == "expand" ? 2 : aspect == "keep" ? 3 : 0;
                const std::string stretch = view["stretch"] ? view["stretch"].as<std::string>() : std::string("window");
                loaded.Stretch = stretch == "fixed" ? 1 : 0;
                if (view["render_scale"])
                    loaded.RenderScale = std::clamp(view["render_scale"].as<float>(), 0.2f, 1.0f);
            }
            settings = loaded;
            return true;
        }
        catch (const std::exception &e)
        {
            SetError(error, file.string() + ": " + e.what());
            return false;
        }
    }

    bool Project::Save(const fs::path &directory, const ProjectSettings &settings, std::string *error)
    {
        YAML::Emitter out;
        out << YAML::BeginMap << YAML::Key << "Project" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << settings.Name;
        out << YAML::Key << "version" << YAML::Value << settings.Version;
        out << YAML::Key << "engine" << YAML::Value << (settings.EngineVersion.empty() ? EngineVersion : settings.EngineVersion);
        out << YAML::Key << "start_scene" << YAML::Value << settings.StartScene;
        out << YAML::Key << "window" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "title" << YAML::Value << settings.WindowTitle;
        out << YAML::Key << "width" << YAML::Value << settings.WindowWidth;
        out << YAML::Key << "height" << YAML::Value << settings.WindowHeight;
        out << YAML::Key << "fullscreen" << YAML::Value << settings.Fullscreen;
        out << YAML::Key << "resizable" << YAML::Value << settings.Resizable;
        out << YAML::Key << "vsync" << YAML::Value << settings.VSync;
        out << YAML::Key << "maximized" << YAML::Value << settings.Maximized;
        out << YAML::Key << "borderless" << YAML::Value << settings.Borderless;
        out << YAML::Key << "orientation" << YAML::Value << settings.Orientation;
        out << YAML::EndMap;
        out << YAML::Key << "display" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "width" << YAML::Value << settings.ViewWidth;
        out << YAML::Key << "height" << YAML::Value << settings.ViewHeight;
        out << YAML::Key << "aspect" << YAML::Value
            << (settings.Aspect == 1 ? "keep_width" : settings.Aspect == 2 ? "expand" : settings.Aspect == 3 ? "keep" : "keep_height");
        out << YAML::Key << "stretch" << YAML::Value << (settings.Stretch == 1 ? "fixed" : "window");
        out << YAML::Key << "render_scale" << YAML::Value << settings.RenderScale;
        out << YAML::EndMap;
        out << YAML::EndMap << YAML::EndMap;

        if (!WriteText(directory / FileName, std::string(out.c_str()) + "\n"))
        {
            SetError(error, "Could not write " + (directory / FileName).string());
            return false;
        }
        return true;
    }

    bool Project::Create(const fs::path &directory, const ProjectSettings &settings, ProjectTemplate kind, std::string *error)
    {
        std::error_code ec;
        if (fs::exists(directory / FileName, ec))
        {
            SetError(error, "A project already exists in " + directory.string());
            return false;
        }
        if (fs::exists(directory, ec) && !fs::is_empty(directory, ec))
        {
            SetError(error, directory.string() + " is not empty");
            return false;
        }

        for (const char *sub : {"assets/scenes", "assets/scripts", "assets/textures", "assets/audio"})
        {
            fs::create_directories(directory / sub, ec);
            if (ec)
            {
                SetError(error, "Could not create " + (directory / sub).string() + ": " + ec.message());
                return false;
            }
        }

        ProjectSettings created = settings;
        created.EngineVersion = EngineVersion;
        created.StartScene = "assets://scenes/Main.yaml";
        if (!Save(directory, created, error))
            return false;

        if (kind == ProjectTemplate::Platformer && !WriteText(directory / "assets/scripts/player.lua", PlayerScript))
        {
            SetError(error, "Could not write the player script");
            return false;
        }

        if (!WriteStartScene(directory / "assets/scenes/Main.yaml", created, kind))
        {
            SetError(error, "Could not write the start scene");
            return false;
        }

        WriteText(directory / ".gitignore", "builds/\n");
        return true;
    }

    bool Project::Open(const fs::path &fileOrDirectory, std::string *error)
    {
        ProjectSettings settings;
        if (!Load(fileOrDirectory, settings, error))
            return false;

        const fs::path directory = fs::weakly_canonical(FindProjectFile(fileOrDirectory)).parent_path();
        std::error_code ec;
        fs::create_directories(directory / "assets", ec);

        s_Settings = settings;
        s_Directory = directory;
        s_Open = true;
        VFS::Mount("assets", (directory / "assets").string());
        InputActions::LoadProject(directory.string());
        ApplyDisplay();
        CORE_INFO("Opened project '{}' at {}", settings.Name, directory.string());
        return true;
    }

    bool Project::SaveActive(std::string *error)
    {
        if (!s_Open)
        {
            SetError(error, "No project is open");
            return false;
        }
        return Save(s_Directory, s_Settings, error);
    }

    void Project::Close()
    {
        s_Open = false;
        s_Settings = {};
        s_Directory.clear();
        InputActions::DisableUserBindings();
        InputActions::CloseProject();
        Display::Reset();
    }

    void Project::ApplyDisplay()
    {
        DisplaySettings display;
        display.Width = s_Settings.ViewWidth;
        display.Height = s_Settings.ViewHeight;
        display.Aspect = static_cast<AspectMode>(std::clamp(s_Settings.Aspect, 0, 3));
        display.Stretch = s_Settings.Stretch == 1 ? StretchMode::Fixed : StretchMode::Window;
        display.RenderScale = s_Settings.RenderScale;
        Display::Set(display);
    }

    fs::path Project::EngineResourceDirectory()
    {
        std::error_code ec;
        const char *base = SDL_GetBasePath();
        if (!base)
            return {};

        const fs::path baseDir = fs::path(base).lexically_normal();
        const fs::path packaged = baseDir / "engine";
        if (fs::is_regular_file(packaged / "fonts/Inter-Regular.ttf", ec))
            return packaged;

        fs::path ancestor = baseDir;
        for (int depth = 0; depth < 8; ++depth)
        {
            for (const char *name : {"engine", "assets"})
            {
                const fs::path candidate = ancestor / name;
                if (fs::is_regular_file(candidate / "fonts/Inter-Regular.ttf", ec))
                    return candidate;
            }
            const fs::path parent = ancestor.parent_path();
            if (parent.empty() || parent == ancestor)
                break;
            ancestor = parent;
        }
        return {};
    }

    bool Project::MountEngineResources()
    {
        if (VFS::Mounts().count("engine"))
            return true;
        const fs::path directory = EngineResourceDirectory();
        if (directory.empty())
        {
            CORE_WARN("Engine resources (fonts) not found next to the executable");
            return false;
        }
        VFS::Mount("engine", directory.string());
        return true;
    }
}

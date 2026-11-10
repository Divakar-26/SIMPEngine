#pragma once

#include <filesystem>
#include <string>

namespace SIMPEngine
{
    struct ProjectSettings
    {
        std::string Name = "New Game";
        std::string Version = "0.1.0";
        std::string EngineVersion;
        std::string StartScene = "assets://scenes/Main.yaml";
        std::string WindowTitle;
        int WindowWidth = 1280;
        int WindowHeight = 720;
        bool Fullscreen = false;
        bool Resizable = true;
        bool VSync = true;

        const std::string &Title() const { return WindowTitle.empty() ? Name : WindowTitle; }
    };

    enum class ProjectTemplate
    {
        Empty,
        Platformer
    };

    class Project
    {
    public:
        static constexpr const char *FileName = "project.simp";

        static std::filesystem::path FindProjectFile(const std::filesystem::path &fileOrDirectory);
        static bool Load(const std::filesystem::path &fileOrDirectory, ProjectSettings &settings, std::string *error = nullptr);
        static bool Save(const std::filesystem::path &directory, const ProjectSettings &settings, std::string *error = nullptr);
        static bool Create(const std::filesystem::path &directory, const ProjectSettings &settings, ProjectTemplate kind, std::string *error = nullptr);

        static bool Open(const std::filesystem::path &fileOrDirectory, std::string *error = nullptr);
        static bool SaveActive(std::string *error = nullptr);
        static void Close();
        static bool IsOpen() { return s_Open; }
        static ProjectSettings &Settings() { return s_Settings; }
        static const std::filesystem::path &Directory() { return s_Directory; }
        static std::filesystem::path AssetDirectory() { return s_Directory / "assets"; }

        static bool MountEngineResources();
        static std::filesystem::path EngineResourceDirectory();

    private:
        static bool s_Open;
        static ProjectSettings s_Settings;
        static std::filesystem::path s_Directory;
    };
}

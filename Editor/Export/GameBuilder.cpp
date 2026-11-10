#include "GameBuilder.h"

#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>

#include <cctype>
#include <cstdio>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
    constexpr const char *BuildMarker = ".simpbuild";

    bool IsInside(const fs::path &child, const fs::path &parent)
    {
        auto c = child.begin();
        for (auto p = parent.begin(); p != parent.end(); ++p, ++c)
        {
            if (c == child.end() || *c != *p)
                return false;
        }
        return true;
    }

    std::string FormatSize(std::uintmax_t bytes)
    {
        char text[32];
        if (bytes >= 1024 * 1024)
            std::snprintf(text, sizeof(text), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
        else
            std::snprintf(text, sizeof(text), "%.0f KB", static_cast<double>(bytes) / 1024.0);
        return text;
    }
}

GameBuilder::~GameBuilder()
{
    if (m_Thread.joinable())
        m_Thread.join();
}

fs::path GameBuilder::PlayerBinary()
{
#ifdef _WIN32
    const char *name = "SIMPPlayer.exe";
#else
    const char *name = "SIMPPlayer";
#endif
    const char *base = SDL_GetBasePath();
    return base ? fs::path(base) / name : fs::path(name);
}

std::string GameBuilder::PlatformName()
{
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Linux";
#endif
}

std::string GameBuilder::ExecutableName(const std::string &projectName)
{
    std::string name;
    for (char c : projectName)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) || c == '-' || c == '_')
            name += c;
    }
    if (name.empty())
        name = "Game";
#ifdef _WIN32
    name += ".exe";
#endif
    return name;
}

bool GameBuilder::Start(const Options &options)
{
    if (m_State.load() == State::Running)
        return false;
    if (m_Thread.joinable())
        m_Thread.join();

    m_Options = options;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Log.clear();
        m_Executable.clear();
    }
    m_Progress = 0.0f;
    m_State = State::Running;
    m_Thread = std::thread([this] { Run(); });
    return true;
}

std::vector<std::string> GameBuilder::GetLog() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Log;
}

fs::path GameBuilder::GetExecutable() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Executable;
}

void GameBuilder::Log(const std::string &line)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Log.push_back(line);
}

void GameBuilder::Fail(const std::string &line)
{
    Log("Error: " + line);
    CORE_ERROR("Build failed: {}", line);
    m_State = State::Failed;
}

void GameBuilder::Run()
{
    const SIMPEngine::ProjectSettings &settings = m_Options.Settings;
    try
    {
        std::error_code ec;
        const fs::path project = fs::weakly_canonical(m_Options.ProjectDirectory);
        const fs::path assets = project / "assets";
        const fs::path out = fs::weakly_canonical(fs::absolute(m_Options.OutputDirectory));
        Log("Building '" + settings.Name + "' " + settings.Version + " for " + PlatformName());

        const fs::path player = PlayerBinary();
        if (!fs::is_regular_file(player, ec))
        {
            Fail("SIMPPlayer was not found next to the editor (" + player.string() + "). Build the SIMPPlayer target.");
            return;
        }

        const std::string prefix = "assets://";
        if (settings.StartScene.rfind(prefix, 0) != 0 || !fs::is_regular_file(assets / settings.StartScene.substr(prefix.size()), ec))
        {
            Fail("Start scene '" + settings.StartScene + "' does not exist. Pick one in Project Settings.");
            return;
        }

        if (out.empty() || IsInside(project, out) || IsInside(out, assets))
        {
            Fail("The output folder must not contain the project or be inside its assets folder.");
            return;
        }

        if (fs::exists(out, ec) && !fs::is_empty(out, ec))
        {
            if (!fs::exists(out / BuildMarker, ec))
            {
                Fail(out.string() + " is not empty and is not a previous build. Pick an empty folder.");
                return;
            }
            for (const fs::directory_entry &entry : fs::directory_iterator(out))
                fs::remove_all(entry.path());
            Log("Cleaned the previous build");
        }
        fs::create_directories(out);
        if (std::FILE *marker = std::fopen((out / BuildMarker).string().c_str(), "wb"))
            std::fclose(marker);

        const fs::path executable = out / ExecutableName(settings.Name);
        fs::copy_file(player, executable, fs::copy_options::overwrite_existing);
        fs::permissions(executable, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add);
        Log("Executable: " + executable.filename().string());
        m_Progress = 0.05f;

        const fs::path base = player.parent_path();
#ifdef _WIN32
        for (const fs::directory_entry &entry : fs::directory_iterator(base))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".dll")
            {
                fs::copy_file(entry.path(), out / entry.path().filename(), fs::copy_options::overwrite_existing);
                Log("Library: " + entry.path().filename().string());
            }
        }
#else
        if (fs::is_directory(base / "lib", ec))
        {
            fs::create_directories(out / "lib");
            for (const fs::directory_entry &entry : fs::directory_iterator(base / "lib"))
            {
                if (!entry.is_regular_file())
                    continue;
                fs::copy_file(entry.path(), out / "lib" / entry.path().filename(), fs::copy_options::overwrite_existing);
                Log("Library: " + entry.path().filename().string());
            }
        }
#endif
        m_Progress = 0.15f;

        std::vector<fs::path> files;
        std::uintmax_t total = 0;
        for (auto it = fs::recursive_directory_iterator(assets); it != fs::recursive_directory_iterator(); ++it)
        {
            const std::string name = it->path().filename().string();
            if (!name.empty() && name[0] == '.')
            {
                if (it->is_directory())
                    it.disable_recursion_pending();
                continue;
            }
            if (it->is_regular_file())
            {
                files.push_back(it->path());
                total += it->file_size();
            }
        }

        std::uintmax_t copied = 0;
        for (const fs::path &file : files)
        {
            const fs::path target = out / "assets" / fs::relative(file, assets);
            fs::create_directories(target.parent_path());
            fs::copy_file(file, target, fs::copy_options::overwrite_existing);
            copied += fs::file_size(file);
            const double done = total > 0 ? static_cast<double>(copied) / static_cast<double>(total) : 1.0;
            m_Progress = 0.15f + 0.75f * static_cast<float>(done);
        }
        Log("Assets: " + std::to_string(files.size()) + " files, " + FormatSize(total));

        const fs::path fonts = m_Options.EngineResources / "fonts";
        fs::create_directories(out / "engine/fonts");
        for (const char *font : {"Inter-Regular.ttf", "Inter-SemiBold.ttf", "LICENSE-inter.txt"})
        {
            if (fs::is_regular_file(fonts / font, ec))
                fs::copy_file(fonts / font, out / "engine/fonts" / font, fs::copy_options::overwrite_existing);
            else
                Log("Warning: engine font " + std::string(font) + " is missing");
        }
        m_Progress = 0.95f;

        std::string error;
        if (!SIMPEngine::Project::Save(out, settings, &error))
        {
            Fail(error);
            return;
        }

        std::uintmax_t size = 0;
        for (const fs::directory_entry &entry : fs::recursive_directory_iterator(out))
            if (entry.is_regular_file())
                size += entry.file_size();

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Executable = executable;
        }
        Log("Done: " + out.string() + " (" + FormatSize(size) + ")");
        CORE_INFO("Built '{}' into {}", settings.Name, out.string());
        m_Progress = 1.0f;
        m_State = State::Succeeded;
    }
    catch (const std::exception &e)
    {
        Fail(e.what());
    }
}

#include "EditorRecovery.h"
#include "ProjectHub/RecentProjects.h"

#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <Engine/Project/Project.h>

#include <chrono>
#include <cstdint>
#include <cstdio>

namespace fs = std::filesystem;

namespace
{
    constexpr const char *Header = "# SIMPRecovery ";

    std::string Key(const std::string &scenePath)
    {
        std::string text = SIMPEngine::Project::IsOpen() ? SIMPEngine::Project::Directory().string() : std::string();
        text += '|';
        text += scenePath.empty() ? std::string("untitled") : scenePath;

        uint64_t hash = 1469598103934665603ull;
        for (unsigned char c : text)
        {
            hash ^= c;
            hash *= 1099511628211ull;
        }
        char name[32];
        std::snprintf(name, sizeof(name), "%016llx.yaml", static_cast<unsigned long long>(hash));
        return name;
    }

    fs::path FileFor(const std::string &scenePath)
    {
        return EditorRecovery::Directory() / Key(scenePath);
    }
}

fs::path EditorRecovery::Directory()
{
    return RecentProjects::PreferenceDirectory() / "recovery";
}

bool EditorRecovery::Write(const std::string &scenePath, const std::string &yaml)
{
    if (yaml.empty())
        return false;
    return SIMPEngine::FileSystem::WriteText(FileFor(scenePath).string(), Header + scenePath + "\n" + yaml);
}

std::optional<RecoveryFile> EditorRecovery::Find(const std::string &scenePath)
{
    const fs::path file = FileFor(scenePath);
    std::error_code ec;
    if (!fs::exists(file, ec))
        return std::nullopt;

    const auto recoveryTime = fs::last_write_time(file, ec);
    if (ec)
        return std::nullopt;

    if (!scenePath.empty())
    {
        if (auto real = SIMPEngine::VFS::Resolve(scenePath))
        {
            std::error_code sceneError;
            const auto sceneTime = fs::last_write_time(*real, sceneError);
            if (!sceneError && sceneTime >= recoveryTime)
            {
                fs::remove(file, ec);
                return std::nullopt;
            }
        }
    }

    std::optional<std::string> text = SIMPEngine::FileSystem::ReadText(file.string());
    if (!text || text->rfind(Header, 0) != 0)
        return std::nullopt;

    RecoveryFile result;
    result.Path = file;
    const size_t lineEnd = text->find('\n');
    result.ScenePath = text->substr(std::char_traits<char>::length(Header), lineEnd == std::string::npos ? std::string::npos : lineEnd - std::char_traits<char>::length(Header));
    result.Yaml = lineEnd == std::string::npos ? std::string() : text->substr(lineEnd + 1);
    const auto age = fs::file_time_type::clock::now() - recoveryTime;
    result.AgeSeconds = std::chrono::duration_cast<std::chrono::seconds>(age).count();
    if (result.Yaml.empty())
        return std::nullopt;
    return result;
}

void EditorRecovery::Discard(const std::string &scenePath)
{
    std::error_code ec;
    fs::remove(FileFor(scenePath), ec);
}

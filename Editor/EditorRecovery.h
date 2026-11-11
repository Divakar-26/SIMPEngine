#pragma once

#include <filesystem>
#include <optional>
#include <string>

struct RecoveryFile
{
    std::filesystem::path Path;
    std::string ScenePath;
    std::string Yaml;
    long long AgeSeconds = 0;
};

class EditorRecovery
{
public:
    static std::filesystem::path Directory();
    static bool Write(const std::string &scenePath, const std::string &yaml);
    static std::optional<RecoveryFile> Find(const std::string &scenePath);
    static void Discard(const std::string &scenePath);
};

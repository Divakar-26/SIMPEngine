#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct RecentProject
{
    std::string Path;
    std::string Name;
    std::string EngineVersion;
    int64_t LastOpened = 0;
};

class RecentProjects
{
public:
    void Load();
    void Save() const;
    void Touch(const std::filesystem::path &directory, const std::string &name, const std::string &engineVersion);
    void Remove(const std::string &path);

    const std::vector<RecentProject> &Entries() const { return m_Entries; }

    static std::filesystem::path PreferenceDirectory();

private:
    std::vector<RecentProject> m_Entries;
};

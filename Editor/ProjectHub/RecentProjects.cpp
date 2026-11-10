#include "RecentProjects.h"

#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

namespace
{
    int64_t Now()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

fs::path RecentProjects::PreferenceDirectory()
{
    char *pref = SDL_GetPrefPath("SIMPEngine", "Editor");
    if (!pref)
        return fs::current_path();
    fs::path directory(pref);
    SDL_free(pref);
    return directory;
}

void RecentProjects::Load()
{
    m_Entries.clear();
    const fs::path file = PreferenceDirectory() / "projects.yaml";
    std::error_code ec;
    if (!fs::exists(file, ec))
        return;
    try
    {
        YAML::Node root = YAML::LoadFile(file.string());
        for (const YAML::Node &node : root["Projects"])
        {
            RecentProject entry;
            entry.Path = node["path"].as<std::string>("");
            entry.Name = node["name"].as<std::string>("");
            entry.EngineVersion = node["engine"].as<std::string>("");
            entry.LastOpened = node["opened"].as<int64_t>(0);
            if (!entry.Path.empty())
                m_Entries.push_back(entry);
        }
    }
    catch (const std::exception &e)
    {
        CORE_WARN("Could not read the recent project list: {}", e.what());
    }
}

void RecentProjects::Save() const
{
    YAML::Emitter out;
    out << YAML::BeginMap << YAML::Key << "Projects" << YAML::Value << YAML::BeginSeq;
    for (const RecentProject &entry : m_Entries)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "path" << YAML::Value << entry.Path;
        out << YAML::Key << "name" << YAML::Value << entry.Name;
        out << YAML::Key << "engine" << YAML::Value << entry.EngineVersion;
        out << YAML::Key << "opened" << YAML::Value << entry.LastOpened;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq << YAML::EndMap;

    std::ofstream file(PreferenceDirectory() / "projects.yaml");
    if (file)
        file << out.c_str() << "\n";
}

void RecentProjects::Touch(const fs::path &directory, const std::string &name, const std::string &engineVersion)
{
    const std::string path = fs::weakly_canonical(directory).string();
    Remove(path);
    m_Entries.insert(m_Entries.begin(), RecentProject{path, name, engineVersion, Now()});
    Save();
}

void RecentProjects::Remove(const std::string &path)
{
    m_Entries.erase(std::remove_if(m_Entries.begin(), m_Entries.end(), [&](const RecentProject &e) { return e.Path == path; }), m_Entries.end());
}

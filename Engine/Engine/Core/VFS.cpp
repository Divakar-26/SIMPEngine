#include <Engine/Core/VFS.h>
#include <Engine/Core/FileSystem.h>
#include <filesystem>
#include <system_error>
#include <SDL3/SDL.h>

#include "Log.h"

using namespace std;
namespace SIMPEngine
{
    unordered_map<string, string> VFS::s_Mounts;
    void VFS::Mount(const std::string &alias, const std::string &realPath)
    {
        namespace fs = std::filesystem;
        fs::path requested(realPath);
        fs::path resolved;

        if (requested.is_absolute())
        {
            resolved = requested;
        }
        else
        {
            const char *basePath = SDL_GetBasePath();
            if (basePath)
            {
                fs::path ancestor = fs::path(basePath).lexically_normal();

                for (int depth = 0; depth < 8; ++depth)
                {
                    fs::path candidate = (ancestor / requested).lexically_normal();
                    std::error_code ec;
                    bool emptyDir = fs::is_directory(candidate, ec) && fs::is_empty(candidate, ec);
                    if (fs::exists(candidate, ec) && !ec && !emptyDir)
                    {
                        resolved = candidate;
                        break;
                    }

                    const fs::path parent = ancestor.parent_path();
                    if (parent.empty() || parent == ancestor)
                        break;
                    ancestor = parent;
                }
            }

            if (resolved.empty())
                resolved = fs::absolute(requested);
        }

        s_Mounts[alias] = fs::weakly_canonical(resolved).string();

        CORE_INFO("Mounted {} -> {}", alias, s_Mounts[alias]);
    }

    bool VFS::UnMount(const string &alias)
    {
        return s_Mounts.erase(alias) > 0;
    }

    optional<string> VFS::Resolve(const string &vp)
    {
        auto pos = vp.find("://");
        if (pos == string::npos)
            return vp;

        string alias = vp.substr(0, pos);
        string rel = vp.substr(pos + 3);

        auto it = s_Mounts.find(alias);
        if (it == s_Mounts.end())
            return std::nullopt;

        return FileSystem::Normalise(FileSystem::Join(it->second, rel));
    }

    const unordered_map<string, string> &VFS::Mounts() { return s_Mounts; }
}

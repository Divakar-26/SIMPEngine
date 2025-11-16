#pragma once

#include <string>
#include <vector>
#include <optional>

namespace SIMPEngine
{
    struct FileEntry
    {
        std::string path;
        bool isDirectory = false;
    };

    class FileSystem
    {
    public:
        static bool Exists(const std::string & path);
        static bool IsDirectory(const std::string & path);
        static bool CreateDirectories(const std::string & path);
        static bool Remove(const std::string & path);
        static bool Rename(const std::string & from, const std::string &to);

        static std::optional<std::string> ReadText(const std::string & path);
        static bool WriteText(const std::string & path, const std::string &text, bool createDirs=true);

        static std::vector<FileEntry> List(const std::string & dir);
        static std::string Join(const std::string & a, const std::string & b);
        static std::string Normalise(const std::string & p);

        
    private:
    };

}

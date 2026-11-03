#include <Engine/Core/FileSystem.h>

#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

namespace
{
    fs::path ToPath(const std::string &utf8)
    {
        return fs::path(std::u8string(reinterpret_cast<const char8_t *>(utf8.data()), utf8.size()));
    }
}

namespace SIMPEngine
{
    bool FileSystem::Exists(const string &path)
    {
        return fs::exists(ToPath(path));
    }

    bool FileSystem::IsDirectory(const string &path)
    {
        return fs::is_directory(ToPath(path));
    }

    bool FileSystem::CreateDirectories(const string &path)
    {
        std::error_code ec;
        fs::create_directories(ToPath(path), ec);
        return !ec;
    }

    bool FileSystem::Remove(const string &path)
    {
        std::error_code ec;
        fs::remove_all(ToPath(path), ec);
        return !ec;
    }

    bool FileSystem::Rename(const string &from, const string &to)
    {
        std::error_code ec;
        fs::rename(ToPath(from), ToPath(to), ec);
        return !ec;
    }

    optional<string> FileSystem::ReadText(const string &path)
    {
        if (!Exists(path))
            return std::nullopt;
        ifstream f(ToPath(path), ios::binary);
        return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    }

    bool FileSystem::WriteText(const string &path, const string &text, bool createDirs)
    {
        if (createDirs)
        {
            CreateDirectories(ToPath(path).parent_path().string());
        }
        ofstream f(ToPath(path), ios::binary);
        if (!f)
            return false;
        f << text;
        return true;
    }

    vector<FileEntry> FileSystem::List(const string &dir)
    {
        vector<FileEntry> out;
        if (!IsDirectory(dir))
            return out;

        for (auto &e : fs::directory_iterator(ToPath(dir)))
        {
            out.push_back({e.path().string(), e.is_directory()});
        }

        return out;
    }

    string FileSystem::Join(const string &a, const string &b)
    {
        return (ToPath(a) / ToPath(b)).string();
    }

    string FileSystem::Normalise(const string &p)
    {
        std::error_code ec;
        auto np = fs::weakly_canonical(ToPath(p), ec);
        return ec ? ToPath(p).lexically_normal().string() : np.string();
    }
}

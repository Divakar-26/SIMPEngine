#include <Engine/Core/FileSystem.h>

#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;
 // aaaa
namespace SIMPEngine
{
    bool FileSystem::Exists(const string &path)
    {
        return fs::exists(fs::u8path(path));
    }

    bool FileSystem::IsDirectory(const string &path)
    {
        return fs::is_directory(fs::u8path(path));
    }

    bool FileSystem::CreateDirectories(const string &path)
    {
        std::error_code ec;
        fs::create_directories(fs::u8path(path), ec);
        return !ec;
    }

    bool FileSystem::Remove(const string &path)
    {
        std::error_code ec;
        fs::remove_all(fs::u8path(path), ec);
        return !ec;
    }

    bool FileSystem::Rename(const string &from, const string &to)
    {
        std::error_code ec;
        fs::rename(fs::u8path(from), fs::u8path(to), ec);
        return !ec;
    }

    optional<string> FileSystem::ReadText(const string &path)
    {
        if (!Exists(path))
            return std::nullopt;
        ifstream f(fs::u8path(path), ios::binary);
        return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    }

    bool FileSystem::WriteText(const string &path, const string &text, bool createDirs)
    {
        if (createDirs)
        {
            CreateDirectories(fs::u8path(path).parent_path().string());
        }
        ofstream f(fs::u8path(path), ios::binary);
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

        for (auto &e : fs::directory_iterator(fs::u8path(dir)))
        {
            out.push_back({e.path().string(), e.is_directory()});
        }

        return out;
    }

    string FileSystem::Join(const string &a, const string &b)
    {
        return (fs::u8path(a) / fs::u8path(b)).string();
    }

    string FileSystem::Normalise(const string &p)
    {
        std::error_code ec;
        auto np = fs::weakly_canonical(fs::u8path(p), ec);
        return ec ? fs::u8path(p).lexically_normal().string() : np.string();
    }

}
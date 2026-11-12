#include <Engine/Core/FileSystem.h>

#include <filesystem>
#include <chrono>
#include <fstream>
#include <thread>

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
        std::error_code ec;
        return fs::exists(ToPath(path), ec);
    }

    bool FileSystem::IsDirectory(const string &path)
    {
        std::error_code ec;
        return fs::is_directory(ToPath(path), ec);
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
        if (!f)
            return std::nullopt;
        return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    }

    bool FileSystem::WriteText(const string &path, const string &text, bool createDirs)
    {
        const fs::path target = ToPath(path);
        if (createDirs && target.has_parent_path())
            CreateDirectories(target.parent_path().string());

        fs::path temporary = target;
        temporary += ".saving";
        {
            ofstream f(temporary, ios::binary | ios::trunc);
            if (!f)
                return false;
            f.write(text.data(), static_cast<std::streamsize>(text.size()));
            f.flush();
            if (!f)
            {
                f.close();
                std::error_code ignored;
                fs::remove(temporary, ignored);
                return false;
            }
        }

        std::error_code ec;
        for (int attempt = 0; attempt < 5; ++attempt)
        {
            fs::rename(temporary, target, ec);
            if (!ec)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        fs::remove(temporary, ec);
        return false;
    }

    vector<FileEntry> FileSystem::List(const string &dir)
    {
        vector<FileEntry> out;
        if (!IsDirectory(dir))
            return out;

        std::error_code ec;
        for (fs::directory_iterator it(ToPath(dir), fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
        {
            std::error_code typeError;
            out.push_back({it->path().string(), it->is_directory(typeError)});
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

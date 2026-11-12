#include <Engine/Rendering/ImageWriter.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

#include <stb_image_write.h>

namespace SIMPEngine::ImageWriter
{
    namespace
    {
        void Append(void *context, void *data, int size)
        {
            if (size > 0)
                static_cast<std::string *>(context)->append(static_cast<const char *>(data), static_cast<size_t>(size));
        }
    }

    bool EncodePng(int width, int height, const uint8_t *rgba, std::string &out)
    {
        out.clear();
        if (!rgba || width <= 0 || height <= 0)
            return false;
        return stbi_write_png_to_func(Append, &out, width, height, 4, rgba, width * 4) != 0 && !out.empty();
    }

    bool WritePng(const std::string &path, int width, int height, const std::vector<uint8_t> &rgba)
    {
        if (rgba.size() < static_cast<size_t>(width) * static_cast<size_t>(height) * 4)
        {
            CORE_ERROR("ImageWriter: {} has {} bytes for a {}x{} image", path, rgba.size(), width, height);
            return false;
        }
        std::string encoded;
        if (!EncodePng(width, height, rgba.data(), encoded))
        {
            CORE_ERROR("ImageWriter: could not encode {}", path);
            return false;
        }
        std::string real = path;
        if (auto resolved = VFS::Resolve(path))
            real = *resolved;
        if (!FileSystem::WriteText(real, encoded))
        {
            CORE_ERROR("ImageWriter: could not write {}", real);
            return false;
        }
        return true;
    }
}

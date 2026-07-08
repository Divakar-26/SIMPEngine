#include <Engine/Rendering/SVGDocument.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

#include <fstream>
#include <sstream>
#include <vector>

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION

#include <nanosvg.h>
#include <nanosvgrast.h>

namespace SIMPEngine
{
    SvgDocument::~SvgDocument()
    {
        Unload();
    }

    SvgDocument::SvgDocument(SvgDocument &&other) noexcept
    {
        *this = std::move(other);
    }

    SvgDocument &SvgDocument::operator=(SvgDocument &&other) noexcept
    {
        if (this == &other)
            return *this;

        Unload();

        m_Path = std::move(other.m_Path);
        m_Source = std::move(other.m_Source);
        m_Image = other.m_Image;

        other.m_Image = nullptr;
        return *this;
    }

    std::vector<char> SvgDocument::MakeMutableBuffer(const std::string &text)
    {
        std::vector<char> buffer(text.begin(), text.end());
        buffer.push_back('\0');
        return buffer;
    }

    bool SvgDocument::LoadFromFile(const std::string &path)
    {
        Unload();
        m_Path = path;

        std::string realPath = path;
        if (auto resolved = VFS::Resolve(path))
            realPath = *resolved;

        std::ifstream file(realPath, std::ios::binary);
        if (!file)
        {
            CORE_ERROR("Failed to open SVG file: {}", realPath);
            return false;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        return LoadFromMemory(ss.str(), path);
    }

    bool SvgDocument::LoadFromMemory(const std::string &svgText, const std::string &debugName)
    {
        Unload();
        m_Path = debugName;
        m_Source = svgText;

        auto buffer = MakeMutableBuffer(m_Source);

        // NanoSVG parses the document; rasterization comes later.
        m_Image = nsvgParse(buffer.data(), "px", 96.0f);
        if (!m_Image)
        {
            CORE_ERROR("Failed to parse SVG: {}", debugName);
            return false;
        }

        return true;
    }

    void SvgDocument::Unload()
    {
        if (m_Image)
        {
            nsvgDelete(m_Image);
            m_Image = nullptr;
        }

        m_Source.clear();
        m_Path.clear();
    }

    float SvgDocument::GetWidth() const
    {
        return m_Image ? m_Image->width : 0.0f;
    }

    float SvgDocument::GetHeight() const
    {
        return m_Image ? m_Image->height : 0.0f;
    }

    std::shared_ptr<Texture> SvgDocument::RasterizeToTexture(
        int width,
        int height,
        SDL_Color tint) const
    {
        if (!m_Image)
            return nullptr;

        NSVGrasterizer *rast = nsvgCreateRasterizer();

        std::vector<unsigned char> pixels(
            width * height * 4);

        float scaleX = width / m_Image->width;
        float scaleY = height / m_Image->height;

        float scale = std::min(scaleX, scaleY);

        nsvgRasterize(
            rast,
            m_Image,
            0.0f,
            0.0f,
            scale,
            pixels.data(),
            width,
            height,
            width * 4);

        nsvgDeleteRasterizer(rast);

        auto tex = std::make_shared<Texture>();

        tex->UploadToGPU(
            pixels.data(),
            width,
            height,
            4);

        return tex;
    }
}
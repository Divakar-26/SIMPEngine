#pragma once

#include <Engine/Rendering/Texture.h>
#include <SDL3/SDL_pixels.h>

#include <memory>
#include <string>
#include <vector>

struct NSVGimage;

namespace SIMPEngine
{
    class SvgDocument
    {
    public:
        SvgDocument() = default;
        ~SvgDocument();

        SvgDocument(const SvgDocument&) = delete;
        SvgDocument& operator=(const SvgDocument&) = delete;

        SvgDocument(SvgDocument&& other) noexcept;
        SvgDocument& operator=(SvgDocument&& other) noexcept;

        bool LoadFromFile(const std::string& path);
        bool LoadFromMemory(const std::string& svgText, const std::string& debugName = "memory.svg");

        void Unload();

        bool IsLoaded() const { return m_Image != nullptr; }
        const std::string& GetPath() const { return m_Path; }

        float GetWidth() const;
        float GetHeight() const;

        const NSVGimage* GetImage() const { return m_Image; }

        // Later: implement actual rasterization here.
        // For now this is the API hook you asked for.
        std::shared_ptr<Texture> RasterizeToTexture(
            int width,
            int height,
            SDL_Color tint = SDL_Color{255, 255, 255, 255}) const;

    private:
        std::string m_Path;
        std::string m_Source;
        NSVGimage* m_Image = nullptr;

        static std::vector<char> MakeMutableBuffer(const std::string& text);
    };
}
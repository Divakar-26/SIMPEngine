#pragma once
#include <memory>

namespace SIMPEngine
{
    class Texture;
    class SvgDocument;

    class SvgTextureCache
    {
    public:
        static std::shared_ptr<Texture> GetOrCreate(SvgDocument &doc, int width, int height);

        static void Clear();
    };
}

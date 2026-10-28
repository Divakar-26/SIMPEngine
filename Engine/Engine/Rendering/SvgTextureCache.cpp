#include <Engine/Rendering/SvgTextureCache.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/SVGDocument.h>

#include <SDL3/SDL.h>
#include <unordered_map>
#include <functional>

namespace SIMPEngine
{
    namespace
    {
        struct SvgCacheKey
        {
            const SvgDocument *doc;
            int width;
            int height;

            bool operator==(const SvgCacheKey &o) const
            {
                return doc == o.doc && width == o.width && height == o.height;
            }
        };

        struct SvgCacheKeyHash
        {
            size_t operator()(const SvgCacheKey &k) const
            {
                size_t h1 = std::hash<const void *>()(k.doc);
                size_t h2 = std::hash<int>()(k.width);
                size_t h3 = std::hash<int>()(k.height);
                return h1 ^ (h2 << 1) ^ (h3 << 2);
            }
        };

        std::unordered_map<SvgCacheKey, std::shared_ptr<Texture>, SvgCacheKeyHash> s_Cache;
    }

    std::shared_ptr<Texture> SvgTextureCache::GetOrCreate(SvgDocument &doc, int width, int height)
    {
        SvgCacheKey key{&doc, width, height};

        auto it = s_Cache.find(key);
        if (it != s_Cache.end())
            return it->second;

        // Tint is left at white here -- Renderer::DrawSvg applies tint per-draw
        // via the normal sprite color multiply, so the same cached texture can
        // be reused across draws with different tints.
        auto tex = doc.RasterizeToTexture(width, height, SDL_Color{255, 255, 255, 255});
        if (tex)
            s_Cache.emplace(key, tex);

        return tex;
    }

    void SvgTextureCache::Clear()
    {
        s_Cache.clear();
    }
}
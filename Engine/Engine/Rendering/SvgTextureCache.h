#pragma once
#include <memory>

namespace SIMPEngine
{
    class Texture;
    class SvgDocument;

    // Rasterizing an SVG (nanosvgRasterize) is comparatively expensive and
    // its output doesn't change frame to frame, so cache the resulting
    // Texture per (document, pixel size) and reuse it. Once cached, drawing
    // an SVG is just DrawTexture underneath -- it batches with everything
    // else in the quad/sprite batch.
    //
    // Cache key is the SvgDocument's address, so keep icons in long-lived
    // SvgDocument instances (e.g. one per icon asset) rather than
    // reconstructing a temporary each frame.
    class SvgTextureCache
    {
    public:
        static std::shared_ptr<Texture> GetOrCreate(SvgDocument &doc, int width, int height);

        // Drops every cached texture. Call if you reload/mutate SvgDocuments
        // in place, or to reclaim GPU memory (e.g. on a scene change).
        static void Clear();
    };
}
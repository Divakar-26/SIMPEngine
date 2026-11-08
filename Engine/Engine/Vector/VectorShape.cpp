#include "VectorShape.h"

#include <Engine/Rendering/Renderer.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine::VectorShapes
{
    namespace
    {
        void Mix(uint64_t &h, const void *data, size_t size)
        {
            const auto *bytes = static_cast<const unsigned char *>(data);
            for (size_t i = 0; i < size; ++i)
            {
                h ^= bytes[i];
                h *= 1099511628211ull;
            }
        }

        template <typename T>
        void MixValue(uint64_t &h, const T &value)
        {
            Mix(h, &value, sizeof(T));
        }

        glm::vec4 ToVec(const SDL_Color &c)
        {
            return glm::vec4(c.r, c.g, c.b, c.a) / 255.0f;
        }

        float Quantize(float v)
        {
            return std::exp2(std::round(std::log2(std::max(v, 1e-6f)) * 4.0f) / 4.0f);
        }

        void StrokeContours(const std::vector<FlatContour> &outline, float trimStart, float trimEnd, float trimOffset, float dash, float gap, float dashOffset,
                            const StrokeStyle &style, const VectorPaint &paint, float aa, VectorMesh &mesh)
        {
            const bool trimmed = trimStart > 0.0f || trimEnd < 1.0f || trimOffset != 0.0f;
            std::vector<FlatContour> pieces;
            for (const FlatContour &contour : outline)
            {
                if (trimmed)
                    TrimPolyline(contour.Points, contour.Closed, trimStart, trimEnd, trimOffset, pieces);
                else
                    pieces.push_back(contour);
            }
            if (dash > 0.0f)
            {
                std::vector<FlatContour> dashed;
                for (const FlatContour &piece : pieces)
                    DashPolyline(piece.Points, piece.Closed, dash, gap, dashOffset, dashed);
                pieces = std::move(dashed);
            }
            for (const FlatContour &piece : pieces)
                TessellateStroke(piece, style, paint, aa, mesh);
        }

        void SubmitTransformed(const VectorMesh &mesh, const glm::vec2 &position, float rotationDegrees, const glm::vec2 &scale, float zIndex, std::vector<glm::vec2> &world)
        {
            if (mesh.Empty())
                return;
            const float r = glm::radians(rotationDegrees);
            const float c = std::cos(r);
            const float s = std::sin(r);
            world.resize(mesh.Positions.size());
            for (size_t i = 0; i < mesh.Positions.size(); ++i)
            {
                const glm::vec2 p = mesh.Positions[i] * scale;
                world[i] = position + glm::vec2(p.x * c - p.y * s, p.x * s + p.y * c);
            }
            Renderer::DrawTriangles(world.data(), mesh.Colors.data(), world.size(), mesh.Indices.data(), mesh.Indices.size(), zIndex);
        }
    }

    float LocalTolerance(const glm::vec2 &scale)
    {
        const float s = std::max({std::abs(scale.x), std::abs(scale.y), 1e-4f});
        return Quantize(0.35f * Renderer::GetWorldPerPixel() / s);
    }

    VectorShapeCache &CacheOf(const VectorShapeComponent &shape)
    {
        auto &handle = const_cast<VectorCacheHandle &>(shape.cache);
        if (!handle.Ptr)
            handle.Ptr = std::make_shared<VectorShapeCache>();
        return *handle.Ptr;
    }

    void Outline(const VectorShapeComponent &shape, float tolerance, std::vector<FlatContour> &out)
    {
        VectorShapeCache &cache = CacheOf(shape);
        uint64_t flatKey = shape.path.Hash();
        MixValue(flatKey, tolerance);
        if (cache.FlatKey != flatKey)
        {
            shape.path.Flatten(tolerance, cache.Flat);
            cache.FlatKey = flatKey;
        }

        if (shape.morph == 0.0f || shape.morphTarget.Empty())
        {
            out = cache.Flat;
            return;
        }

        uint64_t targetKey = shape.morphTarget.Hash();
        MixValue(targetKey, tolerance);
        if (cache.TargetKey != targetKey)
        {
            shape.morphTarget.Flatten(tolerance, cache.TargetFlat);
            cache.TargetKey = targetKey;
        }
        uint64_t pairsKey = flatKey;
        MixValue(pairsKey, targetKey);
        if (cache.PairsKey != pairsKey)
        {
            BuildMorphPairs(cache.Flat, cache.TargetFlat, cache.Pairs);
            cache.PairsKey = pairsKey;
        }
        BlendMorph(cache.Pairs, std::clamp(shape.morph, -1.0f, 2.0f), out);
    }

    const VectorMesh &LocalMesh(const VectorShapeComponent &shape, float tolerance, float aa)
    {
        VectorShapeCache &cache = CacheOf(shape);
        uint64_t key = shape.path.Hash();
        if (shape.morph != 0.0f)
            MixValue(key, shape.morphTarget.Hash());
        MixValue(key, shape.morph);
        MixValue(key, shape.fill);
        MixValue(key, shape.fillColor);
        MixValue(key, shape.stroke);
        MixValue(key, shape.strokeColor);
        MixValue(key, shape.strokeWidth);
        MixValue(key, shape.cap);
        MixValue(key, shape.join);
        MixValue(key, shape.miterLimit);
        MixValue(key, shape.trimStart);
        MixValue(key, shape.trimEnd);
        MixValue(key, shape.trimOffset);
        MixValue(key, shape.dashLength);
        MixValue(key, shape.dashGap);
        MixValue(key, shape.dashOffset);
        MixValue(key, shape.gradient);
        if (shape.gradient)
        {
            MixValue(key, shape.gradientFrom);
            MixValue(key, shape.gradientTo);
            MixValue(key, shape.gradientFromColor);
            MixValue(key, shape.gradientToColor);
        }
        MixValue(key, tolerance);
        MixValue(key, aa);
        if (cache.MeshKey == key)
            return cache.Mesh;

        std::vector<FlatContour> outline;
        Outline(shape, tolerance, outline);
        cache.Mesh.Clear();

        if (shape.fill && shape.fillColor.a > 0)
        {
            VectorPaint paint;
            paint.Color = ToVec(shape.fillColor);
            paint.Gradient = shape.gradient;
            paint.From = shape.gradientFrom;
            paint.To = shape.gradientTo;
            paint.FromColor = ToVec(shape.gradientFromColor);
            paint.ToColor = ToVec(shape.gradientToColor);
            TessellateFill(outline, paint, aa, cache.Mesh);
        }
        if (shape.stroke && shape.strokeWidth > 0.0f && shape.strokeColor.a > 0)
        {
            VectorPaint paint;
            paint.Color = ToVec(shape.strokeColor);
            StrokeStyle style{shape.strokeWidth, shape.cap, shape.join, shape.miterLimit};
            StrokeContours(outline, shape.trimStart, shape.trimEnd, shape.trimOffset, shape.dashLength, shape.dashGap, shape.dashOffset, style, paint, aa, cache.Mesh);
        }

        cache.BoundsMin = cache.BoundsMax = glm::vec2(0.0f);
        for (size_t i = 0; i < cache.Mesh.Positions.size(); ++i)
        {
            cache.BoundsMin = i ? glm::min(cache.BoundsMin, cache.Mesh.Positions[i]) : cache.Mesh.Positions[i];
            cache.BoundsMax = i ? glm::max(cache.BoundsMax, cache.Mesh.Positions[i]) : cache.Mesh.Positions[i];
        }
        cache.MeshKey = key;
        return cache.Mesh;
    }

    void Draw(const VectorShapeComponent &shape, const glm::vec2 &position, float rotationDegrees, const glm::vec2 &scale, float zIndex)
    {
        VectorShapeCache &cache = CacheOf(shape);
        const float s = std::max({std::abs(scale.x), std::abs(scale.y), 1e-4f});
        if (cache.MeshKey != 0)
        {
            const glm::vec2 extent = glm::max(glm::abs(cache.BoundsMin), glm::abs(cache.BoundsMax)) * s;
            if (!Renderer::IsVisible(position, glm::vec2(glm::length(extent) + shape.strokeWidth * s)))
                return;
        }
        const float wpp = Renderer::GetWorldPerPixel();
        const VectorMesh &mesh = LocalMesh(shape, LocalTolerance(scale), Quantize(wpp / s));
        SubmitTransformed(mesh, position, rotationDegrees, scale, zIndex, cache.World);
    }

    void DrawPath(const VectorPath &path, const glm::vec2 &position, float rotationDegrees, const glm::vec2 &scale, float zIndex,
                  bool fill, const glm::vec4 &fillColor, bool stroke, const glm::vec4 &strokeColor, const StrokeStyle &style,
                  float trimStart, float trimEnd, float trimOffset)
    {
        const float s = std::max({std::abs(scale.x), std::abs(scale.y), 1e-4f});
        const float aa = Renderer::GetWorldPerPixel() / s;
        std::vector<FlatContour> outline;
        path.Flatten(LocalTolerance(scale), outline);
        thread_local VectorMesh mesh;
        thread_local std::vector<glm::vec2> world;
        mesh.Clear();
        if (fill && fillColor.a > 0.0f)
        {
            VectorPaint paint;
            paint.Color = fillColor;
            TessellateFill(outline, paint, aa, mesh);
        }
        if (stroke && strokeColor.a > 0.0f && style.Width > 0.0f)
        {
            VectorPaint paint;
            paint.Color = strokeColor;
            StrokeContours(outline, trimStart, trimEnd, trimOffset, 0.0f, 0.0f, 0.0f, style, paint, aa, mesh);
        }
        SubmitTransformed(mesh, position, rotationDegrees, scale, zIndex, world);
    }

    void DrawPolyline(const std::vector<glm::vec2> &points, bool closed, const StrokeStyle &style, const glm::vec4 &color, float zIndex)
    {
        thread_local VectorMesh mesh;
        mesh.Clear();
        VectorPaint paint;
        paint.Color = color;
        TessellateStroke(FlatContour{points, closed}, style, paint, Renderer::GetWorldPerPixel(), mesh);
        Renderer::DrawTriangles(mesh.Positions.data(), mesh.Colors.data(), mesh.Positions.size(), mesh.Indices.data(), mesh.Indices.size(), zIndex);
    }

    void DrawPolygon(const std::vector<glm::vec2> &points, const glm::vec4 &color, float zIndex)
    {
        thread_local VectorMesh mesh;
        mesh.Clear();
        VectorPaint paint;
        paint.Color = color;
        TessellateFill({FlatContour{points, true}}, paint, Renderer::GetWorldPerPixel(), mesh);
        Renderer::DrawTriangles(mesh.Positions.data(), mesh.Colors.data(), mesh.Positions.size(), mesh.Indices.data(), mesh.Indices.size(), zIndex);
    }

    float WorldLength(const VectorShapeComponent &shape, const glm::vec2 &scale)
    {
        std::vector<FlatContour> outline;
        Outline(shape, 0.25f, outline);
        float total = 0.0f;
        for (const FlatContour &contour : outline)
            total += PolylineLength(contour.Points, contour.Closed);
        return total * (std::abs(scale.x) + std::abs(scale.y)) * 0.5f;
    }
}

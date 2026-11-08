#pragma once

#include <Engine/Vector/VectorPath.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace SIMPEngine
{
    enum class StrokeCap : uint8_t
    {
        Butt,
        Round,
        Square
    };

    enum class StrokeJoin : uint8_t
    {
        Miter,
        Round,
        Bevel
    };

    struct VectorPaint
    {
        glm::vec4 Color{1.0f};
        bool Gradient = false;
        glm::vec2 From{0.0f};
        glm::vec2 To{0.0f, 1.0f};
        glm::vec4 FromColor{1.0f};
        glm::vec4 ToColor{1.0f};

        glm::vec4 At(const glm::vec2 &p) const;
    };

    struct VectorMesh
    {
        std::vector<glm::vec2> Positions;
        std::vector<glm::vec4> Colors;
        std::vector<uint32_t> Indices;

        void Clear();
        bool Empty() const { return Indices.empty(); }
    };

    struct StrokeStyle
    {
        float Width = 4.0f;
        StrokeCap Cap = StrokeCap::Round;
        StrokeJoin Join = StrokeJoin::Round;
        float MiterLimit = 4.0f;
    };

    void TessellateFill(const std::vector<FlatContour> &contours, const VectorPaint &paint, float aa, VectorMesh &out);
    void TessellateStroke(const FlatContour &line, const StrokeStyle &style, const VectorPaint &paint, float aa, VectorMesh &out);
}

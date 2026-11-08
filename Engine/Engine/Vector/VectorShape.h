#pragma once

#include <Engine/Scene/Component.h>
#include <Engine/Vector/VectorPath.h>
#include <Engine/Vector/VectorTessellator.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace SIMPEngine
{
    struct VectorShapeCache
    {
        uint64_t FlatKey = 0;
        std::vector<FlatContour> Flat;
        uint64_t TargetKey = 0;
        std::vector<FlatContour> TargetFlat;
        uint64_t PairsKey = 0;
        std::vector<MorphPair> Pairs;
        uint64_t MeshKey = 0;
        VectorMesh Mesh;
        glm::vec2 BoundsMin{0.0f};
        glm::vec2 BoundsMax{0.0f};
        std::vector<glm::vec2> World;
    };

    namespace VectorShapes
    {
        VectorShapeCache &CacheOf(const VectorShapeComponent &shape);
        void Outline(const VectorShapeComponent &shape, float tolerance, std::vector<FlatContour> &out);
        const VectorMesh &LocalMesh(const VectorShapeComponent &shape, float tolerance, float aa);
        void Draw(const VectorShapeComponent &shape, const glm::vec2 &position, float rotationDegrees, const glm::vec2 &scale, float zIndex);
        void DrawPath(const VectorPath &path, const glm::vec2 &position, float rotationDegrees, const glm::vec2 &scale, float zIndex,
                      bool fill, const glm::vec4 &fillColor, bool stroke, const glm::vec4 &strokeColor, const StrokeStyle &style,
                      float trimStart = 0.0f, float trimEnd = 1.0f, float trimOffset = 0.0f);
        void DrawPolyline(const std::vector<glm::vec2> &points, bool closed, const StrokeStyle &style, const glm::vec4 &color, float zIndex);
        void DrawPolygon(const std::vector<glm::vec2> &points, const glm::vec4 &color, float zIndex);
        float WorldLength(const VectorShapeComponent &shape, const glm::vec2 &scale);
        float LocalTolerance(const glm::vec2 &scale);
    }
}

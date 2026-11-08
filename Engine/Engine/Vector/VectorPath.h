#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL_pixels.h>

#include <cstdint>
#include <string>
#include <vector>

namespace SIMPEngine
{
    struct VectorNode
    {
        glm::vec2 Position{0.0f};
        glm::vec2 In{0.0f};
        glm::vec2 Out{0.0f};
    };

    struct VectorContour
    {
        std::vector<VectorNode> Nodes;
        bool Closed = true;
    };

    using Polyline = std::vector<glm::vec2>;

    struct FlatContour
    {
        Polyline Points;
        bool Closed = true;
    };

    struct VectorStyleHint
    {
        bool HasFill = false;
        SDL_Color Fill{255, 255, 255, 255};
        bool HasStroke = false;
        SDL_Color Stroke{0, 0, 0, 255};
        float StrokeWidth = 1.0f;
    };

    class VectorPath
    {
    public:
        std::vector<VectorContour> Contours;

        void Clear() { Contours.clear(); }
        bool Empty() const;

        VectorPath &MoveTo(float x, float y);
        VectorPath &LineTo(float x, float y);
        VectorPath &QuadTo(float cx, float cy, float x, float y);
        VectorPath &CubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
        VectorPath &ArcTo(float cx, float cy, float radius, float startDegrees, float endDegrees);
        VectorPath &Close();
        VectorPath &Append(const VectorPath &other);

        void Translate(float x, float y);
        void Scale(float x, float y);
        void Rotate(float degrees);

        static VectorPath Rect(float width, float height, float radius = 0.0f);
        static VectorPath Ellipse(float rx, float ry);
        static VectorPath Circle(float radius) { return Ellipse(radius, radius); }
        static VectorPath Polygon(int sides, float radius);
        static VectorPath Star(int points, float outer, float inner);
        static VectorPath Heart(float size);
        static VectorPath Line(float x1, float y1, float x2, float y2);
        static VectorPath Arc(float radius, float startDegrees, float endDegrees);
        static VectorPath Points(const std::vector<glm::vec2> &points, bool closed);

        bool LoadSvgText(const std::string &svg, float fitSize = 0.0f, VectorStyleHint *hint = nullptr);
        bool LoadSvgFile(const std::string &path, float fitSize = 0.0f, VectorStyleHint *hint = nullptr);
        bool LoadSvgPathData(const std::string &data, float fitSize = 0.0f);

        void Flatten(float tolerance, std::vector<FlatContour> &out) const;
        bool GetBounds(glm::vec2 &min, glm::vec2 &max) const;
        uint64_t Hash() const;
    };

    float PolylineLength(const Polyline &points, bool closed);
    void ResamplePolyline(const Polyline &points, bool closed, int count, Polyline &out);
    bool PointAtDistance(const Polyline &points, bool closed, float distance, glm::vec2 &point, glm::vec2 &tangent);
    void TrimPolyline(const Polyline &points, bool closed, float start, float end, float offset, std::vector<FlatContour> &out);
    void DashPolyline(const Polyline &points, bool closed, float dash, float gap, float offset, std::vector<FlatContour> &out);
    float SignedArea(const Polyline &points);

    struct MorphPair
    {
        Polyline From;
        Polyline To;
        bool ClosedFrom = true;
        bool ClosedTo = true;
    };

    void BuildMorphPairs(const std::vector<FlatContour> &from, const std::vector<FlatContour> &to, std::vector<MorphPair> &out);
    void BlendMorph(const std::vector<MorphPair> &pairs, float t, std::vector<FlatContour> &out);
}

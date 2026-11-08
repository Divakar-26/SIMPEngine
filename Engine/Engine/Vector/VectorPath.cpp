#include "VectorPath.h"

#include <Engine/Core/VFS.h>

#include <nanosvg.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;
        constexpr float Kappa = 0.5522847498f;

        bool Near(const glm::vec2 &a, const glm::vec2 &b, float eps = 1e-4f)
        {
            return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps;
        }

        glm::vec2 RotateVec(const glm::vec2 &v, float c, float s)
        {
            return {v.x * c - v.y * s, v.x * s + v.y * c};
        }

        float DistanceToLine(const glm::vec2 &p, const glm::vec2 &a, const glm::vec2 &b)
        {
            glm::vec2 ab = b - a;
            float len = glm::length(ab);
            if (len < 1e-6f)
                return glm::length(p - a);
            return std::abs(ab.x * (p.y - a.y) - ab.y * (p.x - a.x)) / len;
        }

        void FlattenCubic(const glm::vec2 &p0, const glm::vec2 &p1, const glm::vec2 &p2, const glm::vec2 &p3, float tolerance, int depth, Polyline &out)
        {
            if (depth >= 12 || (DistanceToLine(p1, p0, p3) <= tolerance && DistanceToLine(p2, p0, p3) <= tolerance))
            {
                out.push_back(p3);
                return;
            }
            const glm::vec2 p01 = (p0 + p1) * 0.5f;
            const glm::vec2 p12 = (p1 + p2) * 0.5f;
            const glm::vec2 p23 = (p2 + p3) * 0.5f;
            const glm::vec2 p012 = (p01 + p12) * 0.5f;
            const glm::vec2 p123 = (p12 + p23) * 0.5f;
            const glm::vec2 mid = (p012 + p123) * 0.5f;
            FlattenCubic(p0, p01, p012, mid, tolerance, depth + 1, out);
            FlattenCubic(mid, p123, p23, p3, tolerance, depth + 1, out);
        }

        void HashBytes(uint64_t &h, const void *data, size_t size)
        {
            const auto *bytes = static_cast<const unsigned char *>(data);
            for (size_t i = 0; i < size; ++i)
            {
                h ^= bytes[i];
                h *= 1099511628211ull;
            }
        }

        std::vector<float> Cumulative(const Polyline &points, bool closed)
        {
            std::vector<float> lengths(points.size() + 1, 0.0f);
            for (size_t i = 1; i < points.size(); ++i)
                lengths[i] = lengths[i - 1] + glm::length(points[i] - points[i - 1]);
            lengths[points.size()] = lengths[points.size() - 1] + (closed && points.size() > 1 ? glm::length(points.front() - points.back()) : 0.0f);
            return lengths;
        }

        glm::vec2 PointOnSegment(const Polyline &points, size_t index, float t)
        {
            const glm::vec2 &a = points[index % points.size()];
            const glm::vec2 &b = points[(index + 1) % points.size()];
            return a + (b - a) * t;
        }

        void ExtractRange(const Polyline &points, bool closed, const std::vector<float> &lengths, float d0, float d1, Polyline &out)
        {
            const size_t segments = closed ? points.size() : points.size() - 1;
            for (size_t i = 0; i < segments; ++i)
            {
                const float a = lengths[i];
                const float b = lengths[i + 1];
                if (b < d0 || a > d1)
                    continue;
                const float len = std::max(b - a, 1e-6f);
                if (out.empty())
                    out.push_back(PointOnSegment(points, i, std::clamp((d0 - a) / len, 0.0f, 1.0f)));
                const float endT = std::clamp((d1 - a) / len, 0.0f, 1.0f);
                const glm::vec2 end = PointOnSegment(points, i, endT);
                if (!Near(out.back(), end, 1e-5f))
                    out.push_back(end);
                if (b >= d1)
                    break;
            }
        }

        glm::vec2 Centroid(const Polyline &points)
        {
            glm::vec2 sum(0.0f);
            for (const glm::vec2 &p : points)
                sum += p;
            return points.empty() ? sum : sum / static_cast<float>(points.size());
        }
    }

    bool VectorPath::Empty() const
    {
        for (const VectorContour &contour : Contours)
            if (!contour.Nodes.empty())
                return false;
        return true;
    }

    VectorPath &VectorPath::MoveTo(float x, float y)
    {
        VectorContour contour;
        contour.Closed = false;
        contour.Nodes.push_back({{x, y}, {0.0f, 0.0f}, {0.0f, 0.0f}});
        Contours.push_back(std::move(contour));
        return *this;
    }

    VectorPath &VectorPath::LineTo(float x, float y)
    {
        if (Contours.empty() || Contours.back().Nodes.empty())
            return MoveTo(x, y);
        if (Contours.back().Closed)
        {
            glm::vec2 start = Contours.back().Nodes.front().Position;
            MoveTo(start.x, start.y);
        }
        Contours.back().Nodes.push_back({{x, y}, {0.0f, 0.0f}, {0.0f, 0.0f}});
        return *this;
    }

    VectorPath &VectorPath::CubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
    {
        if (Contours.empty() || Contours.back().Nodes.empty())
            MoveTo(0.0f, 0.0f);
        if (Contours.back().Closed)
        {
            glm::vec2 start = Contours.back().Nodes.front().Position;
            MoveTo(start.x, start.y);
        }
        VectorNode &last = Contours.back().Nodes.back();
        last.Out = glm::vec2(c1x, c1y) - last.Position;
        VectorNode node;
        node.Position = {x, y};
        node.In = glm::vec2(c2x, c2y) - node.Position;
        Contours.back().Nodes.push_back(node);
        return *this;
    }

    VectorPath &VectorPath::QuadTo(float cx, float cy, float x, float y)
    {
        if (Contours.empty() || Contours.back().Nodes.empty())
            MoveTo(0.0f, 0.0f);
        const glm::vec2 p0 = Contours.back().Nodes.back().Position;
        const glm::vec2 c(cx, cy);
        const glm::vec2 p(x, y);
        const glm::vec2 c1 = p0 + (c - p0) * (2.0f / 3.0f);
        const glm::vec2 c2 = p + (c - p) * (2.0f / 3.0f);
        return CubicTo(c1.x, c1.y, c2.x, c2.y, x, y);
    }

    VectorPath &VectorPath::ArcTo(float cx, float cy, float radius, float startDegrees, float endDegrees)
    {
        const float a0 = glm::radians(startDegrees);
        const float a1 = glm::radians(endDegrees);
        const glm::vec2 center(cx, cy);
        const glm::vec2 start = center + glm::vec2(std::cos(a0), std::sin(a0)) * radius;
        if (Contours.empty() || Contours.back().Nodes.empty() || Contours.back().Closed)
            MoveTo(start.x, start.y);
        else if (!Near(Contours.back().Nodes.back().Position, start))
            LineTo(start.x, start.y);

        const float sweep = a1 - a0;
        const int pieces = std::max(1, static_cast<int>(std::ceil(std::abs(sweep) / (Pi * 0.5f) - 1e-4f)));
        const float step = sweep / static_cast<float>(pieces);
        const float k = 4.0f / 3.0f * std::tan(step / 4.0f);
        for (int i = 0; i < pieces; ++i)
        {
            const float s = a0 + step * static_cast<float>(i);
            const float e = s + step;
            const glm::vec2 p0 = center + glm::vec2(std::cos(s), std::sin(s)) * radius;
            const glm::vec2 p1 = center + glm::vec2(std::cos(e), std::sin(e)) * radius;
            const glm::vec2 c1 = p0 + glm::vec2(-std::sin(s), std::cos(s)) * (k * radius);
            const glm::vec2 c2 = p1 - glm::vec2(-std::sin(e), std::cos(e)) * (k * radius);
            CubicTo(c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        }
        return *this;
    }

    VectorPath &VectorPath::Close()
    {
        if (Contours.empty())
            return *this;
        VectorContour &contour = Contours.back();
        if (contour.Nodes.size() >= 2 && Near(contour.Nodes.front().Position, contour.Nodes.back().Position))
        {
            contour.Nodes.front().In = contour.Nodes.back().In;
            contour.Nodes.pop_back();
        }
        contour.Closed = true;
        return *this;
    }

    VectorPath &VectorPath::Append(const VectorPath &other)
    {
        Contours.insert(Contours.end(), other.Contours.begin(), other.Contours.end());
        return *this;
    }

    void VectorPath::Translate(float x, float y)
    {
        for (VectorContour &contour : Contours)
            for (VectorNode &node : contour.Nodes)
                node.Position += glm::vec2(x, y);
    }

    void VectorPath::Scale(float x, float y)
    {
        const glm::vec2 s(x, y);
        for (VectorContour &contour : Contours)
            for (VectorNode &node : contour.Nodes)
            {
                node.Position *= s;
                node.In *= s;
                node.Out *= s;
            }
    }

    void VectorPath::Rotate(float degrees)
    {
        const float r = glm::radians(degrees);
        const float c = std::cos(r);
        const float s = std::sin(r);
        for (VectorContour &contour : Contours)
            for (VectorNode &node : contour.Nodes)
            {
                node.Position = RotateVec(node.Position, c, s);
                node.In = RotateVec(node.In, c, s);
                node.Out = RotateVec(node.Out, c, s);
            }
    }

    VectorPath VectorPath::Rect(float width, float height, float radius)
    {
        VectorPath path;
        const float w = std::abs(width) * 0.5f;
        const float h = std::abs(height) * 0.5f;
        const float r = std::clamp(radius, 0.0f, std::min(w, h));
        if (r <= 0.0f)
        {
            path.MoveTo(-w, -h).LineTo(w, -h).LineTo(w, h).LineTo(-w, h).Close();
            return path;
        }
        path.MoveTo(-w + r, -h);
        path.LineTo(w - r, -h);
        path.ArcTo(w - r, -h + r, r, -90.0f, 0.0f);
        path.LineTo(w, h - r);
        path.ArcTo(w - r, h - r, r, 0.0f, 90.0f);
        path.LineTo(-w + r, h);
        path.ArcTo(-w + r, h - r, r, 90.0f, 180.0f);
        path.LineTo(-w, -h + r);
        path.ArcTo(-w + r, -h + r, r, 180.0f, 270.0f);
        path.Close();
        return path;
    }

    VectorPath VectorPath::Ellipse(float rx, float ry)
    {
        VectorPath path;
        VectorContour contour;
        const float kx = rx * Kappa;
        const float ky = ry * Kappa;
        contour.Nodes = {
            {{rx, 0.0f}, {0.0f, -ky}, {0.0f, ky}},
            {{0.0f, ry}, {kx, 0.0f}, {-kx, 0.0f}},
            {{-rx, 0.0f}, {0.0f, ky}, {0.0f, -ky}},
            {{0.0f, -ry}, {-kx, 0.0f}, {kx, 0.0f}}};
        contour.Closed = true;
        path.Contours.push_back(contour);
        return path;
    }

    VectorPath VectorPath::Polygon(int sides, float radius)
    {
        sides = std::clamp(sides, 3, 512);
        std::vector<glm::vec2> points;
        for (int i = 0; i < sides; ++i)
        {
            const float a = Pi * 0.5f + 2.0f * Pi * static_cast<float>(i) / static_cast<float>(sides);
            points.push_back(glm::vec2(std::cos(a), std::sin(a)) * radius);
        }
        return Points(points, true);
    }

    VectorPath VectorPath::Star(int points, float outer, float inner)
    {
        points = std::clamp(points, 2, 256);
        std::vector<glm::vec2> vertices;
        for (int i = 0; i < points * 2; ++i)
        {
            const float a = Pi * 0.5f + Pi * static_cast<float>(i) / static_cast<float>(points);
            vertices.push_back(glm::vec2(std::cos(a), std::sin(a)) * (i % 2 == 0 ? outer : inner));
        }
        return Points(vertices, true);
    }

    VectorPath VectorPath::Heart(float size)
    {
        VectorPath path;
        path.MoveTo(0.0f, -0.3f);
        path.CubicTo(0.0f, 0.0f, -0.5f, 0.1f, -0.5f, -0.25f);
        path.CubicTo(-0.5f, -0.55f, 0.0f, -0.75f, 0.0f, -1.0f);
        path.CubicTo(0.0f, -0.75f, 0.5f, -0.55f, 0.5f, -0.25f);
        path.CubicTo(0.5f, 0.1f, 0.0f, 0.0f, 0.0f, -0.3f);
        path.Close();
        glm::vec2 min, max;
        path.GetBounds(min, max);
        path.Translate(-(min.x + max.x) * 0.5f, -(min.y + max.y) * 0.5f);
        const float extent = std::max(max.x - min.x, max.y - min.y);
        if (extent > 0.0f)
            path.Scale(size / extent, size / extent);
        return path;
    }

    VectorPath VectorPath::Line(float x1, float y1, float x2, float y2)
    {
        VectorPath path;
        path.MoveTo(x1, y1).LineTo(x2, y2);
        return path;
    }

    VectorPath VectorPath::Arc(float radius, float startDegrees, float endDegrees)
    {
        VectorPath path;
        path.ArcTo(0.0f, 0.0f, radius, startDegrees, endDegrees);
        return path;
    }

    VectorPath VectorPath::Points(const std::vector<glm::vec2> &points, bool closed)
    {
        VectorPath path;
        if (points.empty())
            return path;
        VectorContour contour;
        contour.Closed = closed;
        for (const glm::vec2 &p : points)
            contour.Nodes.push_back({p, {0.0f, 0.0f}, {0.0f, 0.0f}});
        path.Contours.push_back(std::move(contour));
        return path;
    }

    bool VectorPath::LoadSvgText(const std::string &svg, float fitSize, VectorStyleHint *hint)
    {
        std::vector<char> buffer(svg.begin(), svg.end());
        buffer.push_back('\0');
        NSVGimage *image = nsvgParse(buffer.data(), "px", 96.0f);
        if (!image)
            return false;

        VectorPath result;
        bool styled = false;
        for (NSVGshape *shape = image->shapes; shape; shape = shape->next)
        {
            if (!(shape->flags & NSVG_FLAGS_VISIBLE))
                continue;
            if (hint && !styled)
            {
                auto toColor = [&](unsigned int c)
                {
                    return SDL_Color{static_cast<Uint8>(c & 0xFF), static_cast<Uint8>((c >> 8) & 0xFF), static_cast<Uint8>((c >> 16) & 0xFF),
                                     static_cast<Uint8>(std::clamp(static_cast<float>((c >> 24) & 0xFF) * shape->opacity, 0.0f, 255.0f))};
                };
                hint->HasFill = shape->fill.type == NSVG_PAINT_COLOR;
                if (hint->HasFill)
                    hint->Fill = toColor(shape->fill.color);
                hint->HasStroke = shape->stroke.type == NSVG_PAINT_COLOR && shape->strokeWidth > 0.0f;
                if (hint->HasStroke)
                    hint->Stroke = toColor(shape->stroke.color);
                hint->StrokeWidth = shape->strokeWidth;
                styled = true;
            }
            for (NSVGpath *path = shape->paths; path; path = path->next)
            {
                if (path->npts < 1)
                    continue;
                const float *p = path->pts;
                result.MoveTo(p[0], -p[1]);
                for (int i = 0; i + 3 < path->npts; i += 3)
                {
                    const float *c = &p[i * 2 + 2];
                    result.CubicTo(c[0], -c[1], c[2], -c[3], c[4], -c[5]);
                }
                if (path->closed)
                    result.Close();
            }
        }
        nsvgDelete(image);

        if (result.Empty())
            return false;

        glm::vec2 min, max;
        result.GetBounds(min, max);
        result.Translate(-(min.x + max.x) * 0.5f, -(min.y + max.y) * 0.5f);
        const float extent = std::max(max.x - min.x, max.y - min.y);
        if (fitSize > 0.0f && extent > 0.0f)
        {
            const float scale = fitSize / extent;
            result.Scale(scale, scale);
            if (hint)
                hint->StrokeWidth *= scale;
        }
        *this = std::move(result);
        return true;
    }

    bool VectorPath::LoadSvgFile(const std::string &path, float fitSize, VectorStyleHint *hint)
    {
        std::string real = path;
        if (auto resolved = VFS::Resolve(path))
            real = *resolved;
        std::ifstream file(real, std::ios::binary);
        if (!file)
            return false;
        std::stringstream buffer;
        buffer << file.rdbuf();
        return LoadSvgText(buffer.str(), fitSize, hint);
    }

    bool VectorPath::LoadSvgPathData(const std::string &data, float fitSize)
    {
        if (data.find('"') != std::string::npos || data.find('<') != std::string::npos)
            return false;
        return LoadSvgText("<svg xmlns=\"http://www.w3.org/2000/svg\"><path d=\"" + data + "\"/></svg>", fitSize, nullptr);
    }

    void VectorPath::Flatten(float tolerance, std::vector<FlatContour> &out) const
    {
        out.clear();
        tolerance = std::max(tolerance, 1e-4f);
        for (const VectorContour &contour : Contours)
        {
            const size_t count = contour.Nodes.size();
            if (count == 0)
                continue;
            FlatContour flat;
            flat.Closed = contour.Closed && count > 2;
            flat.Points.push_back(contour.Nodes[0].Position);
            const size_t segments = contour.Closed ? count : count - 1;
            for (size_t i = 0; i < segments; ++i)
            {
                const VectorNode &a = contour.Nodes[i];
                const VectorNode &b = contour.Nodes[(i + 1) % count];
                const bool curved = a.Out != glm::vec2(0.0f) || b.In != glm::vec2(0.0f);
                if (!curved)
                    flat.Points.push_back(b.Position);
                else
                    FlattenCubic(a.Position, a.Position + a.Out, b.Position + b.In, b.Position, tolerance, 0, flat.Points);
            }

            Polyline cleaned;
            cleaned.reserve(flat.Points.size());
            for (const glm::vec2 &p : flat.Points)
                if (cleaned.empty() || !Near(cleaned.back(), p, 1e-5f))
                    cleaned.push_back(p);
            if (contour.Closed && cleaned.size() > 1 && Near(cleaned.front(), cleaned.back(), 1e-5f))
                cleaned.pop_back();
            flat.Points = std::move(cleaned);
            if (contour.Closed && flat.Points.size() > 2)
                flat.Closed = true;
            out.push_back(std::move(flat));
        }
    }

    bool VectorPath::GetBounds(glm::vec2 &min, glm::vec2 &max) const
    {
        std::vector<FlatContour> flat;
        Flatten(0.05f, flat);
        bool any = false;
        for (const FlatContour &contour : flat)
            for (const glm::vec2 &p : contour.Points)
            {
                min = any ? glm::min(min, p) : p;
                max = any ? glm::max(max, p) : p;
                any = true;
            }
        if (!any)
            min = max = glm::vec2(0.0f);
        return any;
    }

    uint64_t VectorPath::Hash() const
    {
        uint64_t h = 1469598103934665603ull;
        const size_t count = Contours.size();
        HashBytes(h, &count, sizeof(count));
        for (const VectorContour &contour : Contours)
        {
            const size_t nodes = contour.Nodes.size();
            HashBytes(h, &nodes, sizeof(nodes));
            HashBytes(h, &contour.Closed, sizeof(contour.Closed));
            if (nodes)
                HashBytes(h, contour.Nodes.data(), nodes * sizeof(VectorNode));
        }
        return h;
    }

    float SignedArea(const Polyline &points)
    {
        float area = 0.0f;
        for (size_t i = 0, n = points.size(); i < n; ++i)
        {
            const glm::vec2 &a = points[i];
            const glm::vec2 &b = points[(i + 1) % n];
            area += a.x * b.y - b.x * a.y;
        }
        return area * 0.5f;
    }

    float PolylineLength(const Polyline &points, bool closed)
    {
        if (points.size() < 2)
            return 0.0f;
        return Cumulative(points, closed).back();
    }

    void ResamplePolyline(const Polyline &points, bool closed, int count, Polyline &out)
    {
        out.clear();
        count = std::max(count, 2);
        if (points.empty())
        {
            out.assign(static_cast<size_t>(count), glm::vec2(0.0f));
            return;
        }
        if (points.size() == 1)
        {
            out.assign(static_cast<size_t>(count), points[0]);
            return;
        }
        const std::vector<float> lengths = Cumulative(points, closed);
        const float total = lengths.back();
        if (total <= 1e-6f)
        {
            out.assign(static_cast<size_t>(count), points[0]);
            return;
        }
        const size_t segments = closed ? points.size() : points.size() - 1;
        const float step = closed ? total / static_cast<float>(count) : total / static_cast<float>(count - 1);
        size_t segment = 0;
        out.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
        {
            const float d = std::min(step * static_cast<float>(i), total);
            while (segment + 1 < segments && lengths[segment + 1] < d)
                ++segment;
            const float len = std::max(lengths[segment + 1] - lengths[segment], 1e-6f);
            out.push_back(PointOnSegment(points, segment, std::clamp((d - lengths[segment]) / len, 0.0f, 1.0f)));
        }
    }

    bool PointAtDistance(const Polyline &points, bool closed, float distance, glm::vec2 &point, glm::vec2 &tangent)
    {
        if (points.empty())
            return false;
        if (points.size() == 1)
        {
            point = points[0];
            tangent = {1.0f, 0.0f};
            return true;
        }
        const std::vector<float> lengths = Cumulative(points, closed);
        const float total = lengths.back();
        if (closed && total > 0.0f)
            distance = distance - std::floor(distance / total) * total;
        distance = std::clamp(distance, 0.0f, total);
        const size_t segments = closed ? points.size() : points.size() - 1;
        size_t segment = 0;
        while (segment + 1 < segments && lengths[segment + 1] < distance)
            ++segment;
        const float len = std::max(lengths[segment + 1] - lengths[segment], 1e-6f);
        point = PointOnSegment(points, segment, std::clamp((distance - lengths[segment]) / len, 0.0f, 1.0f));
        const glm::vec2 d = points[(segment + 1) % points.size()] - points[segment];
        tangent = glm::length(d) > 1e-6f ? glm::normalize(d) : glm::vec2(1.0f, 0.0f);
        return true;
    }

    void TrimPolyline(const Polyline &points, bool closed, float start, float end, float offset, std::vector<FlatContour> &out)
    {
        if (points.size() < 2)
            return;
        const float span = end - start;
        if (span <= 1e-5f)
            return;
        const std::vector<float> lengths = Cumulative(points, closed);
        const float total = lengths.back();
        if (total <= 1e-6f)
            return;

        if (closed)
        {
            if (span >= 1.0f - 1e-5f)
            {
                out.push_back({points, true});
                return;
            }
            float s = start + offset;
            s -= std::floor(s);
            const float e = s + span;
            FlatContour piece;
            piece.Closed = false;
            if (e <= 1.0f)
                ExtractRange(points, true, lengths, s * total, e * total, piece.Points);
            else
            {
                ExtractRange(points, true, lengths, s * total, total, piece.Points);
                Polyline tail;
                ExtractRange(points, true, lengths, 0.0f, (e - 1.0f) * total, tail);
                for (const glm::vec2 &p : tail)
                    if (piece.Points.empty() || !Near(piece.Points.back(), p, 1e-5f))
                        piece.Points.push_back(p);
            }
            if (piece.Points.size() >= 2)
                out.push_back(std::move(piece));
            return;
        }

        const float s = std::clamp(start + offset, 0.0f, 1.0f);
        const float e = std::clamp(end + offset, 0.0f, 1.0f);
        if (e - s <= 1e-5f)
            return;
        if (s <= 0.0f && e >= 1.0f)
        {
            out.push_back({points, false});
            return;
        }
        FlatContour piece;
        piece.Closed = false;
        ExtractRange(points, false, lengths, s * total, e * total, piece.Points);
        if (piece.Points.size() >= 2)
            out.push_back(std::move(piece));
    }

    void DashPolyline(const Polyline &points, bool closed, float dash, float gap, float offset, std::vector<FlatContour> &out)
    {
        if (points.size() < 2 || dash <= 0.0f)
            return;
        gap = std::max(gap, 0.0f);
        const std::vector<float> lengths = Cumulative(points, closed);
        const float total = lengths.back();
        const float period = dash + gap;
        if (total <= 1e-6f || period <= 1e-6f)
            return;
        float d = -(offset - std::floor(offset / period) * period);
        int guard = 0;
        while (d < total && guard++ < 4000)
        {
            const float s = std::max(d, 0.0f);
            const float e = std::min(d + dash, total);
            if (e > s)
            {
                FlatContour piece;
                piece.Closed = false;
                ExtractRange(points, closed, lengths, s, e, piece.Points);
                if (piece.Points.size() >= 2)
                    out.push_back(std::move(piece));
            }
            d += period;
        }
    }

    void BuildMorphPairs(const std::vector<FlatContour> &from, const std::vector<FlatContour> &to, std::vector<MorphPair> &out)
    {
        out.clear();
        auto prepare = [](const std::vector<FlatContour> &source)
        {
            std::vector<FlatContour> list;
            for (const FlatContour &contour : source)
            {
                if (contour.Points.empty())
                    continue;
                FlatContour copy = contour;
                if (copy.Closed && SignedArea(copy.Points) < 0.0f)
                    std::reverse(copy.Points.begin(), copy.Points.end());
                list.push_back(std::move(copy));
            }
            std::stable_sort(list.begin(), list.end(), [](const FlatContour &a, const FlatContour &b)
                             {
                const float sa = a.Closed ? std::abs(SignedArea(a.Points)) : 0.0f;
                const float sb = b.Closed ? std::abs(SignedArea(b.Points)) : 0.0f;
                if (sa != sb)
                    return sa > sb;
                return PolylineLength(a.Points, a.Closed) > PolylineLength(b.Points, b.Closed); });
            return list;
        };

        const std::vector<FlatContour> a = prepare(from);
        const std::vector<FlatContour> b = prepare(to);
        const size_t pairs = std::max(a.size(), b.size());
        for (size_t i = 0; i < pairs; ++i)
        {
            const FlatContour *ca = i < a.size() ? &a[i] : nullptr;
            const FlatContour *cb = i < b.size() ? &b[i] : nullptr;
            const size_t na = ca ? ca->Points.size() : 0;
            const size_t nb = cb ? cb->Points.size() : 0;
            const int count = std::clamp(static_cast<int>(std::max(na, nb) * 3), 32, 512);

            MorphPair pair;
            pair.ClosedFrom = ca ? ca->Closed : cb->Closed;
            pair.ClosedTo = cb ? cb->Closed : ca->Closed;
            if (ca)
                ResamplePolyline(ca->Points, ca->Closed, count, pair.From);
            else
                pair.From.assign(static_cast<size_t>(count), Centroid(cb->Points));
            if (cb)
                ResamplePolyline(cb->Points, cb->Closed, count, pair.To);
            else
                pair.To.assign(static_cast<size_t>(count), Centroid(ca->Points));

            if (ca && cb && ca->Closed && cb->Closed)
            {
                const size_t n = pair.To.size();
                const size_t stride = n > 256 ? 2 : 1;
                size_t best = 0;
                float bestCost = std::numeric_limits<float>::max();
                for (size_t shift = 0; shift < n; shift += stride)
                {
                    float cost = 0.0f;
                    for (size_t k = 0; k < n && cost < bestCost; k += stride)
                    {
                        const glm::vec2 d = pair.From[k] - pair.To[(k + shift) % n];
                        cost += d.x * d.x + d.y * d.y;
                    }
                    if (cost < bestCost)
                    {
                        bestCost = cost;
                        best = shift;
                    }
                }
                std::rotate(pair.To.begin(), pair.To.begin() + static_cast<std::ptrdiff_t>(best), pair.To.end());
            }
            out.push_back(std::move(pair));
        }
    }

    void BlendMorph(const std::vector<MorphPair> &pairs, float t, std::vector<FlatContour> &out)
    {
        out.clear();
        for (const MorphPair &pair : pairs)
        {
            FlatContour contour;
            contour.Closed = t < 0.5f ? pair.ClosedFrom : pair.ClosedTo;
            contour.Points.resize(pair.From.size());
            for (size_t i = 0; i < pair.From.size(); ++i)
                contour.Points[i] = pair.From[i] + (pair.To[i] - pair.From[i]) * t;
            out.push_back(std::move(contour));
        }
    }
}

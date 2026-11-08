#include "VectorTessellator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;

        float Cross(const glm::vec2 &a, const glm::vec2 &b)
        {
            return a.x * b.y - a.y * b.x;
        }

        glm::vec2 SafeNormalize(const glm::vec2 &v, const glm::vec2 &fallback = {1.0f, 0.0f})
        {
            const float len = glm::length(v);
            return len > 1e-8f ? v / len : fallback;
        }

        bool SamePoint(const glm::vec2 &a, const glm::vec2 &b)
        {
            return std::abs(a.x - b.x) < 1e-6f && std::abs(a.y - b.y) < 1e-6f;
        }

        bool PointInPolygon(const glm::vec2 &p, const Polyline &poly)
        {
            bool inside = false;
            for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++)
            {
                const glm::vec2 &a = poly[i];
                const glm::vec2 &b = poly[j];
                if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
                    inside = !inside;
            }
            return inside;
        }

        bool PointInTriangle(const glm::vec2 &p, const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c)
        {
            const float d1 = Cross(b - a, p - a);
            const float d2 = Cross(c - b, p - b);
            const float d3 = Cross(a - c, p - c);
            return d1 >= -1e-7f && d2 >= -1e-7f && d3 >= -1e-7f;
        }

        uint32_t AddVertex(VectorMesh &mesh, const glm::vec2 &p, const glm::vec4 &color)
        {
            mesh.Positions.push_back(p);
            mesh.Colors.push_back(color);
            return static_cast<uint32_t>(mesh.Positions.size() - 1);
        }

        void AddTriangle(VectorMesh &mesh, uint32_t a, uint32_t b, uint32_t c)
        {
            mesh.Indices.push_back(a);
            mesh.Indices.push_back(b);
            mesh.Indices.push_back(c);
        }

        void AddQuad(VectorMesh &mesh, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        {
            AddTriangle(mesh, a, b, c);
            AddTriangle(mesh, a, c, d);
        }

        glm::vec4 Transparent(glm::vec4 c)
        {
            c.a = 0.0f;
            return c;
        }

        Polyline BridgeHole(const Polyline &outer, const Polyline &hole)
        {
            size_t m = 0;
            for (size_t i = 1; i < hole.size(); ++i)
                if (hole[i].x > hole[m].x)
                    m = i;
            const glm::vec2 M = hole[m];

            float bestX = std::numeric_limits<float>::max();
            size_t candidate = std::numeric_limits<size_t>::max();
            glm::vec2 hit(0.0f);
            for (size_t i = 0; i < outer.size(); ++i)
            {
                const glm::vec2 &a = outer[i];
                const glm::vec2 &b = outer[(i + 1) % outer.size()];
                if ((a.y <= M.y && b.y > M.y) || (b.y <= M.y && a.y > M.y))
                {
                    const float x = a.x + (M.y - a.y) * (b.x - a.x) / (b.y - a.y);
                    if (x >= M.x && x < bestX)
                    {
                        bestX = x;
                        hit = {x, M.y};
                        candidate = a.x > b.x ? i : (i + 1) % outer.size();
                    }
                }
            }

            if (candidate == std::numeric_limits<size_t>::max())
            {
                float best = std::numeric_limits<float>::max();
                for (size_t i = 0; i < outer.size(); ++i)
                {
                    const float d = glm::length(outer[i] - M);
                    if (d < best)
                    {
                        best = d;
                        candidate = i;
                    }
                }
            }
            else
            {
                const glm::vec2 P = outer[candidate];
                float bestAngle = std::numeric_limits<float>::max();
                size_t chosen = candidate;
                for (size_t i = 0; i < outer.size(); ++i)
                {
                    const glm::vec2 &v = outer[i];
                    if (v.x < M.x)
                        continue;
                    const glm::vec2 &prev = outer[(i + outer.size() - 1) % outer.size()];
                    const glm::vec2 &next = outer[(i + 1) % outer.size()];
                    const bool reflex = Cross(v - prev, next - v) <= 0.0f;
                    if (i != candidate && !reflex)
                        continue;
                    const bool inside = i == candidate || PointInTriangle(v, M, hit, P) || PointInTriangle(v, M, P, hit);
                    if (!inside)
                        continue;
                    const float angle = std::atan2(std::abs(v.y - M.y), std::max(v.x - M.x, 1e-6f));
                    if (angle < bestAngle || (angle == bestAngle && v.x < outer[chosen].x))
                    {
                        bestAngle = angle;
                        chosen = i;
                    }
                }
                candidate = chosen;
            }

            Polyline merged;
            merged.reserve(outer.size() + hole.size() + 2);
            for (size_t i = 0; i <= candidate; ++i)
                merged.push_back(outer[i]);
            for (size_t k = 0; k <= hole.size(); ++k)
                merged.push_back(hole[(m + k) % hole.size()]);
            merged.push_back(outer[candidate]);
            for (size_t i = candidate + 1; i < outer.size(); ++i)
                merged.push_back(outer[i]);
            return merged;
        }

        void EarClip(const Polyline &poly, std::vector<uint32_t> &triangles)
        {
            const size_t n = poly.size();
            if (n < 3)
                return;

            bool convex = true;
            for (size_t i = 0; i < n && convex; ++i)
                if (Cross(poly[(i + 1) % n] - poly[i], poly[(i + 2) % n] - poly[(i + 1) % n]) < -1e-7f)
                    convex = false;
            if (convex)
            {
                for (size_t i = 1; i + 1 < n; ++i)
                {
                    triangles.push_back(0);
                    triangles.push_back(static_cast<uint32_t>(i));
                    triangles.push_back(static_cast<uint32_t>(i + 1));
                }
                return;
            }

            std::vector<size_t> prev(n), next(n);
            for (size_t i = 0; i < n; ++i)
            {
                prev[i] = (i + n - 1) % n;
                next[i] = (i + 1) % n;
            }

            auto isEar = [&](size_t i)
            {
                const glm::vec2 &a = poly[prev[i]];
                const glm::vec2 &b = poly[i];
                const glm::vec2 &c = poly[next[i]];
                if (Cross(b - a, c - b) <= 1e-9f)
                    return false;
                for (size_t k = next[next[i]]; k != prev[i]; k = next[k])
                {
                    const glm::vec2 &p = poly[k];
                    if (SamePoint(p, a) || SamePoint(p, b) || SamePoint(p, c))
                        continue;
                    const glm::vec2 &pp = poly[prev[k]];
                    const glm::vec2 &pn = poly[next[k]];
                    if (Cross(p - pp, pn - p) > 0.0f)
                        continue;
                    if (PointInTriangle(p, a, b, c))
                        return false;
                }
                return true;
            };

            size_t remaining = n;
            size_t i = 0;
            size_t stall = 0;
            while (remaining > 3)
            {
                if (isEar(i) || stall > remaining)
                {
                    const glm::vec2 &a = poly[prev[i]];
                    const glm::vec2 &b = poly[i];
                    const glm::vec2 &c = poly[next[i]];
                    if (stall <= remaining || Cross(b - a, c - b) > 0.0f)
                    {
                        triangles.push_back(static_cast<uint32_t>(prev[i]));
                        triangles.push_back(static_cast<uint32_t>(i));
                        triangles.push_back(static_cast<uint32_t>(next[i]));
                    }
                    next[prev[i]] = next[i];
                    prev[next[i]] = prev[i];
                    const size_t after = next[i];
                    --remaining;
                    i = after;
                    stall = 0;
                    continue;
                }
                i = next[i];
                ++stall;
            }
            triangles.push_back(static_cast<uint32_t>(prev[i]));
            triangles.push_back(static_cast<uint32_t>(i));
            triangles.push_back(static_cast<uint32_t>(next[i]));
        }

        void AddFillFringe(const Polyline &poly, const VectorPaint &paint, float aa, VectorMesh &mesh)
        {
            const size_t n = poly.size();
            if (n < 3 || aa <= 0.0f)
                return;
            const uint32_t base = static_cast<uint32_t>(mesh.Positions.size());
            for (size_t i = 0; i < n; ++i)
            {
                const glm::vec2 &p = poly[i];
                const glm::vec2 d0 = SafeNormalize(p - poly[(i + n - 1) % n]);
                const glm::vec2 d1 = SafeNormalize(poly[(i + 1) % n] - p);
                const glm::vec2 n0(d0.y, -d0.x);
                const glm::vec2 n1(d1.y, -d1.x);
                const glm::vec2 m = SafeNormalize(n0 + n1, n1);
                const float scale = 1.0f / std::max(glm::dot(m, n1), 0.35f);
                const glm::vec4 color = paint.At(p);
                AddVertex(mesh, p, color);
                AddVertex(mesh, p + m * (aa * scale), Transparent(color));
            }
            for (uint32_t i = 0; i < n; ++i)
            {
                const uint32_t j = (i + 1) % static_cast<uint32_t>(n);
                AddQuad(mesh, base + i * 2, base + j * 2, base + j * 2 + 1, base + i * 2 + 1);
            }
        }

        struct StrokeBuilder
        {
            VectorMesh &Mesh;
            const VectorPaint &Paint;
            float Core;
            float Fringe;
            float Alpha;
            float HalfWidth;
            bool HasRow = false;
            uint32_t Row[4] = {0, 0, 0, 0};
            uint32_t FirstRow[4] = {0, 0, 0, 0};

            glm::vec4 ColorAt(const glm::vec2 &p, float alpha) const
            {
                glm::vec4 c = Paint.At(p);
                c.a *= alpha;
                return c;
            }

            void AddRow(const glm::vec2 &p, const glm::vec2 &normal)
            {
                uint32_t row[4];
                row[0] = AddVertex(Mesh, p + normal * (Core + Fringe), ColorAt(p, 0.0f));
                row[1] = AddVertex(Mesh, p + normal * Core, ColorAt(p, Alpha));
                row[2] = AddVertex(Mesh, p - normal * Core, ColorAt(p, Alpha));
                row[3] = AddVertex(Mesh, p - normal * (Core + Fringe), ColorAt(p, 0.0f));
                if (HasRow)
                    Connect(Row, row);
                else
                    std::copy(row, row + 4, FirstRow);
                std::copy(row, row + 4, Row);
                HasRow = true;
            }

            void Connect(const uint32_t *a, const uint32_t *b)
            {
                for (int k = 0; k < 3; ++k)
                    AddQuad(Mesh, a[k], b[k], b[k + 1], a[k + 1]);
            }

            void EndStrip() { HasRow = false; }

            void AddFan(const glm::vec2 &center, const std::vector<glm::vec2> &outline)
            {
                if (outline.size() < 2)
                    return;
                const uint32_t c = AddVertex(Mesh, center, ColorAt(center, Alpha));
                std::vector<uint32_t> inner, outer;
                for (const glm::vec2 &o : outline)
                {
                    const glm::vec2 dir = SafeNormalize(o - center);
                    inner.push_back(AddVertex(Mesh, o, ColorAt(o, Alpha)));
                    outer.push_back(AddVertex(Mesh, o + dir * Fringe, ColorAt(o, 0.0f)));
                }
                for (size_t i = 0; i + 1 < outline.size(); ++i)
                {
                    AddTriangle(Mesh, c, inner[i], inner[i + 1]);
                    AddQuad(Mesh, inner[i], outer[i], outer[i + 1], inner[i + 1]);
                }
            }

            void AddArc(const glm::vec2 &center, const glm::vec2 &from, const glm::vec2 &to, bool clockwise)
            {
                const float r = std::max(Core, 1e-4f);
                float a0 = std::atan2(from.y, from.x);
                float a1 = std::atan2(to.y, to.x);
                float sweep = a1 - a0;
                if (clockwise)
                {
                    while (sweep > 0.0f)
                        sweep -= 2.0f * Pi;
                }
                else
                {
                    while (sweep < 0.0f)
                        sweep += 2.0f * Pi;
                }
                const float tolerance = std::max(Fringe * 0.25f, r * 0.002f);
                const float stepAngle = 2.0f * std::acos(std::clamp(1.0f - tolerance / r, -1.0f, 1.0f));
                const int steps = std::clamp(static_cast<int>(std::ceil(std::abs(sweep) / std::max(stepAngle, 0.05f))), 2, 48);
                std::vector<glm::vec2> outline;
                for (int i = 0; i <= steps; ++i)
                {
                    const float a = a0 + sweep * static_cast<float>(i) / static_cast<float>(steps);
                    outline.push_back(center + glm::vec2(std::cos(a), std::sin(a)) * r);
                }
                AddFan(center, outline);
            }

            void ButtCap(const glm::vec2 &p, const glm::vec2 &normal, const glm::vec2 &outward)
            {
                const uint32_t a = AddVertex(Mesh, p + normal * (Core + Fringe), ColorAt(p, 0.0f));
                const uint32_t b = AddVertex(Mesh, p + normal * Core, ColorAt(p, Alpha));
                const uint32_t c = AddVertex(Mesh, p - normal * Core, ColorAt(p, Alpha));
                const uint32_t d = AddVertex(Mesh, p - normal * (Core + Fringe), ColorAt(p, 0.0f));
                const glm::vec2 e = outward * Fringe;
                const uint32_t ea = AddVertex(Mesh, p + normal * (Core + Fringe) + e, ColorAt(p, 0.0f));
                const uint32_t eb = AddVertex(Mesh, p + normal * Core + e, ColorAt(p, 0.0f));
                const uint32_t ec = AddVertex(Mesh, p - normal * Core + e, ColorAt(p, 0.0f));
                const uint32_t ed = AddVertex(Mesh, p - normal * (Core + Fringe) + e, ColorAt(p, 0.0f));
                AddQuad(Mesh, a, b, eb, ea);
                AddQuad(Mesh, b, c, ec, eb);
                AddQuad(Mesh, c, d, ed, ec);
            }
        };
    }

    glm::vec4 VectorPaint::At(const glm::vec2 &p) const
    {
        if (!Gradient)
            return Color;
        const glm::vec2 axis = To - From;
        const float len2 = glm::dot(axis, axis);
        const float t = len2 > 1e-8f ? std::clamp(glm::dot(p - From, axis) / len2, 0.0f, 1.0f) : 0.0f;
        return (FromColor + (ToColor - FromColor) * t) * glm::vec4(1.0f, 1.0f, 1.0f, Color.a);
    }

    void VectorMesh::Clear()
    {
        Positions.clear();
        Colors.clear();
        Indices.clear();
    }

    void TessellateFill(const std::vector<FlatContour> &contours, const VectorPaint &paint, float aa, VectorMesh &out)
    {
        std::vector<Polyline> polys;
        std::vector<float> areas;
        for (const FlatContour &contour : contours)
        {
            if (contour.Points.size() < 3)
                continue;
            const float area = SignedArea(contour.Points);
            if (std::abs(area) < 1e-8f)
                continue;
            polys.push_back(contour.Points);
            areas.push_back(area);
        }
        if (polys.empty())
            return;

        const size_t count = polys.size();
        std::vector<int> depth(count, 0);
        std::vector<int> parent(count, -1);
        for (size_t i = 0; i < count; ++i)
        {
            float parentArea = std::numeric_limits<float>::max();
            for (size_t j = 0; j < count; ++j)
            {
                if (i == j || std::abs(areas[j]) <= std::abs(areas[i]))
                    continue;
                if (PointInPolygon(polys[i][0], polys[j]))
                {
                    ++depth[i];
                    if (std::abs(areas[j]) < parentArea)
                    {
                        parentArea = std::abs(areas[j]);
                        parent[i] = static_cast<int>(j);
                    }
                }
            }
        }

        for (size_t i = 0; i < count; ++i)
        {
            const bool hole = depth[i] % 2 == 1;
            if ((areas[i] < 0.0f) != hole)
                std::reverse(polys[i].begin(), polys[i].end());
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (depth[i] % 2 == 1)
                continue;
            std::vector<size_t> holes;
            for (size_t j = 0; j < count; ++j)
                if (depth[j] % 2 == 1 && parent[j] == static_cast<int>(i))
                    holes.push_back(j);
            std::sort(holes.begin(), holes.end(), [&](size_t a, size_t b)
                      {
                float ma = -std::numeric_limits<float>::max(), mb = ma;
                for (const glm::vec2 &p : polys[a]) ma = std::max(ma, p.x);
                for (const glm::vec2 &p : polys[b]) mb = std::max(mb, p.x);
                return ma > mb; });

            Polyline merged = polys[i];
            for (size_t h : holes)
                merged = BridgeHole(merged, polys[h]);

            std::vector<uint32_t> triangles;
            EarClip(merged, triangles);
            const uint32_t base = static_cast<uint32_t>(out.Positions.size());
            for (const glm::vec2 &p : merged)
                AddVertex(out, p, paint.At(p));
            for (uint32_t index : triangles)
                out.Indices.push_back(base + index);
        }

        for (const Polyline &poly : polys)
            AddFillFringe(poly, paint, aa, out);
    }

    void TessellateStroke(const FlatContour &line, const StrokeStyle &style, const VectorPaint &paint, float aa, VectorMesh &out)
    {
        Polyline pts;
        for (const glm::vec2 &p : line.Points)
            if (pts.empty() || glm::length(p - pts.back()) > 1e-5f)
                pts.push_back(p);
        bool closed = line.Closed;
        if (closed && pts.size() > 1 && glm::length(pts.front() - pts.back()) <= 1e-5f)
            pts.pop_back();
        if (closed && pts.size() < 3)
            closed = false;

        const float hw = std::max(style.Width, 0.0f) * 0.5f;
        if (hw <= 0.0f || pts.empty())
            return;

        StrokeBuilder builder{out, paint, hw, std::max(aa, 1e-4f), 1.0f, hw};
        const bool thin = style.Width < aa;
        if (thin)
        {
            builder.Core = 0.0f;
            builder.Alpha = std::clamp(style.Width / std::max(aa, 1e-4f), 0.0f, 1.0f);
        }

        if (pts.size() == 1)
        {
            if (style.Cap == StrokeCap::Round)
                builder.AddArc(pts[0], {1.0f, 0.0f}, {0.999f, -0.001f}, false);
            return;
        }

        const size_t n = pts.size();
        const size_t segments = closed ? n : n - 1;
        std::vector<glm::vec2> dirs(segments), normals(segments);
        for (size_t i = 0; i < segments; ++i)
        {
            dirs[i] = SafeNormalize(pts[(i + 1) % n] - pts[i]);
            normals[i] = {-dirs[i].y, dirs[i].x};
        }

        const float smoothCos = thin ? -2.0f : 0.866f;
        auto isSmooth = [&](size_t v)
        {
            const glm::vec2 &in = dirs[(v + segments - 1) % segments];
            const glm::vec2 &outDir = dirs[v % segments];
            return glm::dot(in, outDir) > smoothCos;
        };
        auto miter = [&](size_t v)
        {
            const glm::vec2 &nIn = normals[(v + segments - 1) % segments];
            const glm::vec2 &nOut = normals[v % segments];
            const glm::vec2 m = SafeNormalize(nIn + nOut, nOut);
            return m / std::max(glm::dot(m, nOut), 0.5f);
        };
        auto join = [&](size_t v)
        {
            const glm::vec2 &p = pts[v];
            const glm::vec2 &inDir = dirs[(v + segments - 1) % segments];
            const glm::vec2 &outDir = dirs[v % segments];
            const glm::vec2 &nIn = normals[(v + segments - 1) % segments];
            const glm::vec2 &nOut = normals[v % segments];
            const float turn = Cross(inDir, outDir);
            const float side = turn > 0.0f ? -1.0f : 1.0f;
            const glm::vec2 a = nIn * side;
            const glm::vec2 b = nOut * side;
            if (style.Join == StrokeJoin::Round)
            {
                builder.AddArc(p, a, b, turn < 0.0f);
                return;
            }
            const glm::vec2 pa = p + a * builder.Core;
            const glm::vec2 pb = p + b * builder.Core;
            if (style.Join == StrokeJoin::Miter)
            {
                const glm::vec2 md = SafeNormalize(a + b, a);
                const float ratio = 1.0f / std::max(glm::dot(md, a), 1e-4f);
                if (ratio <= style.MiterLimit)
                {
                    builder.AddFan(p, {pa, p + md * (builder.Core * ratio), pb});
                    return;
                }
            }
            builder.AddFan(p, {pa, pb});
        };

        if (closed)
        {
            size_t start = n;
            for (size_t v = 0; v < n; ++v)
                if (!isSmooth(v))
                {
                    start = v;
                    break;
                }
            if (start == n)
            {
                for (size_t v = 0; v < n; ++v)
                    builder.AddRow(pts[v], miter(v));
                builder.Connect(builder.Row, builder.FirstRow);
                return;
            }
            builder.AddRow(pts[start], normals[start]);
            for (size_t k = 1; k <= n; ++k)
            {
                const size_t v = (start + k) % n;
                if (k < n && isSmooth(v))
                {
                    builder.AddRow(pts[v], miter(v));
                    continue;
                }
                builder.AddRow(pts[v], normals[(v + segments - 1) % segments]);
                builder.EndStrip();
                join(v);
                if (k < n)
                    builder.AddRow(pts[v], normals[v]);
            }
            return;
        }

        if (style.Cap == StrokeCap::Square)
        {
            pts.front() -= dirs.front() * hw;
            pts.back() += dirs.back() * hw;
        }

        builder.AddRow(pts[0], normals[0]);
        for (size_t v = 1; v + 1 < n; ++v)
        {
            if (isSmooth(v))
            {
                builder.AddRow(pts[v], miter(v));
                continue;
            }
            builder.AddRow(pts[v], normals[v - 1]);
            builder.EndStrip();
            join(v);
            builder.AddRow(pts[v], normals[v]);
        }
        builder.AddRow(pts[n - 1], normals[segments - 1]);
        builder.EndStrip();

        if (style.Cap == StrokeCap::Round && !thin)
        {
            builder.AddArc(pts[0], normals[0], -normals[0], false);
            builder.AddArc(pts[n - 1], -normals[segments - 1], normals[segments - 1], false);
        }
        else
        {
            builder.ButtCap(pts[0], normals[0], -dirs[0]);
            builder.ButtCap(pts[n - 1], normals[segments - 1], dirs[segments - 1]);
        }
    }
}

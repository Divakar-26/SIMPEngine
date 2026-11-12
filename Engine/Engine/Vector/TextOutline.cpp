#include <Engine/Vector/TextOutline.h>

#include <algorithm>

namespace SIMPEngine
{
    namespace
    {
        void AppendGlyph(const std::vector<Font::OutlineVertex> &shape, const glm::vec2 &origin, float scale, VectorPath &out)
        {
            bool open = false;
            for (const Font::OutlineVertex &v : shape)
            {
                const glm::vec2 p = origin + v.Point * scale;
                const glm::vec2 c0 = origin + v.Control0 * scale;
                const glm::vec2 c1 = origin + v.Control1 * scale;
                switch (v.Type)
                {
                case Font::OutlineVertex::Kind::Move:
                    if (open)
                        out.Close();
                    out.MoveTo(p.x, p.y);
                    open = true;
                    break;
                case Font::OutlineVertex::Kind::Line:
                    out.LineTo(p.x, p.y);
                    break;
                case Font::OutlineVertex::Kind::Quad:
                    out.QuadTo(c0.x, c0.y, p.x, p.y);
                    break;
                case Font::OutlineVertex::Kind::Cubic:
                    out.CubicTo(c0.x, c0.y, c1.x, c1.y, p.x, p.y);
                    break;
                }
            }
            if (open)
                out.Close();
        }
    }

    VectorPath TextOutline::GlyphPath(size_t glyph) const
    {
        VectorPath path;
        if (glyph >= Glyphs.size())
            return path;
        const TextOutlineGlyph &g = Glyphs[glyph];
        const auto first = Path.Contours.begin() + g.FirstContour;
        path.Contours.assign(first, first + g.ContourCount);
        return path;
    }

    bool BuildTextOutline(const Font &font, const std::string &text, float size, TextAlign align, TextOutline &out)
    {
        out = TextOutline{};
        if (text.empty() || size <= 0.0f || font.GetBaseSize() <= 0.0f)
            return false;

        const float s = size / font.GetBaseSize();
        float baseline = -font.GetAscent() * s;
        std::vector<Font::OutlineVertex> shape;
        bool any = false;
        int index = 0;
        int line = 0;

        const char *lineStart = text.data();
        const char *end = text.data() + text.size();
        for (const char *lp = lineStart; lp <= end; ++lp)
        {
            if (lp != end && *lp != '\n')
                continue;

            const float lineW = font.MeasureLine(lineStart, lp, size);
            float penX = align == TextAlign::Center ? -lineW * 0.5f : align == TextAlign::Right ? -lineW : 0.0f;

            unsigned int prev = 0;
            const char *p = lineStart;
            while (p < lp)
            {
                const unsigned int cp = DecodeUtf8(p, lp);
                const Font::Glyph *g = font.GetGlyph(cp);
                if (!g)
                {
                    ++index;
                    continue;
                }
                if (prev)
                    penX += font.GetKerning(prev, cp) * s;
                prev = cp;

                const glm::vec2 origin(penX, baseline);
                if (font.GetGlyphOutline(cp, shape) && !shape.empty())
                {
                    VectorPath glyphPath;
                    AppendGlyph(shape, origin, s, glyphPath);
                    glm::vec2 gmin, gmax;
                    if (!glyphPath.Contours.empty() && glyphPath.GetBounds(gmin, gmax))
                    {
                        TextOutlineGlyph info;
                        info.Codepoint = cp;
                        info.Index = index;
                        info.Line = line;
                        info.FirstContour = static_cast<int>(out.Path.Contours.size());
                        info.ContourCount = static_cast<int>(glyphPath.Contours.size());
                        info.Origin = origin;
                        info.Min = gmin;
                        info.Max = gmax;
                        out.Path.Append(glyphPath);
                        out.Glyphs.push_back(info);
                        out.Min = any ? glm::min(out.Min, gmin) : gmin;
                        out.Max = any ? glm::max(out.Max, gmax) : gmax;
                        any = true;
                    }
                }

                penX += g->Advance * s;
                ++index;
            }

            baseline -= font.GetLineHeight() * s;
            lineStart = lp + 1;
            ++line;
            if (lp != end)
                ++index;
        }
        return any;
    }
}

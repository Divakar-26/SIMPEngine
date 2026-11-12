#include "TestFramework.h"

#include <Engine/Rendering/Font.h>
#include <Engine/Rendering/ImageWriter.h>
#include <Engine/Vector/TextOutline.h>
#include <Engine/Vector/VectorTessellator.h>

#include <cmath>
#include <string>
#include <vector>

using namespace SIMPEngine;

namespace
{
    const std::string FontPath = std::string(SIMP_TEST_ASSETS) + "/fonts/Inter-Regular.ttf";

    float MeshArea(const VectorMesh &mesh)
    {
        float area = 0.0f;
        for (size_t i = 0; i + 2 < mesh.Indices.size(); i += 3)
        {
            const glm::vec2 a = mesh.Positions[mesh.Indices[i]];
            const glm::vec2 b = mesh.Positions[mesh.Indices[i + 1]];
            const glm::vec2 c = mesh.Positions[mesh.Indices[i + 2]];
            if (mesh.Colors[mesh.Indices[i]].a < 0.99f || mesh.Colors[mesh.Indices[i + 1]].a < 0.99f || mesh.Colors[mesh.Indices[i + 2]].a < 0.99f)
                continue;
            area += std::abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) * 0.5f;
        }
        return area;
    }
}

TEST_CASE("font: loads without a GL context and bakes glyphs outside Latin-1 on first use")
{
    Font font;
    CHECK(font.LoadFromFile(FontPath));

    const Font::Glyph *question = font.GetGlyph('?');
    CHECK(question != nullptr);
    CHECK(font.HasGlyph(0x03C0));

    const Font::Glyph *pi = font.GetGlyph(0x03C0);
    CHECK(pi != nullptr);
    CHECK(pi != question);
    if (pi)
    {
        CHECK(pi->Width > 0.0f);
        CHECK(pi->U1 > pi->U0);
        CHECK(pi->V1 > pi->V0);
    }

    const Font::Glyph *again = font.GetGlyph(0x03C0);
    CHECK(again == pi);
    CHECK(!font.HasGlyph(0x10FFFD));
    CHECK(font.GetGlyph(0x10FFFD) != nullptr);
}

TEST_CASE("font: the atlas grows when glyphs no longer fit and keeps earlier glyphs in place")
{
    Font font;
    CHECK(font.LoadFromFile(FontPath));
    const Font::Glyph *a = font.GetGlyph('A');
    CHECK(a != nullptr);
    if (!a)
        return;
    const float pixelTop = a->V0 * static_cast<float>(font.GetAtlasHeight());

    for (unsigned int cp = 0x0100; cp < 0x0500; ++cp)
        font.GetGlyph(cp);

    CHECK(font.GetAtlasHeight() > 1024);
    CHECK_NEAR(a->V0 * static_cast<float>(font.GetAtlasHeight()), pixelTop, 0.01);
}

TEST_CASE("text outline: one entry per visible glyph with its own contours and holes")
{
    Font font;
    CHECK(font.LoadFromFile(FontPath));

    TextOutline outline;
    CHECK(BuildTextOutline(font, "o i", 100.0f, TextAlign::Left, outline));
    CHECK(outline.Glyphs.size() == 2);
    if (outline.Glyphs.size() != 2)
        return;
    CHECK(outline.Glyphs[0].Codepoint == 'o');
    CHECK(outline.Glyphs[0].ContourCount == 2);
    CHECK(outline.Glyphs[1].Codepoint == 'i');
    CHECK(outline.Glyphs[1].Index == 2);
    CHECK(outline.Glyphs[1].ContourCount == 2);
    CHECK(outline.Glyphs[1].FirstContour == 2);
    CHECK(outline.Path.Contours.size() == 4);
    CHECK(outline.Glyphs[1].Min.x > outline.Glyphs[0].Max.x);
    CHECK(outline.Max.y <= 0.0f);
    CHECK(outline.Min.y > -100.0f);

    VectorPath o = outline.GlyphPath(0);
    CHECK(o.Contours.size() == 2);
    std::vector<FlatContour> flat;
    o.Flatten(0.1f, flat);
    VectorMesh filled;
    TessellateFill(flat, VectorPaint{}, 0.0f, filled);
    CHECK(!filled.Empty());
    const glm::vec2 size = outline.Glyphs[0].Max - outline.Glyphs[0].Min;
    const float ellipse = 3.14159265f * size.x * size.y * 0.25f;
    CHECK(MeshArea(filled) < ellipse * 0.75f);
    CHECK(MeshArea(filled) > ellipse * 0.2f);
}

TEST_CASE("text outline: alignment and lines match Draw.Text layout")
{
    Font font;
    CHECK(font.LoadFromFile(FontPath));

    TextOutline centered;
    CHECK(BuildTextOutline(font, "HH", 80.0f, TextAlign::Center, centered));
    CHECK_NEAR(centered.Min.x + centered.Max.x, 0.0, 4.0);

    TextOutline right;
    CHECK(BuildTextOutline(font, "HH", 80.0f, TextAlign::Right, right));
    CHECK(right.Max.x <= 0.5f);

    TextOutline twoLines;
    CHECK(BuildTextOutline(font, "H\nH", 80.0f, TextAlign::Left, twoLines));
    CHECK(twoLines.Glyphs.size() == 2);
    if (twoLines.Glyphs.size() == 2)
    {
        CHECK(twoLines.Glyphs[1].Line == 1);
        CHECK(twoLines.Glyphs[1].Index == 2);
        CHECK_NEAR(twoLines.Glyphs[0].Origin.y - twoLines.Glyphs[1].Origin.y, font.GetLineHeight() * 80.0f / font.GetBaseSize(), 0.01);
    }

    TextOutline greek;
    CHECK(BuildTextOutline(font, "\xCE\xB1+\xCF\x80", 64.0f, TextAlign::Left, greek));
    CHECK(greek.Glyphs.size() == 3);

    TextOutline blank;
    CHECK(!BuildTextOutline(font, "   ", 64.0f, TextAlign::Left, blank));
}

TEST_CASE("image writer: encodes RGBA pixels as PNG")
{
    std::vector<uint8_t> pixels(4 * 3 * 4, 0);
    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i] = static_cast<uint8_t>(i);
        pixels[i + 3] = 255;
    }
    std::string png;
    CHECK(ImageWriter::EncodePng(4, 3, pixels.data(), png));
    CHECK(png.size() > 8);
    CHECK(png.compare(0, 4, "\x89PNG") == 0);
    CHECK(!ImageWriter::EncodePng(0, 3, pixels.data(), png));
    CHECK(!ImageWriter::WritePng("unused.png", 8, 8, pixels));
}

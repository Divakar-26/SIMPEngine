#pragma once

#include <Engine/Rendering/Font.h>
#include <Engine/Vector/VectorPath.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace SIMPEngine
{
    struct TextOutlineGlyph
    {
        unsigned int Codepoint = 0;
        int Index = 0;
        int Line = 0;
        int FirstContour = 0;
        int ContourCount = 0;
        glm::vec2 Origin{0.0f};
        glm::vec2 Min{0.0f};
        glm::vec2 Max{0.0f};
    };

    struct TextOutline
    {
        VectorPath Path;
        std::vector<TextOutlineGlyph> Glyphs;
        glm::vec2 Min{0.0f};
        glm::vec2 Max{0.0f};

        VectorPath GlyphPath(size_t glyph) const;
    };

    bool BuildTextOutline(const Font &font, const std::string &text, float size, TextAlign align, TextOutline &out);
}

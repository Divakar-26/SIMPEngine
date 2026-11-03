#include <Engine/Rendering/Font.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

#include <glad/glad.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <algorithm>
#include <cstring>
#include <fstream>

namespace SIMPEngine
{
    std::unordered_map<std::string, std::shared_ptr<Font>> Font::s_Cache;

    namespace
    {
        constexpr int AtlasWidth = 2048;
        constexpr int AtlasHeight = 1024;
        constexpr int SdfPadding = 8;
        constexpr unsigned char OnEdge = 128;
        constexpr float DistScale = 128.0f / SdfPadding;
    }

    unsigned int DecodeUtf8(const char *&p, const char *end)
    {
        unsigned char c = (unsigned char)*p++;
        if (c < 0x80)
            return c;

        int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
        unsigned int cp = c & (0x3F >> extra);
        for (int i = 0; i < extra && p < end; ++i)
            cp = (cp << 6) | ((unsigned char)*p++ & 0x3F);
        return cp;
    }

    Font::~Font()
    {
        if (m_Texture)
            glDeleteTextures(1, &m_Texture);
        delete static_cast<stbtt_fontinfo *>(m_Info);
    }

    bool Font::LoadFromFile(const std::string &path)
    {
        std::string real = path;
        if (auto resolved = VFS::Resolve(path))
            real = *resolved;

        std::ifstream file(real, std::ios::binary);
        if (!file)
        {
            CORE_ERROR("Font: cannot open {}", real);
            return false;
        }
        m_FontData.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        m_Path = path;

        auto *info = new stbtt_fontinfo();
        m_Info = info;
        if (!stbtt_InitFont(info, m_FontData.data(), stbtt_GetFontOffsetForIndex(m_FontData.data(), 0)))
        {
            CORE_ERROR("Font: invalid font file {}", real);
            return false;
        }

        m_Scale = stbtt_ScaleForPixelHeight(info, m_BaseSize);
        int ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);
        m_Ascent = ascent * m_Scale;
        m_LineHeight = (ascent - descent + lineGap) * m_Scale;

        std::vector<unsigned char> atlas(AtlasWidth * AtlasHeight, 0);
        int penX = 1, penY = 1, rowH = 0;

        auto bake = [&](unsigned int cp)
        {
            int glyphIndex = stbtt_FindGlyphIndex(info, (int)cp);
            if (glyphIndex == 0 && cp != ' ')
                return;

            int advance = 0, lsb = 0;
            stbtt_GetGlyphHMetrics(info, glyphIndex, &advance, &lsb);

            Glyph g;
            g.Advance = advance * m_Scale;

            int w = 0, h = 0, xoff = 0, yoff = 0;
            unsigned char *sdf = stbtt_GetGlyphSDF(info, m_Scale, glyphIndex, SdfPadding, OnEdge, DistScale, &w, &h, &xoff, &yoff);
            if (sdf && w > 0 && h > 0)
            {
                if (penX + w + 1 >= AtlasWidth)
                {
                    penX = 1;
                    penY += rowH + 1;
                    rowH = 0;
                }
                if (penY + h + 1 < AtlasHeight)
                {
                    for (int y = 0; y < h; ++y)
                        std::memcpy(&atlas[(penY + y) * AtlasWidth + penX], &sdf[y * w], w);

                    g.U0 = (float)penX / AtlasWidth;
                    g.V0 = (float)penY / AtlasHeight;
                    g.U1 = (float)(penX + w) / AtlasWidth;
                    g.V1 = (float)(penY + h) / AtlasHeight;
                    g.Width = (float)w;
                    g.Height = (float)h;
                    g.OffsetX = (float)xoff;
                    g.OffsetY = (float)yoff;

                    penX += w + 1;
                    rowH = std::max(rowH, h);
                }
            }
            if (sdf)
                stbtt_FreeSDF(sdf, nullptr);

            m_Glyphs[cp] = g;
        };

        for (unsigned int cp = 32; cp < 127; ++cp)
            bake(cp);
        for (unsigned int cp = 160; cp < 256; ++cp)
            bake(cp);
        bake(0x2026);
        bake(0x2022);

        glGenTextures(1, &m_Texture);
        glBindTexture(GL_TEXTURE_2D, m_Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasWidth, AtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        GLint swizzle[4] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        return true;
    }

    std::shared_ptr<Font> Font::Load(const std::string &path)
    {
        auto it = s_Cache.find(path);
        if (it != s_Cache.end())
            return it->second;

        auto font = std::make_shared<Font>();
        if (!font->LoadFromFile(path))
            font = nullptr;
        s_Cache[path] = font;
        return font;
    }

    std::shared_ptr<Font> Font::GetDefault()
    {
        static const char *candidates[] = {
            "assets://fonts/Inter-SemiBold.ttf",
            "assets://fonts/Inter-Regular.ttf",
            "assets://fonts/JetBrainsMonoNerdFont-Regular.ttf"};

        for (const char *c : candidates)
        {
            auto real = VFS::Resolve(c);
            if (!real)
                continue;
            std::ifstream probe(*real);
            if (!probe)
                continue;
            if (auto font = Load(c))
                return font;
        }
        return nullptr;
    }

    void Font::ClearCache()
    {
        s_Cache.clear();
    }

    const Font::Glyph *Font::GetGlyph(unsigned int codepoint) const
    {
        auto it = m_Glyphs.find(codepoint);
        if (it != m_Glyphs.end())
            return &it->second;
        it = m_Glyphs.find('?');
        return it != m_Glyphs.end() ? &it->second : nullptr;
    }

    float Font::GetKerning(unsigned int left, unsigned int right) const
    {
        if (!m_Info)
            return 0.0f;
        return stbtt_GetCodepointKernAdvance(static_cast<stbtt_fontinfo *>(m_Info), (int)left, (int)right) * m_Scale;
    }

    float Font::MeasureLine(const char *begin, const char *end, float size) const
    {
        float s = size / m_BaseSize;
        float w = 0.0f;
        unsigned int prev = 0;
        const char *p = begin;
        while (p < end)
        {
            unsigned int cp = DecodeUtf8(p, end);
            if (const Glyph *g = GetGlyph(cp))
            {
                if (prev)
                    w += GetKerning(prev, cp) * s;
                w += g->Advance * s;
            }
            prev = cp;
        }
        return w;
    }

    glm::vec2 Font::MeasureText(const std::string &text, float size) const
    {
        float s = size / m_BaseSize;
        float maxW = 0.0f;
        int lines = 1;
        const char *start = text.data();
        const char *end = text.data() + text.size();
        for (const char *p = start; p <= end; ++p)
        {
            if (p == end || *p == '\n')
            {
                maxW = std::max(maxW, MeasureLine(start, p, size));
                if (p != end)
                    ++lines;
                start = p + 1;
            }
        }
        return {maxW, lines * m_LineHeight * s};
    }
}

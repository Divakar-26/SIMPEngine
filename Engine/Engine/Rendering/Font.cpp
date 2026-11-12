#include <Engine/Rendering/Font.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

#include <Engine/Rendering/GL.h>

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
        constexpr int InitialAtlasHeight = 1024;
        constexpr int MaxAtlasHeight = 4096;
        constexpr int SdfPadding = 8;
        constexpr unsigned char OnEdge = 128;
        constexpr float DistScale = 128.0f / static_cast<float>(SdfPadding);
    }

    unsigned int DecodeUtf8(const char *&p, const char *end)
    {
        unsigned char c = static_cast<unsigned char>(*p++);
        if (c < 0x80)
            return c;

        int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
        unsigned int cp = static_cast<unsigned int>(c & (0x3F >> extra));
        for (int i = 0; i < extra && p < end; ++i)
            cp = (cp << 6) | static_cast<unsigned int>(static_cast<unsigned char>(*p++) & 0x3F);
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
        m_Ascent = static_cast<float>(ascent) * m_Scale;
        m_LineHeight = static_cast<float>(ascent - descent + lineGap) * m_Scale;

        m_AtlasHeight = InitialAtlasHeight;
        m_Atlas.assign(static_cast<size_t>(AtlasWidth) * static_cast<size_t>(m_AtlasHeight), 0);
        m_PenX = 1;
        m_PenY = 1;
        m_RowHeight = 0;

        for (unsigned int cp = 32; cp < 127; ++cp)
            Bake(cp);
        for (unsigned int cp = 160; cp < 256; ++cp)
            Bake(cp);
        Bake(0x2026);
        Bake(0x2022);
        Bake(0xFFFD);
        m_AtlasDirty = true;

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
            "engine://fonts/Inter-SemiBold.ttf",
            "engine://fonts/Inter-Regular.ttf",
            "assets://fonts/Inter-SemiBold.ttf",
            "assets://fonts/Inter-Regular.ttf"};

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

    bool Font::GrowAtlas() const
    {
        if (m_AtlasHeight >= MaxAtlasHeight)
            return false;
        const int newHeight = std::min(m_AtlasHeight * 2, MaxAtlasHeight);
        const float ratio = static_cast<float>(m_AtlasHeight) / static_cast<float>(newHeight);
        m_Atlas.resize(static_cast<size_t>(AtlasWidth) * static_cast<size_t>(newHeight), 0);
        for (auto &[cp, glyph] : m_Glyphs)
        {
            glyph.V0 *= ratio;
            glyph.V1 *= ratio;
        }
        m_AtlasHeight = newHeight;
        m_AtlasDirty = true;
        return true;
    }

    const Font::Glyph *Font::Bake(unsigned int codepoint) const
    {
        auto *info = static_cast<stbtt_fontinfo *>(m_Info);
        if (!info)
            return nullptr;

        const int glyphIndex = stbtt_FindGlyphIndex(info, static_cast<int>(codepoint));
        if (glyphIndex == 0 && codepoint != ' ')
        {
            m_Missing.insert(codepoint);
            return nullptr;
        }

        int advance = 0, lsb = 0;
        stbtt_GetGlyphHMetrics(info, glyphIndex, &advance, &lsb);

        Glyph g;
        g.Advance = static_cast<float>(advance) * m_Scale;

        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char *sdf = stbtt_GetGlyphSDF(info, m_Scale, glyphIndex, SdfPadding, OnEdge, DistScale, &w, &h, &xoff, &yoff);
        if (sdf && w > 0 && h > 0 && w + 2 < AtlasWidth)
        {
            if (m_PenX + w + 1 >= AtlasWidth)
            {
                m_PenX = 1;
                m_PenY += m_RowHeight + 1;
                m_RowHeight = 0;
            }
            while (m_PenY + h + 1 >= m_AtlasHeight && GrowAtlas())
            {
            }
            if (m_PenY + h + 1 < m_AtlasHeight)
            {
                for (int y = 0; y < h; ++y)
                    std::memcpy(&m_Atlas[static_cast<size_t>((m_PenY + y) * AtlasWidth + m_PenX)], &sdf[y * w], static_cast<size_t>(w));

                g.U0 = static_cast<float>(m_PenX) / static_cast<float>(AtlasWidth);
                g.V0 = static_cast<float>(m_PenY) / static_cast<float>(m_AtlasHeight);
                g.U1 = static_cast<float>(m_PenX + w) / static_cast<float>(AtlasWidth);
                g.V1 = static_cast<float>(m_PenY + h) / static_cast<float>(m_AtlasHeight);
                g.Width = static_cast<float>(w);
                g.Height = static_cast<float>(h);
                g.OffsetX = static_cast<float>(xoff);
                g.OffsetY = static_cast<float>(yoff);

                m_PenX += w + 1;
                m_RowHeight = std::max(m_RowHeight, h);
                m_AtlasDirty = true;
            }
            else
                CORE_WARN("Font: atlas of {} is full, U+{:04X} is drawn blank", m_Path, codepoint);
        }
        if (sdf)
            stbtt_FreeSDF(sdf, nullptr);

        auto [it, inserted] = m_Glyphs.insert_or_assign(codepoint, g);
        (void)inserted;
        return &it->second;
    }

    unsigned int Font::GetTextureID() const
    {
        if (!m_AtlasDirty || m_Atlas.empty())
            return m_Texture;

        if (!m_Texture)
        {
            glGenTextures(1, &m_Texture);
            m_TextureHeight = 0;
        }
        glBindTexture(GL_TEXTURE_2D, m_Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (m_TextureHeight != m_AtlasHeight)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasWidth, m_AtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, m_Atlas.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            m_TextureHeight = m_AtlasHeight;
        }
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AtlasWidth, m_AtlasHeight, GL_RED, GL_UNSIGNED_BYTE, m_Atlas.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_AtlasDirty = false;
        return m_Texture;
    }

    const Font::Glyph *Font::GetGlyph(unsigned int codepoint) const
    {
        auto it = m_Glyphs.find(codepoint);
        if (it != m_Glyphs.end())
            return &it->second;
        if (!m_Missing.count(codepoint))
        {
            if (const Glyph *baked = Bake(codepoint))
                return baked;
        }
        for (unsigned int fallback : {0xFFFDu, static_cast<unsigned int>('?')})
        {
            it = m_Glyphs.find(fallback);
            if (it != m_Glyphs.end())
                return &it->second;
        }
        return nullptr;
    }

    bool Font::HasGlyph(unsigned int codepoint) const
    {
        auto *info = static_cast<stbtt_fontinfo *>(m_Info);
        return info && (codepoint == ' ' || stbtt_FindGlyphIndex(info, static_cast<int>(codepoint)) != 0);
    }

    bool Font::GetGlyphOutline(unsigned int codepoint, std::vector<OutlineVertex> &out) const
    {
        out.clear();
        auto *info = static_cast<stbtt_fontinfo *>(m_Info);
        if (!info)
            return false;
        int glyphIndex = stbtt_FindGlyphIndex(info, static_cast<int>(codepoint));
        if (glyphIndex == 0)
        {
            glyphIndex = stbtt_FindGlyphIndex(info, 0xFFFD);
            if (glyphIndex == 0)
                glyphIndex = stbtt_FindGlyphIndex(info, '?');
            if (glyphIndex == 0)
                return false;
        }

        stbtt_vertex *vertices = nullptr;
        const int count = stbtt_GetGlyphShape(info, glyphIndex, &vertices);
        out.reserve(static_cast<size_t>(std::max(count, 0)));
        for (int i = 0; i < count; ++i)
        {
            const stbtt_vertex &v = vertices[i];
            OutlineVertex o;
            o.Point = glm::vec2(static_cast<float>(v.x), static_cast<float>(v.y)) * m_Scale;
            o.Control0 = glm::vec2(static_cast<float>(v.cx), static_cast<float>(v.cy)) * m_Scale;
            o.Control1 = glm::vec2(static_cast<float>(v.cx1), static_cast<float>(v.cy1)) * m_Scale;
            switch (v.type)
            {
            case STBTT_vmove:
                o.Type = OutlineVertex::Kind::Move;
                break;
            case STBTT_vline:
                o.Type = OutlineVertex::Kind::Line;
                break;
            case STBTT_vcurve:
                o.Type = OutlineVertex::Kind::Quad;
                break;
            default:
                o.Type = OutlineVertex::Kind::Cubic;
                break;
            }
            out.push_back(o);
        }
        if (vertices)
            stbtt_FreeShape(info, vertices);
        return true;
    }

    float Font::GetKerning(unsigned int left, unsigned int right) const
    {
        if (!m_Info)
            return 0.0f;
        return static_cast<float>(stbtt_GetCodepointKernAdvance(static_cast<stbtt_fontinfo *>(m_Info), static_cast<int>(left), static_cast<int>(right))) * m_Scale;
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
        return {maxW, static_cast<float>(lines) * m_LineHeight * s};
    }
}

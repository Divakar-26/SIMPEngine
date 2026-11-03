#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    enum class TextAlign
    {
        Left,
        Center,
        Right
    };

    class Font
    {
    public:
        struct Glyph
        {
            float U0 = 0, V0 = 0, U1 = 0, V1 = 0;
            float Width = 0, Height = 0;
            float OffsetX = 0, OffsetY = 0;
            float Advance = 0;
        };

        Font() = default;
        ~Font();

        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;

        bool LoadFromFile(const std::string &path);

        static std::shared_ptr<Font> Load(const std::string &path);
        static std::shared_ptr<Font> GetDefault();
        static void ClearCache();

        const Glyph *GetGlyph(unsigned int codepoint) const;
        float GetKerning(unsigned int left, unsigned int right) const;

        glm::vec2 MeasureText(const std::string &text, float size) const;
        float MeasureLine(const char *begin, const char *end, float size) const;

        float GetBaseSize() const { return m_BaseSize; }
        float GetAscent() const { return m_Ascent; }
        float GetLineHeight() const { return m_LineHeight; }
        unsigned int GetTextureID() const { return m_Texture; }
        const std::string &GetPath() const { return m_Path; }

    private:
        std::string m_Path;
        std::vector<unsigned char> m_FontData;
        void *m_Info = nullptr;

        std::unordered_map<unsigned int, Glyph> m_Glyphs;
        unsigned int m_Texture = 0;

        float m_BaseSize = 64.0f;
        float m_Scale = 1.0f;
        float m_Ascent = 0.0f;
        float m_LineHeight = 0.0f;

        static std::unordered_map<std::string, std::shared_ptr<Font>> s_Cache;
    };

    unsigned int DecodeUtf8(const char *&p, const char *end);
}

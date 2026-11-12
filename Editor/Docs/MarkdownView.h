#pragma once

#include "Markdown.h"

#include <imgui.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    class Texture;
}

class MarkdownView
{
public:
    void SetDocument(Markdown::Document document, const std::string &baseDirectory);
    const Markdown::Document &GetDocument() const { return m_Document; }

    void ScrollToAnchor(const std::string &anchor);
    void ScrollToTop();
    void SetScale(float scale);
    float GetScale() const { return m_Scale; }

    std::string Draw();

private:
    enum class Tone
    {
        Plain,
        Keyword,
        String,
        Number,
        Comment,
        Function
    };

    struct Token
    {
        float X = 0.0f;
        float Y = 0.0f;
        float W = 0.0f;
        int Style = 0;
        std::string Text;
        std::string Url;
    };

    struct Segment
    {
        std::string Text;
        Tone Kind = Tone::Plain;
    };

    struct Cell
    {
        float X = 0.0f;
        float Width = 0.0f;
        std::vector<Token> Tokens;
    };

    struct Laid
    {
        float Top = 0.0f;
        float Height = 0.0f;
        float Indent = 0.0f;
        float Size = 15.0f;
        float LineHeight = 20.0f;
        std::vector<Token> Tokens;
        std::vector<std::vector<Cell>> Rows;
        std::vector<float> RowTop;
        std::vector<float> RowHeight;
        std::vector<float> ColumnX;
        std::vector<float> ColumnWidth;
        std::vector<std::vector<Segment>> Lines;
        float CodeWidth = 0.0f;
        std::string Bullet;
        std::string ImageUrl;
        std::string ImageAlt;
        std::shared_ptr<SIMPEngine::Texture> Image;
        ImVec2 ImageSize{0.0f, 0.0f};
    };

    void Layout(float width);
    float LayoutSpans(const std::vector<Markdown::Span> &spans, float x0, float width, float size, std::vector<Token> &out, float lineHeight);
    void Highlight(const std::string &lang, const std::string &code, std::vector<std::vector<Segment>> &lines, float size, float &maxWidth);
    void DrawTokens(ImDrawList *list, const ImVec2 &origin, const std::vector<Token> &tokens, float size, float lineHeight, ImU32 color, std::string &clicked);

    Markdown::Document m_Document;
    std::string m_BaseDirectory;
    std::vector<Laid> m_Laid;
    float m_Width = -1.0f;
    float m_Total = 0.0f;
    float m_Scale = 1.0f;
    bool m_Dirty = true;
    std::string m_PendingAnchor;
    bool m_PendingTop = false;
    std::unordered_map<size_t, float> m_CodeScroll;
};

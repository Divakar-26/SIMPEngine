#include "MarkdownView.h"

#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <imgui_internal.h>

#include <Engine/Core/FileSystem.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/TextureManager.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <set>

namespace
{
    using Markdown::Span;

    ImFont *FontFor(int style)
    {
        if (style & Markdown::StyleCode)
            return EditorUI::GetFont(EditorUI::FontRole::Mono);
        if (style & Markdown::StyleBold)
            return EditorUI::GetFont(EditorUI::FontRole::Strong);
        return EditorUI::GetFont(EditorUI::FontRole::Regular);
    }

    float SizeFor(int style, float size)
    {
        return (style & Markdown::StyleCode) ? size * 0.92f : size;
    }

    float Measure(const std::string &text, int style, float size)
    {
        ImFont *font = FontFor(style);
        const float s = SizeFor(style, size);
        return font->CalcTextSizeA(s, 100000.0f, 0.0f, text.c_str(), text.c_str() + text.size()).x;
    }

    bool IsDarkTheme()
    {
        const ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        return (bg.x + bg.y + bg.z) / 3.0f < 0.5f;
    }

    ImU32 ToneColor(int tone, bool dark)
    {
        static const ImU32 darkColors[] = {0, IM_COL32(197, 154, 255, 255), IM_COL32(152, 214, 136, 255), IM_COL32(255, 170, 110, 255), IM_COL32(130, 142, 170, 255), IM_COL32(110, 200, 255, 255)};
        static const ImU32 lightColors[] = {0, IM_COL32(120, 60, 190, 255), IM_COL32(30, 120, 40, 255), IM_COL32(190, 90, 20, 255), IM_COL32(120, 128, 140, 255), IM_COL32(10, 100, 160, 255)};
        return dark ? darkColors[tone] : lightColors[tone];
    }

    std::string NormalLang(std::string lang)
    {
        std::transform(lang.begin(), lang.end(), lang.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        size_t space = lang.find_first_of(" {");
        if (space != std::string::npos)
            lang = lang.substr(0, space);
        if (lang == "c++" || lang == "c" || lang == "h" || lang == "hpp" || lang == "glsl" || lang == "cc" || lang == "js" || lang == "java" || lang == "cs")
            return "cpp";
        if (lang == "yml")
            return "yaml";
        if (lang == "bash" || lang == "sh" || lang == "shell" || lang == "cmake" || lang == "console")
            return "sh";
        return lang;
    }

    const std::set<std::string> &Keywords(const std::string &lang)
    {
        static const std::set<std::string> lua = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while", "self"};
        static const std::set<std::string> cpp = {"auto", "bool", "break", "case", "catch", "char", "class", "const", "constexpr", "continue", "default", "delete", "do", "double", "else", "enum", "explicit", "false", "float", "for", "if", "include", "inline", "int", "namespace", "new", "nullptr", "operator", "override", "private", "protected", "public", "return", "short", "static", "struct", "switch", "template", "this", "throw", "true", "try", "typedef", "typename", "uint", "uniform", "unsigned", "using", "vec2", "vec3", "vec4", "virtual", "void", "while", "in", "out", "sampler2D", "std"};
        static const std::set<std::string> yaml = {"true", "false", "null"};
        static const std::set<std::string> sh = {"if", "then", "else", "fi", "for", "do", "done", "cmake", "cpack", "ctest", "ninja", "export", "echo", "cd", "target_link_libraries", "add_executable", "add_library", "find_package", "set", "install", "project"};
        static const std::set<std::string> none;
        if (lang == "lua")
            return lua;
        if (lang == "cpp")
            return cpp;
        if (lang == "yaml")
            return yaml;
        if (lang == "sh")
            return sh;
        return none;
    }
}

void MarkdownView::SetDocument(Markdown::Document document, const std::string &baseDirectory)
{
    m_Document = std::move(document);
    m_BaseDirectory = baseDirectory;
    m_Laid.clear();
    m_CodeScroll.clear();
    m_Dirty = true;
    m_PendingTop = true;
}

void MarkdownView::ScrollToAnchor(const std::string &anchor)
{
    m_PendingAnchor = anchor;
}

void MarkdownView::ScrollToTop()
{
    m_PendingTop = true;
}

void MarkdownView::SetScale(float scale)
{
    m_Scale = std::clamp(scale, 0.7f, 1.8f);
    m_Dirty = true;
}

float MarkdownView::LayoutSpans(const std::vector<Span> &spans, float x0, float width, float size, std::vector<Token> &out, float lineHeight)
{
    float x = 0.0f;
    float y = 0.0f;
    float pendingSpace = 0.0f;
    bool lineHasContent = false;
    const float spaceWidth = Measure(" ", 0, size);

    auto place = [&](const std::string &word, int style, const std::string &url)
    {
        const float w = Measure(word, style, size);
        if (lineHasContent && x + pendingSpace + w > width)
        {
            x = 0.0f;
            y += lineHeight;
            pendingSpace = 0.0f;
            lineHasContent = false;
        }
        if (lineHasContent)
            x += pendingSpace;
        Token token;
        token.X = x0 + x;
        token.Y = y;
        token.W = w;
        token.Style = style;
        token.Text = word;
        token.Url = url;
        out.push_back(std::move(token));
        x += w;
        pendingSpace = 0.0f;
        lineHasContent = true;
    };

    for (const Span &span : spans)
    {
        int style = span.Style;
        std::string text = span.Text;
        if (style & Markdown::StyleImage)
        {
            text = "[image: " + span.Text + "]";
            style = (style & ~Markdown::StyleImage) | Markdown::StyleItalic;
        }

        const bool code = (style & Markdown::StyleCode) != 0;
        if (code)
            pendingSpace += 3.0f;
        std::string word;
        auto flushWord = [&]()
        {
            if (!word.empty())
            {
                place(word, style, span.Url);
                word.clear();
            }
        };
        for (size_t i = 0; i < text.size(); ++i)
        {
            const char c = text[i];
            if (c == '\n')
            {
                flushWord();
                x = 0.0f;
                y += lineHeight;
                pendingSpace = 0.0f;
                lineHasContent = false;
            }
            else if (c == ' ' || c == '\t')
            {
                flushWord();
                pendingSpace += code ? Measure(" ", style, size) : spaceWidth;
            }
            else
                word += c;
        }
        flushWord();
        if (code)
            pendingSpace += 3.0f;
    }
    return y + lineHeight;
}

void MarkdownView::Highlight(const std::string &langName, const std::string &code, std::vector<std::vector<Segment>> &lines, float size, float &maxWidth)
{
    const std::string lang = NormalLang(langName);
    const std::set<std::string> &keywords = Keywords(lang);
    bool blockComment = false;

    size_t start = 0;
    while (start <= code.size())
    {
        size_t end = code.find('\n', start);
        if (end == std::string::npos)
            end = code.size();
        const std::string line = code.substr(start, end - start);
        std::vector<Segment> segments;
        auto push = [&](const std::string &text, Tone kind)
        {
            if (text.empty())
                return;
            if (!segments.empty() && segments.back().Kind == kind)
                segments.back().Text += text;
            else
                segments.push_back({text, kind});
        };

        size_t i = 0;
        while (i < line.size())
        {
            const char c = line[i];
            if (blockComment)
            {
                const size_t close = line.find(lang == "lua" ? "]]" : "*/", i);
                if (close == std::string::npos)
                {
                    push(line.substr(i), Tone::Comment);
                    i = line.size();
                }
                else
                {
                    push(line.substr(i, close + 2 - i), Tone::Comment);
                    i = close + 2;
                    blockComment = false;
                }
                continue;
            }
            const bool lineComment = (lang == "lua" && line.compare(i, 2, "--") == 0) || (lang == "cpp" && line.compare(i, 2, "//") == 0) ||
                                     ((lang == "yaml" || lang == "sh") && c == '#');
            if (lineComment)
            {
                if (lang == "lua" && line.compare(i, 4, "--[[") == 0)
                {
                    blockComment = true;
                    push("--[[", Tone::Comment);
                    i += 4;
                    continue;
                }
                push(line.substr(i), Tone::Comment);
                break;
            }
            if (lang == "cpp" && line.compare(i, 2, "/*") == 0)
            {
                blockComment = true;
                push("/*", Tone::Comment);
                i += 2;
                continue;
            }
            if (c == '"' || c == '\'' || (c == '`' && lang == "cpp"))
            {
                size_t j = i + 1;
                while (j < line.size() && line[j] != c)
                    j += line[j] == '\\' ? 2 : 1;
                j = std::min(j + 1, line.size());
                push(line.substr(i, j - i), Tone::String);
                i = j;
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && i + 1 < line.size() && std::isdigit(static_cast<unsigned char>(line[i + 1]))))
            {
                size_t j = i;
                while (j < line.size() && (std::isalnum(static_cast<unsigned char>(line[j])) || line[j] == '.'))
                    ++j;
                push(line.substr(i, j - i), Tone::Number);
                i = j;
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
            {
                size_t j = i;
                while (j < line.size() && (std::isalnum(static_cast<unsigned char>(line[j])) || line[j] == '_'))
                    ++j;
                const std::string word = line.substr(i, j - i);
                size_t k = j;
                while (k < line.size() && line[k] == ' ')
                    ++k;
                Tone kind = Tone::Plain;
                if (keywords.count(word))
                    kind = Tone::Keyword;
                else if (lang == "yaml" && k < line.size() && line[k] == ':')
                    kind = Tone::Function;
                else if ((lang == "lua" || lang == "cpp") && j < line.size() && (line[j] == '(' || line[j] == '{' || line[j] == '"') && (lang != "lua" || line[j] == '('))
                    kind = Tone::Function;
                push(word, kind);
                i = j;
                continue;
            }
            push(std::string(1, c), Tone::Plain);
            ++i;
        }
        float width = 0.0f;
        for (const Segment &segment : segments)
            width += Measure(segment.Text, Markdown::StyleCode, size);
        maxWidth = std::max(maxWidth, width);
        lines.push_back(std::move(segments));
        if (end >= code.size())
            break;
        start = end + 1;
    }
}

void MarkdownView::Layout(float width)
{
    m_Laid.clear();
    m_Laid.reserve(m_Document.Blocks.size());
    const float base = 15.0f * m_Scale;
    float y = 0.0f;
    float previousGap = 0.0f;

    for (size_t index = 0; index < m_Document.Blocks.size(); ++index)
    {
        const Markdown::Block &block = m_Document.Blocks[index];
        Laid laid;
        laid.Size = base;
        laid.LineHeight = base * 1.5f;
        float before = 0.0f;
        float after = 10.0f * m_Scale;

        switch (block.Type)
        {
        case Markdown::BlockType::Heading:
        {
            static const float scales[] = {0.0f, 1.9f, 1.5f, 1.25f, 1.12f, 1.0f, 1.0f};
            laid.Size = base * scales[std::clamp(block.Level, 1, 6)];
            laid.LineHeight = laid.Size * 1.3f;
            before = (block.Level == 1 ? 8.0f : block.Level == 2 ? 22.0f : 16.0f) * m_Scale;
            after = (block.Level <= 2 ? 12.0f : 6.0f) * m_Scale;
            std::vector<Span> spans = block.Spans;
            for (Span &span : spans)
                span.Style |= Markdown::StyleBold;
            laid.Height = LayoutSpans(spans, 0.0f, width, laid.Size, laid.Tokens, laid.LineHeight);
            if (block.Level <= 2)
                laid.Height += 8.0f * m_Scale;
            break;
        }
        case Markdown::BlockType::Paragraph:
        {
            if (block.Spans.size() == 1 && (block.Spans[0].Style & Markdown::StyleImage))
            {
                laid.ImageUrl = block.Spans[0].Url;
                laid.ImageAlt = block.Spans[0].Text;
                std::filesystem::path path = std::filesystem::path(m_BaseDirectory) / laid.ImageUrl;
                std::error_code ec;
                std::string ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
                               { return static_cast<char>(std::tolower(c)); });
                if ((ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") && std::filesystem::is_regular_file(path, ec))
                {
                    laid.Image = SIMPEngine::TextureManager::Get().LoadTexture("doc:" + path.string(), path.string());
                    if (laid.Image && laid.Image->GetWidth() > 0)
                    {
                        const float w = static_cast<float>(laid.Image->GetWidth());
                        const float h = static_cast<float>(laid.Image->GetHeight());
                        const float scale = std::min(1.0f, std::min(width, 720.0f * m_Scale) / w);
                        laid.ImageSize = ImVec2(w * scale, h * scale);
                    }
                    else
                        laid.Image.reset();
                }
                if (laid.Image)
                {
                    laid.Height = laid.ImageSize.y;
                    break;
                }
            }
            laid.Height = LayoutSpans(block.Spans, 0.0f, width, laid.Size, laid.Tokens, laid.LineHeight);
            break;
        }
        case Markdown::BlockType::ListItem:
        {
            laid.Indent = (22.0f + block.Level * 24.0f) * m_Scale;
            laid.Bullet = !block.Anchor.empty() ? block.Anchor : block.Ordered ? std::to_string(block.Number) + "." : (block.Level % 2 == 0 ? "\xE2\x80\xA2" : "-");
            laid.Height = LayoutSpans(block.Spans, laid.Indent, width, laid.Size, laid.Tokens, laid.LineHeight);
            after = 3.0f * m_Scale;
            const bool prevItem = index > 0 && m_Document.Blocks[index - 1].Type == Markdown::BlockType::ListItem;
            before = prevItem ? 0.0f : 2.0f * m_Scale;
            if (index + 1 < m_Document.Blocks.size() && m_Document.Blocks[index + 1].Type != Markdown::BlockType::ListItem)
                after = 12.0f * m_Scale;
            break;
        }
        case Markdown::BlockType::Quote:
        {
            laid.Indent = 18.0f * m_Scale;
            laid.Height = LayoutSpans(block.Spans, laid.Indent, width - 8.0f, laid.Size, laid.Tokens, laid.LineHeight) + 12.0f * m_Scale;
            for (Token &token : laid.Tokens)
                token.Y += 6.0f * m_Scale;
            break;
        }
        case Markdown::BlockType::Code:
        {
            laid.Size = base * 0.92f;
            laid.LineHeight = laid.Size * 1.4f;
            laid.CodeWidth = 0.0f;
            Highlight(block.Lang, block.Code, laid.Lines, laid.Size, laid.CodeWidth);
            laid.Height = laid.Lines.size() * laid.LineHeight + 24.0f * m_Scale;
            after = 12.0f * m_Scale;
            break;
        }
        case Markdown::BlockType::Rule:
        {
            laid.Height = 2.0f;
            before = 8.0f * m_Scale;
            after = 14.0f * m_Scale;
            break;
        }
        case Markdown::BlockType::Table:
        {
            const size_t columns = block.Align.size();
            const float pad = 9.0f * m_Scale;
            std::vector<float> natural(columns, 0.0f), minimum(columns, 0.0f);
            const float size = base * 0.95f;
            laid.Size = size;
            laid.LineHeight = size * 1.4f;
            for (size_t r = 0; r < block.Rows.size(); ++r)
                for (size_t c = 0; c < columns; ++c)
                {
                    std::vector<Span> spans = block.Rows[r][c];
                    if (r == 0 && block.HasHeader)
                        for (Span &span : spans)
                            span.Style |= Markdown::StyleBold;
                    std::vector<Token> probe;
                    LayoutSpans(spans, 0.0f, 100000.0f, size, probe, laid.LineHeight);
                    float total = 0.0f;
                    for (const Token &token : probe)
                    {
                        total = std::max(total, token.X + token.W);
                        minimum[c] = std::max(minimum[c], token.W);
                    }
                    natural[c] = std::max(natural[c], total);
                }
            float sumNatural = 0.0f, sumMin = 0.0f;
            for (size_t c = 0; c < columns; ++c)
            {
                sumNatural += natural[c] + pad * 2.0f;
                sumMin += minimum[c] + pad * 2.0f;
            }
            std::vector<float> widths(columns);
            if (sumNatural <= width)
            {
                const float extra = (width - sumNatural) / std::max<size_t>(columns, 1);
                for (size_t c = 0; c < columns; ++c)
                    widths[c] = natural[c] + pad * 2.0f + extra;
            }
            else
            {
                const float room = std::max(0.0f, width - sumMin);
                const float flex = std::max(sumNatural - sumMin, 1.0f);
                for (size_t c = 0; c < columns; ++c)
                    widths[c] = minimum[c] + pad * 2.0f + room * ((natural[c] + pad * 2.0f - minimum[c] - pad * 2.0f) / flex);
            }
            laid.ColumnWidth = widths;
            float x = 0.0f;
            for (size_t c = 0; c < columns; ++c)
            {
                laid.ColumnX.push_back(x);
                x += widths[c];
            }
            float rowY = 0.0f;
            for (size_t r = 0; r < block.Rows.size(); ++r)
            {
                std::vector<Cell> cells(columns);
                float rowHeight = laid.LineHeight;
                for (size_t c = 0; c < columns; ++c)
                {
                    std::vector<Span> spans = block.Rows[r][c];
                    if (r == 0 && block.HasHeader)
                        for (Span &span : spans)
                            span.Style |= Markdown::StyleBold;
                    cells[c].X = laid.ColumnX[c];
                    cells[c].Width = widths[c];
                    const float h = LayoutSpans(spans, 0.0f, widths[c] - pad * 2.0f, size, cells[c].Tokens, laid.LineHeight);
                    rowHeight = std::max(rowHeight, h);
                    if (block.Align[c] != 0)
                    {
                        float used = 0.0f;
                        for (const Token &token : cells[c].Tokens)
                            if (token.Y < 0.5f)
                                used = std::max(used, token.X + token.W);
                        const float free = widths[c] - pad * 2.0f - used;
                        if (cells[c].Tokens.size() && free > 0.0f && h <= laid.LineHeight + 0.5f)
                            for (Token &token : cells[c].Tokens)
                                token.X += block.Align[c] == 2 ? free : free * 0.5f;
                    }
                }
                rowHeight += pad;
                laid.RowTop.push_back(rowY);
                laid.RowHeight.push_back(rowHeight);
                laid.Rows.push_back(std::move(cells));
                rowY += rowHeight;
            }
            laid.Height = rowY;
            after = 14.0f * m_Scale;
            break;
        }
        }

        y += std::max(before, previousGap);
        laid.Top = y;
        y += laid.Height;
        previousGap = after;
        m_Laid.push_back(std::move(laid));
    }
    m_Total = y + 40.0f;
    m_Width = width;
    m_Dirty = false;
}

void MarkdownView::DrawTokens(ImDrawList *list, const ImVec2 &origin, const std::vector<Token> &tokens, float size, float lineHeight, ImU32 color, std::string &clicked)
{
    const bool dark = IsDarkTheme();
    const ImVec4 accent = EditorUI::Colors::Accent;
    const ImU32 linkColor = ImGui::GetColorU32(accent);
    const ImU32 codeBg = ImGui::GetColorU32(EditorUI::Colors::Frame);
    const ImU32 codeBorder = ImGui::GetColorU32(EditorUI::Colors::Border);
    const ImU32 italicColor = ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(color), accent, 0.28f));
    const bool windowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    (void)dark;

    for (size_t t = 0; t < tokens.size(); ++t)
    {
        const Token &token = tokens[t];
        const float x = origin.x + token.X;
        const float y = origin.y + token.Y;
        if (y + lineHeight < ImGui::GetWindowPos().y || y > ImGui::GetWindowPos().y + ImGui::GetWindowHeight())
            continue;

        const bool code = (token.Style & Markdown::StyleCode) != 0;
        const bool link = (token.Style & Markdown::StyleLink) != 0;
        const ImVec2 min(x, y), max(x + token.W, y + lineHeight);
        const bool hovered = link && windowHovered && ImGui::IsMouseHoveringRect(min, max);

        const bool continues = code && t > 0 && (tokens[t - 1].Style & Markdown::StyleCode) && tokens[t - 1].Y == token.Y && token.X - (tokens[t - 1].X + tokens[t - 1].W) < size * 0.9f;
        if (code && !continues)
        {
            size_t last = t;
            while (last + 1 < tokens.size() && (tokens[last + 1].Style & Markdown::StyleCode) && tokens[last + 1].Y == token.Y &&
                   tokens[last + 1].X - (tokens[last].X + tokens[last].W) < size * 0.9f)
                ++last;
            const float end = origin.x + tokens[last].X + tokens[last].W;
            const ImVec2 a(x - 4.0f, y + lineHeight * 0.12f), b(end + 4.0f, y + lineHeight * 0.88f);
            list->AddRectFilled(a, b, codeBg, 5.0f);
            list->AddRect(a, b, codeBorder, 5.0f);
        }

        ImU32 textColor = color;
        if (link)
            textColor = hovered ? ImGui::GetColorU32(EditorUI::Colors::AccentHover) : linkColor;
        else if (token.Style & Markdown::StyleItalic)
            textColor = italicColor;
        else if (code)
            textColor = ImGui::GetColorU32(ImLerp(ImGui::ColorConvertU32ToFloat4(color), accent, 0.35f));

        const float drawSize = SizeFor(token.Style, size);
        ImFont *font = FontFor(token.Style);
        const float pad = 0.0f;
        const float textY = y + (lineHeight - drawSize) * 0.5f - 1.0f;
        list->AddText(font, drawSize, ImVec2(x + pad, textY), textColor, token.Text.c_str(), token.Text.c_str() + token.Text.size());
        if (token.Style & Markdown::StyleBold && !code)
            list->AddText(font, drawSize, ImVec2(x + pad + 0.5f, textY), textColor, token.Text.c_str(), token.Text.c_str() + token.Text.size());
        if (token.Style & Markdown::StyleStrike)
            list->AddLine(ImVec2(x, y + lineHeight * 0.5f), ImVec2(x + token.W, y + lineHeight * 0.5f), textColor, 1.2f);
        if (link)
            list->AddLine(ImVec2(x, y + lineHeight - 3.0f), ImVec2(x + token.W, y + lineHeight - 3.0f), textColor, hovered ? 1.6f : 1.0f);
        if (hovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("%s", token.Url.c_str());
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                clicked = token.Url;
        }
    }
}

std::string MarkdownView::Draw()
{
    std::string clicked;
    const float width = std::min(std::max(120.0f, ImGui::GetContentRegionAvail().x - 8.0f), 1120.0f * m_Scale);
    if (m_Dirty || std::abs(width - m_Width) > 0.5f)
        Layout(width);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float startCursorY = ImGui::GetCursorPosY();
    ImDrawList *list = ImGui::GetWindowDrawList();
    const ImGuiIO &io = ImGui::GetIO();
    const bool dark = IsDarkTheme();
    const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 dimColor = ImGui::GetColorU32(EditorUI::Colors::TextDim);
    const ImU32 lineColor = ImGui::GetColorU32(EditorUI::Colors::Border);
    const ImU32 accentColor = ImGui::GetColorU32(EditorUI::Colors::Accent);
    const ImVec2 clipMin = ImGui::GetWindowPos();
    const ImVec2 clipMax(clipMin.x + ImGui::GetWindowWidth(), clipMin.y + ImGui::GetWindowHeight());

    ImGui::Dummy(ImVec2(width, m_Total));

    if (m_PendingTop)
    {
        ImGui::SetScrollY(0.0f);
        m_PendingTop = false;
    }
    if (!m_PendingAnchor.empty())
    {
        for (size_t i = 0; i < m_Document.Blocks.size() && i < m_Laid.size(); ++i)
            if (m_Document.Blocks[i].Type == Markdown::BlockType::Heading && m_Document.Blocks[i].Anchor == m_PendingAnchor)
            {
                ImGui::SetScrollY(std::max(0.0f, startCursorY + m_Laid[i].Top - 10.0f));
                break;
            }
        m_PendingAnchor.clear();
    }

    for (size_t i = 0; i < m_Document.Blocks.size() && i < m_Laid.size(); ++i)
    {
        const Markdown::Block &block = m_Document.Blocks[i];
        const Laid &laid = m_Laid[i];
        const float top = origin.y + laid.Top;
        if (top + laid.Height < clipMin.y || top > clipMax.y)
            continue;
        const ImVec2 at(origin.x, top);

        switch (block.Type)
        {
        case Markdown::BlockType::Heading:
        {
            DrawTokens(list, at, laid.Tokens, laid.Size, laid.LineHeight, textColor, clicked);
            if (block.Level <= 2)
                list->AddLine(ImVec2(at.x, top + laid.Height - 2.0f), ImVec2(at.x + m_Width, top + laid.Height - 2.0f), lineColor, 1.0f);
            break;
        }
        case Markdown::BlockType::Paragraph:
            if (laid.Image)
            {
                list->AddImage(EditorUI::TextureId(laid.Image->GetID()), at, ImVec2(at.x + laid.ImageSize.x, at.y + laid.ImageSize.y));
                list->AddRect(at, ImVec2(at.x + laid.ImageSize.x, at.y + laid.ImageSize.y), lineColor, 4.0f);
            }
            else
                DrawTokens(list, at, laid.Tokens, laid.Size, laid.LineHeight, textColor, clicked);
            break;
        case Markdown::BlockType::ListItem:
        {
            const float bx = at.x + laid.Indent - 18.0f * m_Scale;
            const std::string &bullet = laid.Bullet;
            ImFont *font = EditorUI::GetFont(EditorUI::FontRole::Regular);
            list->AddText(font, laid.Size, ImVec2(bx + (block.Ordered ? -4.0f : 4.0f), top + (laid.LineHeight - laid.Size) * 0.5f - 1.0f), accentColor, bullet.c_str());
            DrawTokens(list, at, laid.Tokens, laid.Size, laid.LineHeight, textColor, clicked);
            break;
        }
        case Markdown::BlockType::Quote:
        {
            list->AddRectFilled(at, ImVec2(at.x + m_Width, at.y + laid.Height), ImGui::GetColorU32(ImVec4(EditorUI::Colors::Frame.x, EditorUI::Colors::Frame.y, EditorUI::Colors::Frame.z, 0.55f)), 6.0f);
            list->AddRectFilled(at, ImVec2(at.x + 4.0f, at.y + laid.Height), accentColor, 2.0f);
            DrawTokens(list, at, laid.Tokens, laid.Size, laid.LineHeight, dimColor, clicked);
            break;
        }
        case Markdown::BlockType::Rule:
            list->AddLine(ImVec2(at.x, top + 1.0f), ImVec2(at.x + m_Width, top + 1.0f), lineColor, 1.5f);
            break;
        case Markdown::BlockType::Code:
        {
            const ImVec2 max(at.x + m_Width, at.y + laid.Height);
            list->AddRectFilled(at, max, ImGui::GetColorU32(EditorUI::Colors::Frame), 8.0f);
            list->AddRect(at, max, lineColor, 8.0f);
            const float padX = 12.0f * m_Scale;
            const float padY = 12.0f * m_Scale;
            const float visible = m_Width - padX * 2.0f;
            float &scroll = m_CodeScroll[i];
            const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(at, max);
            if (hovered && laid.CodeWidth > visible && io.MouseWheelH != 0.0f)
                scroll -= io.MouseWheelH * 60.0f;
            if (hovered && laid.CodeWidth > visible && io.KeyShift && io.MouseWheel != 0.0f)
                scroll -= io.MouseWheel * 60.0f;
            scroll = std::clamp(scroll, 0.0f, std::max(0.0f, laid.CodeWidth - visible));

            list->PushClipRect(ImVec2(at.x + padX, at.y), ImVec2(max.x - padX, max.y), true);
            ImFont *mono = EditorUI::GetFont(EditorUI::FontRole::Mono);
            for (size_t l = 0; l < laid.Lines.size(); ++l)
            {
                float x = at.x + padX - scroll;
                const float y = at.y + padY + l * laid.LineHeight;
                if (y + laid.LineHeight < clipMin.y || y > clipMax.y)
                    continue;
                for (const Segment &segment : laid.Lines[l])
                {
                    const ImU32 color = segment.Kind == Tone::Plain ? textColor : ToneColor(static_cast<int>(segment.Kind), dark);
                    list->AddText(mono, laid.Size, ImVec2(x, y), color, segment.Text.c_str(), segment.Text.c_str() + segment.Text.size());
                    x += mono->CalcTextSizeA(laid.Size, 100000.0f, 0.0f, segment.Text.c_str(), segment.Text.c_str() + segment.Text.size()).x;
                }
            }
            list->PopClipRect();

            if (laid.CodeWidth > visible)
            {
                const float track = visible;
                const float thumb = std::max(30.0f, track * visible / laid.CodeWidth);
                const float t = scroll / std::max(1.0f, laid.CodeWidth - visible);
                const float sx = at.x + padX + (track - thumb) * t;
                list->AddRectFilled(ImVec2(sx, max.y - 7.0f), ImVec2(sx + thumb, max.y - 4.0f), ImGui::GetColorU32(EditorUI::Colors::BorderStrong), 2.0f);
            }

            const std::string label = block.Lang.empty() ? std::string("copy") : block.Lang + "  |  copy";
            ImFont *font = EditorUI::GetFont(EditorUI::FontRole::Regular);
            const float labelSize = 12.0f * m_Scale;
            const ImVec2 textSize = font->CalcTextSizeA(labelSize, 1000.0f, 0.0f, label.c_str());
            const ImVec2 labelMin(max.x - textSize.x - 16.0f, at.y + 4.0f);
            const ImVec2 labelMax(max.x - 6.0f, at.y + textSize.y + 10.0f);
            const bool overLabel = hovered && ImGui::IsMouseHoveringRect(labelMin, labelMax);
            list->AddText(font, labelSize, ImVec2(labelMin.x + 5.0f, labelMin.y + 2.0f), overLabel ? accentColor : ImGui::GetColorU32(EditorUI::Colors::TextFaint), label.c_str());
            if (overLabel)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    ImGui::SetClipboardText(block.Code.c_str());
            }
            break;
        }
        case Markdown::BlockType::Table:
        {
            const float totalWidth = laid.ColumnX.empty() ? 0.0f : laid.ColumnX.back() + laid.ColumnWidth.back();
            const float pad = 9.0f * m_Scale;
            const ImU32 headerBg = ImGui::GetColorU32(EditorUI::Colors::Header);
            const ImU32 stripe = ImGui::GetColorU32(ImVec4(EditorUI::Colors::Frame.x, EditorUI::Colors::Frame.y, EditorUI::Colors::Frame.z, 0.45f));
            list->AddRectFilled(at, ImVec2(at.x + totalWidth, at.y + laid.Height), ImGui::GetColorU32(ImVec4(0, 0, 0, 0.0f)), 6.0f);
            for (size_t r = 0; r < laid.Rows.size(); ++r)
            {
                const float rowTop = at.y + laid.RowTop[r];
                const float rowBottom = rowTop + laid.RowHeight[r];
                if (rowBottom < clipMin.y || rowTop > clipMax.y)
                    continue;
                if (r == 0 && block.HasHeader)
                    list->AddRectFilled(ImVec2(at.x, rowTop), ImVec2(at.x + totalWidth, rowBottom), headerBg);
                else if (r % 2 == 0)
                    list->AddRectFilled(ImVec2(at.x, rowTop), ImVec2(at.x + totalWidth, rowBottom), stripe);
                list->AddLine(ImVec2(at.x, rowBottom), ImVec2(at.x + totalWidth, rowBottom), lineColor);
                for (const Cell &cell : laid.Rows[r])
                    DrawTokens(list, ImVec2(at.x + cell.X + pad, rowTop + pad * 0.5f), cell.Tokens, laid.Size, laid.LineHeight, textColor, clicked);
            }
            list->AddRect(at, ImVec2(at.x + totalWidth, at.y + laid.Height), lineColor, 4.0f);
            for (size_t c = 1; c < laid.ColumnX.size(); ++c)
                list->AddLine(ImVec2(at.x + laid.ColumnX[c], at.y), ImVec2(at.x + laid.ColumnX[c], at.y + laid.Height), lineColor);
            break;
        }
        }
    }
    return clicked;
}

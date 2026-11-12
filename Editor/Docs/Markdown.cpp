#include "Markdown.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace Markdown
{
    namespace
    {
        bool IsSpace(char c)
        {
            return c == ' ' || c == '\t' || c == '\n' || c == '\r';
        }

        std::string Trim(const std::string &text)
        {
            size_t begin = 0, end = text.size();
            while (begin < end && IsSpace(text[begin]))
                ++begin;
            while (end > begin && IsSpace(text[end - 1]))
                --end;
            return text.substr(begin, end - begin);
        }

        void Emit(std::vector<Span> &out, const std::string &text, int style, const std::string &url)
        {
            if (text.empty())
                return;
            if (!out.empty() && out.back().Style == style && out.back().Url == url)
                out.back().Text += text;
            else
                out.push_back({text, style, url});
        }

        std::string DecodeEntities(const std::string &text)
        {
            if (text.find('&') == std::string::npos)
                return text;
            static const struct
            {
                const char *Name;
                const char *Value;
            } entities[] = {{"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&nbsp;", " "}, {"&#39;", "'"}, {"&mdash;", "-"}, {"&ndash;", "-"}};
            std::string result;
            for (size_t i = 0; i < text.size();)
            {
                bool matched = false;
                if (text[i] == '&')
                    for (const auto &entity : entities)
                    {
                        const std::string name = entity.Name;
                        if (text.compare(i, name.size(), name) == 0)
                        {
                            result += entity.Value;
                            i += name.size();
                            matched = true;
                            break;
                        }
                    }
                if (!matched)
                    result += text[i++];
            }
            return result;
        }

        size_t FindClosingBracket(const std::string &s, size_t open)
        {
            int depth = 0;
            for (size_t i = open; i < s.size(); ++i)
            {
                if (s[i] == '\\')
                {
                    ++i;
                    continue;
                }
                if (s[i] == '`')
                {
                    size_t end = s.find('`', i + 1);
                    if (end == std::string::npos)
                        continue;
                    i = end;
                    continue;
                }
                if (s[i] == '[')
                    ++depth;
                else if (s[i] == ']' && --depth == 0)
                    return i;
            }
            return std::string::npos;
        }

        size_t FindClosingParen(const std::string &s, size_t open)
        {
            int depth = 0;
            for (size_t i = open; i < s.size(); ++i)
            {
                if (s[i] == '(')
                    ++depth;
                else if (s[i] == ')' && --depth == 0)
                    return i;
            }
            return std::string::npos;
        }

        std::string CleanUrl(const std::string &raw)
        {
            std::string url = Trim(raw);
            if (!url.empty() && url[0] == '<')
            {
                size_t end = url.find('>');
                return url.substr(1, end == std::string::npos ? std::string::npos : end - 1);
            }
            size_t space = url.find_first_of(" \t");
            if (space != std::string::npos)
                url = url.substr(0, space);
            return url;
        }

        bool StartsWith(const std::string &s, size_t pos, const char *prefix)
        {
            return s.compare(pos, std::char_traits<char>::length(prefix), prefix) == 0;
        }

        void ParseInto(const std::string &s, int style, const std::string &url, std::vector<Span> &out)
        {
            std::string buffer;
            auto flush = [&]()
            {
                Emit(out, buffer, style, url);
                buffer.clear();
            };

            const size_t n = s.size();
            size_t i = 0;
            while (i < n)
            {
                const char c = s[i];

                if (c == '\\' && i + 1 < n && std::ispunct(static_cast<unsigned char>(s[i + 1])))
                {
                    buffer += s[i + 1];
                    i += 2;
                    continue;
                }

                if (c == '`')
                {
                    size_t run = 1;
                    while (i + run < n && s[i + run] == '`')
                        ++run;
                    const std::string fence(run, '`');
                    size_t close = i + run;
                    size_t found = std::string::npos;
                    while ((close = s.find(fence, close)) != std::string::npos)
                    {
                        size_t length = run;
                        while (close + length < n && s[close + length] == '`')
                            ++length;
                        if (length == run)
                        {
                            found = close;
                            break;
                        }
                        close += length;
                    }
                    if (found != std::string::npos)
                    {
                        flush();
                        std::string content = s.substr(i + run, found - i - run);
                        if (content.size() >= 2 && content.front() == ' ' && content.back() == ' ')
                            content = content.substr(1, content.size() - 2);
                        Emit(out, content, style | StyleCode, url);
                        i = found + run;
                        continue;
                    }
                    buffer.append(run, '`');
                    i += run;
                    continue;
                }

                const bool image = c == '!' && i + 1 < n && s[i + 1] == '[';
                if (c == '[' || image)
                {
                    const size_t open = image ? i + 1 : i;
                    const size_t close = FindClosingBracket(s, open);
                    if (close != std::string::npos && close + 1 < n && s[close + 1] == '(')
                    {
                        const size_t paren = FindClosingParen(s, close + 1);
                        if (paren != std::string::npos)
                        {
                            flush();
                            const std::string label = s.substr(open + 1, close - open - 1);
                            const std::string target = CleanUrl(s.substr(close + 2, paren - close - 2));
                            if (image)
                                Emit(out, label, style | StyleImage, target);
                            else
                                ParseInto(label, style | StyleLink, target, out);
                            i = paren + 1;
                            continue;
                        }
                    }
                }

                if (c == '<')
                {
                    const size_t end = s.find('>', i);
                    if (end != std::string::npos)
                    {
                        const std::string inner = s.substr(i + 1, end - i - 1);
                        if (StartsWith(inner, 0, "http://") || StartsWith(inner, 0, "https://") || StartsWith(inner, 0, "mailto:"))
                        {
                            if (inner.find(' ') == std::string::npos)
                            {
                                flush();
                                Emit(out, inner, style | StyleLink, inner);
                                i = end + 1;
                                continue;
                            }
                        }
                        else if (!inner.empty() && (std::isalpha(static_cast<unsigned char>(inner[0])) || inner[0] == '/' || inner[0] == '!'))
                        {
                            if (inner.rfind("br", 0) == 0)
                                buffer += '\n';
                            i = end + 1;
                            continue;
                        }
                    }
                }

                if ((c == 'h') && (StartsWith(s, i, "http://") || StartsWith(s, i, "https://")) && (i == 0 || !std::isalnum(static_cast<unsigned char>(s[i - 1]))))
                {
                    size_t end = i;
                    while (end < n && !IsSpace(s[end]) && s[end] != ')' && s[end] != '>')
                        ++end;
                    while (end > i && std::strchr(".,;:!?", s[end - 1]))
                        --end;
                    flush();
                    const std::string link = s.substr(i, end - i);
                    Emit(out, link, style | StyleLink, link);
                    i = end;
                    continue;
                }

                if (c == '*' || c == '_' || c == '~')
                {
                    size_t run = 1;
                    while (i + run < n && s[i + run] == c)
                        ++run;
                    const bool strike = c == '~';
                    if (strike && run < 2)
                    {
                        buffer += c;
                        ++i;
                        continue;
                    }
                    const size_t use = std::min<size_t>(run, strike ? 2 : 3);
                    const bool opens = i + use < n && !IsSpace(s[i + use]) &&
                                       (c != '_' || i == 0 || !std::isalnum(static_cast<unsigned char>(s[i - 1])));
                    size_t closer = std::string::npos;
                    if (opens)
                    {
                        const std::string marker(use, c);
                        size_t from = i + use;
                        while ((from = s.find(marker, from)) != std::string::npos)
                        {
                            const bool spaceBefore = IsSpace(s[from - 1]);
                            const bool wordAfter = c == '_' && from + use < n && std::isalnum(static_cast<unsigned char>(s[from + use]));
                            if (!spaceBefore && !wordAfter)
                            {
                                closer = from;
                                break;
                            }
                            from += use;
                        }
                    }
                    if (closer != std::string::npos)
                    {
                        flush();
                        int add = strike ? StyleStrike : (use == 1 ? StyleItalic : use == 2 ? StyleBold : StyleBold | StyleItalic);
                        ParseInto(s.substr(i + use, closer - i - use), style | add, url, out);
                        i = closer + use;
                        continue;
                    }
                    buffer.append(use, c);
                    i += use;
                    continue;
                }

                buffer += c;
                ++i;
            }
            flush();
        }

        struct Line
        {
            std::string Text;
            int Indent = 0;
            std::string Rest;
            bool Blank = true;
        };

        Line Analyse(const std::string &raw)
        {
            Line line;
            line.Text = raw;
            int indent = 0;
            size_t i = 0;
            while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t'))
            {
                indent += raw[i] == '\t' ? 4 : 1;
                ++i;
            }
            line.Indent = indent;
            line.Rest = Trim(raw.substr(i));
            line.Blank = line.Rest.empty();
            return line;
        }

        bool IsRule(const std::string &rest)
        {
            if (rest.size() < 3)
                return false;
            const char c = rest[0];
            if (c != '-' && c != '*' && c != '_')
                return false;
            int count = 0;
            for (char ch : rest)
            {
                if (ch == c)
                    ++count;
                else if (ch != ' ')
                    return false;
            }
            return count >= 3;
        }

        int HeadingLevel(const std::string &rest)
        {
            size_t hashes = 0;
            while (hashes < rest.size() && rest[hashes] == '#')
                ++hashes;
            if (hashes == 0 || hashes > 6 || hashes >= rest.size() || rest[hashes] != ' ')
                return 0;
            return static_cast<int>(hashes);
        }

        bool ListMarker(const Line &line, bool &ordered, int &number, std::string &content)
        {
            const std::string &r = line.Rest;
            if (r.size() >= 2 && (r[0] == '-' || r[0] == '*' || r[0] == '+') && r[1] == ' ')
            {
                ordered = false;
                number = 0;
                content = Trim(r.substr(2));
                return !IsRule(r);
            }
            size_t digits = 0;
            while (digits < r.size() && std::isdigit(static_cast<unsigned char>(r[digits])))
                ++digits;
            if (digits > 0 && digits < 10 && digits + 1 < r.size() && (r[digits] == '.' || r[digits] == ')') && r[digits + 1] == ' ')
            {
                ordered = true;
                number = std::atoi(r.substr(0, digits).c_str());
                content = Trim(r.substr(digits + 2));
                return true;
            }
            return false;
        }

        std::vector<std::string> SplitRow(const std::string &row)
        {
            std::string text = Trim(row);
            if (!text.empty() && text.front() == '|')
                text.erase(0, 1);
            if (!text.empty() && text.back() == '|' && (text.size() < 2 || text[text.size() - 2] != '\\'))
                text.pop_back();
            std::vector<std::string> cells;
            std::string current;
            bool code = false;
            for (size_t i = 0; i < text.size(); ++i)
            {
                const char c = text[i];
                if (c == '\\' && i + 1 < text.size() && text[i + 1] == '|')
                {
                    current += '|';
                    ++i;
                    continue;
                }
                if (c == '`')
                    code = !code;
                if (c == '|' && !code)
                {
                    cells.push_back(Trim(current));
                    current.clear();
                    continue;
                }
                current += c;
            }
            cells.push_back(Trim(current));
            return cells;
        }

        bool IsTableSeparator(const std::string &rest)
        {
            if (rest.find('-') == std::string::npos)
                return false;
            for (char c : rest)
                if (c != '|' && c != '-' && c != ':' && c != ' ')
                    return false;
            return rest.find('|') != std::string::npos || rest.find("--") != std::string::npos;
        }

        std::string JoinLines(const std::vector<std::string> &lines)
        {
            std::string joined;
            for (size_t i = 0; i < lines.size(); ++i)
            {
                std::string text = lines[i];
                bool hard = false;
                if (text.size() >= 2 && text.compare(text.size() - 2, 2, "  ") == 0)
                    hard = true;
                if (!text.empty() && text.back() == '\\')
                {
                    hard = true;
                    text.pop_back();
                }
                text = Trim(text);
                joined += text;
                if (i + 1 < lines.size())
                    joined += hard ? "\n" : " ";
            }
            return joined;
        }
    }

    std::string Slug(const std::string &heading)
    {
        std::string slug;
        for (unsigned char c : heading)
        {
            if (std::isalnum(c))
                slug += static_cast<char>(std::tolower(c));
            else if (c == ' ' || c == '-')
                slug += '-';
            else if (c == '_')
                slug += '_';
            else if (c >= 0x80)
                slug += static_cast<char>(c);
        }
        return slug;
    }

    std::string PlainText(const std::vector<Span> &spans)
    {
        std::string text;
        for (const Span &span : spans)
            text += span.Text;
        return text;
    }

    std::vector<Span> ParseInline(const std::string &text)
    {
        std::vector<Span> spans;
        ParseInto(DecodeEntities(text), 0, std::string(), spans);
        return spans;
    }

    Document Parse(const std::string &source)
    {
        Document doc;
        std::vector<Line> lines;
        {
            std::string current;
            for (char c : source)
            {
                if (c == '\n')
                {
                    lines.push_back(Analyse(current));
                    current.clear();
                }
                else if (c != '\r')
                    current += c;
            }
            lines.push_back(Analyse(current));
        }

        std::vector<std::string> paragraph;
        std::vector<int> listIndents;
        bool previousWasItem = false;

        auto flushParagraph = [&]()
        {
            if (paragraph.empty())
                return;
            Block block;
            block.Type = BlockType::Paragraph;
            block.Spans = ParseInline(JoinLines(paragraph));
            doc.Blocks.push_back(std::move(block));
            paragraph.clear();
        };

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const Line &line = lines[i];

            if (line.Rest.rfind("```", 0) == 0 || line.Rest.rfind("~~~", 0) == 0)
            {
                flushParagraph();
                const std::string fence = line.Rest.substr(0, 3);
                Block block;
                block.Type = BlockType::Code;
                block.Lang = Trim(line.Rest.substr(3));
                std::string code;
                size_t j = i + 1;
                for (; j < lines.size(); ++j)
                {
                    if (lines[j].Rest.rfind(fence, 0) == 0)
                        break;
                    std::string text = lines[j].Text;
                    const int strip = std::min(line.Indent, lines[j].Indent);
                    int removed = 0;
                    size_t k = 0;
                    while (k < text.size() && removed < strip && (text[k] == ' ' || text[k] == '\t'))
                    {
                        removed += text[k] == '\t' ? 4 : 1;
                        ++k;
                    }
                    code += text.substr(k);
                    code += '\n';
                }
                if (!code.empty())
                    code.pop_back();
                block.Code = code;
                doc.Blocks.push_back(std::move(block));
                i = j;
                previousWasItem = false;
                continue;
            }

            if (line.Blank)
            {
                flushParagraph();
                previousWasItem = false;
                listIndents.clear();
                continue;
            }

            const int level = HeadingLevel(line.Rest);
            if (level > 0)
            {
                flushParagraph();
                std::string title = line.Rest.substr(level + 1);
                while (!title.empty() && (title.back() == '#' || title.back() == ' '))
                    title.pop_back();
                Block block;
                block.Type = BlockType::Heading;
                block.Level = level;
                block.Spans = ParseInline(title);
                block.Anchor = Slug(PlainText(block.Spans));
                if (level == 1 && doc.Title.empty())
                    doc.Title = PlainText(block.Spans);
                doc.Blocks.push_back(std::move(block));
                previousWasItem = false;
                listIndents.clear();
                continue;
            }

            if (IsRule(line.Rest))
            {
                flushParagraph();
                Block block;
                block.Type = BlockType::Rule;
                doc.Blocks.push_back(std::move(block));
                previousWasItem = false;
                continue;
            }

            if (line.Rest.find('|') != std::string::npos && i + 1 < lines.size() && IsTableSeparator(lines[i + 1].Rest))
            {
                flushParagraph();
                Block block;
                block.Type = BlockType::Table;
                block.HasHeader = true;
                for (const std::string &cell : SplitRow(lines[i + 1].Rest))
                {
                    const bool left = !cell.empty() && cell.front() == ':';
                    const bool right = !cell.empty() && cell.back() == ':';
                    block.Align.push_back(left && right ? 1 : right ? 2 : 0);
                }
                auto addRow = [&](const std::string &text)
                {
                    std::vector<std::vector<Span>> row;
                    for (const std::string &cell : SplitRow(text))
                        row.push_back(ParseInline(cell));
                    block.Rows.push_back(std::move(row));
                };
                addRow(line.Rest);
                size_t j = i + 2;
                for (; j < lines.size() && !lines[j].Blank && lines[j].Rest.find('|') != std::string::npos; ++j)
                    addRow(lines[j].Rest);
                size_t columns = 0;
                for (const auto &row : block.Rows)
                    columns = std::max(columns, row.size());
                for (auto &row : block.Rows)
                    row.resize(columns);
                block.Align.resize(columns, 0);
                doc.Blocks.push_back(std::move(block));
                i = j - 1;
                previousWasItem = false;
                continue;
            }

            if (line.Rest[0] == '>')
            {
                flushParagraph();
                std::vector<std::string> quote;
                size_t j = i;
                for (; j < lines.size() && !lines[j].Blank && lines[j].Rest[0] == '>'; ++j)
                {
                    std::string text = lines[j].Rest.substr(1);
                    if (!text.empty() && text[0] == ' ')
                        text.erase(0, 1);
                    quote.push_back(text);
                }
                Block block;
                block.Type = BlockType::Quote;
                block.Spans = ParseInline(JoinLines(quote));
                doc.Blocks.push_back(std::move(block));
                i = j - 1;
                previousWasItem = false;
                continue;
            }

            bool ordered = false;
            int number = 0;
            std::string content;
            if (ListMarker(line, ordered, number, content))
            {
                flushParagraph();
                while (!listIndents.empty() && line.Indent < listIndents.back())
                    listIndents.pop_back();
                if (listIndents.empty() || line.Indent > listIndents.back())
                    listIndents.push_back(line.Indent);
                Block block;
                block.Type = BlockType::ListItem;
                block.Level = static_cast<int>(listIndents.size()) - 1;
                block.Ordered = ordered;
                block.Number = number;
                if (content.rfind("[ ] ", 0) == 0 || content.rfind("[x] ", 0) == 0 || content.rfind("[X] ", 0) == 0)
                {
                    block.Anchor = content[1] == ' ' ? "[ ]" : "[x]";
                    content = content.substr(4);
                }
                block.Spans = ParseInline(content);
                doc.Blocks.push_back(std::move(block));
                previousWasItem = true;
                continue;
            }

            if (previousWasItem && paragraph.empty() && !doc.Blocks.empty() && doc.Blocks.back().Type == BlockType::ListItem)
            {
                std::string text = PlainText(doc.Blocks.back().Spans);
                (void)text;
                std::vector<Span> extra = ParseInline(" " + line.Rest);
                for (Span &span : extra)
                    Emit(doc.Blocks.back().Spans, span.Text, span.Style, span.Url);
                continue;
            }

            if (line.Rest[0] == '<' && line.Rest.back() == '>' && line.Rest.find("<img") != 0)
                continue;

            paragraph.push_back(line.Rest);
        }
        flushParagraph();
        return doc;
    }
}

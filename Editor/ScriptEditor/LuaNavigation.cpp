#include "LuaNavigation.h"
#include "LuaContext.h"
#include "LuaLexer.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace ScriptEditor
{
    namespace
    {
        bool IsIdent(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

        std::string Escape(const std::string &s)
        {
            std::string out;
            for (char c : s)
            {
                if (!IsIdent(c))
                    out += '\\';
                out += c;
            }
            return out;
        }

        int FindWord(const std::string &line, const std::string &word, size_t from, size_t to = std::string::npos)
        {
            for (size_t p = line.find(word, from); p != std::string::npos; p = line.find(word, p + 1))
            {
                if (to != std::string::npos && p >= to)
                    return -1;
                const bool leftOk = p == 0 || !IsIdent(line[p - 1]);
                const bool rightOk = p + word.size() >= line.size() || !IsIdent(line[p + word.size()]);
                if (leftOk && rightOk)
                    return static_cast<int>(p);
            }
            return -1;
        }

        bool InsideString(const std::string &line, int column)
        {
            char quote = 0;
            for (int i = 0; i < column && i < static_cast<int>(line.size()); ++i)
            {
                const char c = line[static_cast<size_t>(i)];
                if (quote)
                {
                    if (c == '\\')
                        ++i;
                    else if (c == quote)
                        quote = 0;
                }
                else if (c == '"' || c == '\'')
                    quote = c;
                else if (c == '-' && i + 1 < static_cast<int>(line.size()) && line[static_cast<size_t>(i + 1)] == '-')
                    return false;
            }
            return quote != 0;
        }

        std::string ReceiverBefore(const std::string &text, int end)
        {
            int i = end;
            while (i > 0)
            {
                const char c = text[static_cast<size_t>(i - 1)];
                if (IsIdent(c) || c == '.')
                    --i;
                else
                    break;
            }
            return text.substr(static_cast<size_t>(i), static_cast<size_t>(end - i));
        }

        bool MemberDefinitionIn(const std::vector<std::string> &lines, const std::string &member, const std::string &owner, bool anyOwner, int &lineOut, int &columnOut)
        {
            const std::string m = Escape(member);
            const std::string ownerPattern = anyOwner ? R"([A-Za-z_][A-Za-z0-9_.]*)" : Escape(owner);
            const std::regex fn("\\bfunction\\s+" + ownerPattern + "\\s*[.:]\\s*" + m + "\\b");
            const std::regex assignFn("\\b" + ownerPattern + "\\s*\\.\\s*" + m + "\\s*=");
            const std::regex bare("^\\s*" + m + "\\s*=");
            for (size_t i = 0; i < lines.size(); ++i)
            {
                std::smatch match;
                if (std::regex_search(lines[i], match, fn) || std::regex_search(lines[i], match, assignFn))
                {
                    const int p = FindWord(lines[i], member, static_cast<size_t>(match.position(0)));
                    if (p >= 0)
                    {
                        lineOut = static_cast<int>(i);
                        columnOut = p;
                        return true;
                    }
                }
            }
            if (anyOwner)
            {
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    std::smatch match;
                    if (std::regex_search(lines[i], match, bare))
                    {
                        lineOut = static_cast<int>(i);
                        columnOut = FindWord(lines[i], member, 0);
                        return columnOut >= 0;
                    }
                }
            }
            return false;
        }

        bool RequireAlias(const std::vector<std::string> &lines, const std::string &alias, std::string &module)
        {
            const std::regex pattern("\\blocal\\s+" + Escape(alias) + R"(\s*=\s*require\s*\(?\s*["']([^"']+)["'])");
            for (const std::string &l : lines)
            {
                std::smatch match;
                if (std::regex_search(l, match, pattern))
                {
                    module = match[1];
                    return true;
                }
            }
            return false;
        }

        bool FindLocalDeclaration(const std::vector<std::string> &lines, int line, int column, const std::string &word, int &lineOut, int &columnOut)
        {
            const std::string w = Escape(word);
            const std::regex localFunction("\\blocal\\s+function\\s+" + w + "\\b");
            const std::regex globalFunction("\\bfunction\\s+" + w + "\\s*\\(");
            const std::regex localList("\\blocal\\s+(?:[A-Za-z_]\\w*\\s*,\\s*)*" + w + "\\b");
            const std::regex forLoop("\\bfor\\s+(?:[A-Za-z_]\\w*\\s*,\\s*)*" + w + "\\b\\s*(?:,|=|\\bin\\b)");
            const std::regex parameters("\\bfunction\\b[^()]*\\(([^)]*)\\)");

            for (int i = std::min(line, static_cast<int>(lines.size()) - 1); i >= 0; --i)
            {
                const std::string &text = lines[static_cast<size_t>(i)];
                std::smatch match;
                auto accept = [&](size_t matchStart, size_t matchEnd) -> bool
                {
                    const int p = FindWord(text, word, matchStart, matchEnd);
                    if (p < 0)
                        return false;
                    if (i == line && p == column)
                        return false;
                    lineOut = i;
                    columnOut = p;
                    return true;
                };

                if (std::regex_search(text, match, localFunction) && accept(static_cast<size_t>(match.position(0)), static_cast<size_t>(match.position(0) + match.length(0))))
                    return true;
                if (std::regex_search(text, match, localList) && accept(static_cast<size_t>(match.position(0)), static_cast<size_t>(match.position(0) + match.length(0))))
                    return true;
                if (std::regex_search(text, match, forLoop) && accept(static_cast<size_t>(match.position(0)), static_cast<size_t>(match.position(0) + match.length(0))))
                    return true;
                if (std::regex_search(text, match, parameters))
                {
                    const size_t paramStart = static_cast<size_t>(match.position(1));
                    const size_t paramEnd = paramStart + static_cast<size_t>(match.length(1));
                    if (FindWord(text, word, paramStart, paramEnd) >= 0 && accept(paramStart, paramEnd))
                        return true;
                }
                if (std::regex_search(text, match, globalFunction) && accept(static_cast<size_t>(match.position(0)), static_cast<size_t>(match.position(0) + match.length(0))))
                    return true;
            }

            const std::regex topLevel("^" + w + "\\s*=[^=]");
            for (int i = 0; i < static_cast<int>(lines.size()); ++i)
            {
                std::smatch match;
                if (std::regex_search(lines[static_cast<size_t>(i)], match, topLevel) && !(i == line && column == 0))
                {
                    lineOut = i;
                    columnOut = 0;
                    return true;
                }
            }
            return false;
        }
    }

    WordSpan WordAtColumn(const std::string &line, int column)
    {
        WordSpan span;
        int start = std::clamp(column, 0, static_cast<int>(line.size()));
        int end = start;
        while (start > 0 && IsIdent(line[static_cast<size_t>(start - 1)]))
            --start;
        while (end < static_cast<int>(line.size()) && IsIdent(line[static_cast<size_t>(end)]))
            ++end;
        span.Text = line.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
        span.Start = start;
        span.End = end;
        return span;
    }

    Definition ResolveSymbol(const std::vector<std::string> &lines, int line, int column, const ModuleLoader &loader, const std::function<std::string(const std::string &, const std::string &)> &describeBuiltin)
    {
        Definition result;
        if (line < 0 || line >= static_cast<int>(lines.size()))
            return result;
        const std::string &text = lines[static_cast<size_t>(line)];

        static const std::regex requirePattern(R"(require\s*\(?\s*(["'])([^"']+)\1)");
        for (std::sregex_iterator it(text.begin(), text.end(), requirePattern), end; it != end; ++it)
        {
            const int begin = static_cast<int>(it->position(2));
            const int stop = begin + static_cast<int>(it->length(2));
            if (column >= begin && column <= stop && loader)
            {
                std::string vpath;
                std::vector<std::string> moduleLines;
                if (loader((*it)[2], vpath, moduleLines))
                {
                    result.Found = true;
                    result.VPath = vpath;
                    result.Line = 0;
                    result.Column = 0;
                    result.Length = stop - begin;
                    result.Label = (*it)[2];
                    return result;
                }
            }
        }

        if (InsideString(text, column))
            return result;

        const WordSpan word = WordAtColumn(text, column);
        if (word.Text.empty() || std::isdigit(static_cast<unsigned char>(word.Text[0])) || IsKeyword(word.Text))
            return result;
        result.Length = word.End - word.Start;
        result.Label = word.Text;

        const char before = word.Start > 0 ? text[static_cast<size_t>(word.Start - 1)] : 0;
        const bool dotBefore = before == '.' && !(word.Start > 1 && text[static_cast<size_t>(word.Start - 2)] == '.');
        if (dotBefore || before == ':')
        {
            const std::string receiver = ReceiverBefore(text, word.Start - 1);
            const std::string root = receiver.substr(0, receiver.find('.'));
            int foundLine = 0;
            int foundColumn = 0;

            if (receiver == "self" || (!receiver.empty() && receiver.find('.') == std::string::npos))
            {
                std::string module;
                if (receiver != "self" && RequireAlias(lines, receiver, module) && loader)
                {
                    std::string vpath;
                    std::vector<std::string> moduleLines;
                    if (loader(module, vpath, moduleLines))
                    {
                        result.Found = true;
                        result.VPath = vpath;
                        if (MemberDefinitionIn(moduleLines, word.Text, {}, true, foundLine, foundColumn))
                        {
                            result.Line = foundLine;
                            result.Column = foundColumn;
                        }
                        return result;
                    }
                }
                if (MemberDefinitionIn(lines, word.Text, receiver, receiver == "self", foundLine, foundColumn) && !(foundLine == line && foundColumn == word.Start))
                {
                    result.Found = true;
                    result.Line = foundLine;
                    result.Column = foundColumn;
                    return result;
                }
            }

            if (describeBuiltin)
                result.Info = describeBuiltin(root.empty() ? receiver : receiver, std::string(1, before) + word.Text);
            return result;
        }

        int foundLine = 0;
        int foundColumn = 0;
        if (FindLocalDeclaration(lines, line, word.Start, word.Text, foundLine, foundColumn))
        {
            result.Found = true;
            result.Line = foundLine;
            result.Column = foundColumn;
            return result;
        }

        std::string module;
        if (RequireAlias(lines, word.Text, module) && loader)
        {
            std::string vpath;
            std::vector<std::string> moduleLines;
            if (loader(module, vpath, moduleLines))
            {
                result.Found = true;
                result.VPath = vpath;
                return result;
            }
        }

        if (describeBuiltin)
            result.Info = describeBuiltin(word.Text, {});
        return result;
    }
}

#include "LuaLexer.h"

#include <algorithm>
#include <cctype>

namespace ScriptEditor
{
    namespace
    {
        constexpr LexState Normal = 0;
        constexpr LexState LongComment = 1;
        constexpr LexState LongString = 2;

        LexState Pack(LexState kind, int level) { return static_cast<LexState>(kind | (level << 2)); }
        LexState KindOf(LexState state) { return state & 3; }
        int LevelOf(LexState state) { return state >> 2; }

        bool IsIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
        bool IsIdent(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

        int LongBracketLevel(const std::string &line, size_t i)
        {
            if (i >= line.size() || line[i] != '[')
                return -1;
            size_t j = i + 1;
            int level = 0;
            while (j < line.size() && line[j] == '=')
            {
                ++level;
                ++j;
            }
            return j < line.size() && line[j] == '[' ? level : -1;
        }

        size_t FindLongClose(const std::string &line, size_t from, int level)
        {
            for (size_t i = from; i < line.size(); ++i)
            {
                if (line[i] != ']')
                    continue;
                size_t j = i + 1;
                int count = 0;
                while (j < line.size() && line[j] == '=')
                {
                    ++count;
                    ++j;
                }
                if (count == level && j < line.size() && line[j] == ']')
                    return j + 1;
            }
            return std::string::npos;
        }

        const std::unordered_set<std::string> &ControlWords()
        {
            static const std::unordered_set<std::string> s{"if", "then", "else", "elseif", "end", "for", "while", "do", "repeat", "until", "return", "break", "goto", "in"};
            return s;
        }
    }

    const std::vector<std::string> &Keywords()
    {
        static const std::vector<std::string> s{"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local",
                                                "nil", "not", "or", "repeat", "return", "then", "true", "until", "while", "self"};
        return s;
    }

    bool IsKeyword(const std::string &word)
    {
        static const std::unordered_set<std::string> s(Keywords().begin(), Keywords().end());
        return s.count(word) != 0;
    }

    LexState TokenizeLine(const std::string &line, LexState state, std::vector<Token> &out, const std::unordered_set<std::string> *globals)
    {
        out.clear();
        size_t i = 0;
        const size_t n = line.size();

        if (KindOf(state) != Normal)
        {
            size_t close = FindLongClose(line, 0, LevelOf(state));
            TokenKind kind = KindOf(state) == LongComment ? TokenKind::Comment : TokenKind::String;
            if (close == std::string::npos)
            {
                if (n)
                    out.push_back({0, static_cast<int>(n), kind});
                return state;
            }
            out.push_back({0, static_cast<int>(close), kind});
            i = close;
            state = Normal;
        }

        bool inFunctionName = false;
        char previousPunct = 0;

        while (i < n)
        {
            char c = line[i];
            if (c == ' ' || c == '\t')
            {
                ++i;
                continue;
            }

            if (c == '-' && i + 1 < n && line[i + 1] == '-')
            {
                int level = LongBracketLevel(line, i + 2);
                if (level >= 0)
                {
                    size_t close = FindLongClose(line, i + 4 + level, level);
                    if (close == std::string::npos)
                    {
                        out.push_back({static_cast<int>(i), static_cast<int>(n - i), TokenKind::Comment});
                        return Pack(LongComment, level);
                    }
                    out.push_back({static_cast<int>(i), static_cast<int>(close - i), TokenKind::Comment});
                    i = close;
                    continue;
                }
                out.push_back({static_cast<int>(i), static_cast<int>(n - i), TokenKind::Comment});
                return Normal;
            }

            if (c == '[')
            {
                int level = LongBracketLevel(line, i);
                if (level >= 0)
                {
                    size_t close = FindLongClose(line, i + 2 + level, level);
                    if (close == std::string::npos)
                    {
                        out.push_back({static_cast<int>(i), static_cast<int>(n - i), TokenKind::String});
                        return Pack(LongString, level);
                    }
                    out.push_back({static_cast<int>(i), static_cast<int>(close - i), TokenKind::String});
                    i = close;
                    continue;
                }
            }

            if (c == '"' || c == '\'')
            {
                size_t j = i + 1;
                while (j < n && line[j] != c)
                    j += line[j] == '\\' ? 2 : 1;
                j = std::min(j + 1, n);
                out.push_back({static_cast<int>(i), static_cast<int>(j - i), TokenKind::String});
                i = j;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(line[i + 1]))))
            {
                size_t j = i;
                bool hex = c == '0' && i + 1 < n && (line[i + 1] == 'x' || line[i + 1] == 'X');
                if (hex)
                    j += 2;
                while (j < n)
                {
                    char d = line[j];
                    bool exponent = hex ? (d == 'p' || d == 'P') : (d == 'e' || d == 'E');
                    if (exponent && j + 1 < n && (line[j + 1] == '+' || line[j + 1] == '-'))
                        j += 2;
                    else if (std::isalnum(static_cast<unsigned char>(d)) || d == '.')
                        ++j;
                    else
                        break;
                }
                out.push_back({static_cast<int>(i), static_cast<int>(j - i), TokenKind::Number});
                i = j;
                continue;
            }

            if (IsIdentStart(c))
            {
                size_t j = i;
                while (j < n && IsIdent(line[j]))
                    ++j;
                std::string word = line.substr(i, j - i);

                size_t k = j;
                while (k < n && (line[k] == ' ' || line[k] == '\t'))
                    ++k;
                bool call = k < n && (line[k] == '(' || line[k] == '"' || line[k] == '\'' || line[k] == '{');

                bool qualified = k < n && (line[k] == '.' || line[k] == ':');
                TokenKind kind = TokenKind::Default;
                if (word == "local" || word == "function")
                    kind = TokenKind::Storage;
                else if (word == "and" || word == "or" || word == "not")
                    kind = TokenKind::Operator;
                else if (word == "nil" || word == "true" || word == "false")
                    kind = TokenKind::Constant;
                else if (word == "self")
                    kind = TokenKind::Self;
                else if (ControlWords().count(word))
                    kind = TokenKind::Keyword;
                else if (inFunctionName && !qualified)
                    kind = TokenKind::Function;
                else if (previousPunct == '.' || previousPunct == ':')
                    kind = call ? TokenKind::Function : TokenKind::Member;
                else if (call)
                    kind = TokenKind::Function;
                else if (globals && globals->count(word))
                    kind = TokenKind::Global;

                out.push_back({static_cast<int>(i), static_cast<int>(j - i), kind});
                if (word == "function")
                    inFunctionName = true;
                else if (!qualified)
                    inFunctionName = false;
                previousPunct = 0;
                i = j;
                continue;
            }

            bool punct = c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ';' || c == '.' || c == ':';
            size_t j = i + 1;
            if (c == '.' && j < n && line[j] == '.')
            {
                punct = false;
                ++j;
                if (j < n && line[j] == '.')
                    ++j;
            }
            else if (!punct && j < n && (line[j] == '=' || (c == '/' && line[j] == '/') || (c == '<' && line[j] == '<') || (c == '>' && line[j] == '>')))
            {
                ++j;
            }
            else if (c == ':' && j < n && line[j] == ':')
            {
                ++j;
            }
            out.push_back({static_cast<int>(i), static_cast<int>(j - i), punct ? TokenKind::Punctuation : TokenKind::Operator});
            previousPunct = (c == '.' && j == i + 1) || (c == ':' && j == i + 1) ? c : 0;
            if (c == '(')
                inFunctionName = false;
            i = j;
        }
        return Normal;
    }
}

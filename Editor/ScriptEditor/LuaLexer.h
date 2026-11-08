#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace ScriptEditor
{
    enum class TokenKind : uint8_t
    {
        Default,
        Keyword,
        Storage,
        Operator,
        Constant,
        Number,
        String,
        Comment,
        Function,
        Global,
        Self,
        Member,
        Punctuation,
        Count
    };

    struct Token
    {
        int Start = 0;
        int Length = 0;
        TokenKind Kind = TokenKind::Default;
    };

    using LexState = uint16_t;

    LexState TokenizeLine(const std::string &line, LexState state, std::vector<Token> &out, const std::unordered_set<std::string> *globals);
    bool IsKeyword(const std::string &word);
    const std::vector<std::string> &Keywords();
}

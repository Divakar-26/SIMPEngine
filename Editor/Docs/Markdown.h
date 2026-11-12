#pragma once

#include <string>
#include <vector>

namespace Markdown
{
    enum SpanStyle
    {
        StyleBold = 1,
        StyleItalic = 2,
        StyleCode = 4,
        StyleStrike = 8,
        StyleLink = 16,
        StyleImage = 32
    };

    struct Span
    {
        std::string Text;
        int Style = 0;
        std::string Url;
    };

    enum class BlockType
    {
        Heading,
        Paragraph,
        Code,
        Quote,
        ListItem,
        Table,
        Rule
    };

    struct Block
    {
        BlockType Type = BlockType::Paragraph;
        int Level = 0;
        bool Ordered = false;
        int Number = 0;
        std::vector<Span> Spans;
        std::string Code;
        std::string Lang;
        std::string Anchor;
        std::vector<std::vector<std::vector<Span>>> Rows;
        std::vector<int> Align;
        bool HasHeader = false;
    };

    struct Document
    {
        std::vector<Block> Blocks;
        std::string Title;
    };

    Document Parse(const std::string &text);
    std::vector<Span> ParseInline(const std::string &text);
    std::string Slug(const std::string &heading);
    std::string PlainText(const std::vector<Span> &spans);
}

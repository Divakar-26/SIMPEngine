#include "TestFramework.h"

#include "Docs/Markdown.h"

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace Markdown;

TEST_CASE("Markdown parses headings, paragraphs and anchors")
{
    const Document doc = Parse("# Hello World\n\nSome *text* here\nwith a second line.\n\n## Part 2: Go!\n");
    CHECK(doc.Title == "Hello World");
    CHECK(doc.Blocks.size() == 3);
    CHECK(doc.Blocks[0].Type == BlockType::Heading);
    CHECK(doc.Blocks[0].Level == 1);
    CHECK(doc.Blocks[0].Anchor == "hello-world");
    CHECK(doc.Blocks[1].Type == BlockType::Paragraph);
    CHECK(PlainText(doc.Blocks[1].Spans) == "Some text here with a second line.");
    CHECK(doc.Blocks[2].Anchor == "part-2-go");
    CHECK(Slug("`Scene` API (Lua)") == "scene-api-lua");
}

TEST_CASE("Markdown inline styles, links, code and escapes")
{
    const auto spans = ParseInline("a **bold** and *it* `co*de` ~~gone~~ [link](https://x.org \"t\") \\*lit\\* snake_case_word");
    bool bold = false, italic = false, code = false, strike = false, link = false;
    std::string url;
    for (const Span &span : spans)
    {
        if (span.Text == "bold" && (span.Style & StyleBold))
            bold = true;
        if (span.Text == "it" && (span.Style & StyleItalic))
            italic = true;
        if (span.Text == "co*de" && (span.Style & StyleCode))
            code = true;
        if (span.Text == "gone" && (span.Style & StyleStrike))
            strike = true;
        if (span.Text == "link" && (span.Style & StyleLink))
        {
            link = true;
            url = span.Url;
        }
    }
    CHECK(bold);
    CHECK(italic);
    CHECK(code);
    CHECK(strike);
    CHECK(link);
    CHECK(url == "https://x.org");
    const std::string plain = PlainText(spans);
    CHECK(plain.find("*lit*") != std::string::npos);
    CHECK(plain.find("snake_case_word") != std::string::npos);

    const auto nested = ParseInline("**bold with `code` inside**");
    CHECK(nested.size() == 3);
    CHECK((nested[1].Style & StyleCode) != 0);
    CHECK((nested[1].Style & StyleBold) != 0);

    const auto image = ParseInline("![alt text](images/a.png)");
    CHECK(image.size() == 1);
    CHECK((image[0].Style & StyleImage) != 0);
    CHECK(image[0].Url == "images/a.png");
}

TEST_CASE("Markdown lists, quotes, rules and code blocks")
{
    const Document doc = Parse(
        "- one\n"
        "- two\n"
        "  continued\n"
        "    - nested\n"
        "1. first\n"
        "2. second\n"
        "\n"
        "> quoted\n"
        "> text\n"
        "\n"
        "---\n"
        "\n"
        "```lua\n"
        "local x = 1\n"
        "  print(x)\n"
        "```\n");
    CHECK(doc.Blocks.size() == 8);
    CHECK(doc.Blocks[0].Type == BlockType::ListItem);
    CHECK(PlainText(doc.Blocks[1].Spans) == "two continued");
    CHECK(doc.Blocks[2].Level == 1);
    CHECK(doc.Blocks[3].Ordered);
    CHECK(doc.Blocks[4].Number == 2);
    CHECK(doc.Blocks[5].Type == BlockType::Quote);
    CHECK(PlainText(doc.Blocks[5].Spans) == "quoted text");
    CHECK(doc.Blocks[6].Type == BlockType::Rule);
    CHECK(doc.Blocks[7].Type == BlockType::Code);
    CHECK(doc.Blocks[7].Lang == "lua");
    CHECK(doc.Blocks[7].Code == "local x = 1\n  print(x)");
}

TEST_CASE("Markdown tables keep cells, alignment and escaped pipes")
{
    const Document doc = Parse(
        "| Name | Value |\n"
        "|:--|--:|\n"
        "| `a|b` | 1 |\n"
        "| x \\| y | **2** |\n"
        "after\n");
    CHECK(doc.Blocks.size() == 2);
    const Block &table = doc.Blocks[0];
    CHECK(table.Type == BlockType::Table);
    CHECK(table.Rows.size() == 3);
    CHECK(table.Rows[0].size() == 2);
    CHECK(table.Align[0] == 0);
    CHECK(table.Align[1] == 2);
    CHECK(PlainText(table.Rows[1][0]) == "a|b");
    CHECK(PlainText(table.Rows[2][0]) == "x | y");
    CHECK(doc.Blocks[1].Type == BlockType::Paragraph);
}

TEST_CASE("Markdown copes with the engine's own docs style")
{
    const Document doc = Parse("| `Scene` | `Spawn(name [, x, y [, parent]])`, `Clone(entityOrName)` (copies, returns the copy or `nil`) |\n\nUnclosed *star and `tick\n");
    CHECK(!doc.Blocks.empty());
}

TEST_CASE("Markdown parses every guide in docs without losing structure")
{
    const std::filesystem::path docs = std::filesystem::path(SIMP_TEST_ASSETS).parent_path() / "docs";
    if (!std::filesystem::is_directory(docs))
        return;
    int files = 0;
    for (const auto &entry : std::filesystem::directory_iterator(docs))
    {
        if (entry.path().extension() != ".md")
            continue;
        std::ifstream stream(entry.path());
        std::stringstream buffer;
        buffer << stream.rdbuf();
        const Document doc = Parse(buffer.str());
        CHECK(!doc.Blocks.empty());
        CHECK(!doc.Title.empty());
        ++files;
    }
    CHECK(files >= 5);
}

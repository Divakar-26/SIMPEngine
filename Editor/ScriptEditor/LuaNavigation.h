#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ScriptEditor
{
    struct Definition
    {
        bool Found = false;
        std::string VPath;
        int Line = 0;
        int Column = 0;
        int Length = 0;
        std::string Label;
        std::string Info;
    };

    using ModuleLoader = std::function<bool(const std::string &module, std::string &vpath, std::vector<std::string> &lines)>;

    struct WordSpan
    {
        std::string Text;
        int Start = 0;
        int End = 0;
    };

    WordSpan WordAtColumn(const std::string &line, int column);

    Definition ResolveSymbol(const std::vector<std::string> &lines, int line, int column, const ModuleLoader &loader, const std::function<std::string(const std::string &, const std::string &)> &describeBuiltin);
}

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ScriptEditor
{
    enum class CompletionKind
    {
        Keyword,
        Function,
        Method,
        Field,
        Module,
        Constant,
        Variable,
        Hook
    };

    struct CompletionItem
    {
        std::string Label;
        std::string Detail;
        CompletionKind Kind = CompletionKind::Variable;
        bool Callable = false;
    };

    struct CompletionResult
    {
        std::vector<CompletionItem> Items;
        int ReplaceStart = 0;
        std::string Prefix;
        bool Member = false;
    };

    class LuaCompletion
    {
    public:
        void RefreshApi();
        bool HasApi() const { return m_Loaded; }

        CompletionResult Query(const std::vector<std::string> &lines, int line, int column, bool force);
        const std::unordered_set<std::string> &GlobalNames() const { return m_GlobalNames; }

    private:
        std::vector<CompletionItem> TypeMembers(const std::string &type) const;
        std::string InferType(const std::vector<std::string> &lines, int line, const std::string &expression, int depth) const;
        void Rank(std::vector<CompletionItem> &items, const std::string &prefix, CompletionResult &result) const;

        bool m_Loaded = false;
        std::vector<CompletionItem> m_Globals;
        std::unordered_set<std::string> m_GlobalNames;
        std::unordered_map<std::string, std::vector<CompletionItem>> m_Tables;
        std::unordered_map<std::string, std::vector<CompletionItem>> m_Types;
    };
}

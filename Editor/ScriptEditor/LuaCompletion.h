#pragma once

#include "LuaNavigation.h"

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
        std::string Insert{};
        int CursorOffset = -1;
        bool Curve = false;
    };

    struct CompletionResult
    {
        std::vector<CompletionItem> Items;
        int ReplaceStart = 0;
        std::string Prefix;
        bool Member = false;
        bool ForceOpen = false;
        bool InString = false;
    };

    class LuaCompletion
    {
    public:
        void RefreshApi();
        bool HasApi() const { return m_Loaded; }

        CompletionResult Query(const std::vector<std::string> &lines, int line, int column, bool force);
        const std::unordered_set<std::string> &GlobalNames() const { return m_GlobalNames; }

        void SetModuleLoader(ModuleLoader loader) { m_ModuleLoader = std::move(loader); }
        const ModuleLoader &GetModuleLoader() const { return m_ModuleLoader; }
        std::string Describe(const std::string &name, const std::string &member) const;
        std::string ReceiverType(const std::vector<std::string> &lines, int line, const std::string &receiver) const;

        const std::unordered_map<std::string, std::vector<CompletionItem>> &Tables() const { return m_Tables; }
        const std::unordered_map<std::string, std::vector<CompletionItem>> &Types() const { return m_Types; }
        const std::vector<CompletionItem> &Globals() const { return m_Globals; }

    private:
        std::vector<CompletionItem> TypeMembers(const std::string &type) const;
        std::string InferType(const std::vector<std::string> &lines, int line, const std::string &expression, int depth) const;
        void Rank(std::vector<CompletionItem> &items, const std::string &prefix, CompletionResult &result) const;

        std::vector<CompletionItem> RoleItems(int role, bool inString) const;

        ModuleLoader m_ModuleLoader;
        bool m_Loaded = false;
        std::vector<CompletionItem> m_Globals;
        std::unordered_set<std::string> m_GlobalNames;
        std::unordered_map<std::string, std::vector<CompletionItem>> m_Tables;
        std::unordered_map<std::string, std::vector<CompletionItem>> m_Types;
    };
}

#include "LuaCompletion.h"
#include "LuaContext.h"
#include "LuaLexer.h"

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/Tween.h>

#include <Engine/Scripting/LuaInclude.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <algorithm>
#include <cctype>
#include <regex>

namespace ScriptEditor
{
    namespace
    {
        const char *Hooks[] = {"OnCreate", "OnUpdate", "OnFixedUpdate", "OnRender", "OnDestroy", "OnTriggerEnter", "OnTriggerStay",
                               "OnTriggerExit", "OnCollision", "OnTimer", "OnDamaged", "OnDeath", "OnPathPoint", "OnPathEnd"};

        bool IsIdent(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

        bool IsIdentifier(const std::string &s)
        {
            if (s.empty() || std::isdigit(static_cast<unsigned char>(s[0])))
                return false;
            return std::all_of(s.begin(), s.end(), IsIdent);
        }

        bool IsIdentifierPath(const std::string &s)
        {
            if (s.empty() || s.front() == '.' || s.back() == '.' || s.find("..") != std::string::npos)
                return false;
            return std::all_of(s.begin(), s.end(), [](char c) { return IsIdent(c) || c == '.'; }) && !std::isdigit(static_cast<unsigned char>(s[0]));
        }

        std::string EscapeDots(const std::string &s)
        {
            std::string out;
            for (char c : s)
            {
                if (c == '.')
                    out += '\\';
                out += c;
            }
            return out;
        }

        std::string Lower(std::string s)
        {
            for (char &c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        std::string Trim(const std::string &s)
        {
            size_t a = s.find_first_not_of(" \t");
            if (a == std::string::npos)
                return {};
            size_t b = s.find_last_not_of(" \t");
            return s.substr(a, b - a + 1);
        }

        bool InsideStringOrComment(const std::string &text)
        {
            char quote = 0;
            for (size_t i = 0; i < text.size(); ++i)
            {
                char c = text[i];
                if (quote)
                {
                    if (c == '\\')
                        ++i;
                    else if (c == quote)
                        quote = 0;
                    continue;
                }
                if (c == '"' || c == '\'')
                    quote = c;
                else if (c == '-' && i + 1 < text.size() && text[i + 1] == '-')
                    return true;
            }
            return quote != 0;
        }

        std::string ReceiverBefore(const std::string &text, size_t end)
        {
            int depth = 0;
            size_t i = end;
            while (i > 0)
            {
                char c = text[i - 1];
                if (c == ')' || c == ']')
                    ++depth;
                else if (c == '(' || c == '[')
                {
                    if (depth == 0)
                        break;
                    --depth;
                }
                else if (depth == 0 && !(IsIdent(c) || c == '.' || c == ':'))
                    break;
                --i;
            }
            return text.substr(i, end - i);
        }
    }

    void LuaCompletion::RefreshApi()
    {
        auto &runtime = SIMPEngine::LuaRuntime::Get();
        if (!runtime.IsInitialized())
            runtime.Initialize();
        lua_State *L = runtime.GetState();
        if (!L)
            return;

        m_Globals.clear();
        m_GlobalNames.clear();
        m_Tables.clear();
        m_Types.clear();
        int top = lua_gettop(L);

        lua_pushglobaltable(L);
        lua_pushnil(L);
        while (lua_next(L, -2))
        {
            if (lua_type(L, -2) == LUA_TSTRING)
            {
                std::string name = lua_tostring(L, -2);
                int type = lua_type(L, -1);
                if (!name.empty() && name[0] != '_')
                {
                    CompletionItem item{name, lua_typename(L, type), CompletionKind::Variable, false};
                    if (type == LUA_TFUNCTION)
                    {
                        item.Kind = CompletionKind::Function;
                        item.Callable = true;
                    }
                    else if (type == LUA_TTABLE)
                    {
                        item.Kind = CompletionKind::Module;
                        item.Detail = "table";
                        m_GlobalNames.insert(name);
                        std::vector<CompletionItem> fields;
                        lua_pushnil(L);
                        while (lua_next(L, -2))
                        {
                            if (lua_type(L, -2) == LUA_TSTRING)
                            {
                                std::string field = lua_tostring(L, -2);
                                int fieldType = lua_type(L, -1);
                                CompletionItem f{field, name, CompletionKind::Field, false};
                                if (fieldType == LUA_TFUNCTION)
                                {
                                    f.Kind = CompletionKind::Function;
                                    f.Callable = true;
                                }
                                else if (fieldType == LUA_TNUMBER)
                                {
                                    f.Kind = CompletionKind::Constant;
                                }
                                else if (fieldType == LUA_TTABLE)
                                {
                                    f.Kind = CompletionKind::Module;
                                }
                                if (!field.empty() && field[0] != '_')
                                    fields.push_back(f);
                            }
                            lua_pop(L, 1);
                        }
                        m_Tables[name] = std::move(fields);
                    }
                    m_Globals.push_back(item);
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        lua_pushnil(L);
        while (lua_next(L, LUA_REGISTRYINDEX))
        {
            if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TTABLE)
            {
                std::string key = lua_tostring(L, -2);
                if (key.rfind("SIMP.", 0) == 0)
                {
                    std::string type = key.substr(5);
                    lua_getfield(L, -1, "__index");
                    if (lua_type(L, -1) == LUA_TTABLE)
                    {
                        std::vector<CompletionItem> methods;
                        lua_pushnil(L);
                        while (lua_next(L, -2))
                        {
                            if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TFUNCTION)
                                methods.push_back({lua_tostring(L, -2), type, CompletionKind::Method, true});
                            lua_pop(L, 1);
                        }
                        m_Types[type] = std::move(methods);
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }

        lua_settop(L, top);
        m_Loaded = true;
    }

    std::vector<CompletionItem> LuaCompletion::RoleItems(int roleValue, bool inString) const
    {
        const ArgRole role = static_cast<ArgRole>(roleValue);
        std::vector<CompletionItem> items;

        auto propertyItems = [&](bool quoted)
        {
            for (int i = 0; i < static_cast<int>(SIMPEngine::TweenProperty::Count); ++i)
            {
                const auto property = static_cast<SIMPEngine::TweenProperty>(i);
                const std::string name = SIMPEngine::TweenSystem::PropertyName(property);
                const int channels = SIMPEngine::TweenSystem::ChannelCount(property);
                CompletionItem item;
                item.Label = name;
                item.Detail = channels == 1 ? "number" : channels == 2 ? "{x, y}" : "{r, g, b, a}";
                item.Kind = CompletionKind::Field;
                item.Insert = quoted ? std::string() : name + " = ";
                items.push_back(std::move(item));
            }
        };

        switch (role)
        {
        case ArgRole::Curve:
            for (const std::string &name : SIMPEngine::Curve::Names())
            {
                CompletionItem item;
                item.Label = name;
                item.Detail = "curve";
                item.Kind = CompletionKind::Constant;
                item.Curve = true;
                if (!inString)
                    item.Insert = "\"" + name + "\"";
                items.push_back(std::move(item));
            }
            break;
        case ArgRole::PropertyKeys:
            propertyItems(false);
            break;
        case ArgRole::PropertyName:
            propertyItems(true);
            break;
        case ArgRole::BusName:
            for (const char *bus : {"sfx", "music", "ui"})
            {
                CompletionItem item;
                item.Label = bus;
                item.Detail = "audio bus";
                item.Kind = CompletionKind::Constant;
                if (!inString)
                    item.Insert = std::string("\"") + bus + "\"";
                items.push_back(std::move(item));
            }
            break;
        case ArgRole::TweenOptionKeys:
        case ArgRole::SpringOptionKeys:
        case ArgRole::AlongOptionKeys:
        case ArgRole::DrawPathOptionKeys:
            for (const OptionKey &key : OptionKeysFor(role))
            {
                CompletionItem item;
                item.Label = key.Name;
                item.Detail = key.Detail;
                item.Kind = CompletionKind::Field;
                if (key.Insert)
                {
                    item.Insert = key.Insert;
                    const std::string marker = "function() ";
                    const size_t at = item.Insert.find(marker);
                    if (at != std::string::npos)
                        item.CursorOffset = static_cast<int>(at + marker.size());
                    else if (const size_t paren = item.Insert.find("function("); paren != std::string::npos)
                        item.CursorOffset = static_cast<int>(item.Insert.find(')', paren) + 2);
                }
                else
                    item.Insert = std::string(key.Name) + " = ";
                items.push_back(std::move(item));
            }
            break;
        default:
            break;
        }
        return items;
    }

    std::string LuaCompletion::Describe(const std::string &name, const std::string &member) const
    {
        auto format = [](const Signature &s)
        {
            std::string text = s.Callee + "(";
            for (size_t i = 0; i < s.Params.size(); ++i)
                text += (i ? ", " : "") + s.Params[i];
            return text + ")  " + s.Summary;
        };

        if (!member.empty())
        {
            const std::string key = member[0] == '.' ? name + member : member;
            if (const Signature *s = FindSignature(key))
                return format(*s);
            if (member[0] == ':')
            {
                const std::string method = member.substr(1);
                for (const auto &[type, methods] : m_Types)
                    for (const CompletionItem &m : methods)
                        if (m.Label == method)
                            return type + ":" + method + "()  engine method";
            }
            if (auto table = m_Tables.find(name); table != m_Tables.end())
                return name + member + "  engine API";
            return {};
        }
        if (m_GlobalNames.count(name))
            return name + "  engine table";
        for (const CompletionItem &g : m_Globals)
            if (g.Label == name)
                return name + "  engine " + g.Detail;
        return {};
    }

    std::string LuaCompletion::ReceiverType(const std::vector<std::string> &lines, int line, const std::string &receiver) const
    {
        if (receiver.empty())
            return {};
        const std::string type = InferType(lines, line, receiver, 0);
        return type == "Self" ? std::string() : type;
    }

    std::vector<CompletionItem> LuaCompletion::TypeMembers(const std::string &type) const
    {
        if (auto it = m_Types.find(type); it != m_Types.end())
            return it->second;
        return {};
    }

    std::string LuaCompletion::InferType(const std::vector<std::string> &lines, int line, const std::string &expression, int depth) const
    {
        std::string expr = Trim(expression);
        if (expr.empty() || depth > 3)
            return {};
        if (expr == "self")
            return "Self";

        if (expr.back() == ')')
        {
            static const std::regex call(R"(([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*(?:["']([A-Za-z]+)["'])?[^()]*\)$)");
            std::smatch m;
            if (std::regex_search(expr, m, call))
            {
                std::string name = m[1];
                std::string argument = m[2];
                if ((name == "Get" || name == "Add") && !argument.empty() && m_Types.count(argument))
                    return argument;
                if (name.rfind("Get", 0) == 0 && m_Types.count(name.substr(3)))
                    return name.substr(3);
                if (name == "FindByName" || name == "GetParent" || name == "Spawn" || name == "Find")
                    return "Entity";
            }
            return {};
        }

        size_t cut = expr.find_last_of(".:");
        std::string last = cut == std::string::npos ? expr : expr.substr(cut + 1);
        if (last == "entity" || last == "other" || last == "killer" || last == "source")
            return "Entity";

        if (cut != std::string::npos || !IsIdentifier(last))
            return {};

        std::regex assign("\\b" + last + R"(\s*=\s*([^=].*)$)");
        std::regex loop(R"(for\s+[A-Za-z_,\s]*\b)" + last + R"(\b\s+in\s+.*(GetChildren|FindAll|GetInside))");
        for (int i = std::min(line, static_cast<int>(lines.size()) - 1); i >= 0; --i)
        {
            std::smatch m;
            if (std::regex_search(lines[static_cast<size_t>(i)], m, loop))
                return "Entity";
            if (std::regex_search(lines[static_cast<size_t>(i)], m, assign))
            {
                std::string rhs = Trim(m[1]);
                if (size_t comment = rhs.find("--"); comment != std::string::npos)
                    rhs = Trim(rhs.substr(0, comment));
                return InferType(lines, i - 1, rhs, depth + 1);
            }
        }
        return {};
    }

    void LuaCompletion::Rank(std::vector<CompletionItem> &items, const std::string &prefix, CompletionResult &result) const
    {
        std::string lowerPrefix = Lower(prefix);
        struct Scored
        {
            int Score;
            CompletionItem Item;
        };
        std::vector<Scored> scored;
        std::unordered_set<std::string> seen;
        bool exact = false;
        for (const auto &item : items)
        {
            if (!prefix.empty() && item.Label == prefix)
            {
                if (item.Kind == CompletionKind::Keyword)
                    return;
                exact = true;
            }
        }
        for (auto &item : items)
        {
            if (item.Label == prefix || !seen.insert(item.Label).second)
                continue;
            std::string lower = Lower(item.Label);
            int score = -1;
            if (prefix.empty())
                score = 3;
            else if (item.Label.rfind(prefix, 0) == 0)
                score = 0;
            else if (lower.rfind(lowerPrefix, 0) == 0)
                score = 1;
            else if (!exact && prefix.size() >= 2 && lower.find(lowerPrefix) != std::string::npos)
                score = 2;
            if (score >= 0)
                scored.push_back({score, item});
        }
        std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b)
                  {
            if (a.Score != b.Score)
                return a.Score < b.Score;
            if (a.Item.Kind != b.Item.Kind && (a.Item.Kind == CompletionKind::Keyword || b.Item.Kind == CompletionKind::Keyword))
                return b.Item.Kind == CompletionKind::Keyword;
            return a.Item.Label < b.Item.Label; });
        if (scored.size() > 80)
            scored.resize(80);
        for (auto &s : scored)
            result.Items.push_back(std::move(s.Item));
    }

    CompletionResult LuaCompletion::Query(const std::vector<std::string> &lines, int line, int column, bool force)
    {
        CompletionResult result;
        if (line < 0 || line >= static_cast<int>(lines.size()))
            return result;
        if (!m_Loaded)
            RefreshApi();

        const std::string &full = lines[static_cast<size_t>(line)];
        std::string text = full.substr(0, std::min<size_t>(static_cast<size_t>(column), full.size()));

        const CallContext context = AnalyzeContext(lines, line, column);
        if (context.InComment)
            return result;
        const ArgRole role = ResolveRole(context);
        const bool stringRole = role == ArgRole::Curve || role == ArgRole::PropertyName || role == ArgRole::BusName;
        if (context.InString && !stringRole)
            return result;
        if (!context.InString && InsideStringOrComment(text))
            return result;

        size_t start = text.size();
        while (start > 0 && IsIdent(text[start - 1]))
            --start;
        result.Prefix = text.substr(start);
        result.ReplaceStart = static_cast<int>(start);
        if (!result.Prefix.empty() && std::isdigit(static_cast<unsigned char>(result.Prefix[0])))
            return result;

        char trigger = start > 0 ? text[start - 1] : 0;
        if (trigger == '.' && start > 1 && text[start - 2] == '.')
            trigger = 0;
        std::vector<CompletionItem> items;

        if (context.InString)
        {
            result.InString = true;
            result.ForceOpen = true;
            std::vector<CompletionItem> roleItems = RoleItems(static_cast<int>(role), true);
            Rank(roleItems, result.Prefix, result);
            return result;
        }

        if (trigger != '.' && trigger != ':' && role != ArgRole::None)
        {
            std::vector<CompletionItem> roleItems = RoleItems(static_cast<int>(role), false);
            roleItems.erase(std::remove_if(roleItems.begin(), roleItems.end(), [&](const CompletionItem &item)
                                           { return std::find(context.UsedKeys.begin(), context.UsedKeys.end(), item.Label) != context.UsedKeys.end(); }),
                            roleItems.end());
            Rank(roleItems, result.Prefix, result);
            if (!result.Items.empty())
            {
                result.ForceOpen = true;
                return result;
            }
        }

        static const std::regex word(R"([A-Za-z_][A-Za-z0-9_]*)");
        static const std::regex selfField(R"(\bself\.([A-Za-z_][A-Za-z0-9_]*))");
        static const std::regex method(R"(function\s+([A-Za-z_][A-Za-z0-9_]*)\s*[:.]\s*([A-Za-z_][A-Za-z0-9_]*))");

        if (trigger == '.' || trigger == ':')
        {
            result.Member = true;
            std::string receiver = ReceiverBefore(text, start - 1);
            if (trigger == '.')
            {
                if (auto it = m_Tables.find(receiver); it != m_Tables.end())
                {
                    items = it->second;
                }
                else
                {
                    if (receiver == "self")
                        items.push_back({"entity", "Entity", CompletionKind::Field, false});
                    std::regex field(IsIdentifierPath(receiver) ? "\\b" + EscapeDots(receiver) + R"(\.([A-Za-z_][A-Za-z0-9_]*))" : std::string("$^"));
                    for (const std::string &l : lines)
                    {
                        for (std::sregex_iterator it(l.begin(), l.end(), field), end; it != end; ++it)
                            items.push_back({(*it)[1], "field", CompletionKind::Field, false});
                        for (std::sregex_iterator it(l.begin(), l.end(), method), end; it != end; ++it)
                            if (receiver == "self" || (*it)[1] == receiver)
                                items.push_back({(*it)[2], "function", CompletionKind::Function, true});
                    }
                }
            }
            else
            {
                std::string type = InferType(lines, line, receiver, 0);
                if (type == "Self")
                {
                    for (const std::string &l : lines)
                        for (std::sregex_iterator it(l.begin(), l.end(), method), end; it != end; ++it)
                            items.push_back({(*it)[2], "method", CompletionKind::Method, true});
                }
                else if (!type.empty())
                {
                    items = TypeMembers(type);
                }
                else
                {
                    for (const auto &[name, methods] : m_Types)
                        for (const auto &m : methods)
                            items.push_back(m);
                    std::sort(items.begin(), items.end(), [](const CompletionItem &a, const CompletionItem &b)
                              { return (a.Detail == "Entity") > (b.Detail == "Entity"); });
                }
            }
        }
        else
        {
            if (result.Prefix.empty() && !force)
                return result;
            for (const std::string &k : Keywords())
                items.push_back({k, "keyword", CompletionKind::Keyword, false});
            items.insert(items.end(), m_Globals.begin(), m_Globals.end());
            for (const char *hook : Hooks)
                items.push_back({hook, "script hook", CompletionKind::Hook, false});
            for (size_t i = 0; i < lines.size(); ++i)
            {
                const std::string &l = lines[i];
                for (std::sregex_iterator it(l.begin(), l.end(), word), end; it != end; ++it)
                {
                    std::string w = it->str();
                    if (w.size() < 2 || IsKeyword(w) || (static_cast<int>(i) == line && w == result.Prefix))
                        continue;
                    if (static_cast<int>(i) == line && it->position() + static_cast<long>(w.size()) == static_cast<long>(column))
                        continue;
                    items.push_back({w, "local", CompletionKind::Variable, false});
                }
            }
        }

        Rank(items, result.Prefix, result);
        return result;
    }
}

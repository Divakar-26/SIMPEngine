#include "CodeTheme.h"

#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace ScriptEditor
{
    namespace
    {
        namespace fs = std::filesystem;
        using json = nlohmann::json;

        struct Rule
        {
            std::string Scope;
            ImU32 Color;
        };

        struct RawTheme
        {
            std::unordered_map<std::string, ImU32> Colors;
            std::vector<Rule> Rules;
            std::string Type;
        };

        std::string ReadFile(const fs::path &path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
                return {};
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        std::string StripJsonc(const std::string &text)
        {
            std::string out;
            out.reserve(text.size());
            bool inString = false;
            for (size_t i = 0; i < text.size(); ++i)
            {
                char c = text[i];
                if (inString)
                {
                    out += c;
                    if (c == '\\' && i + 1 < text.size())
                        out += text[++i];
                    else if (c == '"')
                        inString = false;
                    continue;
                }
                if (c == '"')
                {
                    inString = true;
                    out += c;
                }
                else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/')
                {
                    while (i < text.size() && text[i] != '\n')
                        ++i;
                    out += '\n';
                }
                else if (c == '/' && i + 1 < text.size() && text[i + 1] == '*')
                {
                    i += 2;
                    while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/'))
                        ++i;
                    ++i;
                }
                else
                {
                    out += c;
                }
            }

            std::string cleaned;
            cleaned.reserve(out.size());
            inString = false;
            for (size_t i = 0; i < out.size(); ++i)
            {
                char c = out[i];
                if (inString)
                {
                    cleaned += c;
                    if (c == '\\' && i + 1 < out.size())
                        cleaned += out[++i];
                    else if (c == '"')
                        inString = false;
                    continue;
                }
                if (c == '"')
                    inString = true;
                if (c == ',')
                {
                    size_t j = i + 1;
                    while (j < out.size() && std::isspace(static_cast<unsigned char>(out[j])))
                        ++j;
                    if (j < out.size() && (out[j] == '}' || out[j] == ']'))
                        continue;
                }
                cleaned += c;
            }
            return cleaned;
        }

        bool ParseColor(const std::string &text, ImU32 &out)
        {
            if (text.size() < 4 || text[0] != '#')
                return false;
            std::string hex = text.substr(1);
            if (hex.size() == 3 || hex.size() == 4)
            {
                std::string expanded;
                for (char c : hex)
                {
                    expanded += c;
                    expanded += c;
                }
                hex = expanded;
            }
            if (hex.size() != 6 && hex.size() != 8)
                return false;
            char *end = nullptr;
            unsigned long value = std::strtoul(hex.c_str(), &end, 16);
            if (end != hex.c_str() + hex.size())
                return false;
            unsigned r, g, b, a = 255;
            if (hex.size() == 8)
            {
                r = (value >> 24) & 0xFF;
                g = (value >> 16) & 0xFF;
                b = (value >> 8) & 0xFF;
                a = value & 0xFF;
            }
            else
            {
                r = (value >> 16) & 0xFF;
                g = (value >> 8) & 0xFF;
                b = value & 0xFF;
            }
            out = IM_COL32(r, g, b, a);
            return true;
        }

        void AddScopes(const json &scope, ImU32 color, std::vector<Rule> &rules)
        {
            auto addOne = [&](std::string text)
            {
                std::stringstream parts(text);
                std::string part;
                while (std::getline(parts, part, ','))
                {
                    part.erase(0, part.find_first_not_of(" \t\n"));
                    part.erase(part.find_last_not_of(" \t\n") + 1);
                    if (!part.empty() && part.find(' ') == std::string::npos)
                        rules.push_back({part, color});
                }
            };
            if (scope.is_string())
                addOne(scope.get<std::string>());
            else if (scope.is_array())
                for (const auto &s : scope)
                    if (s.is_string())
                        addOne(s.get<std::string>());
        }

        bool LoadRaw(const fs::path &path, RawTheme &raw, int depth)
        {
            if (depth > 4)
                return false;
            std::string text = ReadFile(path);
            if (text.empty())
                return false;
            json data = json::parse(StripJsonc(text), nullptr, false);
            if (data.is_discarded() || !data.is_object())
                return false;

            if (data.contains("include") && data["include"].is_string())
                LoadRaw(path.parent_path() / data["include"].get<std::string>(), raw, depth + 1);

            if (data.contains("type") && data["type"].is_string())
                raw.Type = data["type"].get<std::string>();

            if (data.contains("colors") && data["colors"].is_object())
            {
                for (auto it = data["colors"].begin(); it != data["colors"].end(); ++it)
                {
                    ImU32 color;
                    if (it.value().is_string() && ParseColor(it.value().get<std::string>(), color))
                        raw.Colors[it.key()] = color;
                }
            }

            if (data.contains("tokenColors") && data["tokenColors"].is_array())
            {
                for (const auto &entry : data["tokenColors"])
                {
                    if (!entry.is_object() || !entry.contains("settings") || !entry["settings"].is_object())
                        continue;
                    const auto &settings = entry["settings"];
                    ImU32 color;
                    if (!settings.contains("foreground") || !settings["foreground"].is_string() || !ParseColor(settings["foreground"].get<std::string>(), color))
                        continue;
                    if (entry.contains("scope"))
                        AddScopes(entry["scope"], color, raw.Rules);
                    else
                        raw.Colors.emplace("editor.foreground", color);
                }
            }
            return true;
        }

        float Luminance(ImU32 color)
        {
            ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
            return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
        }

        ImU32 Mix(ImU32 a, ImU32 b, float t)
        {
            ImVec4 x = ImGui::ColorConvertU32ToFloat4(a);
            ImVec4 y = ImGui::ColorConvertU32ToFloat4(b);
            return ImGui::ColorConvertFloat4ToU32(ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t));
        }

        ImU32 WithAlpha(ImU32 color, int alpha)
        {
            return (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);
        }

        bool FindRule(const std::vector<Rule> &rules, const std::vector<const char *> &candidates, ImU32 &out)
        {
            for (const char *candidate : candidates)
            {
                std::string c = candidate;
                size_t bestLength = 0;
                bool found = false;
                for (const Rule &rule : rules)
                {
                    bool match = c == rule.Scope || (c.size() > rule.Scope.size() && c.compare(0, rule.Scope.size(), rule.Scope) == 0 && c[rule.Scope.size()] == '.');
                    if (match && rule.Scope.size() >= bestLength)
                    {
                        bestLength = rule.Scope.size();
                        out = rule.Color;
                        found = true;
                    }
                }
                if (found)
                    return true;
            }
            return false;
        }

        CodeTheme Resolve(const RawTheme &raw, const std::string &name)
        {
            CodeTheme t;
            t.Name = name;
            auto color = [&](std::initializer_list<const char *> keys, ImU32 fallback)
            {
                for (const char *key : keys)
                    if (auto it = raw.Colors.find(key); it != raw.Colors.end())
                        return it->second;
                return fallback;
            };

            t.Background = color({"editor.background"}, IM_COL32(30, 30, 30, 255)) | IM_COL32_A_MASK;
            t.Dark = raw.Type.empty() ? Luminance(t.Background) < 0.5f : raw.Type != "light";
            ImU32 contrast = t.Dark ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
            t.Foreground = color({"editor.foreground", "foreground"}, Mix(t.Background, contrast, 0.82f));
            t.Gutter = color({"editorGutter.background"}, t.Background) | IM_COL32_A_MASK;
            t.LineNumber = color({"editorLineNumber.foreground"}, Mix(t.Background, t.Foreground, 0.4f));
            t.LineNumberActive = color({"editorLineNumber.activeForeground"}, Mix(t.Background, t.Foreground, 0.85f));
            t.CurrentLine = color({"editor.lineHighlightBackground"}, WithAlpha(contrast, 12));
            t.Selection = color({"editor.selectionBackground"}, WithAlpha(Mix(t.Background, IM_COL32(80, 140, 255, 255), 0.6f), 160));
            t.Cursor = color({"editorCursor.foreground"}, t.Foreground);
            t.IndentGuide = color({"editorIndentGuide.background1", "editorIndentGuide.background"}, WithAlpha(contrast, 22));
            t.BracketMatch = color({"editorBracketMatch.background"}, WithAlpha(contrast, 40));
            t.FindMatch = color({"editor.findMatchHighlightBackground", "editor.findMatchBackground"}, IM_COL32(234, 179, 8, 70));
            t.Error = color({"editorError.foreground", "errorForeground"}, IM_COL32(244, 71, 71, 255));
            t.PopupBackground = color({"editorSuggestWidget.background", "editorWidget.background"}, Mix(t.Background, contrast, 0.06f)) | IM_COL32_A_MASK;
            t.PopupForeground = color({"editorSuggestWidget.foreground", "editorWidget.foreground"}, t.Foreground);
            t.PopupSelected = color({"editorSuggestWidget.selectedBackground", "list.activeSelectionBackground", "list.focusBackground"}, Mix(t.Background, IM_COL32(80, 140, 255, 255), 0.45f));
            t.PopupBorder = color({"editorSuggestWidget.border", "editorWidget.border", "widget.border"}, Mix(t.Background, contrast, 0.18f));
            t.PopupHighlight = color({"editorSuggestWidget.highlightForeground", "list.highlightForeground", "focusBorder"}, IM_COL32(90, 160, 255, 255));

            auto token = [&](TokenKind kind, std::vector<const char *> scopes, ImU32 fallback)
            {
                ImU32 value = fallback;
                FindRule(raw.Rules, scopes, value);
                t.Tokens[static_cast<size_t>(kind)] = value;
            };
            token(TokenKind::Default, {"variable.other.readwrite", "source.lua"}, t.Foreground);
            token(TokenKind::Keyword, {"keyword.control.lua", "keyword.control", "keyword"}, t.Foreground);
            ImU32 keyword = t.Tokens[static_cast<size_t>(TokenKind::Keyword)];
            token(TokenKind::Storage, {"storage.type.function", "storage.type", "storage.modifier", "storage", "keyword.local"}, keyword);
            token(TokenKind::Operator, {"keyword.operator"}, t.Foreground);
            token(TokenKind::Constant, {"constant.language", "constant"}, keyword);
            token(TokenKind::Number, {"constant.numeric", "constant"}, t.Foreground);
            token(TokenKind::String, {"string.quoted", "string"}, t.Foreground);
            token(TokenKind::Comment, {"comment.line", "comment"}, Mix(t.Background, t.Foreground, 0.5f));
            token(TokenKind::Function, {"entity.name.function", "support.function", "meta.function-call", "variable.function"}, t.Foreground);
            token(TokenKind::Global, {"support.class", "entity.name.type", "support.type", "entity.name.class", "support.function"}, t.Tokens[static_cast<size_t>(TokenKind::Function)]);
            token(TokenKind::Self, {"variable.language.self", "variable.language", "constant.language"}, keyword);
            token(TokenKind::Member, {"variable.other.property", "variable.other.member", "support.variable.property", "variable.other.object.property"}, t.Foreground);
            token(TokenKind::Punctuation, {"punctuation.separator", "punctuation"}, Mix(t.Background, t.Foreground, 0.8f));
            return t;
        }

        std::string ResolveLabel(const std::string &label, const fs::path &extension)
        {
            if (label.size() < 3 || label.front() != '%' || label.back() != '%')
                return label;
            std::string key = label.substr(1, label.size() - 2);
            json nls = json::parse(StripJsonc(ReadFile(extension / "package.nls.json")), nullptr, false);
            if (!nls.is_discarded() && nls.contains(key) && nls[key].is_string())
                return nls[key].get<std::string>();
            return key;
        }

        void ScanExtensions(const fs::path &root, const std::string &group, std::vector<ThemeEntry> &out)
        {
            std::error_code ec;
            if (!fs::is_directory(root, ec))
                return;
            for (const auto &dir : fs::directory_iterator(root, ec))
            {
                fs::path package = dir.path() / "package.json";
                if (!fs::exists(package, ec))
                    continue;
                std::string text = ReadFile(package);
                if (text.find("\"themes\"") == std::string::npos)
                    continue;
                json data = json::parse(StripJsonc(text), nullptr, false);
                if (data.is_discarded() || !data.contains("contributes") || !data["contributes"].is_object())
                    continue;
                const auto &contributes = data["contributes"];
                if (!contributes.contains("themes") || !contributes["themes"].is_array())
                    continue;
                for (const auto &theme : contributes["themes"])
                {
                    if (!theme.is_object() || !theme.contains("path") || !theme["path"].is_string())
                        continue;
                    fs::path file = (dir.path() / theme["path"].get<std::string>()).lexically_normal();
                    if (file.extension() != ".json" || !fs::exists(file, ec))
                        continue;
                    std::string label = theme.contains("label") && theme["label"].is_string() ? ResolveLabel(theme["label"].get<std::string>(), dir.path()) : file.stem().string();
                    out.push_back({label, file.string(), group});
                }
            }
        }
    }

    bool LoadVSCodeTheme(const std::string &path, const std::string &name, CodeTheme &out)
    {
        RawTheme raw;
        if (!LoadRaw(path, raw, 0))
            return false;
        out = Resolve(raw, name);
        return true;
    }

    std::string ThemeLibrary::BundledFolder()
    {
        if (auto resolved = SIMPEngine::VFS::Resolve("assets://editor/themes"))
            return *resolved;
        return {};
    }

    CodeTheme ThemeLibrary::FallbackTheme()
    {
        RawTheme raw;
        raw.Type = "dark";
        raw.Colors["editor.background"] = IM_COL32(24, 26, 27, 255);
        raw.Colors["editor.foreground"] = IM_COL32(214, 218, 214, 255);
        return Resolve(raw, "Fallback");
    }

    void ThemeLibrary::Refresh()
    {
        m_Entries.clear();
        std::error_code ec;

        std::string bundled = BundledFolder();
        if (!bundled.empty() && fs::is_directory(bundled, ec))
        {
            std::vector<ThemeEntry> local;
            for (const auto &file : fs::directory_iterator(bundled, ec))
            {
                if (file.path().extension() != ".json")
                    continue;
                std::string name = file.path().stem().string();
                json data = json::parse(StripJsonc(ReadFile(file.path())), nullptr, false);
                if (!data.is_discarded() && data.contains("name") && data["name"].is_string())
                    name = data["name"].get<std::string>();
                local.push_back({name, file.path().string(), "Project"});
            }
            std::sort(local.begin(), local.end(), [](const ThemeEntry &a, const ThemeEntry &b) { return a.Name < b.Name; });
            m_Entries.insert(m_Entries.end(), local.begin(), local.end());
        }

        std::vector<ThemeEntry> vscode;
        const char *home = std::getenv("HOME");
        std::vector<std::pair<fs::path, std::string>> roots = {
            {"/usr/share/code/resources/app/extensions", "VS Code"},
            {"/usr/lib/code/extensions", "VS Code"},
            {"/opt/visual-studio-code/resources/app/extensions", "VS Code"},
            {"/snap/code/current/usr/share/code/resources/app/extensions", "VS Code"}};
        if (home)
        {
            roots.push_back({fs::path(home) / ".vscode/extensions", "VS Code extensions"});
            roots.push_back({fs::path(home) / ".vscode-oss/extensions", "VS Code extensions"});
        }
        for (const auto &[root, group] : roots)
            ScanExtensions(root, group, vscode);
        std::sort(vscode.begin(), vscode.end(), [](const ThemeEntry &a, const ThemeEntry &b) { return a.Group == b.Group ? a.Name < b.Name : a.Group < b.Group; });
        vscode.erase(std::unique(vscode.begin(), vscode.end(), [](const ThemeEntry &a, const ThemeEntry &b) { return a.Name == b.Name && a.Group == b.Group; }), vscode.end());
        m_Entries.insert(m_Entries.end(), vscode.begin(), vscode.end());
    }

    bool ThemeLibrary::Select(const std::string &name)
    {
        for (const ThemeEntry &entry : m_Entries)
        {
            if (entry.Name != name)
                continue;
            CodeTheme theme;
            if (LoadVSCodeTheme(entry.Path, entry.Name, theme))
            {
                m_Current = theme;
                return true;
            }
            CORE_WARN("Script editor: could not read theme '{}'", entry.Path);
            return false;
        }
        return false;
    }
}

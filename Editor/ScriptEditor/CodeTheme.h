#pragma once

#include "LuaLexer.h"

#include <imgui.h>

#include <array>
#include <string>
#include <vector>

namespace ScriptEditor
{
    struct CodeTheme
    {
        std::string Name = "Fallback";
        bool Dark = true;

        ImU32 Background = 0;
        ImU32 Foreground = 0;
        ImU32 Gutter = 0;
        ImU32 LineNumber = 0;
        ImU32 LineNumberActive = 0;
        ImU32 CurrentLine = 0;
        ImU32 Selection = 0;
        ImU32 Cursor = 0;
        ImU32 IndentGuide = 0;
        ImU32 BracketMatch = 0;
        ImU32 FindMatch = 0;
        ImU32 Error = 0;
        ImU32 PopupBackground = 0;
        ImU32 PopupForeground = 0;
        ImU32 PopupSelected = 0;
        ImU32 PopupBorder = 0;
        ImU32 PopupHighlight = 0;

        std::array<ImU32, static_cast<size_t>(TokenKind::Count)> Tokens{};
    };

    struct ThemeEntry
    {
        std::string Name;
        std::string Path;
        std::string Group;
    };

    class ThemeLibrary
    {
    public:
        void Refresh();
        const std::vector<ThemeEntry> &Entries() const { return m_Entries; }
        const CodeTheme &Current() const { return m_Current; }
        const std::string &CurrentName() const { return m_Current.Name; }
        bool Select(const std::string &name);
        static std::string BundledFolder();

    private:
        std::vector<ThemeEntry> m_Entries;
        CodeTheme m_Current = FallbackTheme();

        static CodeTheme FallbackTheme();
    };

    bool LoadVSCodeTheme(const std::string &path, const std::string &name, CodeTheme &out);
}

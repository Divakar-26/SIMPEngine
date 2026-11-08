#pragma once

#include "CodeTheme.h"
#include "LuaCompletion.h"
#include "LuaLexer.h"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <vector>

namespace ScriptEditor
{
    struct Coord
    {
        int Line = 0;
        int Column = 0;

        bool operator==(const Coord &o) const { return Line == o.Line && Column == o.Column; }
        bool operator!=(const Coord &o) const { return !(*this == o); }
        bool operator<(const Coord &o) const { return Line != o.Line ? Line < o.Line : Column < o.Column; }
    };

    class CodeEditor
    {
    public:
        CodeEditor();

        void SetText(const std::string &text);
        std::string GetText() const;

        void Render(const char *id, const CodeTheme &theme, ImFont *font, float fontSize, LuaCompletion &completion);

        uint64_t Version() const { return m_Version; }
        Coord GetCursor() const { return m_Cursor; }
        int LineCount() const { return static_cast<int>(m_Lines.size()); }
        void SetError(int line, const std::string &message);
        void GoToLine(int line);
        void Focus() { m_FocusRequest = true; }
        bool IsFocused() const { return m_Focused; }
        void InvalidateHighlight() { m_DirtyFrom = 0; }

        bool ConsumeSaveRequest();
        float ConsumeZoom();

    private:
        enum class EditKind
        {
            None,
            Typing,
            Deleting,
            Other
        };

        struct Snapshot
        {
            std::vector<std::string> Lines;
            Coord Cursor;
            Coord Anchor;
        };

        bool HasSelection() const { return m_Cursor != m_Anchor; }
        Coord SelectionStart() const { return m_Cursor < m_Anchor ? m_Cursor : m_Anchor; }
        Coord SelectionEnd() const { return m_Cursor < m_Anchor ? m_Anchor : m_Cursor; }
        std::string GetRange(Coord a, Coord b) const;
        void DeleteRange(Coord a, Coord b);
        Coord InsertAt(Coord at, const std::string &text);
        void DeleteSelection();
        void InsertText(const std::string &text, EditKind kind = EditKind::Other);

        void BeginEdit(EditKind kind);
        void EndEdit(int fromLine);
        void Undo();
        void Redo();

        int NextColumn(int line, int column) const;
        int PrevColumn(int line, int column) const;
        int WordLeft(int line, int column) const;
        int WordRight(int line, int column) const;
        int VisualColumn(int line, int column) const;
        int ColumnFromVisual(int line, int visual) const;
        Coord Clamp(Coord c) const;
        std::string Indentation(int line) const;
        std::string WordAt(Coord c, int &start, int &end) const;

        void MoveCursor(Coord to, bool select, bool keepPreferred = false);
        void HandleKeyboard(LuaCompletion &completion);
        void HandleMouse(const ImVec2 &origin, float gutterRight);
        void TypeCharacter(unsigned int c);
        void NewLine();
        void Backspace(bool word);
        void DeleteForward(bool word);
        void IndentLines(bool outdent);
        void ToggleComment();
        void DuplicateLines();
        void MoveLines(int direction);
        void Copy(bool cut);
        void Paste();
        void SelectAll();

        void UpdateCompletion(LuaCompletion &completion, bool force);
        void AcceptCompletion();
        void DrawCompletion(const CodeTheme &theme, ImFont *font, float fontSize);

        void DrawFindBar();
        void FindNext(bool backwards);
        void EnsureTokens(int lastLine, const LuaCompletion &completion);
        void ScrollToCursor();
        bool FindBracket(Coord &a, Coord &b) const;

        std::vector<std::string> m_Lines;
        Coord m_Cursor;
        Coord m_Anchor;
        int m_PreferredVisual = -1;
        uint64_t m_Version = 0;

        std::vector<std::vector<Token>> m_Tokens;
        std::vector<LexState> m_States;
        int m_DirtyFrom = 0;

        std::vector<Snapshot> m_UndoStack;
        std::vector<Snapshot> m_RedoStack;
        EditKind m_LastEdit = EditKind::None;
        double m_LastEditTime = 0.0;

        int m_ErrorLine = -1;
        std::string m_ErrorMessage;

        bool m_FocusRequest = false;
        bool m_Focused = false;
        bool m_ScrollToCursor = false;
        bool m_Selecting = false;
        bool m_SaveRequested = false;
        float m_Zoom = 0.0f;
        double m_LastCursorMove = 0.0;

        float m_CharWidth = 8.0f;
        float m_LineHeight = 18.0f;
        ImVec2 m_ViewSize{0, 0};
        ImVec2 m_CursorScreen{0, 0};

        bool m_FindOpen = false;
        bool m_FindFocus = false;
        char m_FindBuffer[128] = "";

        bool m_CompletionOpen = false;
        CompletionResult m_Completion;
        int m_CompletionIndex = 0;
        int m_CompletionLine = -1;
    };
}

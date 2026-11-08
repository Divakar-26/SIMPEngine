#define IMGUI_DEFINE_MATH_OPERATORS
#include "CodeEditor.h"

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>

namespace ScriptEditor
{
    namespace
    {
        constexpr int TabSize = 4;

        bool IsWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
        bool IsContinuation(char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }

        std::string Normalize(const std::string &text)
        {
            std::string out;
            out.reserve(text.size());
            for (char c : text)
            {
                if (c == '\r')
                    continue;
                if (c == '\t')
                    out.append(TabSize, ' ');
                else
                    out += c;
            }
            return out;
        }

        int LeadingSpaces(const std::string &line)
        {
            int n = 0;
            while (n < static_cast<int>(line.size()) && line[static_cast<size_t>(n)] == ' ')
                ++n;
            return n;
        }

        std::string TrimRight(const std::string &s)
        {
            size_t end = s.find_last_not_of(" \t");
            return end == std::string::npos ? std::string() : s.substr(0, end + 1);
        }

        bool EndsWithWord(const std::string &text, const char *word)
        {
            size_t n = std::strlen(word);
            if (text.size() < n || text.compare(text.size() - n, n, word) != 0)
                return false;
            return text.size() == n || !IsWordChar(text[text.size() - n - 1]);
        }

        char MatchingClose(char c)
        {
            switch (c)
            {
            case '(':
                return ')';
            case '[':
                return ']';
            case '{':
                return '}';
            case '"':
                return '"';
            case '\'':
                return '\'';
            default:
                return 0;
            }
        }

        const char *KindBadge(CompletionKind kind)
        {
            switch (kind)
            {
            case CompletionKind::Keyword:
                return "k";
            case CompletionKind::Function:
                return "f";
            case CompletionKind::Method:
                return "m";
            case CompletionKind::Field:
                return "p";
            case CompletionKind::Module:
                return "T";
            case CompletionKind::Constant:
                return "c";
            case CompletionKind::Hook:
                return "h";
            default:
                return "v";
            }
        }

        ImU32 KindColor(CompletionKind kind, const CodeTheme &theme)
        {
            switch (kind)
            {
            case CompletionKind::Keyword:
                return theme.Tokens[static_cast<size_t>(TokenKind::Keyword)];
            case CompletionKind::Function:
            case CompletionKind::Method:
            case CompletionKind::Hook:
                return theme.Tokens[static_cast<size_t>(TokenKind::Function)];
            case CompletionKind::Module:
                return theme.Tokens[static_cast<size_t>(TokenKind::Global)];
            case CompletionKind::Constant:
                return theme.Tokens[static_cast<size_t>(TokenKind::Number)];
            case CompletionKind::Field:
                return theme.Tokens[static_cast<size_t>(TokenKind::Member)];
            default:
                return theme.Foreground;
            }
        }
    }

    CodeEditor::CodeEditor()
    {
        SetText("");
    }

    void CodeEditor::SetText(const std::string &text)
    {
        m_Lines.clear();
        std::string normalized = Normalize(text);
        size_t start = 0;
        while (true)
        {
            size_t nl = normalized.find('\n', start);
            if (nl == std::string::npos)
            {
                m_Lines.push_back(normalized.substr(start));
                break;
            }
            m_Lines.push_back(normalized.substr(start, nl - start));
            start = nl + 1;
        }
        m_Cursor = m_Anchor = {};
        m_UndoStack.clear();
        m_RedoStack.clear();
        m_LastEdit = EditKind::None;
        m_DirtyFrom = 0;
        m_CompletionOpen = false;
        ++m_Version;
    }

    std::string CodeEditor::GetText() const
    {
        std::string out;
        for (size_t i = 0; i < m_Lines.size(); ++i)
        {
            out += m_Lines[i];
            if (i + 1 < m_Lines.size())
                out += '\n';
        }
        return out;
    }

    void CodeEditor::SetError(int line, const std::string &message)
    {
        m_ErrorLine = line;
        m_ErrorMessage = message;
    }

    void CodeEditor::GoToLine(int line)
    {
        line = std::clamp(line, 0, LineCount() - 1);
        m_Cursor = m_Anchor = {line, LeadingSpaces(m_Lines[static_cast<size_t>(line)])};
        m_ScrollToCursor = true;
        m_FocusRequest = true;
    }

    bool CodeEditor::ConsumeSaveRequest()
    {
        bool r = m_SaveRequested;
        m_SaveRequested = false;
        return r;
    }

    float CodeEditor::ConsumeZoom()
    {
        float z = m_Zoom;
        m_Zoom = 0.0f;
        return z;
    }

    Coord CodeEditor::Clamp(Coord c) const
    {
        c.Line = std::clamp(c.Line, 0, LineCount() - 1);
        c.Column = std::clamp(c.Column, 0, static_cast<int>(m_Lines[static_cast<size_t>(c.Line)].size()));
        return c;
    }

    int CodeEditor::NextColumn(int line, int column) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(line)];
        if (column >= static_cast<int>(l.size()))
            return column;
        ++column;
        while (column < static_cast<int>(l.size()) && IsContinuation(l[static_cast<size_t>(column)]))
            ++column;
        return column;
    }

    int CodeEditor::PrevColumn(int line, int column) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(line)];
        if (column <= 0)
            return 0;
        --column;
        while (column > 0 && IsContinuation(l[static_cast<size_t>(column)]))
            --column;
        return column;
    }

    int CodeEditor::WordLeft(int line, int column) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(line)];
        while (column > 0 && l[static_cast<size_t>(column - 1)] == ' ')
            --column;
        if (column > 0 && IsWordChar(l[static_cast<size_t>(column - 1)]))
            while (column > 0 && IsWordChar(l[static_cast<size_t>(column - 1)]))
                --column;
        else if (column > 0)
            --column;
        return column;
    }

    int CodeEditor::WordRight(int line, int column) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(line)];
        int n = static_cast<int>(l.size());
        if (column < n && IsWordChar(l[static_cast<size_t>(column)]))
            while (column < n && IsWordChar(l[static_cast<size_t>(column)]))
                ++column;
        else if (column < n)
            ++column;
        while (column < n && l[static_cast<size_t>(column)] == ' ')
            ++column;
        return column;
    }

    int CodeEditor::VisualColumn(int line, int column) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(line)];
        int visual = 0;
        for (int i = 0; i < column && i < static_cast<int>(l.size()); ++i)
            if (!IsContinuation(l[static_cast<size_t>(i)]))
                ++visual;
        return visual;
    }

    int CodeEditor::ColumnFromVisual(int line, int visual) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(line)];
        int column = 0;
        int count = 0;
        while (column < static_cast<int>(l.size()) && count < visual)
        {
            column = NextColumn(line, column);
            ++count;
        }
        return column;
    }

    std::string CodeEditor::Indentation(int line) const
    {
        return std::string(static_cast<size_t>(LeadingSpaces(m_Lines[static_cast<size_t>(line)])), ' ');
    }

    std::string CodeEditor::WordAt(Coord c, int &start, int &end) const
    {
        const std::string &l = m_Lines[static_cast<size_t>(c.Line)];
        start = end = c.Column;
        while (start > 0 && IsWordChar(l[static_cast<size_t>(start - 1)]))
            --start;
        while (end < static_cast<int>(l.size()) && IsWordChar(l[static_cast<size_t>(end)]))
            ++end;
        return l.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
    }

    std::string CodeEditor::GetRange(Coord a, Coord b) const
    {
        if (a.Line == b.Line)
            return m_Lines[static_cast<size_t>(a.Line)].substr(static_cast<size_t>(a.Column), static_cast<size_t>(b.Column - a.Column));
        std::string out = m_Lines[static_cast<size_t>(a.Line)].substr(static_cast<size_t>(a.Column));
        for (int i = a.Line + 1; i < b.Line; ++i)
            out += "\n" + m_Lines[static_cast<size_t>(i)];
        out += "\n" + m_Lines[static_cast<size_t>(b.Line)].substr(0, static_cast<size_t>(b.Column));
        return out;
    }

    void CodeEditor::DeleteRange(Coord a, Coord b)
    {
        if (b < a)
            std::swap(a, b);
        std::string &first = m_Lines[static_cast<size_t>(a.Line)];
        std::string tail = m_Lines[static_cast<size_t>(b.Line)].substr(static_cast<size_t>(b.Column));
        first = first.substr(0, static_cast<size_t>(a.Column)) + tail;
        if (b.Line > a.Line)
            m_Lines.erase(m_Lines.begin() + a.Line + 1, m_Lines.begin() + b.Line + 1);
    }

    Coord CodeEditor::InsertAt(Coord at, const std::string &text)
    {
        std::string &line = m_Lines[static_cast<size_t>(at.Line)];
        std::string tail = line.substr(static_cast<size_t>(at.Column));
        line.erase(static_cast<size_t>(at.Column));
        size_t start = 0;
        Coord end = at;
        while (true)
        {
            size_t nl = text.find('\n', start);
            std::string part = text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
            m_Lines[static_cast<size_t>(end.Line)] += part;
            end.Column = static_cast<int>(m_Lines[static_cast<size_t>(end.Line)].size());
            if (nl == std::string::npos)
                break;
            m_Lines.insert(m_Lines.begin() + end.Line + 1, std::string());
            ++end.Line;
            start = nl + 1;
        }
        m_Lines[static_cast<size_t>(end.Line)] += tail;
        return end;
    }

    void CodeEditor::DeleteSelection()
    {
        if (!HasSelection())
            return;
        Coord a = SelectionStart();
        DeleteRange(a, SelectionEnd());
        m_Cursor = m_Anchor = a;
    }

    void CodeEditor::BeginEdit(EditKind kind)
    {
        double now = ImGui::GetTime();
        if (kind != m_LastEdit || kind == EditKind::Other || now - m_LastEditTime > 1.0 || m_UndoStack.empty())
        {
            m_UndoStack.push_back({m_Lines, m_Cursor, m_Anchor});
            if (m_UndoStack.size() > 300)
                m_UndoStack.erase(m_UndoStack.begin());
        }
        m_RedoStack.clear();
        m_LastEdit = kind;
        m_LastEditTime = now;
    }

    void CodeEditor::EndEdit(int fromLine)
    {
        m_DirtyFrom = std::min(m_DirtyFrom, std::max(0, fromLine));
        m_Anchor = HasSelection() ? m_Anchor : m_Cursor;
        m_PreferredVisual = -1;
        m_ScrollToCursor = true;
        ++m_Version;
    }

    void CodeEditor::Undo()
    {
        if (m_UndoStack.empty())
            return;
        m_RedoStack.push_back({m_Lines, m_Cursor, m_Anchor});
        Snapshot s = std::move(m_UndoStack.back());
        m_UndoStack.pop_back();
        m_Lines = std::move(s.Lines);
        m_Cursor = s.Cursor;
        m_Anchor = s.Anchor;
        m_LastEdit = EditKind::None;
        m_DirtyFrom = 0;
        m_ScrollToCursor = true;
        m_CompletionOpen = false;
        ++m_Version;
    }

    void CodeEditor::Redo()
    {
        if (m_RedoStack.empty())
            return;
        m_UndoStack.push_back({m_Lines, m_Cursor, m_Anchor});
        Snapshot s = std::move(m_RedoStack.back());
        m_RedoStack.pop_back();
        m_Lines = std::move(s.Lines);
        m_Cursor = s.Cursor;
        m_Anchor = s.Anchor;
        m_LastEdit = EditKind::None;
        m_DirtyFrom = 0;
        m_ScrollToCursor = true;
        m_CompletionOpen = false;
        ++m_Version;
    }

    void CodeEditor::InsertText(const std::string &text, EditKind kind)
    {
        BeginEdit(kind);
        int from = SelectionStart().Line;
        DeleteSelection();
        m_Cursor = m_Anchor = InsertAt(m_Cursor, Normalize(text));
        EndEdit(from);
    }

    void CodeEditor::MoveCursor(Coord to, bool select, bool keepPreferred)
    {
        m_Cursor = Clamp(to);
        if (!select)
            m_Anchor = m_Cursor;
        if (!keepPreferred)
            m_PreferredVisual = -1;
        m_ScrollToCursor = true;
        m_LastEdit = EditKind::None;
        m_LastCursorMove = ImGui::GetTime();
    }

    void CodeEditor::TypeCharacter(unsigned int c)
    {
        char utf8[5] = {};
        ImTextCharToUtf8(utf8, c);
        std::string text = utf8;
        std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        char next = m_Cursor.Column < static_cast<int>(line.size()) ? line[static_cast<size_t>(m_Cursor.Column)] : 0;
        char prev = m_Cursor.Column > 0 ? line[static_cast<size_t>(m_Cursor.Column - 1)] : 0;

        if (!HasSelection() && text.size() == 1 && (text[0] == ')' || text[0] == ']' || text[0] == '}' || text[0] == '"' || text[0] == '\'') && next == text[0])
        {
            MoveCursor({m_Cursor.Line, m_Cursor.Column + 1}, false);
            return;
        }

        char close = text.size() == 1 ? MatchingClose(text[0]) : 0;
        bool quote = text[0] == '"' || text[0] == '\'';
        bool pair = close && (next == 0 || next == ' ' || next == ')' || next == ']' || next == '}' || next == ',') && !(quote && IsWordChar(prev));

        if (HasSelection() && close)
        {
            BeginEdit(EditKind::Other);
            Coord a = SelectionStart(), b = SelectionEnd();
            std::string inner = GetRange(a, b);
            DeleteRange(a, b);
            Coord end = InsertAt(a, text + inner + std::string(1, close));
            m_Anchor = {a.Line, a.Column + 1};
            m_Cursor = {end.Line, end.Column - 1};
            EndEdit(a.Line);
            return;
        }

        BeginEdit(EditKind::Typing);
        DeleteSelection();
        Coord end = InsertAt(m_Cursor, pair ? text + std::string(1, close) : text);
        m_Cursor = m_Anchor = pair ? Coord{end.Line, end.Column - 1} : end;

        std::string &current = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        std::string trimmed = TrimRight(current.substr(static_cast<size_t>(LeadingSpaces(current))));
        bool dedentWord = trimmed == "end" || trimmed == "else" || trimmed == "elseif" || trimmed == "until" || trimmed == "}";
        if (dedentWord && m_Cursor.Column == static_cast<int>(current.size()) && m_Cursor.Line > 0)
        {
            int prevLine = m_Cursor.Line - 1;
            while (prevLine > 0 && TrimRight(m_Lines[static_cast<size_t>(prevLine)]).empty())
                --prevLine;
            int indent = LeadingSpaces(current);
            int prevIndent = LeadingSpaces(m_Lines[static_cast<size_t>(prevLine)]);
            std::string prevTrim = TrimRight(m_Lines[static_cast<size_t>(prevLine)]);
            bool opener = EndsWithWord(prevTrim, "then") || EndsWithWord(prevTrim, "do") || EndsWithWord(prevTrim, "else") ||
                          EndsWithWord(prevTrim, "repeat") || (!prevTrim.empty() && prevTrim.back() == '{');
            int target = opener ? prevIndent : prevIndent - TabSize;
            if (indent > 0 && target >= 0 && indent == prevIndent && target < indent)
            {
                current.erase(0, static_cast<size_t>(indent - target));
                m_Cursor.Column -= indent - target;
                m_Anchor = m_Cursor;
            }
        }
        EndEdit(m_Cursor.Line);
    }

    void CodeEditor::NewLine()
    {
        BeginEdit(EditKind::Other);
        DeleteSelection();
        const std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        std::string before = TrimRight(line.substr(0, static_cast<size_t>(m_Cursor.Column)));
        std::string indent = Indentation(m_Cursor.Line);
        if (static_cast<int>(indent.size()) > m_Cursor.Column)
            indent.resize(static_cast<size_t>(m_Cursor.Column));
        char next = m_Cursor.Column < static_cast<int>(line.size()) ? line[static_cast<size_t>(m_Cursor.Column)] : 0;
        char last = before.empty() ? 0 : before.back();

        bool open = EndsWithWord(before, "then") || EndsWithWord(before, "do") || EndsWithWord(before, "else") || EndsWithWord(before, "repeat") ||
                    last == '{' || last == '(' || last == '[' ||
                    (before.find("function") != std::string::npos && last == ')' && before.find(" end") == std::string::npos);
        bool between = (last == '{' && next == '}') || (last == '(' && next == ')') || (last == '[' && next == ']');

        std::string inner = open ? indent + std::string(TabSize, ' ') : indent;
        int from = m_Cursor.Line;
        Coord at = InsertAt(m_Cursor, "\n" + inner);
        if (between)
            InsertAt(at, "\n" + indent);
        m_Cursor = m_Anchor = at;
        EndEdit(from);
    }

    void CodeEditor::Backspace(bool word)
    {
        if (HasSelection())
        {
            BeginEdit(EditKind::Deleting);
            int from = SelectionStart().Line;
            DeleteSelection();
            EndEdit(from);
            return;
        }
        if (m_Cursor.Column == 0 && m_Cursor.Line == 0)
            return;

        BeginEdit(EditKind::Deleting);
        Coord start = m_Cursor;
        const std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        if (m_Cursor.Column == 0)
        {
            start = {m_Cursor.Line - 1, static_cast<int>(m_Lines[static_cast<size_t>(m_Cursor.Line - 1)].size())};
        }
        else if (word)
        {
            start.Column = WordLeft(m_Cursor.Line, m_Cursor.Column);
        }
        else if (LeadingSpaces(line) >= m_Cursor.Column)
        {
            int remove = (m_Cursor.Column - 1) % TabSize + 1;
            start.Column = m_Cursor.Column - remove;
        }
        else
        {
            start.Column = PrevColumn(m_Cursor.Line, m_Cursor.Column);
            char removed = line[static_cast<size_t>(start.Column)];
            char next = m_Cursor.Column < static_cast<int>(line.size()) ? line[static_cast<size_t>(m_Cursor.Column)] : 0;
            if (MatchingClose(removed) && next == MatchingClose(removed))
            {
                DeleteRange(start, {m_Cursor.Line, m_Cursor.Column + 1});
                m_Cursor = m_Anchor = start;
                EndEdit(start.Line);
                return;
            }
        }
        DeleteRange(start, m_Cursor);
        m_Cursor = m_Anchor = start;
        EndEdit(start.Line);
    }

    void CodeEditor::DeleteForward(bool word)
    {
        if (HasSelection())
        {
            Backspace(false);
            return;
        }
        const std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        if (m_Cursor.Column >= static_cast<int>(line.size()) && m_Cursor.Line + 1 >= LineCount())
            return;
        BeginEdit(EditKind::Deleting);
        Coord end = m_Cursor;
        if (m_Cursor.Column >= static_cast<int>(line.size()))
            end = {m_Cursor.Line + 1, 0};
        else
            end.Column = word ? WordRight(m_Cursor.Line, m_Cursor.Column) : NextColumn(m_Cursor.Line, m_Cursor.Column);
        DeleteRange(m_Cursor, end);
        EndEdit(m_Cursor.Line);
    }

    void CodeEditor::IndentLines(bool outdent)
    {
        BeginEdit(EditKind::Other);
        Coord a = SelectionStart(), b = SelectionEnd();
        int last = (b.Column == 0 && b.Line > a.Line) ? b.Line - 1 : b.Line;
        for (int i = a.Line; i <= last; ++i)
        {
            std::string &l = m_Lines[static_cast<size_t>(i)];
            int delta;
            if (outdent)
            {
                delta = -std::min(LeadingSpaces(l), TabSize);
                l.erase(0, static_cast<size_t>(-delta));
            }
            else
            {
                if (TrimRight(l).empty())
                    continue;
                delta = TabSize;
                l.insert(0, static_cast<size_t>(TabSize), ' ');
            }
            if (m_Cursor.Line == i)
                m_Cursor.Column = std::max(0, m_Cursor.Column + delta);
            if (m_Anchor.Line == i)
                m_Anchor.Column = std::max(0, m_Anchor.Column + delta);
        }
        m_DirtyFrom = std::min(m_DirtyFrom, a.Line);
        m_ScrollToCursor = true;
        ++m_Version;
    }

    void CodeEditor::ToggleComment()
    {
        BeginEdit(EditKind::Other);
        Coord a = SelectionStart(), b = SelectionEnd();
        int last = (b.Column == 0 && b.Line > a.Line) ? b.Line - 1 : b.Line;
        bool allCommented = true;
        int minIndent = 1 << 30;
        for (int i = a.Line; i <= last; ++i)
        {
            const std::string &l = m_Lines[static_cast<size_t>(i)];
            if (TrimRight(l).empty())
                continue;
            int indent = LeadingSpaces(l);
            minIndent = std::min(minIndent, indent);
            if (l.compare(static_cast<size_t>(indent), 2, "--") != 0)
                allCommented = false;
        }
        if (minIndent == 1 << 30)
            return;
        for (int i = a.Line; i <= last; ++i)
        {
            std::string &l = m_Lines[static_cast<size_t>(i)];
            if (TrimRight(l).empty())
                continue;
            int delta;
            if (allCommented)
            {
                size_t at = static_cast<size_t>(LeadingSpaces(l));
                size_t count = l.compare(at, 3, "-- ") == 0 ? 3 : 2;
                l.erase(at, count);
                delta = -static_cast<int>(count);
            }
            else
            {
                l.insert(static_cast<size_t>(minIndent), "-- ");
                delta = 3;
            }
            if (m_Cursor.Line == i && m_Cursor.Column >= minIndent)
                m_Cursor.Column = std::max(minIndent, m_Cursor.Column + delta);
            if (m_Anchor.Line == i && m_Anchor.Column >= minIndent)
                m_Anchor.Column = std::max(minIndent, m_Anchor.Column + delta);
        }
        m_Cursor = Clamp(m_Cursor);
        m_Anchor = Clamp(m_Anchor);
        m_DirtyFrom = std::min(m_DirtyFrom, a.Line);
        ++m_Version;
    }

    void CodeEditor::DuplicateLines()
    {
        BeginEdit(EditKind::Other);
        Coord a = SelectionStart(), b = SelectionEnd();
        std::vector<std::string> copy(m_Lines.begin() + a.Line, m_Lines.begin() + b.Line + 1);
        m_Lines.insert(m_Lines.begin() + b.Line + 1, copy.begin(), copy.end());
        int count = static_cast<int>(copy.size());
        m_Cursor.Line += count;
        m_Anchor.Line += count;
        EndEdit(a.Line);
    }

    void CodeEditor::MoveLines(int direction)
    {
        Coord a = SelectionStart(), b = SelectionEnd();
        if ((direction < 0 && a.Line == 0) || (direction > 0 && b.Line + 1 >= LineCount()))
            return;
        BeginEdit(EditKind::Other);
        if (direction < 0)
        {
            std::string moved = m_Lines[static_cast<size_t>(a.Line - 1)];
            m_Lines.erase(m_Lines.begin() + a.Line - 1);
            m_Lines.insert(m_Lines.begin() + b.Line, moved);
        }
        else
        {
            std::string moved = m_Lines[static_cast<size_t>(b.Line + 1)];
            m_Lines.erase(m_Lines.begin() + b.Line + 1);
            m_Lines.insert(m_Lines.begin() + a.Line, moved);
        }
        m_Cursor.Line += direction;
        m_Anchor.Line += direction;
        m_DirtyFrom = std::min(m_DirtyFrom, std::max(0, a.Line - 1));
        m_ScrollToCursor = true;
        ++m_Version;
    }

    void CodeEditor::Copy(bool cut)
    {
        if (HasSelection())
        {
            ImGui::SetClipboardText(GetRange(SelectionStart(), SelectionEnd()).c_str());
            if (cut)
                Backspace(false);
            return;
        }
        std::string line = m_Lines[static_cast<size_t>(m_Cursor.Line)] + "\n";
        ImGui::SetClipboardText(line.c_str());
        if (cut)
        {
            BeginEdit(EditKind::Other);
            if (LineCount() > 1)
                m_Lines.erase(m_Lines.begin() + m_Cursor.Line);
            else
                m_Lines[0].clear();
            m_Cursor = m_Anchor = Clamp({m_Cursor.Line, 0});
            EndEdit(m_Cursor.Line);
        }
    }

    void CodeEditor::Paste()
    {
        const char *text = ImGui::GetClipboardText();
        if (text && *text)
            InsertText(text);
    }

    void CodeEditor::SelectAll()
    {
        m_Anchor = {0, 0};
        m_Cursor = {LineCount() - 1, static_cast<int>(m_Lines.back().size())};
    }

    void CodeEditor::FindNext(bool backwards)
    {
        std::string needle = m_FindBuffer;
        if (needle.empty())
            return;
        Coord from = backwards ? SelectionStart() : SelectionEnd();
        int count = LineCount();
        for (int step = 0; step <= count; ++step)
        {
            int line = backwards ? ((from.Line - step) % count + count) % count : (from.Line + step) % count;
            const std::string &l = m_Lines[static_cast<size_t>(line)];
            size_t pos;
            if (backwards)
            {
                size_t limit = step == 0 ? static_cast<size_t>(std::max(0, from.Column - 1)) : std::string::npos;
                pos = step == 0 && from.Column == 0 ? std::string::npos : l.rfind(needle, limit);
            }
            else
            {
                pos = l.find(needle, step == 0 ? static_cast<size_t>(from.Column) : 0);
            }
            if (pos != std::string::npos)
            {
                m_Anchor = {line, static_cast<int>(pos)};
                m_Cursor = {line, static_cast<int>(pos + needle.size())};
                m_ScrollToCursor = true;
                return;
            }
        }
    }

    bool CodeEditor::FindBracket(Coord &a, Coord &b) const
    {
        const std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        auto bracketAt = [&](int column) -> char
        {
            if (column < 0 || column >= static_cast<int>(line.size()))
                return 0;
            char c = line[static_cast<size_t>(column)];
            return std::strchr("()[]{}", c) ? c : 0;
        };
        int column = m_Cursor.Column;
        char c = bracketAt(column);
        if (!c)
        {
            column -= 1;
            c = bracketAt(column);
        }
        if (!c)
            return false;
        const char *opens = "([{";
        const char *closes = ")]}";
        bool forward = std::strchr(opens, c) != nullptr;
        char match = forward ? closes[std::strchr(opens, c) - opens] : opens[std::strchr(closes, c) - closes];
        int depth = 0;
        int l = m_Cursor.Line, col = column;
        for (int steps = 0; steps < 20000; ++steps)
        {
            const std::string &text = m_Lines[static_cast<size_t>(l)];
            if (col >= 0 && col < static_cast<int>(text.size()))
            {
                char x = text[static_cast<size_t>(col)];
                if (x == c)
                    ++depth;
                else if (x == match && --depth == 0)
                {
                    a = {m_Cursor.Line, column};
                    b = {l, col};
                    return true;
                }
            }
            if (forward)
            {
                if (++col >= static_cast<int>(text.size()))
                {
                    if (++l >= LineCount())
                        return false;
                    col = -1;
                }
            }
            else
            {
                if (--col < 0)
                {
                    if (--l < 0)
                        return false;
                    col = static_cast<int>(m_Lines[static_cast<size_t>(l)].size());
                }
            }
        }
        return false;
    }

    void CodeEditor::EnsureTokens(int lastLine, const LuaCompletion &completion)
    {
        int count = LineCount();
        if (static_cast<int>(m_Tokens.size()) != count)
        {
            m_Tokens.resize(static_cast<size_t>(count));
            m_DirtyFrom = std::min(m_DirtyFrom, count);
        }
        m_States.resize(static_cast<size_t>(count + 1));
        lastLine = std::min(lastLine, count - 1);
        for (int i = m_DirtyFrom; i <= lastLine; ++i)
            m_States[static_cast<size_t>(i + 1)] = TokenizeLine(m_Lines[static_cast<size_t>(i)], m_States[static_cast<size_t>(i)], m_Tokens[static_cast<size_t>(i)], &completion.GlobalNames());
        m_DirtyFrom = std::max(m_DirtyFrom, lastLine + 1);
    }

    void CodeEditor::ScrollToCursor()
    {
        float y = static_cast<float>(m_Cursor.Line) * m_LineHeight;
        float scrollY = ImGui::GetScrollY();
        if (y < scrollY + m_LineHeight)
            ImGui::SetScrollY(std::max(0.0f, y - m_LineHeight));
        else if (y + m_LineHeight * 2.0f > scrollY + m_ViewSize.y)
            ImGui::SetScrollY(y + m_LineHeight * 2.0f - m_ViewSize.y);

        float x = static_cast<float>(VisualColumn(m_Cursor.Line, m_Cursor.Column)) * m_CharWidth;
        float scrollX = ImGui::GetScrollX();
        float textWidth = m_ViewSize.x - 80.0f;
        if (x < scrollX)
            ImGui::SetScrollX(std::max(0.0f, x - m_CharWidth * 4.0f));
        else if (x > scrollX + textWidth)
            ImGui::SetScrollX(x - textWidth + m_CharWidth * 4.0f);
    }

    void CodeEditor::UpdateCompletion(LuaCompletion &completion, bool force)
    {
        CompletionResult result;
        try
        {
            result = completion.Query(m_Lines, m_Cursor.Line, m_Cursor.Column, force);
        }
        catch (const std::exception &)
        {
            m_CompletionOpen = false;
            return;
        }
        if (result.Items.empty() || (!force && !result.Member && result.Prefix.empty()))
        {
            m_CompletionOpen = false;
            return;
        }
        bool wasOpen = m_CompletionOpen;
        std::string previous = wasOpen && m_CompletionIndex < static_cast<int>(m_Completion.Items.size()) ? m_Completion.Items[static_cast<size_t>(m_CompletionIndex)].Label : std::string();
        m_Completion = std::move(result);
        m_CompletionOpen = true;
        m_CompletionLine = m_Cursor.Line;
        m_CompletionIndex = 0;
        if (!previous.empty())
            for (size_t i = 0; i < m_Completion.Items.size(); ++i)
                if (m_Completion.Items[i].Label == previous)
                    m_CompletionIndex = static_cast<int>(i);
    }

    void CodeEditor::AcceptCompletion()
    {
        if (!m_CompletionOpen || m_Completion.Items.empty())
            return;
        const CompletionItem &item = m_Completion.Items[static_cast<size_t>(std::clamp(m_CompletionIndex, 0, static_cast<int>(m_Completion.Items.size()) - 1))];
        BeginEdit(EditKind::Other);
        std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
        int end = m_Cursor.Column;
        while (end < static_cast<int>(line.size()) && IsWordChar(line[static_cast<size_t>(end)]))
            ++end;
        int start = std::min(m_Completion.ReplaceStart, m_Cursor.Column);
        line.replace(static_cast<size_t>(start), static_cast<size_t>(end - start), item.Label);
        int column = start + static_cast<int>(item.Label.size());
        bool hasParen = column < static_cast<int>(line.size()) && line[static_cast<size_t>(column)] == '(';
        if (item.Callable && !hasParen)
        {
            line.insert(static_cast<size_t>(column), "()");
            column += 1;
        }
        else if (item.Callable && hasParen)
        {
            column += 1;
        }
        m_Cursor = m_Anchor = {m_Cursor.Line, column};
        m_CompletionOpen = false;
        EndEdit(m_Cursor.Line);
    }

    void CodeEditor::HandleKeyboard(LuaCompletion &completion)
    {
        ImGuiIO &io = ImGui::GetIO();
        auto shortcut = [&](ImGuiKeyChord chord) { return ImGui::IsKeyChordPressed(chord, ImGuiInputFlags_Repeat, ImGuiKeyOwner_Any); };
        auto pressed = [&](ImGuiKey key) { return ImGui::IsKeyPressed(key, true); };
        bool shift = io.KeyShift;
        bool ctrl = io.KeyCtrl;
        bool alt = io.KeyAlt;
        uint64_t before = m_Version;
        Coord cursorBefore = m_Cursor;

        if (m_CompletionOpen)
        {
            int count = static_cast<int>(m_Completion.Items.size());
            if (pressed(ImGuiKey_DownArrow))
            {
                m_CompletionIndex = (m_CompletionIndex + 1) % count;
                return;
            }
            if (pressed(ImGuiKey_UpArrow))
            {
                m_CompletionIndex = (m_CompletionIndex + count - 1) % count;
                return;
            }
            if (pressed(ImGuiKey_PageDown))
            {
                m_CompletionIndex = std::min(count - 1, m_CompletionIndex + 8);
                return;
            }
            if (pressed(ImGuiKey_PageUp))
            {
                m_CompletionIndex = std::max(0, m_CompletionIndex - 8);
                return;
            }
            if (pressed(ImGuiKey_Enter) || pressed(ImGuiKey_KeypadEnter) || pressed(ImGuiKey_Tab))
            {
                AcceptCompletion();
                return;
            }
            if (pressed(ImGuiKey_Escape))
            {
                m_CompletionOpen = false;
                return;
            }
        }

        if (shortcut(ImGuiMod_Ctrl | ImGuiKey_Space))
        {
            UpdateCompletion(completion, true);
            return;
        }
        if (shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z) || shortcut(ImGuiMod_Ctrl | ImGuiKey_Y))
            Redo();
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_Z))
            Undo();
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_A))
            SelectAll();
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
            Copy(false);
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_X))
            Copy(true);
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_V))
            Paste();
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_S))
            m_SaveRequested = true;
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_D))
            DuplicateLines();
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_Slash))
            ToggleComment();
        else if (shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
        {
            m_FindOpen = true;
            m_FindFocus = true;
            if (HasSelection() && SelectionStart().Line == SelectionEnd().Line)
            {
                std::string selected = GetRange(SelectionStart(), SelectionEnd());
                std::snprintf(m_FindBuffer, sizeof(m_FindBuffer), "%s", selected.c_str());
            }
        }
        else if (shortcut(ImGuiMod_Shift | ImGuiKey_F3))
            FindNext(true);
        else if (shortcut(ImGuiKey_F3))
            FindNext(false);
        else if (shortcut(ImGuiMod_Alt | ImGuiKey_UpArrow))
            MoveLines(-1);
        else if (shortcut(ImGuiMod_Alt | ImGuiKey_DownArrow))
            MoveLines(1);
        else if (shortcut(ImGuiKey_Delete) || shortcut(ImGuiMod_Ctrl | ImGuiKey_Delete))
            DeleteForward(ctrl);
        else if (pressed(ImGuiKey_Escape))
        {
            if (m_FindOpen)
                m_FindOpen = false;
            else
                m_Anchor = m_Cursor;
        }
        else if (pressed(ImGuiKey_LeftArrow))
        {
            if (HasSelection() && !shift)
                MoveCursor(SelectionStart(), false);
            else if (m_Cursor.Column == 0 && m_Cursor.Line > 0)
                MoveCursor({m_Cursor.Line - 1, static_cast<int>(m_Lines[static_cast<size_t>(m_Cursor.Line - 1)].size())}, shift);
            else
                MoveCursor({m_Cursor.Line, ctrl ? WordLeft(m_Cursor.Line, m_Cursor.Column) : PrevColumn(m_Cursor.Line, m_Cursor.Column)}, shift);
        }
        else if (pressed(ImGuiKey_RightArrow))
        {
            if (HasSelection() && !shift)
                MoveCursor(SelectionEnd(), false);
            else if (m_Cursor.Column >= static_cast<int>(m_Lines[static_cast<size_t>(m_Cursor.Line)].size()) && m_Cursor.Line + 1 < LineCount())
                MoveCursor({m_Cursor.Line + 1, 0}, shift);
            else
                MoveCursor({m_Cursor.Line, ctrl ? WordRight(m_Cursor.Line, m_Cursor.Column) : NextColumn(m_Cursor.Line, m_Cursor.Column)}, shift);
        }
        else if (pressed(ImGuiKey_UpArrow) || pressed(ImGuiKey_DownArrow) || pressed(ImGuiKey_PageUp) || pressed(ImGuiKey_PageDown))
        {
            int page = std::max(1, static_cast<int>(m_ViewSize.y / m_LineHeight) - 2);
            int delta = pressed(ImGuiKey_UpArrow) ? -1 : pressed(ImGuiKey_DownArrow) ? 1 : pressed(ImGuiKey_PageUp) ? -page : page;
            if (m_PreferredVisual < 0)
                m_PreferredVisual = VisualColumn(m_Cursor.Line, m_Cursor.Column);
            int line = std::clamp(m_Cursor.Line + delta, 0, LineCount() - 1);
            if (ctrl && (delta == 1 || delta == -1))
            {
                ImGui::SetScrollY(ImGui::GetScrollY() + static_cast<float>(delta) * m_LineHeight);
            }
            else
            {
                MoveCursor({line, ColumnFromVisual(line, m_PreferredVisual)}, shift, true);
                if (line == m_Cursor.Line && delta < 0 && m_Cursor.Line == 0)
                    MoveCursor({0, 0}, shift);
            }
        }
        else if (pressed(ImGuiKey_Home))
        {
            if (ctrl)
                MoveCursor({0, 0}, shift);
            else
            {
                int indent = LeadingSpaces(m_Lines[static_cast<size_t>(m_Cursor.Line)]);
                MoveCursor({m_Cursor.Line, m_Cursor.Column == indent ? 0 : indent}, shift);
            }
        }
        else if (pressed(ImGuiKey_End))
        {
            if (ctrl)
                MoveCursor({LineCount() - 1, static_cast<int>(m_Lines.back().size())}, shift);
            else
                MoveCursor({m_Cursor.Line, static_cast<int>(m_Lines[static_cast<size_t>(m_Cursor.Line)].size())}, shift);
        }
        else if (pressed(ImGuiKey_Backspace))
        {
            Backspace(ctrl);
        }
        else if (pressed(ImGuiKey_Enter) || pressed(ImGuiKey_KeypadEnter))
        {
            NewLine();
        }
        else if (pressed(ImGuiKey_Tab))
        {
            if (shift || (HasSelection() && SelectionStart().Line != SelectionEnd().Line))
                IndentLines(shift);
            else
                InsertText(std::string(static_cast<size_t>(TabSize - VisualColumn(m_Cursor.Line, m_Cursor.Column) % TabSize), ' '), EditKind::Typing);
        }

        bool typed = false;
        if (!ctrl || alt)
        {
            for (ImWchar c : io.InputQueueCharacters)
            {
                if (c < 32 || c == 127)
                    continue;
                TypeCharacter(c);
                typed = true;
            }
        }
        io.InputQueueCharacters.resize(0);

        if (typed)
        {
            const std::string &line = m_Lines[static_cast<size_t>(m_Cursor.Line)];
            char last = m_Cursor.Column > 0 ? line[static_cast<size_t>(m_Cursor.Column - 1)] : 0;
            if (m_CompletionOpen || IsWordChar(last) || last == '.' || last == ':')
                UpdateCompletion(completion, false);
        }
        else if (m_Version != before && m_CompletionOpen)
        {
            UpdateCompletion(completion, false);
        }
        else if (m_Cursor != cursorBefore && m_CompletionOpen)
        {
            m_CompletionOpen = false;
        }
    }

    void CodeEditor::HandleMouse(const ImVec2 &origin, float gutterRight)
    {
        ImGuiIO &io = ImGui::GetIO();
        auto toCoord = [&](ImVec2 mouse)
        {
            int line = std::clamp(static_cast<int>(std::floor((mouse.y - origin.y) / m_LineHeight)), 0, LineCount() - 1);
            int visual = std::max(0, static_cast<int>(std::round((mouse.x - origin.x) / m_CharWidth)));
            return Coord{line, ColumnFromVisual(line, visual)};
        };

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            Coord c = toCoord(io.MousePos);
            m_CompletionOpen = false;
            if (io.MousePos.x < gutterRight)
            {
                m_Anchor = {c.Line, 0};
                m_Cursor = c.Line + 1 < LineCount() ? Coord{c.Line + 1, 0} : Coord{c.Line, static_cast<int>(m_Lines[static_cast<size_t>(c.Line)].size())};
            }
            else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                int start, end;
                WordAt(c, start, end);
                m_Anchor = {c.Line, start};
                m_Cursor = {c.Line, end};
            }
            else
            {
                MoveCursor(c, io.KeyShift);
                m_Selecting = true;
            }
            m_PreferredVisual = -1;
            m_LastCursorMove = ImGui::GetTime();
        }

        if (m_Selecting)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_Cursor = toCoord(io.MousePos);
                ImVec2 min = ImGui::GetWindowPos();
                ImVec2 max = min + ImGui::GetWindowSize();
                if (io.MousePos.y < min.y + m_LineHeight)
                    ImGui::SetScrollY(ImGui::GetScrollY() - m_LineHeight * 0.5f);
                else if (io.MousePos.y > max.y - m_LineHeight)
                    ImGui::SetScrollY(ImGui::GetScrollY() + m_LineHeight * 0.5f);
            }
            else
            {
                m_Selecting = false;
            }
        }

        if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0f)
            m_Zoom += io.MouseWheel;
    }

    void CodeEditor::DrawFindBar()
    {
        if (!m_FindOpen)
            return;
        ImGui::PushID("find");
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Find");
        ImGui::SameLine();
        if (m_FindFocus)
        {
            ImGui::SetKeyboardFocusHere();
            m_FindFocus = false;
        }
        ImGui::SetNextItemWidth(std::min(320.0f, ImGui::GetContentRegionAvail().x * 0.5f));
        if (ImGui::InputText("##needle", m_FindBuffer, sizeof(m_FindBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            FindNext(ImGui::GetIO().KeyShift);
            m_FindFocus = true;
        }
        if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_FindOpen = false;
            m_FocusRequest = true;
        }

        int count = 0;
        std::string needle = m_FindBuffer;
        if (!needle.empty())
            for (const std::string &l : m_Lines)
                for (size_t p = l.find(needle); p != std::string::npos; p = l.find(needle, p + needle.size()))
                    ++count;
        ImGui::SameLine();
        ImGui::TextDisabled(needle.empty() ? "" : count == 1 ? "1 match" : "%d matches", count);
        ImGui::SameLine();
        if (ImGui::SmallButton("Prev"))
            FindNext(true);
        ImGui::SameLine();
        if (ImGui::SmallButton("Next"))
            FindNext(false);
        ImGui::SameLine();
        if (ImGui::SmallButton("Close"))
        {
            m_FindOpen = false;
            m_FocusRequest = true;
        }
        ImGui::PopID();
    }

    void CodeEditor::DrawCompletion(const CodeTheme &theme, ImFont *font, float fontSize)
    {
        if (!m_CompletionOpen || m_Completion.Items.empty())
            return;
        if (m_CompletionLine != m_Cursor.Line)
        {
            m_CompletionOpen = false;
            return;
        }

        const int visible = std::min(10, static_cast<int>(m_Completion.Items.size()));
        float rowH = std::floor(fontSize * 1.55f);
        float labelW = 0.0f, detailW = 0.0f;
        for (const auto &item : m_Completion.Items)
        {
            labelW = std::max(labelW, font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, item.Label.c_str()).x);
            detailW = std::max(detailW, font->CalcTextSizeA(fontSize * 0.85f, FLT_MAX, 0.0f, item.Detail.c_str()).x);
        }
        float width = std::clamp(rowH + labelW + detailW + 40.0f, 220.0f, 560.0f);
        float height = rowH * static_cast<float>(visible) + 6.0f;

        ImGuiViewport *viewport = ImGui::GetWindowViewport();
        ImVec2 pos(m_CursorScreen.x - rowH - 4.0f, m_CursorScreen.y + m_LineHeight + 2.0f);
        if (pos.y + height > viewport->Pos.y + viewport->Size.y)
            pos.y = m_CursorScreen.y - height - 2.0f;
        pos.x = std::min(pos.x, viewport->Pos.x + viewport->Size.x - width - 4.0f);

        int first = std::clamp(m_CompletionIndex - visible + 1, 0, std::max(0, static_cast<int>(m_Completion.Items.size()) - visible));
        if (m_CompletionIndex < first)
            first = m_CompletionIndex;

        ImDrawList *dl = ImGui::GetForegroundDrawList(viewport);
        ImVec2 max(pos.x + width, pos.y + height);
        dl->AddRectFilled(pos + ImVec2(2, 3), max + ImVec2(2, 3), IM_COL32(0, 0, 0, 60), 6.0f);
        dl->AddRectFilled(pos, max, theme.PopupBackground, 6.0f);
        dl->AddRect(pos, max, theme.PopupBorder, 6.0f);

        ImGuiIO &io = ImGui::GetIO();
        bool hovered = io.MousePos.x >= pos.x && io.MousePos.x < max.x && io.MousePos.y >= pos.y && io.MousePos.y < max.y;
        if (hovered && io.MouseWheel != 0.0f)
            m_CompletionIndex = std::clamp(m_CompletionIndex - static_cast<int>(io.MouseWheel), 0, static_cast<int>(m_Completion.Items.size()) - 1);

        for (int i = 0; i < visible; ++i)
        {
            int index = first + i;
            if (index >= static_cast<int>(m_Completion.Items.size()))
                break;
            const auto &item = m_Completion.Items[static_cast<size_t>(index)];
            ImVec2 rowMin(pos.x + 3.0f, pos.y + 3.0f + rowH * static_cast<float>(i));
            ImVec2 rowMax(max.x - 3.0f, rowMin.y + rowH);
            bool rowHovered = hovered && io.MousePos.y >= rowMin.y && io.MousePos.y < rowMax.y;
            if (index == m_CompletionIndex)
                dl->AddRectFilled(rowMin, rowMax, theme.PopupSelected, 4.0f);
            else if (rowHovered)
                dl->AddRectFilled(rowMin, rowMax, (theme.PopupSelected & 0x00FFFFFF) | (90u << IM_COL32_A_SHIFT), 4.0f);
            if (rowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_CompletionIndex = index;
                AcceptCompletion();
                m_FocusRequest = true;
                return;
            }

            ImU32 badgeColor = KindColor(item.Kind, theme);
            float badge = rowH - 8.0f;
            ImVec2 bMin(rowMin.x + 5.0f, rowMin.y + 4.0f);
            dl->AddRectFilled(bMin, bMin + ImVec2(badge, badge), (badgeColor & 0x00FFFFFF) | (50u << IM_COL32_A_SHIFT), 3.0f);
            const char *letter = KindBadge(item.Kind);
            ImVec2 ls = font->CalcTextSizeA(fontSize * 0.8f, FLT_MAX, 0.0f, letter);
            dl->AddText(font, fontSize * 0.8f, bMin + ImVec2((badge - ls.x) * 0.5f, (badge - ls.y) * 0.5f), badgeColor, letter);

            float textY = rowMin.y + (rowH - fontSize) * 0.5f;
            float x = bMin.x + badge + 8.0f;
            size_t matched = 0;
            if (!m_Completion.Prefix.empty() && item.Label.size() >= m_Completion.Prefix.size())
            {
                bool same = true;
                for (size_t k = 0; k < m_Completion.Prefix.size(); ++k)
                    if (std::tolower(static_cast<unsigned char>(item.Label[k])) != std::tolower(static_cast<unsigned char>(m_Completion.Prefix[k])))
                        same = false;
                if (same)
                    matched = m_Completion.Prefix.size();
            }
            if (matched)
            {
                dl->AddText(font, fontSize, ImVec2(x, textY), theme.PopupHighlight, item.Label.c_str(), item.Label.c_str() + matched);
                x += font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, item.Label.c_str(), item.Label.c_str() + matched).x;
            }
            dl->AddText(font, fontSize, ImVec2(x, textY), theme.PopupForeground, item.Label.c_str() + matched);

            ImVec2 ds = font->CalcTextSizeA(fontSize * 0.85f, FLT_MAX, 0.0f, item.Detail.c_str());
            ImU32 faint = (theme.PopupForeground & 0x00FFFFFF) | (120u << IM_COL32_A_SHIFT);
            dl->AddText(font, fontSize * 0.85f, ImVec2(rowMax.x - ds.x - 8.0f, rowMin.y + (rowH - ds.y) * 0.5f), faint, item.Detail.c_str());
        }
    }

    void CodeEditor::Render(const char *id, const CodeTheme &theme, ImFont *font, float fontSize, LuaCompletion &completion)
    {
        ImGui::PushID(id);
        DrawFindBar();

        ImGui::PushFont(font, fontSize);
        m_CharWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "MMMMMMMMMM").x * 0.1f;
        m_LineHeight = std::floor(fontSize * 1.5f);
        int digits = std::max(3, static_cast<int>(std::log10(std::max(1, LineCount()))) + 1);
        float gutterWidth = std::floor(static_cast<float>(digits + 3) * m_CharWidth);
        const float leftPad = 12.0f;

        size_t longest = 0;
        for (const std::string &l : m_Lines)
            longest = std::max(longest, l.size());

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(theme.Background));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoMove;
        if (ImGui::GetIO().KeyCtrl)
            flags |= ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::BeginChild("##code", ImVec2(0, 0), ImGuiChildFlags_None, flags);
        ImGui::PopStyleVar();

        if (m_FocusRequest)
        {
            ImGui::SetWindowFocus();
            m_FocusRequest = false;
        }
        m_ViewSize = ImGui::GetWindowSize();
        bool focused = ImGui::IsWindowFocused();
        m_Focused = focused;
        if (focused)
        {
            ImGuiID owner = ImGui::GetID("##keys");
            for (ImGuiKey key : {ImGuiKey_Escape, ImGuiKey_Tab, ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Space, ImGuiKey_Backspace, ImGuiKey_Delete,
                                 ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_PageUp, ImGuiKey_PageDown,
                                 ImGuiKey_Home, ImGuiKey_End})
                ImGui::SetKeyOwner(key, owner);
            ImGui::SetNextFrameWantCaptureKeyboard(true);
            HandleKeyboard(completion);
        }

        float contentWidth = gutterWidth + leftPad + static_cast<float>(longest + 8) * m_CharWidth;
        float contentHeight = static_cast<float>(LineCount()) * m_LineHeight + m_ViewSize.y * 0.6f;
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::Dummy(ImVec2(contentWidth, contentHeight));

        if (m_ScrollToCursor)
        {
            ScrollToCursor();
            m_ScrollToCursor = false;
        }

        ImVec2 winPos = ImGui::GetWindowPos();
        float scrollX = ImGui::GetScrollX();
        float scrollY = ImGui::GetScrollY();
        ImVec2 origin(winPos.x + gutterWidth + leftPad - scrollX, winPos.y + 4.0f - scrollY);
        HandleMouse(origin, winPos.x + gutterWidth);

        int first = std::max(0, static_cast<int>(scrollY / m_LineHeight) - 1);
        int last = std::min(LineCount() - 1, static_cast<int>((scrollY + m_ViewSize.y) / m_LineHeight) + 1);
        EnsureTokens(last, completion);

        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 clipMin(winPos.x + gutterWidth, winPos.y);
        ImVec2 clipMax(winPos.x + m_ViewSize.x, winPos.y + m_ViewSize.y);
        dl->PushClipRect(clipMin, clipMax, true);

        auto lineY = [&](int line) { return origin.y + static_cast<float>(line) * m_LineHeight; };
        auto colX = [&](int line, int column) { return origin.x + static_cast<float>(VisualColumn(line, column)) * m_CharWidth; };

        if (!HasSelection())
            dl->AddRectFilled(ImVec2(clipMin.x, lineY(m_Cursor.Line)), ImVec2(clipMax.x, lineY(m_Cursor.Line) + m_LineHeight), theme.CurrentLine);

        if (m_FindOpen && m_FindBuffer[0])
        {
            std::string needle = m_FindBuffer;
            for (int i = first; i <= last; ++i)
            {
                const std::string &l = m_Lines[static_cast<size_t>(i)];
                for (size_t p = l.find(needle); p != std::string::npos; p = l.find(needle, p + needle.size()))
                    dl->AddRectFilled(ImVec2(colX(i, static_cast<int>(p)), lineY(i)), ImVec2(colX(i, static_cast<int>(p + needle.size())), lineY(i) + m_LineHeight), theme.FindMatch, 2.0f);
            }
        }

        if (HasSelection())
        {
            Coord a = SelectionStart(), b = SelectionEnd();
            for (int i = std::max(first, a.Line); i <= std::min(last, b.Line); ++i)
            {
                float x0 = i == a.Line ? colX(i, a.Column) : origin.x;
                float x1 = i == b.Line ? colX(i, b.Column) : colX(i, static_cast<int>(m_Lines[static_cast<size_t>(i)].size())) + m_CharWidth * 0.6f;
                dl->AddRectFilled(ImVec2(x0, lineY(i)), ImVec2(std::max(x1, x0 + 2.0f), lineY(i) + m_LineHeight), theme.Selection);
            }
        }

        Coord bracketA, bracketB;
        if (!HasSelection() && FindBracket(bracketA, bracketB))
        {
            for (Coord c : {bracketA, bracketB})
                if (c.Line >= first && c.Line <= last)
                    dl->AddRectFilled(ImVec2(colX(c.Line, c.Column), lineY(c.Line)), ImVec2(colX(c.Line, c.Column) + m_CharWidth, lineY(c.Line) + m_LineHeight), theme.BracketMatch, 2.0f);
        }

        for (int i = first; i <= last; ++i)
        {
            const std::string &l = m_Lines[static_cast<size_t>(i)];
            int indent = LeadingSpaces(l);
            if (indent == static_cast<int>(l.size()))
            {
                int j = i + 1;
                while (j < LineCount() && j < i + 50 && TrimRight(m_Lines[static_cast<size_t>(j)]).empty())
                    ++j;
                indent = j < LineCount() ? LeadingSpaces(m_Lines[static_cast<size_t>(j)]) : 0;
            }
            for (int level = TabSize; level < indent; level += TabSize)
            {
                float x = std::floor(origin.x + static_cast<float>(level) * m_CharWidth) + 0.5f;
                dl->AddLine(ImVec2(x, lineY(i)), ImVec2(x, lineY(i) + m_LineHeight), theme.IndentGuide);
            }
        }

        for (int i = first; i <= last; ++i)
        {
            const std::string &l = m_Lines[static_cast<size_t>(i)];
            float y = lineY(i) + (m_LineHeight - fontSize) * 0.5f;
            for (const Token &t : m_Tokens[static_cast<size_t>(i)])
            {
                ImU32 color = theme.Tokens[static_cast<size_t>(t.Kind)];
                dl->AddText(font, fontSize, ImVec2(colX(i, t.Start), y), color, l.c_str() + t.Start, l.c_str() + t.Start + t.Length);
            }
        }

        if (m_ErrorLine >= first && m_ErrorLine <= last && m_ErrorLine < LineCount())
        {
            const std::string &l = m_Lines[static_cast<size_t>(m_ErrorLine)];
            float x0 = colX(m_ErrorLine, LeadingSpaces(l));
            float x1 = std::max(colX(m_ErrorLine, static_cast<int>(l.size())), x0 + m_CharWidth * 2.0f);
            float y = lineY(m_ErrorLine) + m_LineHeight - 3.0f;
            ImVec2 points[256];
            int count = 0;
            for (float x = x0; x < x1 && count < 255; x += 3.0f, ++count)
                points[count] = ImVec2(x, y + ((count % 2) ? 1.5f : -1.5f));
            if (count > 1)
                dl->AddPolyline(points, count, theme.Error, ImDrawFlags_None, 1.2f);
        }

        m_CursorScreen = ImVec2(colX(m_Cursor.Line, m_Cursor.Column), lineY(m_Cursor.Line));
        if (focused)
        {
            ImGuiPlatformImeData &ime = ImGui::GetCurrentContext()->PlatformImeData;
            ime.WantVisible = true;
            ime.WantTextInput = true;
            ime.InputPos = m_CursorScreen;
            ime.InputLineHeight = m_LineHeight;
            ime.ViewportId = ImGui::GetWindowViewport()->ID;
        }
        double sinceMove = ImGui::GetTime() - m_LastCursorMove;
        bool blinkOn = sinceMove < 0.5 || std::fmod(sinceMove, 1.0) < 0.55;
        if (focused && blinkOn)
            dl->AddRectFilled(m_CursorScreen, m_CursorScreen + ImVec2(2.0f, m_LineHeight), theme.Cursor);
        dl->PopClipRect();

        ImVec2 gutterMax(winPos.x + gutterWidth, winPos.y + m_ViewSize.y);
        dl->PushClipRect(winPos, gutterMax, true);
        dl->AddRectFilled(winPos, gutterMax, theme.Gutter);
        for (int i = first; i <= last; ++i)
        {
            char number[16];
            std::snprintf(number, sizeof(number), "%d", i + 1);
            ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, number);
            float y = lineY(i) + (m_LineHeight - fontSize) * 0.5f;
            bool active = i == m_Cursor.Line;
            dl->AddText(font, fontSize, ImVec2(gutterMax.x - size.x - m_CharWidth * 1.2f, y), active ? theme.LineNumberActive : theme.LineNumber, number);
            if (i == m_ErrorLine)
                dl->AddCircleFilled(ImVec2(winPos.x + m_CharWidth * 0.8f, lineY(i) + m_LineHeight * 0.5f), m_CharWidth * 0.32f, theme.Error);
        }
        dl->PopClipRect();

        if (m_ErrorLine >= 0 && ImGui::IsWindowHovered())
        {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            if (mouse.y >= lineY(m_ErrorLine) && mouse.y < lineY(m_ErrorLine) + m_LineHeight)
                ImGui::SetTooltip("%s", m_ErrorMessage.c_str());
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        DrawCompletion(theme, font, fontSize);
        ImGui::PopFont();
        ImGui::PopID();
    }
}

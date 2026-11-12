#include "DocsPanel.h"

#include "UI/Theme.h"
#include "UI/Widgets.h"
#include "ScriptEditor/LuaContext.h"

#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Project/Project.h>

#include <SDL3/SDL_misc.h>

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
    std::string Lower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    std::string Basename(const std::string &url)
    {
        std::string path = url;
        const size_t hash = path.find('#');
        if (hash != std::string::npos)
            path = path.substr(0, hash);
        const size_t slash = path.find_last_of("/\\");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }
}

void DocsPanel::Open(const std::string &file, const std::string &anchor)
{
    m_Visible = true;
    m_Pending = file;
    m_PendingAnchor = anchor;
    m_HasPending = true;
}

void DocsPanel::Load()
{
    m_Loaded = true;
    m_Entries.clear();
    const fs::path engine = SIMPEngine::Project::EngineResourceDirectory();
    std::error_code ec;
    fs::path docs;
    for (const fs::path &candidate : {engine / "docs", engine.parent_path() / "docs"})
        if (fs::is_directory(candidate, ec))
        {
            docs = candidate;
            break;
        }
    m_Folder = docs.string();

    if (!docs.empty())
    {
        static const struct
        {
            const char *File;
            const char *Title;
        } known[] = {
            {"HOW_TO.md", "How To"},
            {"SCRIPTING.md", "Scripting with Lua"},
            {"ANIMATION.md", "Tweens and Vector Shapes"},
            {"EFFECTS.md", "Effects and Easing"},
            {"API_GUIDE.md", "C++ API Guide"},
            {"ARCHITECTURE.md", "Architecture"},
            {"SHIPPING.md", "Projects and Shipping"},
            {"ANDROID.md", "Android"},
            {"USING_THE_LIBRARY.md", "Using the Library"},
        };
        std::vector<std::string> seen;
        auto add = [&](const fs::path &path, const std::string &title)
        {
            const std::optional<std::string> text = SIMPEngine::FileSystem::ReadText(path.string());
            if (!text)
                return;
            Entry entry;
            entry.File = path.filename().string();
            entry.Path = path.string();
            entry.Document = Markdown::Parse(*text);
            entry.Title = !title.empty() ? title : (!entry.Document.Title.empty() ? entry.Document.Title : entry.File);
            std::string heading = entry.Title;
            for (const Markdown::Block &block : entry.Document.Blocks)
            {
                if (block.Type == Markdown::BlockType::Heading)
                    heading = Markdown::PlainText(block.Spans);
                std::string plain;
                if (block.Type == Markdown::BlockType::Code)
                    plain = block.Code;
                else if (block.Type == Markdown::BlockType::Table)
                {
                    for (const auto &row : block.Rows)
                    {
                        for (const auto &cell : row)
                            plain += Markdown::PlainText(cell) + "  ";
                        plain += "\n";
                    }
                }
                else
                    plain = Markdown::PlainText(block.Spans);
                if (!plain.empty())
                    entry.Lines.emplace_back(block.Type == Markdown::BlockType::Heading ? block.Anchor : std::string(), heading + "\x1f" + plain);
            }
            m_Entries.push_back(std::move(entry));
            seen.push_back(path.filename().string());
        };
        for (const auto &item : known)
            if (fs::is_regular_file(docs / item.File, ec))
                add(docs / item.File, item.Title);
        std::vector<fs::path> extra;
        for (const auto &file : fs::directory_iterator(docs, ec))
            if (file.is_regular_file(ec) && file.path().extension() == ".md" && std::find(seen.begin(), seen.end(), file.path().filename().string()) == seen.end())
                extra.push_back(file.path());
        std::sort(extra.begin(), extra.end());
        for (const fs::path &path : extra)
            add(path, std::string());
    }

    for (auto &[key, params] : ScriptEditor::ListSignatures())
    {
        std::string text = key;
        const bool method = key.find(':') != std::string::npos;
        text += "(" + params + ")";
        m_Api.emplace_back(key, text);
        (void)method;
    }
    std::sort(m_Api.begin(), m_Api.end());
}

void DocsPanel::Show(int doc, const std::string &anchor, bool record)
{
    if (doc < 0 || doc >= static_cast<int>(m_Entries.size()))
        return;
    const bool changed = doc != m_Current;
    if (changed)
    {
        m_Current = doc;
        m_View.SetDocument(m_Entries[doc].Document, fs::path(m_Entries[doc].Path).parent_path().string());
    }
    if (!anchor.empty())
        m_View.ScrollToAnchor(anchor);
    else if (!changed)
        m_View.ScrollToTop();
    if (record)
    {
        m_History.resize(m_Position + 1);
        m_History.push_back({doc, anchor});
        m_Position = static_cast<int>(m_History.size()) - 1;
    }
}

void DocsPanel::Navigate(const std::string &url)
{
    if (url.empty())
        return;
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 || url.rfind("mailto:", 0) == 0)
    {
        if (!SDL_OpenURL(url.c_str()))
            CORE_WARN("Could not open {}: {}", url, SDL_GetError());
        return;
    }
    if (url[0] == '#')
    {
        Show(m_Current, url.substr(1), true);
        return;
    }
    const std::string name = Basename(url);
    const size_t hash = url.find('#');
    const std::string anchor = hash == std::string::npos ? std::string() : url.substr(hash + 1);
    for (size_t i = 0; i < m_Entries.size(); ++i)
        if (m_Entries[i].File == name)
        {
            Show(static_cast<int>(i), anchor, true);
            return;
        }
}

void DocsPanel::Search()
{
    m_Hits.clear();
    const std::string query = Lower(m_Query);
    if (query.size() < 2)
        return;
    for (size_t d = 0; d < m_Entries.size(); ++d)
    {
        int perDoc = 0;
        for (const auto &[anchor, packed] : m_Entries[d].Lines)
        {
            const size_t split = packed.find('\x1f');
            const std::string heading = packed.substr(0, split);
            const std::string text = packed.substr(split + 1);
            const std::string lower = Lower(text);
            const size_t at = lower.find(query);
            if (at == std::string::npos)
                continue;
            Hit hit;
            hit.Doc = static_cast<int>(d);
            hit.Anchor = anchor;
            hit.Heading = heading;
            const size_t from = at > 40 ? at - 40 : 0;
            hit.Snippet = (from > 0 ? "..." : "") + text.substr(from, 120);
            std::replace(hit.Snippet.begin(), hit.Snippet.end(), '\n', ' ');
            if (hit.Anchor.empty())
            {
                for (const Markdown::Block &block : m_Entries[d].Document.Blocks)
                    if (block.Type == Markdown::BlockType::Heading && Markdown::PlainText(block.Spans) == heading)
                        hit.Anchor = block.Anchor;
            }
            m_Hits.push_back(std::move(hit));
            if (++perDoc >= 14)
                break;
        }
    }
}

void DocsPanel::DrawSidebar(float width)
{
    using namespace EditorUI;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Frame);
    ImGui::BeginChild("##docsSide", ImVec2(width, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##docsSearch", "Search the guides", m_Query, sizeof(m_Query));
    if (m_LastQuery != m_Query)
    {
        m_LastQuery = m_Query;
        Search();
    }
    ImGui::Spacing();

    if (!m_LastQuery.empty() && m_LastQuery.size() >= 2)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::Text("%d result%s", static_cast<int>(m_Hits.size()), m_Hits.size() == 1 ? "" : "s");
        ImGui::PopStyleColor();
        ImGui::BeginChild("##docsHits", ImVec2(0, 0), ImGuiChildFlags_None);
        int index = 0;
        for (const Hit &hit : m_Hits)
        {
            ImGui::PushID(index++);
            const ImVec2 start = ImGui::GetCursorScreenPos();
            const float w = ImGui::GetContentRegionAvail().x;
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Accent);
            ImGui::TextUnformatted(m_Entries[hit.Doc].Title.c_str());
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Text);
            ImGui::TextUnformatted(hit.Heading.c_str());
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w);
            ImGui::TextUnformatted(hit.Snippet.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            const ImVec2 end(start.x + w, ImGui::GetCursorScreenPos().y);
            ImGui::SetCursorScreenPos(start);
            if (ImGui::InvisibleButton("##hit", ImVec2(w, std::max(1.0f, end.y - start.y))))
            {
                m_Tab = 0;
                Show(hit.Doc, hit.Anchor, true);
            }
            if (ImGui::IsItemHovered())
                ImGui::GetWindowDrawList()->AddRectFilled(start, end, U32(Colors::Hover, 0.5f), 4.0f);
            ImGui::SetCursorScreenPos(ImVec2(start.x, end.y + 6.0f));
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::EndChild();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextUnformatted("GUIDES");
    ImGui::PopStyleColor();
    for (size_t i = 0; i < m_Entries.size(); ++i)
    {
        if (ImGui::Selectable(m_Entries[i].Title.c_str(), static_cast<int>(i) == m_Current && m_Tab == 0))
        {
            m_Tab = 0;
            Show(static_cast<int>(i), std::string(), true);
        }
    }
    if (ImGui::Selectable("Lua API reference", m_Tab == 1))
        m_Tab = 1;

    if (m_Tab == 0 && m_Current >= 0)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextUnformatted("ON THIS PAGE");
        ImGui::PopStyleColor();
        ImGui::BeginChild("##docsOutline", ImVec2(0, 0), ImGuiChildFlags_None);
        for (const Markdown::Block &block : m_Entries[m_Current].Document.Blocks)
        {
            if (block.Type != Markdown::BlockType::Heading || block.Level > 3 || block.Level < 2)
                continue;
            ImGui::PushID(block.Anchor.c_str());
            if (block.Level == 3)
                ImGui::Indent(12.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, block.Level == 2 ? Colors::Text : Colors::TextDim);
            const std::string title = Markdown::PlainText(block.Spans);
            if (ImGui::Selectable(title.c_str(), false))
                Show(m_Current, block.Anchor, false);
            ImGui::PopStyleColor();
            if (block.Level == 3)
                ImGui::Unindent(12.0f);
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void DocsPanel::DrawGuides()
{
    using namespace EditorUI;
    if (m_Entries.empty())
    {
        ImGui::Spacing();
        ImGui::TextWrapped("The docs folder was not found next to the editor. Expected a docs/ folder beside the engine resources.");
        return;
    }

    const bool canBack = m_Position > 0;
    const bool canForward = m_Position + 1 < static_cast<int>(m_History.size());
    ImGui::BeginDisabled(!canBack);
    if (ImGui::Button("Back"))
    {
        --m_Position;
        Show(m_History[m_Position].Doc, m_History[m_Position].Anchor, false);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!canForward);
    if (ImGui::Button("Forward"))
    {
        ++m_Position;
        Show(m_History[m_Position].Doc, m_History[m_Position].Anchor, false);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    if (m_Current >= 0)
        ImGui::TextUnformatted(m_Entries[m_Current].File.c_str());
    ImGui::PopStyleColor();
    const float right = ImGui::GetContentRegionAvail().x;
    ImGui::SameLine(ImGui::GetCursorPosX() + right - 150.0f);
    if (ImGui::Button("A-"))
        m_View.SetScale(m_View.GetScale() - 0.1f);
    ImGui::SameLine();
    if (ImGui::Button("A+"))
        m_View.SetScale(m_View.GetScale() + 0.1f);
    ImGui::SameLine();
    if (ImGui::Button("100%"))
        m_View.SetScale(1.0f);

    ImGui::Spacing();
    ImGui::BeginChild("##docsBody", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollWithMouse * 0);
    ImGui::Dummy(ImVec2(0, 4));
    const std::string clicked = m_View.Draw();
    ImGui::EndChild();
    if (!clicked.empty())
        Navigate(clicked);
}

void DocsPanel::DrawApi()
{
    using namespace EditorUI;
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##apiSearch", "Filter functions, e.g. Draw, SetColor, Physics.Raycast", m_ApiQuery, sizeof(m_ApiQuery));
    const std::string query = Lower(m_ApiQuery);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextUnformatted("Every function the engine registers for Lua, with its argument names. ? marks optional arguments.");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::BeginChild("##apiList", ImVec2(0, 0), ImGuiChildFlags_None);
    ImFont *mono = GetFont(FontRole::Mono);
    std::string lastGroup;
    for (const auto &[key, text] : m_Api)
    {
        if (!query.empty() && Lower(key).find(query) == std::string::npos)
            continue;
        const size_t cut = key.find_first_of(".:");
        const std::string group = cut == std::string::npos ? std::string("Global") : key.substr(0, cut);
        if (group != lastGroup)
        {
            lastGroup = group;
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Accent);
            ImGui::TextUnformatted(group.c_str());
            ImGui::PopStyleColor();
            ImGui::Separator();
        }
        ImGui::PushFont(mono, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::Text);
        ImGui::Selectable(text.c_str(), false);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) || (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)))
            ImGui::SetClipboardText(text.c_str());
    }
    ImGui::EndChild();
}

void DocsPanel::OnImGuiRender()
{
    if (!m_Visible)
        return;

    if (ImGuiWindow *viewport = ImGui::FindWindowByName("Viewport###Viewport"); viewport && viewport->DockId)
        ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Documentation###Docs", &m_Visible))
    {
        ImGui::End();
        return;
    }

    if (!m_Loaded)
        Load();

    if (m_HasPending)
    {
        m_HasPending = false;
        m_Tab = 0;
        int doc = m_Current >= 0 ? m_Current : (m_Entries.empty() ? -1 : 0);
        if (!m_Pending.empty())
            for (size_t i = 0; i < m_Entries.size(); ++i)
                if (m_Entries[i].File == m_Pending)
                    doc = static_cast<int>(i);
        if (doc >= 0)
            Show(doc, m_PendingAnchor, true);
        m_Pending.clear();
        m_PendingAnchor.clear();
    }
    else if (m_Current < 0 && !m_Entries.empty())
        Show(0, std::string(), true);

    DrawSidebar(270.0f);
    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 14.0f));
    ImGui::BeginChild("##docsMain", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    if (m_Tab == 0)
        DrawGuides();
    else
        DrawApi();
    ImGui::EndChild();
    ImGui::End();
}

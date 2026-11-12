#include "ScriptEditorPanel.h"
#include <Engine/Core/FileSystem.h>
#include "AssetPickerPanel.h"
#include "EditorContext.h"
#include "UI/Visuals.h"
#include "UI/Widgets.h"

#include <imgui_internal.h>

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Scripting/LuaInclude.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace
{
    namespace fs = std::filesystem;

    constexpr const char *ScriptTemplate =
        "local Script = {}\n"
        "\n"
        "function Script:OnCreate()\n"
        "end\n"
        "\n"
        "function Script:OnUpdate(dt)\n"
        "end\n"
        "\n"
        "return Script\n";

    std::string FileName(const std::string &path)
    {
        size_t slash = path.find_last_of("/\\");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    bool ReadText(const std::string &path, std::string &out)
    {
        std::ifstream file(EditorUI::Utf8Path(path), std::ios::binary);
        if (!file)
            return false;
        std::stringstream buffer;
        buffer << file.rdbuf();
        out = buffer.str();
        return true;
    }

    fs::file_time_type DiskTime(const std::string &path)
    {
        std::error_code ec;
        auto time = fs::last_write_time(EditorUI::Utf8Path(path), ec);
        return ec ? fs::file_time_type{} : time;
    }

    bool ParseLuaError(const std::string &message, int &line, std::string &text)
    {
        for (size_t i = 0; i < message.size(); ++i)
        {
            if (message[i] != ':' || i + 1 >= message.size() || !std::isdigit(static_cast<unsigned char>(message[i + 1])))
                continue;
            size_t j = i + 1;
            while (j < message.size() && std::isdigit(static_cast<unsigned char>(message[j])))
                ++j;
            if (j >= message.size() || message[j] != ':')
                continue;
            line = std::atoi(message.substr(i + 1, j - i - 1).c_str()) - 1;
            text = message.substr(std::min(message.size(), j + 2));
            return true;
        }
        line = -1;
        text = message;
        return false;
    }
}

ScriptEditorPanel::ScriptEditorPanel(EditorContext *context)
    : m_Context(context)
{
}

ScriptEditorPanel::~ScriptEditorPanel()
{
    if (m_Checker)
        lua_close(m_Checker);
}

bool ScriptEditorPanel::HasUnsavedChanges() const
{
    return std::any_of(m_Documents.begin(), m_Documents.end(), [](const auto &doc) { return doc->Dirty(); });
}

ScriptEditorPanel::Document *ScriptEditorPanel::Active()
{
    if (m_Active < 0 || m_Active >= static_cast<int>(m_Documents.size()))
        return nullptr;
    return m_Documents[static_cast<size_t>(m_Active)].get();
}

void ScriptEditorPanel::FocusEditor()
{
    m_FocusWindow = true;
    if (Document *doc = Active())
        doc->Editor.Focus();
}

void ScriptEditorPanel::EnsureInitialized()
{
    if (m_Initialized)
        return;
    m_Initialized = true;
    m_Themes.Refresh();
    ApplyTheme(EditorUI::Settings().ScriptTheme);
    m_Completion.RefreshApi();
    m_Completion.SetModuleLoader([this](const std::string &module, std::string &vpath, std::vector<std::string> &lines)
                                 { return LoadModule(module, vpath, lines); });
    m_Checker = luaL_newstate();
}

void ScriptEditorPanel::ApplyTheme(const std::string &name)
{
    if (!m_Themes.Select(name))
    {
        for (const auto &entry : m_Themes.Entries())
            if (m_Themes.Select(entry.Name))
                break;
    }
    if (EditorUI::Settings().ScriptTheme != m_Themes.CurrentName())
    {
        EditorUI::Settings().ScriptTheme = m_Themes.CurrentName();
        ImGui::MarkIniSettingsDirty();
    }
}

ScriptEditorPanel::Document *ScriptEditorPanel::FindDocument(const std::string &vpath)
{
    for (auto &doc : m_Documents)
        if (doc->VPath == vpath)
            return doc.get();
    return nullptr;
}

bool ScriptEditorPanel::LoadModule(const std::string &module, std::string &vpath, std::vector<std::string> &lines)
{
    std::string relative = module;
    for (char &c : relative)
        if (c == '.')
            c = '/';
    if (relative.empty() || relative.find("..") != std::string::npos)
        return false;

    for (const std::string &candidate : {relative + ".lua", relative + "/init.lua"})
    {
        const std::string virtualPath = "assets://scripts/" + candidate;
        auto resolved = SIMPEngine::VFS::Resolve(virtualPath);
        if (!resolved || !SIMPEngine::FileSystem::Exists(*resolved))
            continue;

        std::string text;
        if (Document *open = FindDocument(virtualPath))
            text = open->Editor.GetText();
        else if (!ReadText(*resolved, text))
            continue;

        lines.clear();
        size_t from = 0;
        while (from <= text.size())
        {
            size_t end = text.find('\n', from);
            std::string line = text.substr(from, end == std::string::npos ? std::string::npos : end - from);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(std::move(line));
            if (end == std::string::npos)
                break;
            from = end + 1;
        }
        vpath = virtualPath;
        return true;
    }
    return false;
}

void ScriptEditorPanel::HandleNavigation(Document &from, const ScriptEditor::NavigationRequest &request)
{
    if (request.Back)
    {
        if (m_JumpHistory.empty())
        {
            m_Status = "Nothing to go back to";
            m_StatusTime = ImGui::GetTime();
            return;
        }
        const JumpEntry entry = m_JumpHistory.back();
        m_JumpHistory.pop_back();
        Open(entry.VPath);
        if (Document *doc = FindDocument(entry.VPath))
            doc->Editor.GoToPosition(entry.Line, entry.Column);
        return;
    }

    m_JumpHistory.push_back({from.VPath, request.FromLine, request.FromColumn});
    if (m_JumpHistory.size() > 64)
        m_JumpHistory.erase(m_JumpHistory.begin());

    const std::string target = request.VPath.empty() ? from.VPath : request.VPath;
    Open(target);
    if (Document *doc = FindDocument(target))
        doc->Editor.GoToPosition(request.Line, request.Column);
}

void ScriptEditorPanel::Open(const std::string &vpath)
{
    EnsureInitialized();
    m_Visible = true;
    m_FocusWindow = true;

    for (size_t i = 0; i < m_Documents.size(); ++i)
    {
        if (m_Documents[i]->VPath == vpath)
        {
            m_SelectTab = static_cast<int>(i);
            m_Documents[i]->Editor.Focus();
            return;
        }
    }

    auto resolved = SIMPEngine::VFS::Resolve(vpath);
    std::string real = resolved ? *resolved : vpath;
    std::string text;
    if (!ReadText(real, text))
    {
        LOG_ERROR("Script editor: could not open {}", vpath);
        return;
    }

    auto doc = std::make_unique<Document>();
    doc->VPath = vpath;
    doc->RealPath = real;
    doc->Name = FileName(vpath);
    doc->Editor.SetText(text);
    doc->SavedVersion = doc->Editor.Version();
    doc->LastEditVersion = doc->SavedVersion;
    doc->DiskTime = DiskTime(real);
    doc->Editor.Focus();
    CheckSyntax(*doc);
    m_Documents.push_back(std::move(doc));
    m_SelectTab = static_cast<int>(m_Documents.size()) - 1;
}

bool ScriptEditorPanel::Save(Document &doc)
{
    std::string text = doc.Editor.GetText();
    if (text.empty() || text.back() != '\n')
        text += '\n';
    if (!SIMPEngine::FileSystem::WriteText(doc.RealPath, text, false))
    {
        LOG_ERROR("Script editor: could not write {}", doc.RealPath);
        m_Status = "Could not save " + doc.Name;
        m_StatusTime = ImGui::GetTime();
        return false;
    }
    doc.SavedVersion = doc.Editor.Version();
    doc.DiskTime = DiskTime(doc.RealPath);
    m_Status = "Saved " + doc.Name;
    m_StatusTime = ImGui::GetTime();
    return true;
}

void ScriptEditorPanel::ReloadFromDisk(Document &doc)
{
    std::string text;
    if (!ReadText(doc.RealPath, text))
        return;
    ScriptEditor::Coord cursor = doc.Editor.GetCursor();
    doc.Editor.SetText(text);
    doc.Editor.GoToLine(cursor.Line);
    doc.SavedVersion = doc.Editor.Version();
    doc.CheckedVersion = UINT64_MAX;
    m_Status = "Reloaded " + doc.Name + " from disk";
    m_StatusTime = ImGui::GetTime();
}

void ScriptEditorPanel::Close(size_t index)
{
    if (index >= m_Documents.size())
        return;
    m_Documents.erase(m_Documents.begin() + static_cast<std::ptrdiff_t>(index));
    if (m_Active >= static_cast<int>(m_Documents.size()))
        m_Active = static_cast<int>(m_Documents.size()) - 1;
}

void ScriptEditorPanel::CheckSyntax(Document &doc)
{
    doc.CheckedVersion = doc.Editor.Version();
    if (!m_Checker)
        return;
    std::string text = doc.Editor.GetText();
    std::string chunk = "=" + doc.Name;
    int status = luaL_loadbufferx(m_Checker, text.data(), text.size(), chunk.c_str(), "t");
    if (status == LUA_OK)
    {
        lua_pop(m_Checker, 1);
        doc.ErrorLine = -1;
        doc.ErrorMessage.clear();
    }
    else
    {
        const char *message = lua_tostring(m_Checker, -1);
        ParseLuaError(message ? message : "syntax error", doc.ErrorLine, doc.ErrorMessage);
        lua_pop(m_Checker, 1);
    }
    doc.Editor.SetError(doc.ErrorLine, doc.ErrorMessage);
}

std::string ScriptEditorPanel::CreateScript(const std::string &name)
{
    auto directory = SIMPEngine::VFS::Resolve("assets://scripts");
    if (!directory)
        return {};
    std::error_code ec;
    fs::path root = EditorUI::Utf8Path(*directory);
    fs::create_directories(root, ec);

    std::string base;
    for (char c : name)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')
            base += c;
        else if (c == ' ' && !base.empty() && base.back() != '_')
            base += '_';
    }
    if (base.size() > 4 && base.compare(base.size() - 4, 4, ".lua") == 0)
        base.resize(base.size() - 4);
    if (base.empty())
        base = "Script";

    std::string filename = base + ".lua";
    for (int i = 2; i < 1000 && fs::exists(root / EditorUI::Utf8Path(filename), ec); ++i)
        filename = base + "_" + std::to_string(i) + ".lua";

    std::ofstream file(root / EditorUI::Utf8Path(filename), std::ios::binary);
    if (!file)
        return {};
    file << ScriptTemplate;
    return "assets://scripts/" + filename;
}

void ScriptEditorPanel::DrawThemeMenu()
{
    using namespace EditorUI;

    ImGui::SetNextItemWidth(std::min(220.0f, ImGui::GetContentRegionAvail().x));
    ImGui::SetNextWindowSizeConstraints(ImVec2(240, 0), ImVec2(FLT_MAX, 420));
    if (!ImGui::BeginCombo("##Theme", m_Themes.CurrentName().c_str(), ImGuiComboFlags_HeightLarge))
        return;

    if (ImGui::IsWindowAppearing())
    {
        m_ThemeFilter[0] = '\0';
        ImGui::SetKeyboardFocusHere();
    }
    SearchBox("##ThemeFilter", m_ThemeFilter, sizeof(m_ThemeFilter), "Filter themes");

    std::string group;
    int shown = 0;
    for (const auto &entry : m_Themes.Entries())
    {
        if (m_ThemeFilter[0] && !ContainsInsensitive(entry.Name.c_str(), m_ThemeFilter))
            continue;
        if (entry.Group != group)
        {
            group = entry.Group;
            ImGui::SeparatorText(group.c_str());
        }
        ImGui::PushID(entry.Path.c_str());
        bool selected = entry.Name == m_Themes.CurrentName();
        if (ImGui::Selectable(entry.Name.c_str(), selected))
            ApplyTheme(entry.Name);
        if (selected && ImGui::IsWindowAppearing())
            ImGui::SetScrollHereY();
        ImGui::PopID();
        ++shown;
    }
    if (shown == 0)
        ImGui::TextDisabled("No themes match");

    ImGui::Separator();
    ImGui::TextDisabled("Drop VS Code theme .json files into");
    ImGui::TextDisabled("assets/editor/themes");
    ImGui::EndCombo();
}

void ScriptEditorPanel::DrawToolbar()
{
    using namespace EditorUI;

    BeginToolbar("##ScriptToolbar");
    if (IconButton("##New", ICON_LC_FILE_PLUS, "New script"))
    {
        m_ShowNewScript = true;
        m_NewScriptName[0] = '\0';
    }
    ImGui::SameLine();
    if (IconButton("##Open", ICON_LC_FOLDER_OPEN, "Open script") && m_AssetPicker)
        m_AssetPicker->Open("Open Lua Script", "assets://", [this](const std::string &vpath) { Open(vpath); }, {".lua"});
    ImGui::SameLine();
    Document *doc = Active();
    ImGui::BeginDisabled(!doc || !doc->Dirty());
    if (IconButton("##Save", ICON_LC_SAVE, "Save (Ctrl+S)") && doc)
        Save(*doc);
    ImGui::EndDisabled();

    ImGui::SameLine();
    VerticalSeparator();
    ImGui::SameLine();
    float &size = Settings().ScriptFontSize;
    if (IconButton("##ZoomOut", ICON_LC_ZOOM_OUT, "Smaller text (Ctrl+Wheel)"))
    {
        size = std::max(9.0f, size - 1.0f);
        ImGui::MarkIniSettingsDirty();
    }
    ImGui::SameLine();
    if (IconButton("##ZoomIn", ICON_LC_ZOOM_IN, "Larger text (Ctrl+Wheel)"))
    {
        size = std::min(32.0f, size + 1.0f);
        ImGui::MarkIniSettingsDirty();
    }

    float comboWidth = std::min(220.0f, ImGui::GetContentRegionAvail().x);
    float right = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(), right - comboWidth - ImGui::GetFrameHeight() - 4.0f));
    if (IconButton("##ReloadThemes", ICON_LC_REFRESH_CW, "Rescan themes"))
    {
        std::string current = m_Themes.CurrentName();
        m_Themes.Refresh();
        ApplyTheme(current);
        for (auto &d : m_Documents)
            d->Editor.InvalidateHighlight();
    }
    ImGui::SameLine();
    DrawThemeMenu();
    EndToolbar();
}

void ScriptEditorPanel::DrawStatusBar(Document *doc)
{
    using namespace EditorUI;
    const ScriptEditor::CodeTheme &theme = m_Themes.Current();

    float height = ImGui::GetFrameHeight();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(theme.Gutter));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
    ImGui::BeginChild("##ScriptStatus", ImVec2(0, height), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetWindowPos();
    dl->AddLine(min, ImVec2(min.x + ImGui::GetWindowWidth(), min.y), theme.PopupBorder);

    ImVec4 text = ImGui::ColorConvertU32ToFloat4(theme.LineNumberActive);
    ImVec4 faint = ImGui::ColorConvertU32ToFloat4(theme.LineNumber);
    ImGui::SetCursorPosY((height - ImGui::GetTextLineHeight()) * 0.5f);

    if (doc)
    {
        if (doc->ErrorLine >= 0 || !doc->ErrorMessage.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(theme.Error));
            char label[512];
            std::snprintf(label, sizeof(label), "Line %d: %s", doc->ErrorLine + 1, doc->ErrorMessage.c_str());
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked() && doc->ErrorLine >= 0)
                doc->Editor.GoToLine(doc->ErrorLine);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, faint);
            ImGui::TextUnformatted("No problems");
            ImGui::PopStyleColor();
        }

        bool flash = !m_Status.empty() && ImGui::GetTime() - m_StatusTime < 2.5;
        char right[256];
        ScriptEditor::Coord cursor = doc->Editor.GetCursor();
        std::snprintf(right, sizeof(right), "%s%sLn %d, Col %d    %d lines    Lua    %s", flash ? m_Status.c_str() : "", flash ? "    " : "",
                      cursor.Line + 1, cursor.Column + 1, doc->Editor.LineCount(), theme.Name.c_str());
        float width = ImGui::CalcTextSize(right).x;
        ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 20.0f, ImGui::GetWindowWidth() - width - 10.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, text);
        ImGui::TextUnformatted(right);
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ScriptEditorPanel::DrawTabs()
{
    using namespace EditorUI;

    Document *active = nullptr;
    ImGuiTabBarFlags flags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll;
    if (ImGui::BeginTabBar("##ScriptTabs", flags))
    {
        for (size_t i = 0; i < m_Documents.size(); ++i)
        {
            Document &doc = *m_Documents[i];
            ImGuiTabItemFlags tabFlags = doc.Dirty() ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None;
            if (m_SelectTab == static_cast<int>(i))
                tabFlags |= ImGuiTabItemFlags_SetSelected;
            bool open = true;
            std::string label = doc.Name + "###" + doc.VPath;
            bool visible = ImGui::BeginTabItem(label.c_str(), &open, tabFlags);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("%s", doc.VPath.c_str());
            if (visible)
            {
                m_Active = static_cast<int>(i);
                active = &doc;
                ImGui::EndTabItem();
            }
            if (!open)
            {
                if (doc.Dirty())
                    m_CloseRequest = static_cast<int>(i);
                else
                {
                    Close(i);
                    break;
                }
            }
        }
        ImGui::EndTabBar();
    }
    m_SelectTab = -1;

    if (!active)
        return;

    float fontSize = Settings().ScriptFontSize;
    float statusHeight = ImGui::GetFrameHeight();
    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::BeginChild("##ScriptBody", ImVec2(0, std::max(40.0f, ImGui::GetContentRegionAvail().y - statusHeight)), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
    active->Editor.Render(active->VPath.c_str(), m_Themes.Current(), GetFont(FontRole::Mono), fontSize, m_Completion);
    {
        ScriptEditor::NavigationRequest navigation;
        if (active->Editor.ConsumeNavigation(navigation))
            HandleNavigation(*active, navigation);
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    m_EditorFocused = active->Editor.IsFocused();

    float zoom = active->Editor.ConsumeZoom();
    if (zoom != 0.0f)
    {
        Settings().ScriptFontSize = std::clamp(fontSize + zoom, 9.0f, 32.0f);
        ImGui::MarkIniSettingsDirty();
    }
    if (active->Editor.ConsumeSaveRequest())
        Save(*active);

    if (active->Editor.Version() != active->LastEditVersion)
    {
        active->LastEditVersion = active->Editor.Version();
        active->LastEdit = ImGui::GetTime();
    }
    if (active->CheckedVersion != active->Editor.Version() && ImGui::GetTime() - active->LastEdit > 0.35)
        CheckSyntax(*active);

    DrawStatusBar(active);
    ImGui::PopStyleVar();
}

void ScriptEditorPanel::DrawNewScriptPopup()
{
    using namespace EditorUI;

    if (m_ShowNewScript)
    {
        ImGui::OpenPopup("New Script###NewScript");
        m_ShowNewScript = false;
    }
    ImGui::SetNextWindowSize(ImVec2(340, 0));
    if (!ImGui::BeginPopupModal("New Script###NewScript", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::TextUnformatted("Name");
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool enter = ImGui::InputTextWithHint("##Name", "PlayerController", m_NewScriptName, sizeof(m_NewScriptName), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::TextDisabled("Created in assets://scripts");
    ImGui::Spacing();

    float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if (Button("Create", ButtonKind::Primary, ImVec2(half, 0)) || enter)
    {
        std::string path = CreateScript(m_NewScriptName[0] ? m_NewScriptName : "Script");
        if (path.empty())
            LOG_ERROR("Script editor: could not create a script under assets://scripts");
        else
            Open(path);
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void ScriptEditorPanel::DrawClosePopup()
{
    using namespace EditorUI;

    if (m_CloseRequest >= static_cast<int>(m_Documents.size()))
        m_CloseRequest = -1;
    if (m_CloseRequest >= 0 && !ImGui::IsPopupOpen("Unsaved Script###CloseScript"))
        ImGui::OpenPopup("Unsaved Script###CloseScript");
    if (!ImGui::BeginPopupModal("Unsaved Script###CloseScript", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
        return;

    if (m_CloseRequest < 0)
    {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    Document &doc = *m_Documents[static_cast<size_t>(m_CloseRequest)];
    ImGui::Text("Save changes to %s before closing?", doc.Name.c_str());
    ImGui::Spacing();
    const float width = 96.0f;
    if (Button("Save", ButtonKind::Primary, ImVec2(width, 0)))
    {
        if (Save(doc))
            Close(static_cast<size_t>(m_CloseRequest));
        m_CloseRequest = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button("Don't Save", ButtonKind::Danger, ImVec2(width, 0)))
    {
        Close(static_cast<size_t>(m_CloseRequest));
        m_CloseRequest = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(width, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        m_CloseRequest = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ScriptEditorPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (m_Context && !m_Context->OpenScriptRequest.empty())
    {
        std::string path = std::move(m_Context->OpenScriptRequest);
        m_Context->OpenScriptRequest.clear();
        Open(path);
    }

    m_EditorFocused = false;
    if (!m_Visible)
        return;
    EnsureInitialized();

    double now = ImGui::GetTime();
    if (now - m_LastDiskCheck > 1.0)
    {
        m_LastDiskCheck = now;
        for (auto &doc : m_Documents)
        {
            auto time = DiskTime(doc->RealPath);
            if (time != fs::file_time_type{} && time != doc->DiskTime)
            {
                doc->DiskTime = time;
                if (!doc->Dirty())
                    ReloadFromDisk(*doc);
            }
        }
    }

    if (m_FocusWindow)
    {
        ImGui::SetNextWindowFocus();
        m_FocusWindow = false;
    }

    if (ImGuiWindow *viewport = ImGui::FindWindowByName("Viewport###Viewport"); viewport && viewport->DockId)
        ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::Begin("Script###ScriptEditor", &m_Visible);
    ImGui::PopStyleVar();
    if (visible)
    {
        DrawToolbar();
        if (m_Documents.empty())
        {
            EmptyState("No script open", "Open a .lua file, double-click one in the FileSystem, or press Edit on a Script component");
            m_Active = -1;
        }
        else
        {
            DrawTabs();
        }
    }
    DrawNewScriptPopup();
    DrawClosePopup();
    ImGui::End();
}

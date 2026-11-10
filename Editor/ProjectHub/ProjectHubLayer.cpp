#include "ProjectHubLayer.h"

#include "UI/FileDialogs.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <Engine/Application/Application.h>
#include <Engine/Core/Version.h>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>

namespace fs = std::filesystem;

namespace
{
    constexpr const char *NewLocationDialog = "hub-new-location";
    constexpr const char *OpenDialog = "hub-open";

    std::string DefaultLocation()
    {
        if (const char *documents = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS))
            return (fs::path(documents) / "SIMPEngine Projects").string();
        if (const char *home = SDL_GetUserFolder(SDL_FOLDER_HOME))
            return (fs::path(home) / "SIMPEngine Projects").string();
        return fs::current_path().string();
    }

    std::string TimeAgo(int64_t then)
    {
        if (then <= 0)
            return "";
        const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        const int64_t seconds = std::max<int64_t>(0, now - then);
        char text[48];
        if (seconds < 60)
            return "Just now";
        if (seconds < 3600)
            std::snprintf(text, sizeof(text), "%lld min ago", static_cast<long long>(seconds / 60));
        else if (seconds < 86400)
            std::snprintf(text, sizeof(text), "%lld h ago", static_cast<long long>(seconds / 3600));
        else if (seconds < 2 * 86400)
            return "Yesterday";
        else if (seconds < 30 * 86400)
            std::snprintf(text, sizeof(text), "%lld days ago", static_cast<long long>(seconds / 86400));
        else
        {
            std::time_t t = static_cast<std::time_t>(then);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            std::strftime(text, sizeof(text), "%d %b %Y", &tm);
        }
        return text;
    }

    std::string Initials(const std::string &name)
    {
        std::string result;
        bool start = true;
        for (char c : name)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if (std::isalnum(u))
            {
                if (start && result.size() < 2)
                    result += static_cast<char>(std::toupper(u));
                start = false;
            }
            else
                start = true;
        }
        return result.empty() ? "?" : result;
    }

    const ImVec4 &TileColor(const std::string &name)
    {
        using namespace EditorUI;
        const ImVec4 *palette[] = {&Colors::Accent, &Colors::Success, &Colors::Warning, &Colors::AxisX, &Colors::AxisY, &Colors::FolderTint};
        unsigned hash = 2166136261u;
        for (char c : name)
            hash = (hash ^ static_cast<unsigned char>(c)) * 16777619u;
        return *palette[hash % 6];
    }

    bool NavItem(const char *icon, const char *label, bool selected, float width)
    {
        using namespace EditorUI;
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        const ImVec2 pos = window->DC.CursorPos;
        const float h = 30.0f;
        const ImRect bb(pos, ImVec2(pos.x + width, pos.y + h));
        const ImGuiID id = window->GetID(label);
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;
        bool hovered = false, held = false;
        const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        ImDrawList *dl = window->DrawList;
        if (selected || hovered)
            dl->AddRectFilled(bb.Min, bb.Max, U32(selected ? Colors::Selection : Colors::Hover), Metrics::FrameRounding);
        const float fs = ImGui::GetFontSize();
        const float cy = bb.GetCenter().y;
        dl->AddText(ImVec2(bb.Min.x + 10.0f, std::floor(cy - fs * 0.5f)), U32(selected ? Colors::Text : Colors::TextDim), icon);
        dl->AddText(ImVec2(bb.Min.x + 34.0f, std::floor(cy - fs * 0.5f)), U32(selected ? Colors::Text : Colors::TextDim), label);
        return pressed;
    }
}

ProjectHubLayer::ProjectHubLayer(OpenCallback onOpen)
    : Layer("ProjectHub"), m_OnOpen(std::move(onOpen))
{
}

void ProjectHubLayer::OnAttach()
{
    EditorUI::RegisterSettingsHandler();
    EditorUI::LoadFonts();
    EditorUI::ApplyTheme(EditorUI::Settings().Theme);
    m_Recent.Load();
    m_NewLocation = DefaultLocation();
    FindGuides();
    SDL_SetWindowTitle(SIMPEngine::Application::Get().GetWindow().GetNativeWindow(), (std::string(SIMPEngine::EngineName) + " " + SIMPEngine::EngineVersion + " - Projects").c_str());
}

void ProjectHubLayer::FindGuides()
{
    m_Guides.clear();
    const fs::path engine = SIMPEngine::Project::EngineResourceDirectory();
    std::error_code ec;
    fs::path docs;
    for (const fs::path &candidate : {engine / "docs", engine.parent_path() / "docs"})
    {
        if (fs::is_directory(candidate, ec))
        {
            docs = candidate;
            break;
        }
    }
    if (docs.empty())
        return;

    const struct
    {
        const char *File;
        const char *Title;
        const char *Summary;
    } known[] = {
        {"SHIPPING.md", "Projects and Shipping", "Project settings, Run Standalone, Build Game and what goes into a build."},
        {"HOW_TO.md", "How To", "Short recipes for the things you do every day: sprites, cameras, tilemaps, audio."},
        {"SCRIPTING.md", "Scripting with Lua", "Hooks, input, components from scripts, events and the script editor."},
        {"ANIMATION.md", "Tweens and Vector Shapes", "Flutter-style tweens, curves, springs, paths and morphing shapes."},
        {"API_GUIDE.md", "C++ API Guide", "Entities, components and systems from C++."},
        {"ARCHITECTURE.md", "Architecture", "How the engine, editor and renderer fit together."},
        {"USING_THE_LIBRARY.md", "Using the Library", "Link SIMPEngine into your own CMake project."},
    };
    for (const auto &guide : known)
        if (fs::is_regular_file(docs / guide.File, ec))
            m_Guides.push_back({guide.Title, guide.Summary, docs / guide.File});
}

void ProjectHubLayer::ShowError(const std::string &message)
{
    m_Error = message;
    m_ShowError = true;
}

void ProjectHubLayer::Open(const fs::path &path)
{
    std::string error;
    SIMPEngine::ProjectSettings settings;
    if (!SIMPEngine::Project::Load(path, settings, &error))
    {
        ShowError(error);
        return;
    }
    const fs::path directory = SIMPEngine::Project::FindProjectFile(path).parent_path();
    m_Recent.Touch(directory, settings.Name, settings.EngineVersion);
    if (!m_OnOpen(directory, error))
        ShowError(error);
}

void ProjectHubLayer::OnRender()
{
    using namespace EditorUI;

    if (auto folder = TakePickedFolder(NewLocationDialog))
        m_NewLocation = *folder;
    if (auto folder = TakePickedFolder(OpenDialog))
    {
        m_OpenPath = *folder;
        if (!SIMPEngine::Project::FindProjectFile(m_OpenPath).empty())
        {
            m_ShowOpen = false;
            Open(m_OpenPath);
        }
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::Canvas);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("###ProjectHub", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    const float sidebar = 232.0f;
    DrawSidebar(sidebar, viewport->WorkSize.y);
    ImGui::SameLine(0.0f, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32, 26));
    ImGui::BeginChild("##hubmain", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    if (m_Page == Page::Projects)
        DrawProjects();
    else
        DrawLearn();
    ImGui::EndChild();

    DrawNewProjectModal();
    DrawOpenModal();
    DrawErrorModal();
    ImGui::End();
}

void ProjectHubLayer::DrawSidebar(float width, float height)
{
    using namespace EditorUI;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Window);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 22));
    ImGui::BeginChild("##hubsidebar", ImVec2(width, height), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 logo = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(ImVec2(logo.x + 4, logo.y), ImVec2(logo.x + 38, logo.y + 34), U32(Colors::Accent), 8.0f);
    ImGui::PushFont(GetFont(FontRole::Strong), 20.0f);
    const ImVec2 letter = ImGui::CalcTextSize("S");
    dl->AddText(ImVec2(std::floor(logo.x + 21 - letter.x * 0.5f), std::floor(logo.y + 17 - letter.y * 0.5f)), U32(Colors::TextOnAccent), "S");
    ImGui::PopFont();

    ImGui::SetCursorScreenPos(ImVec2(logo.x + 48, logo.y - 1));
    ImGui::PushFont(GetFont(FontRole::Strong), 17.0f);
    ImGui::TextUnformatted(SIMPEngine::EngineName);
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(logo.x + 48, logo.y + 20));
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::Text("Version %s", SIMPEngine::EngineVersion);
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(logo.x, logo.y + 58));

    const float itemW = ImGui::GetContentRegionAvail().x;
    if (NavItem(ICON_LC_FOLDER_OPEN, "Projects", m_Page == Page::Projects, itemW))
        m_Page = Page::Projects;
    if (NavItem(ICON_LC_BOOK_OPEN, "Learn", m_Page == Page::Learn, itemW))
        m_Page = Page::Learn;

    const float footer = ImGui::GetWindowHeight() - 54.0f;
    if (ImGui::GetCursorPosY() < footer)
        ImGui::SetCursorPosY(footer);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextUnformatted("2D engine and editor");
    ImGui::TextUnformatted("SDL3 + OpenGL + Lua");
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ProjectHubLayer::DrawProjects()
{
    using namespace EditorUI;

    const float buttonW = 128.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    ImGui::PushFont(GetFont(FontRole::Strong), 24.0f);
    ImGui::TextUnformatted("Projects");
    ImGui::PopFont();
    ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonW * 2.0f - spacing);
    if (Button("Open...", ButtonKind::Secondary, ImVec2(buttonW, 30)))
    {
        m_OpenPath.clear();
        m_ShowOpen = true;
    }
    ImGui::SameLine();
    if (Button("New Project", ButtonKind::Primary, ImVec2(buttonW, 30)))
    {
        m_NewName = "My Game";
        m_ShowNew = true;
    }

    ImGui::Dummy(ImVec2(0, 6));
    SearchBox("##hubfilter", m_Filter, sizeof(m_Filter), "Search projects", 320.0f);
    ImGui::Dummy(ImVec2(0, 6));

    const auto &entries = m_Recent.Entries();
    if (entries.empty())
    {
        ImGui::Dummy(ImVec2(0, 40));
        EmptyState("No projects yet", "Create a new project, or open a folder that has a project.simp file.");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Window);
    ImGui::BeginChild("##projectlist", ImVec2(0, 0), ImGuiChildFlags_Borders);
    int shown = 0;
    int removeIndex = -1;
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        const RecentProject &project = entries[i];
        if (m_Filter[0] && !ContainsInsensitive(project.Name.c_str(), m_Filter) && !ContainsInsensitive(project.Path.c_str(), m_Filter))
            continue;
        std::error_code ec;
        const bool missing = !fs::is_regular_file(fs::path(project.Path) / SIMPEngine::Project::FileName, ec);
        ImGui::PushID(i);
        DrawProjectRow(i, project, missing);
        if (ImGui::BeginPopupContextItem("##projectmenu"))
        {
            if (ImGui::MenuItem("Open", nullptr, false, !missing))
                Open(project.Path);
            if (ImGui::MenuItem("Show in File Manager", nullptr, false, !missing))
                OpenInFileManager(project.Path);
            ImGui::Separator();
            if (ImGui::MenuItem("Remove from List"))
                removeIndex = i;
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ++shown;
    }
    if (shown == 0)
        EmptyState("No match", "No project name or folder matches the search.");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (removeIndex >= 0)
    {
        const std::string path = entries[removeIndex].Path;
        m_Recent.Remove(path);
        m_Recent.Save();
        m_Selected = -1;
    }
    else if (m_Selected >= 0 && m_Selected < static_cast<int>(entries.size()) && ImGui::IsKeyPressed(ImGuiKey_Enter) && !ImGui::IsAnyItemActive() && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
        Open(entries[m_Selected].Path);
}

void ProjectHubLayer::DrawProjectRow(int index, const RecentProject &project, bool missing)
{
    using namespace EditorUI;

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    const float rowH = 62.0f;
    const ImVec2 pos = window->DC.CursorPos;
    const float left = window->InnerRect.Min.x;
    const float right = window->InnerRect.Max.x;
    const ImRect bb(ImVec2(left, pos.y), ImVec2(right, pos.y + rowH));
    const ImGuiID id = window->GetID("##row");
    ImGui::ItemSize(ImVec2(right - left, rowH), 0.0f);
    if (!ImGui::ItemAdd(bb, id))
        return;

    bool hovered = false, held = false;
    ImGuiButtonFlags buttonFlags = ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight;
    buttonFlags |= ImGuiButtonFlags_PressedOnClick;
    if (ImGui::ButtonBehavior(bb, id, &hovered, &held, buttonFlags))
        m_Selected = index;
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        if (missing)
            ShowError("The project folder no longer exists:\n" + project.Path + "\n\nRight-click the row to remove it from the list.");
        else
            Open(project.Path);
    }

    ImDrawList *dl = window->DrawList;
    const bool selected = m_Selected == index;
    if (selected)
        dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Selection));
    else if (hovered)
        dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Hover));
    dl->AddLine(ImVec2(bb.Min.x, bb.Max.y - 0.5f), ImVec2(bb.Max.x, bb.Max.y - 0.5f), U32(Colors::Border));

    const float cy = bb.GetCenter().y;
    const ImVec2 tileMin(left + 16.0f, cy - 20.0f);
    const ImVec2 tileMax(left + 56.0f, cy + 20.0f);
    const ImVec4 &tint = TileColor(project.Name);
    dl->AddRectFilled(tileMin, tileMax, U32(tint, missing ? 0.12f : 0.22f), 8.0f);
    const std::string initials = Initials(project.Name);
    ImGui::PushFont(GetFont(FontRole::Strong), 16.0f);
    const ImVec2 ts = ImGui::CalcTextSize(initials.c_str());
    dl->AddText(ImVec2(std::floor((tileMin.x + tileMax.x - ts.x) * 0.5f), std::floor((tileMin.y + tileMax.y - ts.y) * 0.5f)), U32(tint, missing ? 0.5f : 1.0f), initials.c_str());
    ImGui::PopFont();

    const float textX = tileMax.x + 14.0f;
    const std::string when = missing ? "Missing" : TimeAgo(project.LastOpened);
    const std::string version = project.EngineVersion.empty() ? "" : "v" + project.EngineVersion;
    const float whenW = ImGui::CalcTextSize(when.c_str()).x;
    const float versionW = version.empty() ? 0.0f : ImGui::CalcTextSize(version.c_str()).x + 14.0f;
    const float textRight = right - 24.0f - std::max(whenW, versionW) - 16.0f;

    ImGui::PushClipRect(ImVec2(textX, bb.Min.y), ImVec2(textRight, bb.Max.y), true);
    ImGui::PushFont(GetFont(FontRole::Medium), 15.0f);
    dl->AddText(ImVec2(textX, cy - 19.0f), U32(missing ? Colors::TextDim : Colors::Text), project.Name.c_str());
    ImGui::PopFont();
    ImGui::PopClipRect();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::RenderTextEllipsis(dl, ImVec2(textX, cy + 2.0f), ImVec2(textRight, cy + 2.0f + ImGui::GetFontSize()), textRight, project.Path.c_str(), nullptr, nullptr);
    ImGui::PopStyleColor();
    if (hovered && ImGui::CalcTextSize(project.Path.c_str()).x > textRight - textX)
        ImGui::SetTooltip("%s", project.Path.c_str());

    dl->AddText(ImVec2(right - 24.0f - whenW, cy - 18.0f), U32(missing ? Colors::Error : Colors::TextDim), when.c_str());
    if (!version.empty())
    {
        const ImVec2 chipMax(right - 24.0f, cy + 18.0f);
        const ImVec2 chipMin(chipMax.x - versionW, cy + 1.0f);
        dl->AddRect(chipMin, chipMax, U32(Colors::Border), Metrics::FrameRounding);
        dl->AddText(ImVec2(chipMin.x + 7.0f, std::floor((chipMin.y + chipMax.y - ImGui::GetFontSize()) * 0.5f)), U32(Colors::TextFaint), version.c_str());
    }
    ImGui::SetCursorScreenPos(ImVec2(pos.x, bb.Max.y));
}

void ProjectHubLayer::DrawLearn()
{
    using namespace EditorUI;

    ImGui::PushFont(GetFont(FontRole::Strong), 24.0f);
    ImGui::TextUnformatted("Learn");
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::TextUnformatted("Guides that ship with the engine. They open in your default Markdown viewer.");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 10));

    if (m_Guides.empty())
    {
        EmptyState("No guides found", "The docs folder was not found next to the editor.");
        return;
    }

    const float spacing = 14.0f;
    const float avail = ImGui::GetContentRegionAvail().x;
    const int columns = std::max(1, static_cast<int>((avail + spacing) / (300.0f + spacing)));
    const float cardW = (avail - spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float cardH = 96.0f;

    for (size_t i = 0; i < m_Guides.size(); ++i)
    {
        const Guide &guide = m_Guides[i];
        if (i % static_cast<size_t>(columns) != 0)
            ImGui::SameLine(0.0f, spacing);
        ImGui::PushID(static_cast<int>(i));
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 max(min.x + cardW, min.y + cardH);
        if (ImGui::InvisibleButton("##guide", ImVec2(cardW, cardH)))
            OpenInFileManager(guide.File.string());
        const bool hovered = ImGui::IsItemHovered();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(min, max, U32(hovered ? Colors::Hover : Colors::Window), 6.0f);
        dl->AddRect(min, max, U32(hovered ? Colors::BorderStrong : Colors::Border), 6.0f);
        dl->AddText(ImVec2(min.x + 16.0f, min.y + 16.0f), U32(Colors::Accent), ICON_LC_FILE_TEXT);
        ImGui::PushFont(GetFont(FontRole::Medium), 15.0f);
        dl->AddText(ImVec2(min.x + 40.0f, min.y + 14.0f), U32(Colors::Text), guide.Title.c_str());
        ImGui::PopFont();
        dl->AddText(nullptr, 0.0f, ImVec2(min.x + 16.0f, min.y + 42.0f), U32(Colors::TextDim), guide.Summary.c_str(), nullptr, cardW - 32.0f);
        ImGui::PopID();
    }
}

void ProjectHubLayer::DrawNewProjectModal()
{
    using namespace EditorUI;

    if (!BeginModal("New Project", &m_ShowNew, ImVec2(560, 0)))
        return;
    PushEditSuppression();

    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    PropertyLabel("Name");
    bool submit = ImGui::InputText("##name", &m_NewName, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

    PropertyLabel("Location");
    const float browseW = 90.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseW - ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputText("##location", &m_NewLocation);
    ImGui::SameLine();
    if (Button("Browse...", ButtonKind::Secondary, ImVec2(browseW, 0)))
        PickFolder(NewLocationDialog, m_NewLocation);

    PropertyLabel("Template");
    const char *titles[] = {"Empty", "Platformer"};
    const char *summaries[] = {"A camera and a title. Start from scratch.", "A player with a Character Controller, platforms and a follow camera."};
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float cardW = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    const float cardH = 88.0f;
    for (int i = 0; i < 2; ++i)
    {
        if (i > 0)
            ImGui::SameLine(0.0f, gap);
        ImGui::PushID(i);
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 max(min.x + cardW, min.y + cardH);
        if (ImGui::InvisibleButton("##template", ImVec2(cardW, cardH)))
            m_NewTemplate = i;
        const bool hovered = ImGui::IsItemHovered();
        const bool selected = m_NewTemplate == i;
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(min, max, U32(selected ? Colors::Selection : (hovered ? Colors::Hover : Colors::Frame)), 6.0f);
        dl->AddRect(min, max, U32(selected ? Colors::Accent : Colors::Border), 6.0f, 0, selected ? 1.5f : 1.0f);
        dl->AddText(ImVec2(min.x + 10.0f, min.y + 9.0f), U32(selected ? Colors::Accent : Colors::TextDim), i == 0 ? ICON_LC_BOX : ICON_LC_GAMEPAD_2);
        dl->AddText(GetFont(FontRole::Medium), ImGui::GetFontSize(), ImVec2(min.x + 32.0f, min.y + 8.0f), U32(Colors::Text), titles[i]);
        dl->AddText(nullptr, 0.0f, ImVec2(min.x + 10.0f, min.y + 32.0f), U32(Colors::TextDim), summaries[i], nullptr, cardW - 20.0f);
        ImGui::PopID();
    }

    std::string name = m_NewName;
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back())))
        name.pop_back();
    const bool badName = name.empty() || name.find_first_of("/\\:*?\"<>|") != std::string::npos;
    const fs::path target = fs::path(m_NewLocation) / name;
    std::error_code ec;
    const bool exists = !badName && fs::exists(target, ec) && !fs::is_empty(target, ec);

    ImGui::Dummy(ImVec2(0, 4));
    ImGui::PushStyleColor(ImGuiCol_Text, badName || exists ? Colors::Warning : Colors::TextFaint);
    if (badName)
        ImGui::TextWrapped("Enter a name without / \\ : * ? \" < > |");
    else if (exists)
        ImGui::TextWrapped("%s already exists and is not empty.", target.string().c_str());
    else
        ImGui::TextWrapped("Creates %s", target.string().c_str());
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 4));
    const float bw = 100.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw * 2.0f - gap);
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(bw, 0)))
    {
        m_ShowNew = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    const bool valid = !badName && !exists && !m_NewLocation.empty();
    ImGui::BeginDisabled(!valid);
    if (Button("Create", ButtonKind::Primary, ImVec2(bw, 0)) || (submit && valid))
    {
        SIMPEngine::ProjectSettings settings;
        settings.Name = name;
        std::string error;
        const auto kind = m_NewTemplate == 0 ? SIMPEngine::ProjectTemplate::Empty : SIMPEngine::ProjectTemplate::Platformer;
        m_ShowNew = false;
        ImGui::CloseCurrentPopup();
        if (SIMPEngine::Project::Create(target, settings, kind, &error))
            Open(target);
        else
            ShowError(error);
    }
    ImGui::EndDisabled();

    PopEditSuppression();
    EndModal();
}

void ProjectHubLayer::DrawOpenModal()
{
    using namespace EditorUI;

    if (!BeginModal("Open Project", &m_ShowOpen, ImVec2(560, 0)))
        return;
    PushEditSuppression();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::TextWrapped("Pick the project folder (the one with project.simp in it).");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 2));

    PropertyLabel("Folder");
    const float browseW = 90.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - browseW - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    const bool submit = ImGui::InputText("##openpath", &m_OpenPath, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (Button("Browse...", ButtonKind::Secondary, ImVec2(browseW, 0)))
        PickFolder(OpenDialog, m_OpenPath.empty() ? DefaultLocation() : m_OpenPath);

    const bool found = !m_OpenPath.empty() && !SIMPEngine::Project::FindProjectFile(m_OpenPath).empty();
    if (!m_OpenPath.empty() && !found)
    {
        PropertyLabel("");
        ImGui::TextColored(Colors::Warning, "No project.simp in this folder.");
    }

    ImGui::Dummy(ImVec2(0, 4));
    const float bw = 100.0f;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw * 2.0f - gap);
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(bw, 0)))
    {
        m_ShowOpen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!found);
    if (Button("Open", ButtonKind::Primary, ImVec2(bw, 0)) || (submit && found))
    {
        m_ShowOpen = false;
        ImGui::CloseCurrentPopup();
        Open(m_OpenPath);
    }
    ImGui::EndDisabled();

    PopEditSuppression();
    EndModal();
}

void ProjectHubLayer::DrawErrorModal()
{
    using namespace EditorUI;

    if (!BeginModal("Something went wrong", &m_ShowError, ImVec2(480, 0)))
        return;
    ImGui::TextWrapped("%s", m_Error.c_str());
    ImGui::Dummy(ImVec2(0, 6));
    const float bw = 100.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - bw);
    if (Button("OK", ButtonKind::Primary, ImVec2(bw, 0)))
    {
        m_ShowError = false;
        ImGui::CloseCurrentPopup();
    }
    EndModal();
}

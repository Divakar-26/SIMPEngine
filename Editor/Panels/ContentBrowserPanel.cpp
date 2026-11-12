#include "ContentBrowserPanel.h"
#include "../EditorCommands.h"
#include <Engine/Scene/Loaders/TiledLoader.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Assets/AssetManager.h>
#include "UI/Visuals.h"
#include "UI/Widgets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    constexpr const char *ItemPayload = "CONTENT_BROWSER_ITEM";
    constexpr const char *TexturePayload = "TEXTURE_ASSET";

    bool IsInside(const std::filesystem::path &child, const std::filesystem::path &parent)
    {
        auto c = std::filesystem::weakly_canonical(child);
        auto p = std::filesystem::weakly_canonical(parent);
        auto mismatch = std::mismatch(p.begin(), p.end(), c.begin(), c.end());
        return mismatch.first == p.end();
    }
}

std::string ContentBrowserPanel::ToVirtualChild(const std::string &parent, const std::string &filename) const
{
    if (parent.ends_with('/'))
        return parent + filename;
    return parent + "/" + filename;
}

std::string ContentBrowserPanel::ParentOf(const std::string &vdir) const
{
    if (vdir == m_Root)
        return m_Root;
    size_t slash = vdir.find_last_of('/');
    if (slash == std::string::npos || slash < m_Root.size())
        return m_Root;
    return vdir.substr(0, slash);
}

std::vector<ContentBrowserPanel::Item> ContentBrowserPanel::ListItems(const std::string &vdir) const
{
    std::vector<Item> items;
    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
        return items;

    for (auto &e : SIMPEngine::FileSystem::List(*real))
    {
        std::string name = EditorUI::Utf8Path(e.path).filename().string();
        if (name.empty() || name[0] == '.')
            continue;
        items.push_back({name, ToVirtualChild(vdir, name), e.isDirectory});
    }

    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b)
              {
        if (a.IsDirectory != b.IsDirectory)
            return a.IsDirectory;
        return a.Name < b.Name; });
    return items;
}

void ContentBrowserPanel::Navigate(const std::string &vdir, bool record)
{
    if (vdir == m_CurrentDir)
        return;
    if (record)
    {
        m_Back.push_back(m_CurrentDir);
        m_Forward.clear();
    }
    m_CurrentDir = vdir;
    m_Selected.clear();
}

void ContentBrowserPanel::OnImGuiRender()
{
    using namespace EditorUI;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::Begin("FileSystem###ContentBrowser");
    ImGui::PopStyleVar();
    if (!visible)
    {
        ImGui::End();
        return;
    }

    DrawToolbar();

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
    if (ImGui::BeginTable("##cb", 2, flags, ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
    {
        ImGui::TableSetupColumn("tree", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("items", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::BeginChild("##tree", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
        DrawDirectory(m_Root);
        ImGui::EndChild();
        ImGui::PopStyleVar(2);

        ImGui::TableSetColumnIndex(1);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::BeginChild("##items", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
        DrawItems();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::EndTable();
    }
    ImGui::PopStyleVar();

    DrawFooter();

    ImGui::End();
}

void ContentBrowserPanel::DrawToolbar()
{
    using namespace EditorUI;
    BeginToolbar("##cb_toolbar");

    ImGui::BeginDisabled(m_Back.empty());
    if (IconButton("##back", ICON_LC_ARROW_LEFT, "Back"))
    {
        m_Forward.push_back(m_CurrentDir);
        std::string target = m_Back.back();
        m_Back.pop_back();
        Navigate(target, false);
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, 2);

    ImGui::BeginDisabled(m_Forward.empty());
    if (IconButton("##fwd", ICON_LC_ARROW_RIGHT, "Forward"))
    {
        m_Back.push_back(m_CurrentDir);
        std::string target = m_Forward.back();
        m_Forward.pop_back();
        Navigate(target, false);
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, 2);

    ImGui::BeginDisabled(m_CurrentDir == m_Root);
    if (IconButton("##up", ICON_LC_ARROW_UP, "Parent folder"))
        Navigate(ParentOf(m_CurrentDir));
    ImGui::EndDisabled();
    ImGui::SameLine(0, 4);
    VerticalSeparator();
    ImGui::SameLine(0, 4);

    float rightW = 200.0f + ImGui::GetFrameHeight() * 3.0f + 16.0f;
    float crumbsMax = ImGui::GetContentRegionAvail().x - rightW;

    ImGui::BeginChild("##crumbs", ImVec2(std::max(60.0f, crumbsMax), ImGui::GetFrameHeight()), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
    std::string target;
    if (Breadcrumbs(m_CurrentDir, target))
        Navigate(target);
    ImGui::EndChild();

    ImGui::SameLine(0, 4);
    SearchBox("##search", m_Search, sizeof(m_Search), "Search", 200.0f);
    ImGui::SameLine(0, 4);
    if (IconButton("##grid", ICON_LC_LAYOUT_GRID, "Grid view", !m_ListView))
        m_ListView = false;
    ImGui::SameLine(0, 2);
    if (IconButton("##list", ICON_LC_LIST, "List view", m_ListView))
        m_ListView = true;
    ImGui::SameLine(0, 2);
    if (IconButton("##newfolder", ICON_LC_FOLDER_PLUS, "New folder"))
        CreateFolder();

    EndToolbar();
}

void ContentBrowserPanel::DrawDirectory(const std::string &vdir)
{
    using namespace EditorUI;

    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
        return;

    std::string label = EditorUI::Utf8Path(*real).filename().string();
    if (label.empty() || vdir == m_Root)
        label = m_Root.substr(0, m_Root.find("://"));

    std::vector<Item> children;
    for (auto &item : ListItems(vdir))
        if (item.IsDirectory)
            children.push_back(item);

    TreeRowOptions opt;
    opt.Leaf = children.empty();
    opt.Selected = (vdir == m_CurrentDir);
    opt.DefaultOpen = (vdir == m_Root);

    bool isRoot = vdir == m_Root;
    const char *icon = ICON_LC_FOLDER;
    ImGuiStorage *storage = ImGui::GetStateStorage();
    if (!opt.Leaf && storage->GetBool(ImGui::GetID(vdir.c_str()), isRoot))
        icon = ICON_LC_FOLDER_OPEN;

    auto row = TreeRow(vdir.c_str(), icon, Colors::FolderTint, label.c_str(), opt);
    if (row.Clicked)
        Navigate(vdir);

    HandleDropInto(vdir);

    if (row.Open)
    {
        for (auto &child : children)
            DrawDirectory(child.VPath);
        TreeRowPop();
    }
}

void ContentBrowserPanel::HandleDropInto(const std::string &vdir)
{
    if (!ImGui::BeginDragDropTarget())
        return;

    const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(ItemPayload);
    if (!payload)
        payload = ImGui::AcceptDragDropPayload(TexturePayload);

    if (payload)
    {
        const char *srcVPath = static_cast<const char *>(payload->Data);
        auto srcReal = SIMPEngine::VFS::Resolve(srcVPath);
        auto dstReal = SIMPEngine::VFS::Resolve(vdir);

        if (srcReal && dstReal)
        {
            std::filesystem::path srcPath = *srcReal;
            std::filesystem::path dstPath = std::filesystem::path(*dstReal) / srcPath.filename();

            if (srcPath.parent_path() == std::filesystem::path(*dstReal) || IsInside(*dstReal, srcPath))
            {
                CORE_WARN("Cannot move {} into {}", srcPath.string(), dstReal->c_str());
            }
            else if (SIMPEngine::FileSystem::Exists(dstPath.string()))
            {
                CORE_WARN("{} already exists", dstPath.string());
            }
            else
            {
                try
                {
                    std::filesystem::rename(srcPath, dstPath);
                    CORE_INFO("Moved: {} -> {}", srcPath.string(), dstPath.string());
                }
                catch (const std::exception &e)
                {
                    CORE_ERROR("Move failed: {}", e.what());
                }
            }
        }
    }

    ImGui::EndDragDropTarget();
}

void ContentBrowserPanel::DrawItems()
{
    using namespace EditorUI;

    std::vector<Item> items;
    for (auto &item : ListItems(m_CurrentDir))
        if (ContainsInsensitive(item.Name.c_str(), m_Search))
            items.push_back(item);

    if (items.empty())
    {
        if (m_Search[0])
            EmptyState("No matches");
        else
            EmptyState("This folder is empty");
    }
    else if (m_ListView)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
        for (auto &item : items)
        {
            Visual v = FileVisual(item.Name, item.IsDirectory);
            SIMPEngine::Texture *thumb = item.IsDirectory ? nullptr : GetThumbnail(item.VPath);
            ImTextureID tex = thumb ? EditorUI::TextureId(thumb->GetID()) : ImTextureID{};
            ImVec2 texSize = thumb ? ImVec2(static_cast<float>(thumb->GetWidth()), static_cast<float>(thumb->GetHeight())) : ImVec2(0, 0);

            if (AssetRow(item.VPath.c_str(), tex, texSize, v.Icon, v.Color(), item.Name.c_str(), v.Type, m_Selected == item.VPath))
                m_Selected = item.VPath;
            bool open = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

            if (ImGui::BeginDragDropSource())
            {
                bool image = IsImageExtension(LowerExtension(item.Name));
                ImGui::SetDragDropPayload(image ? TexturePayload : ItemPayload, item.VPath.c_str(), item.VPath.size() + 1);
                IconText(v.Icon, v.Color(), item.Name.c_str());
                ImGui::EndDragDropSource();
            }
            if (item.IsDirectory)
                HandleDropInto(item.VPath);
            if (ImGui::BeginPopupContextItem())
            {
                DrawItemContextMenu(item);
                ImGui::EndPopup();
            }
            if (open && item.IsDirectory)
            {
                Navigate(item.VPath);
                break;
            }
            if (open && m_Context && (LowerExtension(item.Name) == ".lua" || LowerExtension(item.Name) == ".glsl"))
                m_Context->OpenScriptRequest = item.VPath;
        }
        ImGui::PopStyleVar();
    }
    else
    {
        float spacing = 4.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, static_cast<int>((avail + spacing) / (m_TileSize + spacing)));
        float tile = std::floor((avail - spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns));
        tile = std::min(tile, m_TileSize * 1.3f);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
        int index = 0;
        for (auto &item : items)
        {
            if (index % columns)
                ImGui::SameLine();
            ++index;

            Visual v = FileVisual(item.Name, item.IsDirectory);
            SIMPEngine::Texture *thumb = item.IsDirectory ? nullptr : GetThumbnail(item.VPath);
            ImTextureID tex = thumb ? EditorUI::TextureId(thumb->GetID()) : ImTextureID{};
            ImVec2 texSize = thumb ? ImVec2(static_cast<float>(thumb->GetWidth()), static_cast<float>(thumb->GetHeight())) : ImVec2(0, 0);

            auto r = AssetTile(item.VPath.c_str(), tex, texSize, v.Icon, v.Color(), item.Name.c_str(), m_Selected == item.VPath, tile);
            if (r.Clicked)
                m_Selected = item.VPath;
            if (r.Hovered && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                if (thumb)
                    ImGui::SetTooltip("%s\n%s  %d x %d", item.Name.c_str(), v.Type, thumb->GetWidth(), thumb->GetHeight());
                else
                    ImGui::SetTooltip("%s\n%s", item.Name.c_str(), v.Type);
            }

            if (ImGui::BeginDragDropSource())
            {
                bool image = IsImageExtension(LowerExtension(item.Name));
                ImGui::SetDragDropPayload(image ? TexturePayload : ItemPayload, item.VPath.c_str(), item.VPath.size() + 1);
                IconText(v.Icon, v.Color(), item.Name.c_str());
                ImGui::EndDragDropSource();
            }
            if (item.IsDirectory)
                HandleDropInto(item.VPath);
            if (ImGui::BeginPopupContextItem())
            {
                DrawItemContextMenu(item);
                ImGui::EndPopup();
            }
            if (r.DoubleClicked && item.IsDirectory)
            {
                Navigate(item.VPath);
                break;
            }
            if (r.DoubleClicked && m_Context && (LowerExtension(item.Name) == ".lua" || LowerExtension(item.Name) == ".glsl"))
                m_Context->OpenScriptRequest = item.VPath;
        }
        ImGui::PopStyleVar();
    }

    ImVec2 rest = ImGui::GetContentRegionAvail();
    if (rest.y > 4.0f)
    {
        ImGui::InvisibleButton("##bg", ImVec2(std::max(1.0f, rest.x), rest.y));
        if (ImGui::IsItemClicked())
            m_Selected.clear();
        HandleDropInto(m_CurrentDir);
        if (ImGui::BeginPopupContextItem("##bgctx"))
        {
            if (ImGui::MenuItem("New Folder"))
                CreateFolder();
            if (ImGui::MenuItem("Copy Folder Path"))
                ImGui::SetClipboardText(m_CurrentDir.c_str());
            ImGui::EndPopup();
        }
    }
}

void ContentBrowserPanel::DrawItemContextMenu(const Item &item)
{
    ImGui::TextDisabled("%s", item.Name.c_str());
    ImGui::Separator();

    if (item.IsDirectory && ImGui::MenuItem("Open"))
        Navigate(item.VPath);

    std::string ext = EditorUI::LowerExtension(EditorUI::Utf8Path(item.Name));
    if (!item.IsDirectory && m_Context && (ext == ".json" || ext == ".tmj") && SIMPEngine::TiledLoader::IsTiledMap(item.VPath))
    {
        bool canEdit = m_Context->IsEditing();
        if (ImGui::MenuItem("Import Tiled Map", nullptr, false, canEdit))
            EditorCommands::ImportTiledMap(*m_Context, item.VPath);
    }

    if (ImGui::MenuItem("Copy Path"))
        ImGui::SetClipboardText(item.VPath.c_str());
}

void ContentBrowserPanel::DrawFooter()
{
    using namespace EditorUI;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::AlignTextToFramePadding();
    EditorUI::TextEllipsis(m_Selected.empty() ? m_CurrentDir.c_str() : m_Selected.c_str());
    ImGui::PopStyleColor();

    if (!m_ListView)
    {
        float w = 110.0f;
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - w - 8.0f);
        ImGui::SetNextItemWidth(w);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 8.0f);
        ImGui::SliderFloat("##tile", &m_TileSize, 72.0f, 180.0f, "");
        ImGui::PopStyleVar(2);
        Tooltip("Tile size");
    }
}

void ContentBrowserPanel::CreateFolder()
{
    auto real = SIMPEngine::VFS::Resolve(m_CurrentDir);
    if (!real)
        return;

    std::filesystem::path base(*real);
    std::string name = "New Folder";
    for (int i = 2; i < 10000 && SIMPEngine::FileSystem::Exists((base / name).string()); ++i)
        name = "New Folder " + std::to_string(i);

    if (SIMPEngine::FileSystem::CreateDirectories((base / name).string()))
        m_Selected = ToVirtualChild(m_CurrentDir, name);
}

SIMPEngine::Texture *ContentBrowserPanel::GetThumbnail(const std::string &vpath)
{
    if (!EditorUI::IsImageExtension(EditorUI::LowerExtension(vpath)))
        return nullptr;

    auto real = SIMPEngine::VFS::Resolve(vpath);
    if (!real)
        return nullptr;

    auto [it, inserted] = m_TextureCache.try_emplace(*real);
    if (inserted && !it->second.LoadFromFile(real->c_str()))
        CORE_WARN("Could not load thumbnail {}", *real);

    return it->second.GetID() ? &it->second : nullptr;
}

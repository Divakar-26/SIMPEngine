#include "AssetPickerPanel.h"
#include "UI/Visuals.h"
#include "UI/Widgets.h"
#include <Engine/Core/Log.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
    struct PickerItem
    {
        std::string Name;
        std::string VPath;
        bool IsDirectory = false;
    };

    std::vector<PickerItem> ListSorted(const std::string &real, const std::string &vdir)
    {
        std::vector<PickerItem> items;
        for (auto &e : SIMPEngine::FileSystem::List(real))
        {
            std::string name = std::filesystem::path(e.path).filename().string();
            if (name.empty() || name[0] == '.')
                continue;
            std::string child = vdir.ends_with('/') ? vdir + name : vdir + "/" + name;
            items.push_back({name, child, e.isDirectory});
        }
        std::sort(items.begin(), items.end(), [](const PickerItem &a, const PickerItem &b)
                  {
            if (a.IsDirectory != b.IsDirectory)
                return a.IsDirectory;
            return a.Name < b.Name; });
        return items;
    }
}

AssetPickerPanel::AssetPickerPanel(const std::string &rootVirtual, SIMPEngine::AssetManager *am)
    : m_AM(am), m_Root(rootVirtual), m_CurrentDir(rootVirtual)
{
}

bool AssetPickerPanel::Accepts(const std::string &vpath) const
{
    if (m_Extensions.empty())
        return true;
    std::string ext = EditorUI::LowerExtension(vpath);
    return std::find(m_Extensions.begin(), m_Extensions.end(), ext) != m_Extensions.end();
}

void AssetPickerPanel::Open(const std::string &title,
                            const std::string &startDirVirtual,
                            ConfirmCallback onConfirm,
                            std::vector<std::string> extensions)
{
    m_Extensions = std::move(extensions);
    m_Title = title;
    m_OnConfirm = std::move(onConfirm);
    m_SelectedVPath.clear();
    m_Search[0] = '\0';
    m_CurrentDir = startDirVirtual.empty() ? m_Root : startDirVirtual;
    m_Open = true;
}

void AssetPickerPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Open)
        return;

    if (!BeginModal(m_Title.c_str(), &m_Open, ImVec2(900, 560)))
        return;

    auto rootReal = SIMPEngine::VFS::Resolve(m_Root);
    if (!rootReal)
    {
        ImGui::TextColored(Colors::Error, "Failed to resolve root: %s", m_Root.c_str());
        EndModal();
        return;
    }

    std::string target;
    ImGui::BeginChild("##crumbs", ImVec2(ImGui::GetContentRegionAvail().x - 260.0f, ImGui::GetFrameHeight()), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
    if (Breadcrumbs(m_CurrentDir, target))
        m_CurrentDir = target;
    ImGui::EndChild();
    ImGui::SameLine();
    SearchBox("##search", m_Search, sizeof(m_Search), "Search", -FLT_MIN);

    float footerH = ImGui::GetFrameHeight() + 14.0f;
    float bodyH = ImGui::GetContentRegionAvail().y - footerH;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Header);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 6));
    ImGui::BeginChild("##tree", ImVec2(200, bodyH), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    DrawDirectory(m_Root);
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("##grid", ImVec2(ImGui::GetContentRegionAvail().x - 236.0f, bodyH), ImGuiChildFlags_None);
    DrawFiles(m_CurrentDir);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::BeginChild("##preview", ImVec2(0, bodyH), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    DrawPreview();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, 4));

    const bool valid = !m_SelectedVPath.empty();
    float bw = 110.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(valid ? m_SelectedVPath.c_str() : "Select a file, or double-click to pick it");
    ImGui::PopStyleColor();

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - bw * 2.0f - spacing);
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(bw, 0)))
    {
        Close();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid);
    if (Button("Select", ButtonKind::Primary, ImVec2(bw, 0)))
    {
        Confirm();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    if (!m_Open)
        ImGui::CloseCurrentPopup();

    EndModal();
}

void AssetPickerPanel::Confirm()
{
    if (m_OnConfirm && !m_SelectedVPath.empty())
        m_OnConfirm(m_SelectedVPath);
    Close();
}

void AssetPickerPanel::Close()
{
    m_Open = false;
    m_SelectedVPath.clear();
    m_OnConfirm = nullptr;
}

std::string AssetPickerPanel::ToVirtualChild(const std::string &parent, const std::string &filename) const
{
    if (parent.ends_with('/'))
        return parent + filename;
    return parent + "/" + filename;
}

SIMPEngine::Texture *AssetPickerPanel::GetThumbnailFor(const std::string &vpath)
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

void AssetPickerPanel::DrawDirectory(const std::string &vdir)
{
    using namespace EditorUI;

    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
        return;

    bool isRoot = vdir == m_Root;
    std::string label = isRoot ? m_Root.substr(0, m_Root.find("://")) : std::filesystem::path(*real).filename().string();

    std::vector<PickerItem> dirs;
    for (auto &item : ListSorted(*real, vdir))
        if (item.IsDirectory)
            dirs.push_back(item);

    TreeRowOptions opt;
    opt.Leaf = dirs.empty();
    opt.Selected = vdir == m_CurrentDir;
    opt.DefaultOpen = isRoot;

    auto row = TreeRow(vdir.c_str(), ICON_LC_FOLDER, Colors::FolderTint, label.c_str(), opt);
    if (row.Clicked)
        m_CurrentDir = vdir;

    if (row.Open)
    {
        for (auto &d : dirs)
            DrawDirectory(d.VPath);
        TreeRowPop();
    }
}

void AssetPickerPanel::DrawFiles(const std::string &vdir)
{
    using namespace EditorUI;

    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
    {
        EmptyState("Folder not found");
        return;
    }

    std::vector<PickerItem> items;
    for (auto &item : ListSorted(*real, vdir))
        if (ContainsInsensitive(item.Name.c_str(), m_Search) && (item.IsDirectory || Accepts(item.Name)))
            items.push_back(item);

    if (items.empty())
    {
        EmptyState("Nothing here", m_Search[0] ? "No files match your search." : nullptr);
        return;
    }

    float spacing = 10.0f;
    float tileW = 108.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, (int)((avail + spacing) / (tileW + spacing)));
    tileW = std::floor((avail - spacing * (columns - 1)) / columns);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
    int index = 0;
    std::string navigateTo;
    for (auto &item : items)
    {
        if (index++ % columns)
            ImGui::SameLine();

        Visual v = FileVisual(item.Name, item.IsDirectory);
        SIMPEngine::Texture *thumb = item.IsDirectory ? nullptr : GetThumbnailFor(item.VPath);
        ImTextureID tex = thumb ? (ImTextureID)(intptr_t)thumb->GetID() : (ImTextureID)0;
        ImVec2 texSize = thumb ? ImVec2((float)thumb->GetWidth(), (float)thumb->GetHeight()) : ImVec2(0, 0);

        auto r = AssetTile(item.VPath.c_str(), tex, texSize, v.Icon, v.Color(), item.Name.c_str(), m_SelectedVPath == item.VPath, tileW);
        if (r.Clicked && !item.IsDirectory)
            m_SelectedVPath = item.VPath;
        if (r.DoubleClicked)
        {
            if (item.IsDirectory)
                navigateTo = item.VPath;
            else
            {
                m_SelectedVPath = item.VPath;
                Confirm();
            }
        }
    }
    ImGui::PopStyleVar();

    if (!navigateTo.empty())
        m_CurrentDir = navigateTo;
}

void AssetPickerPanel::DrawPreview()
{
    using namespace EditorUI;

    if (m_SelectedVPath.empty())
    {
        EmptyState("No file selected");
        return;
    }

    std::filesystem::path p(m_SelectedVPath);
    Visual v = FileVisual(p.filename(), false);
    float w = ImGui::GetContentRegionAvail().x;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 max(pos.x + w, pos.y + w);
    ImDrawList *dl = ImGui::GetWindowDrawList();

    SIMPEngine::Texture *tex = GetThumbnailFor(m_SelectedVPath);
    if (tex)
    {
        dl->PushClipRect(pos, max, true);
        DrawCheckerboard(dl, pos, max, 8.0f);
        dl->PopClipRect();
        DrawImageFit(dl, (ImTextureID)(intptr_t)tex->GetID(), ImVec2((float)tex->GetWidth(), (float)tex->GetHeight()), ImVec2(pos.x + 6, pos.y + 6), ImVec2(max.x - 6, max.y - 6));
    }
    else
    {
        dl->AddRectFilled(pos, max, U32(Colors::Frame), Metrics::FrameRounding);
        float size = std::floor(w * 0.30f);
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, v.Icon);
        dl->AddText(ImGui::GetFont(), size, ImVec2(std::floor(pos.x + (w - ts.x) * 0.5f), std::floor(pos.y + (w - ts.y) * 0.5f)), U32(v.Color()), v.Icon);
    }
    ImGui::Dummy(ImVec2(w, w));
    ImGui::Dummy(ImVec2(0, 4));

    ImGui::PushFont(GetFont(FontRole::Medium), 0.0f);
    ImGui::TextWrapped("%s", p.filename().string().c_str());
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    if (tex)
        ImGui::Text("%s  ·  %d x %d", v.Type, tex->GetWidth(), tex->GetHeight());
    else
        ImGui::TextUnformatted(v.Type);
    ImGui::PopStyleColor();
}

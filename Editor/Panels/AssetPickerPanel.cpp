#include "AssetPickerPanel.h"
#include "EditorUtility.h"
#include <Engine/Core/Log.h>
#include <algorithm>
#include <cctype>

AssetPickerPanel::AssetPickerPanel(const std::string &rootVirtual, SIMPEngine::AssetManager *am)
    : m_AM(am), m_Root(rootVirtual), m_CurrentDir(rootVirtual)
{
    m_FolderIcon.LoadFromMemory(folder_png, folder_png_len);
}

void AssetPickerPanel::Open(const std::string &title,
                            const std::string &startDirVirtual,
                            ConfirmCallback onConfirm)
{
    m_Title = title;
    m_OnConfirm = std::move(onConfirm);
    m_SelectedVPath.clear();
    m_CurrentDir = startDirVirtual.empty() ? m_Root : startDirVirtual;
    m_Open = true;
}

void AssetPickerPanel::OnImGuiRender()
{
    if (!m_Open)
        return;

    ImGui::OpenPopup(m_Title.c_str());

    if (ImGui::BeginPopupModal(m_Title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto rootReal = SIMPEngine::VFS::Resolve(m_Root);
        if (!rootReal)
        {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to resolve root: %s", m_Root.c_str());
            ImGui::EndPopup();
            return;
        }

        ImGui::TextUnformatted("Choose an asset");
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::BeginChild("##tree", ImVec2(220, 360), true);
        DrawDirectory(m_Root);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##grid", ImVec2(420, 360), true);
        DrawFiles(m_CurrentDir);
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 4));
        DrawPreview();
        ImGui::Dummy(ImVec2(0, 4));

        const bool valid = !m_SelectedVPath.empty();

        if (!valid)
            ImGui::BeginDisabled();

        if (ImGui::Button("OK"))
        {
            if (m_OnConfirm)
                m_OnConfirm(m_SelectedVPath);

            Close();
            ImGui::CloseCurrentPopup();
        }

        if (!valid)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            Close();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
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

bool AssetPickerPanel::IsImageFile(const std::filesystem::path &path) const
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".webp";
}

SIMPEngine::Texture &AssetPickerPanel::GetThumbnailFor(const std::string &vpath)
{
    auto real = SIMPEngine::VFS::Resolve(vpath);
    if (!real)
        return m_FolderIcon;

    auto it = m_TextureCache.find(*real);
    if (it != m_TextureCache.end())
        return it->second;

    m_TextureCache[*real] = SIMPEngine::Texture();

    if (m_TextureCache[*real].LoadFromFile(real->c_str()))
        return m_TextureCache[*real];

    m_TextureCache.erase(*real);
    return m_FolderIcon;
}

void AssetPickerPanel::DrawDirectory(const std::string &vdir)
{
    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
        return;

    std::string label = std::filesystem::path(*real).filename().string();
    if (label.empty())
        label = vdir;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_DefaultOpen;

    bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked())
        m_CurrentDir = vdir;

    if (opened)
    {
        auto entries = SIMPEngine::FileSystem::List(*real);
        for (auto &e : entries)
        {
            if (!e.isDirectory)
                continue;

            std::string filename = std::filesystem::path(e.path).filename().string();
            std::string childV = ToVirtualChild(vdir, filename);
            DrawDirectory(childV);
        }

        ImGui::TreePop();
    }
}

void AssetPickerPanel::DrawFiles(const std::string &vdir)
{
    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
    {
        ImGui::TextUnformatted("No directory");
        return;
    }

    auto entries = SIMPEngine::FileSystem::List(*real);

    float cell = 96.0f;
    int columns = (int)(ImGui::GetContentRegionAvail().x / cell);
    if (columns < 1)
        columns = 1;

    ImGui::Columns(columns, nullptr, false);

    for (auto &e : entries)
    {
        std::string filename = std::filesystem::path(e.path).filename().string();
        std::string childV = ToVirtualChild(vdir, filename);
        DrawEntry(childV, e.isDirectory);
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
}

void AssetPickerPanel::DrawEntry(const std::string &vpath, bool isDir)
{
    auto real = SIMPEngine::VFS::Resolve(vpath);
    if (!real)
        return;

    std::string name = std::filesystem::path(*real).filename().string();

    ImGui::PushID(vpath.c_str());
    ImGui::BeginGroup();

    if (isDir)
    {
        CORE_INFO("Picker folder: {}x{}",
                  m_FolderIcon.GetWidth(),
                  m_FolderIcon.GetHeight());

        if (m_FolderIcon.GetID())
        {
            if (EditorUI::DrawThumbnailButton("##folder", m_FolderIcon, 72.0f, m_SelectedVPath == vpath))
                m_CurrentDir = vpath;
        }
    }
    else if (IsImageFile(*real))
    {
        SIMPEngine::Texture &thumb = GetThumbnailFor(vpath);
        if (thumb.GetID())
        {
            if (EditorUI::DrawThumbnailButton("##img", thumb, 72.0f, m_SelectedVPath == vpath))
                m_SelectedVPath = vpath;
        }
    }
    else
    {
        if (ImGui::Button("FILE", ImVec2(72, 72)))
            m_SelectedVPath = vpath;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && isDir)
        m_CurrentDir = vpath;

    if (ImGui::IsItemClicked() && !isDir)
        m_SelectedVPath = vpath;

    ImGui::TextWrapped("%s", name.c_str());

    if (m_SelectedVPath == vpath)
        ImGui::TextUnformatted("Selected");

    ImGui::EndGroup();
    ImGui::PopID();
}

void AssetPickerPanel::DrawPreview()
{
    if (m_SelectedVPath.empty())
    {
        ImGui::TextUnformatted("Selected: none");
        return;
    }

    ImGui::Text("Selected: %s", m_SelectedVPath.c_str());

    auto real = SIMPEngine::VFS::Resolve(m_SelectedVPath);
    if (!real)
        return;

    if (IsImageFile(*real))
    {
        auto &tex = GetThumbnailFor(m_SelectedVPath);
        if (tex.GetID())
        {
            EditorUI::DrawThumbnailImage(tex, 160.0f);
        }
    }
}
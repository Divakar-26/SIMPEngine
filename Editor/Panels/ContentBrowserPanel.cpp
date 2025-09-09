// ContentBrowserPanel.cpp
#include "ContentBrowserPanel.h"
#include "Core/VFS.h"
#include "Core/FileSystem.h"
#include "Core/Log.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"

std::string ContentBrowserPanel::ToVirtualChild(const std::string &parent, const std::string &filename)
{
    if (parent.ends_with('/'))
        return parent + filename;
    return parent + "/" + filename;
}

void ContentBrowserPanel::OnImGuiRender()
{
    ImGui::Begin("Content Browser");

    // left tree
    ImGui::BeginChild("##tree", ImVec2(220, 0), true);
    DrawDirectory(m_Root);
    ImGui::EndChild();

    ImGui::SameLine();

    // right grid
    ImGui::BeginChild("##grid", ImVec2(0, 0), true);
    auto real = SIMPEngine::VFS::Resolve(m_CurrentDir);
    if (real)
    {
        auto entries = SIMPEngine::FileSystem::List(*real);
        float cell = 96.0f;
        int columns = (int)(ImGui::GetContentRegionAvail().x / cell);
        if (columns < 1)
            columns = 1;
        ImGui::Columns(columns, 0, false);
        for (auto &e : entries)
        {
            std::string filename = std::filesystem::u8path(e.path).filename().string();
            DrawEntry(ToVirtualChild(m_CurrentDir, filename), e.isDirectory);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
    ImGui::EndChild();

    ImGui::End();
}

void ContentBrowserPanel::DrawDirectory(const std::string &vdir)
{
    auto real = SIMPEngine::VFS::Resolve(vdir);
    if (!real)
        return;
    auto label = std::filesystem::u8path(*real).filename().string();
    if (label.empty())
        label = vdir;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
    bool opened = ImGui::TreeNodeEx(label.c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        m_CurrentDir = vdir;
    }

    if (opened)
    {
        for (auto &e : SIMPEngine::FileSystem::List(*real))
        {
            if (e.isDirectory)
            {
                std::string childV = ToVirtualChild(vdir, std::filesystem::u8path(e.path).filename().string());
                DrawDirectory(childV);
            }
        }
        ImGui::TreePop();
    }
}

void ContentBrowserPanel::DrawEntry(const std::string &vpath, bool isDir)
{
    auto real = SIMPEngine::VFS::Resolve(vpath);
    if (!real)
        return;

    std::string name = std::filesystem::u8path(*real).filename().string();

    ImGui::PushID(vpath.c_str());
    ImGui::BeginGroup();

    // Always use folder icon for now
    SIMPEngine::Texture &icon = GetIconFor(vpath, isDir);
    // SIMPEngine::Texture &icon = m_TextureCache["/home/divakar/Desktop/SDLGameEngine/build/assets/folder.png"];

    if (icon.GetSDLTexture())
    {
        // Image button with transparent background
        if (ImGui::ImageButton(
                vpath.c_str(),
                (ImTextureID)icon.GetSDLTexture(),
                ImVec2(72, 72),
                ImVec2(0, 0), ImVec2(1, 1),
                ImVec4(0, 0, 0, 0),
                ImVec4(1, 1, 1, 1)))
        {
            // Click logic
            if (isDir)
            {
                m_CurrentDir = vpath;
                CORE_INFO("Changed directory to: {}", vpath);
            }
            else
            {
                CORE_INFO("Clicked file: {}", vpath);
            }
        }

        // Drag & Drop
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            if (!isDir)
            {
                SIMPEngine::AssetType type = SIMPEngine::AssetType::Unknown;
                auto ext = std::filesystem::u8path(*real).extension().string();
                if (ext == ".png" || ext == ".jpg")
                    type = SIMPEngine::AssetType::Texture2D;
                if (ext == ".scene")
                    type = SIMPEngine::AssetType::Scene;

                auto handle = m_AM->ImportIfNeeded(vpath, type);
                ImGui::SetDragDropPayload("ASSET_HANDLE", &handle, sizeof(handle));
            }
            ImGui::TextUnformatted(name.c_str());
            ImGui::EndDragDropSource();
        }
    }
    else
    {
        ImGui::Button("📁", ImVec2(72, 72));
    }

    ImGui::TextWrapped("%s", name.c_str());

    ImGui::EndGroup();
    ImGui::PopID();
}

SIMPEngine::Texture *LoadTextureFromVirtual(const std::string &vpath)
{
    // resolve virtual path to real OS path
    // auto real = SIMPEngine::VFS::Resolve(vpath);
    // if (!real)
    // {
    //     SIMPEngine::Log::Error("VFS: Cannot resolve path %s", vpath.c_str());
    //     return nullptr;
    // }

    // // delegate to asset manager
    // SIMPEngine::AssetHandle handle = m_AssetManager->ImportIfNeeded(vpath, SIMPEngine::AssetType::Texture2D);
    // if (handle == SIMPEngine::AssetHandle::Invalid)
    // {
    //     SIMPEngine::Log::Error("Failed to import texture %s", vpath.c_str());
    //     return nullptr;
    // }

    // return m_AssetManager->Get<SIMPEngine::Texture2D>(handle);
}

SIMPEngine::Texture &ContentBrowserPanel::GetIconFor(const std::string &vpath, bool isDir)
{
    if (isDir)
    {
        return m_FolderIcon;
    }

    auto real = SIMPEngine::VFS::Resolve(vpath);
    if (!real)
    {
        CORE_ERROR("PATH COULD NOT BE RESOLVED");
        return m_FileIcon;
    }

    std::string ext = std::filesystem::u8path(*real).extension().string();
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
    {
        CORE_ERROR("Trying to load: {}", *real);
        auto it = m_TextureCache.find(*real);
        if (it != m_TextureCache.end())
        {
            CORE_ERROR("Found in cache");
            return it->second;
        }

        CORE_ERROR("Loading new texture");
        m_TextureCache[*real] = SIMPEngine::Texture();
        if (m_TextureCache[*real].LoadFromFile(SIMPEngine::Renderer::GetSDLRenderer(), real->c_str()))
        {
            CORE_ERROR("Successfully loaded texture");
            return m_TextureCache[*real];
        }
        else
        {
            CORE_ERROR("COULD NOT LOAD TEXTURE: {}", *real);
            m_TextureCache.erase(*real);
            return m_FileIcon;
        }
    }

    return m_FileIcon;
}

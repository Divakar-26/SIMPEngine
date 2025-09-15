// ContentBrowserPanel.cpp
#include "ContentBrowserPanel.h"
#include "Core/VFS.h"
#include "Core/FileSystem.h"
#include "Core/Log.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "../icons.h"

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

    // make this droppable
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char *srcVPath = (const char *)payload->Data;
            auto srcReal = SIMPEngine::VFS::Resolve(srcVPath);
            auto dstReal = SIMPEngine::VFS::Resolve(m_CurrentDir);

            if (srcReal && dstReal)
            {
                std::filesystem::path srcPath = *srcReal;
                std::filesystem::path dstPath = *dstReal / srcPath.filename();

                try
                {
                    std::filesystem::rename(srcPath, dstPath);
                    CORE_INFO("Moved to current directory: {} -> {}", srcPath.string(), dstPath.string());
                }
                catch (std::exception &e)
                {
                    CORE_ERROR("Move failed: {}", e.what());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

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

    std::string label = std::string(ICON_CAMERA) + std::filesystem::u8path(*real).filename().string();
    if (label.empty())
        label = vdir;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

    // Pass c_str() to ImGui
    bool opened = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        m_CurrentDir = vdir;
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char *srcVPath = (const char *)payload->Data;
            auto srcReal = SIMPEngine::VFS::Resolve(srcVPath);
            auto dstReal = SIMPEngine::VFS::Resolve(vdir);

            if (srcReal && dstReal)
            {
                std::filesystem::path srcPath = *srcReal;
                std::filesystem::path dstPath = *dstReal / srcPath.filename();

                try
                {
                    // This works for both files and folders
                    std::filesystem::rename(srcPath, dstPath);
                    CORE_INFO("Moved: {} -> {}", srcPath.string(), dstPath.string());
                }
                catch (std::exception &e)
                {
                    CORE_ERROR("Move failed: {}", e.what());
                }
            }
        }

        ImGui::EndDragDropTarget();
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

    SIMPEngine::Texture &icon = GetIconFor(vpath, isDir);

    // if (icon.GetSDLTexture())
    // {
    //     if (ImGui::ImageButton(
    //             vpath.c_str(),
    //             (ImTextureID)icon.GetSDLTexture(),
    //             ImVec2(72, 72),
    //             ImVec2(0, 0), ImVec2(1, 1),
    //             ImVec4(0, 0, 0, 0),
    //             ImVec4(1, 1, 1, 1)))
    //     {
    //         if (isDir)
    //         {
    //             m_CurrentDir = vpath;
    //             CORE_INFO("Changed directory to: {}", vpath);
    //         }
    //         else
    //         {
    //             CORE_INFO("Clicked file: {}", vpath);
    //         }
    //     }

    //     if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    //     {
    //         // Only use CONTENT_BROWSER_ITEM payload for both files and folders
    //         ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", vpath.c_str(), vpath.size() + 1);

    //         ImGui::TextUnformatted(name.c_str());
    //         ImGui::EndDragDropSource();
    //     }

    //     if (isDir && ImGui::BeginDragDropTarget())
    //     {
    //         if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
    //         {
    //             const char *srcVPath = (const char *)payload->Data;
    //             auto srcReal = SIMPEngine::VFS::Resolve(srcVPath);
    //             auto dstReal = SIMPEngine::VFS::Resolve(vpath);

    //             if (srcReal && dstReal)
    //             {
    //                 std::filesystem::path srcPath = *srcReal;
    //                 std::filesystem::path dstPath = *dstReal / srcPath.filename();

    //                 try
    //                 {
    //                     std::filesystem::rename(srcPath, dstPath);
    //                     CORE_INFO("Moved to folder: {} -> {}", srcPath.string(), dstPath.string());
    //                 }
    //                 catch (std::exception &e)
    //                 {
    //                     CORE_ERROR("Move failed: {}", e.what());
    //                 }
    //             }
    //         }
    //         ImGui::EndDragDropTarget();
    //     }
    // }
    // else
    // {
    //     ImGui::Button("📁", ImVec2(72, 72));
    // }

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

// TODO -> Make this load scaled down image, not the full one.
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
        // CORE_INFO("Trying to load: {}", *real);
        auto it = m_TextureCache.find(*real);
        if (it != m_TextureCache.end())
        {
            // CORE_ERROR("Found in cache");
            return it->second;
        }

        // CORE_INFO("Loading new texture");
        m_TextureCache[*real] = SIMPEngine::Texture();
        if (m_TextureCache[*real].LoadFromFile(real->c_str()))
        {
            // CORE_INFO("Successfully loaded texture");
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

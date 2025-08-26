// ContentBrowserPanel.h
#pragma once
#include <string>
#include "Assets/AssetManager.h"
#include <imgui.h>
#include <string>
#include <filesystem>
#include "Rendering/Texture.h"
#include "Rendering/Renderer.h"
#include "Core/VFS.h"

class ContentBrowserPanel
{
public:
    ContentBrowserPanel(const std::string &rootVirtual, SIMPEngine::AssetManager *am)
        : m_AM(am), m_Root(rootVirtual), m_CurrentDir(rootVirtual)
    {
        std::string s = "assets://folder.png";
        auto it = SIMPEngine::VFS::Resolve(s);
        if (it.has_value())
        {
            m_FolderIcon.LoadFromFile(
                SIMPEngine::Renderer::GetSDLRenderer(),
                it->c_str() // ✅ unwrap optional
            );
        }
        else
        {
            // handle error gracefully
            // CORE_ERROR("Failed to resolve {}", s);
        }
    }
    void OnImGuiRender();

private:
    SIMPEngine::AssetManager *m_AM;
    std::string m_Root;
    std::string m_CurrentDir;

    void DrawDirectory(const std::string &vdir);
    void DrawEntry(const std::string &vpath, bool isDir);

    std::string ToVirtualChild(const std::string &parentV, const std::string &filename);

    SIMPEngine::Texture m_FolderIcon;
    SIMPEngine::Texture m_FileIcon;
};

// ContentBrowserPanel.h
#pragma once
#include <string>
#include <Engine/Assets/AssetManager.h>
#include <imgui.h>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Core/VFS.h>
#include "icons/folder_icon.h"

class ContentBrowserPanel
{
public:
    ContentBrowserPanel(const std::string &rootVirtual, SIMPEngine::AssetManager *am)
        : m_AM(am), m_Root(rootVirtual), m_CurrentDir(rootVirtual), m_TextureCache()
    {
        m_FolderIcon.LoadFromMemory(folder_png, folder_png_len);

        std::string s2 = "assets://file.png";
        auto it2 = SIMPEngine::VFS::Resolve(s2);
        if (it2)
        {
            m_FileIcon.LoadFromFile(it2->c_str());
        }
    }
    void OnImGuiRender();

    SIMPEngine::Texture &GetIconFor(const std::string &vpath, bool isDir);
    void LoadTexture(const std::string &vpath, bool isDir);

private:
    SIMPEngine::AssetManager *m_AM;
    std::string m_Root;
    std::string m_CurrentDir;

    void DrawDirectory(const std::string &vdir);
    void DrawEntry(const std::string &vpath, bool isDir);

    std::string ToVirtualChild(const std::string &parentV, const std::string &filename);

    SIMPEngine::Texture m_FolderIcon;
    SIMPEngine::Texture m_FileIcon;
    SIMPEngine::Texture m_oneImage;

    std::unordered_map<std::string, SIMPEngine::Texture> m_TextureCache;
    std::unordered_map<std::string, SIMPEngine::Texture> m_Textures;
};

#pragma once
#include <string>
#include <Engine/Assets/AssetManager.h>
#include <imgui.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Core/VFS.h>
#include <EditorPanel.h>

class ContentBrowserPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###ContentBrowser";

    ContentBrowserPanel(const std::string &rootVirtual, SIMPEngine::AssetManager *am)
        : m_AM(am), m_Root(rootVirtual), m_CurrentDir(rootVirtual)
    {
    }
    void OnImGuiRender() override;

    SIMPEngine::Texture *GetThumbnail(const std::string &vpath);

private:
    struct Item
    {
        std::string Name;
        std::string VPath;
        bool IsDirectory = false;
    };

    void DrawToolbar();
    void DrawDirectory(const std::string &vdir);
    void DrawItems();
    void DrawFooter();
    void DrawItemContextMenu(const Item &item);
    void HandleDropInto(const std::string &vdir);
    void Navigate(const std::string &vdir, bool record = true);
    void CreateFolder();

    std::vector<Item> ListItems(const std::string &vdir) const;
    std::string ToVirtualChild(const std::string &parentV, const std::string &filename) const;
    std::string ParentOf(const std::string &vdir) const;

    SIMPEngine::AssetManager *m_AM;
    std::string m_Root;
    std::string m_CurrentDir;
    std::string m_Selected;

    std::vector<std::string> m_Back;
    std::vector<std::string> m_Forward;

    char m_Search[128] = "";
    bool m_ListView = false;
    float m_TileSize = 84.0f;

    std::unordered_map<std::string, SIMPEngine::Texture> m_TextureCache;
};

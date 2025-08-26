// ContentBrowserPanel.h
#pragma once
#include <string>
#include "Assets/AssetManager.h"
#include <imgui.h>  // Missing ImGui includes
#include <string>
#include <filesystem>


class ContentBrowserPanel {
public:
    ContentBrowserPanel( const std::string& rootVirtual, SIMPEngine::AssetManager* am)
        : m_AM(am), m_Root(rootVirtual), m_CurrentDir(rootVirtual) {}
    void OnImGuiRender();

private:
    SIMPEngine::AssetManager * m_AM;
    std::string m_Root;
    std::string m_CurrentDir;

    void DrawDirectory(const std::string& vdir);
    void DrawEntry(const std::string& vpath, bool isDir);

    std::string ToVirtualChild(const std::string& parentV, const std::string& filename);
};


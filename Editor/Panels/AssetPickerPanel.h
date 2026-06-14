#pragma once

#include <EditorPanel.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/Texture.h>
#include <imgui.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "icons/folder_icon.h"

class AssetPickerPanel : public EditorPanel
{
public:
    using ConfirmCallback = std::function<void(const std::string& vpath)>;

    AssetPickerPanel(const std::string& rootVirtual, SIMPEngine::AssetManager* am);

    void OnImGuiRender() override;

    void Open(const std::string& title,
              const std::string& startDirVirtual,
              ConfirmCallback onConfirm);

    bool IsOpen() const { return m_Open; }

private:
    std::string ToVirtualChild(const std::string& parent, const std::string& filename) const;
    void DrawDirectory(const std::string& vdir);
    void DrawFiles(const std::string& vdir);
    void DrawEntry(const std::string& vpath, bool isDir);
    void DrawPreview();
    bool IsImageFile(const std::filesystem::path& path) const;
    SIMPEngine::Texture& GetThumbnailFor(const std::string& vpath);
    void Close();

private:
    SIMPEngine::AssetManager* m_AM = nullptr;

    std::string m_Root;
    std::string m_CurrentDir;
    std::string m_Title = "Asset Picker";

    bool m_Open = false;
    std::string m_SelectedVPath;
    ConfirmCallback m_OnConfirm;

    SIMPEngine::Texture m_FolderIcon;
    std::unordered_map<std::string, SIMPEngine::Texture> m_TextureCache;
};
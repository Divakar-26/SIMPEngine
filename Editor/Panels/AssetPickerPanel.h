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
#include <vector>

class AssetPickerPanel : public EditorPanel
{
public:
    using ConfirmCallback = std::function<void(const std::string& vpath)>;

    AssetPickerPanel(const std::string& rootVirtual, SIMPEngine::AssetManager* am);

    void OnImGuiRender() override;

    void Open(const std::string& title,
              const std::string& startDirVirtual,
              ConfirmCallback onConfirm,
              std::vector<std::string> extensions = {});

    bool IsOpen() const { return m_Open; }
    bool Accepts(const std::string& vpath) const;

private:
    std::string ToVirtualChild(const std::string& parent, const std::string& filename) const;
    void DrawDirectory(const std::string& vdir);
    void DrawFiles(const std::string& vdir);
    void DrawPreview();
    SIMPEngine::Texture* GetThumbnailFor(const std::string& vpath);
    void Confirm();
    void Close();

private:
    SIMPEngine::AssetManager* m_AM = nullptr;

    std::string m_Root;
    std::string m_CurrentDir;
    std::string m_Title = "Asset Picker";

    bool m_Open = false;
    std::string m_SelectedVPath;
    ConfirmCallback m_OnConfirm;
    char m_Search[128] = "";
    std::vector<std::string> m_Extensions;

    std::unordered_map<std::string, SIMPEngine::Texture> m_TextureCache;
};

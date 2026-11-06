#pragma once

#include <EditorPanel.h>
#include <Engine/Particles/ParticleEffect.h>
#include <Engine/Rendering/Framebuffer.h>

#include <imgui.h>
#include <glm/glm.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class ParticlePickerPanel : public EditorPanel
{
public:
    using ConfirmCallback = std::function<void(const SIMPEngine::ParticleEffect &effect)>;

    void Open(const std::string &title, ConfirmCallback onConfirm, const std::string &current = {});
    bool IsOpen() const { return m_Open; }

    void OnImGuiRender() override;

    static std::string ProjectFolder() { return "assets://particles"; }

private:
    struct Entry
    {
        std::string Name;
        std::string Category;
        std::string Description;
        std::string Source;
        SIMPEngine::ParticleEffect Effect;
        float ViewHeight = 260.0f;
        glm::vec2 Anchor{0.0f};
    };

    struct Preview
    {
        SIMPEngine::ParticleState State;
        std::unique_ptr<SIMPEngine::Framebuffer> Target;
        bool Warmed = false;
        int LastFrame = -1;
        float Idle = 0.0f;
    };

    void Refresh();
    void DrawSidebar(float height);
    void DrawGrid(float height);
    void DrawDetails(float height);
    bool Visible(const Entry &entry) const;
    ImTextureID RenderPreview(const Entry &entry, Preview &preview, int width, int height);
    void Confirm();
    void Close();

    std::vector<Entry> m_Entries;
    std::vector<Preview> m_Previews;
    Preview m_DetailPreview;
    int m_DetailIndex = -1;

    std::string m_Title = "Particle Effects";
    std::string m_Category;
    char m_Search[64] = "";
    int m_Selected = -1;
    bool m_Open = false;
    ConfirmCallback m_OnConfirm;
};

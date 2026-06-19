#pragma once

#include <EditorPanel.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Rendering/Texture.h>

#include <memory>
#include <string>

namespace SIMPEngine
{
    class Animation;
}

class AnimationMakerPanel : public EditorPanel
{
public:
    AnimationMakerPanel() = default;

    void Open(SIMPEngine::Entity targetEntity, const std::string& spriteSheetVPath);
    void OnImGuiRender() override;

private:
    void Close();
    bool LoadSpriteSheet(const std::string& vpath);

private:
    bool m_Open = false;

    SIMPEngine::Entity m_TargetEntity{};
    std::string m_SpriteSheetVPath;
    std::shared_ptr<SIMPEngine::Texture> m_SpriteSheetTexture = nullptr;

    int m_FrameWidth = 32;
    int m_FrameHeight = 32;
    int m_FrameCount = 1;
    float m_FrameDuration = 0.1f;
    bool m_Loop = true;

    int   m_PreviewFrame   = 0;
    float m_PreviewElapsed = 0.0f;
    bool  m_PreviewPlaying = true;

    void TickPreview(float dt);
};
#pragma once

#include <Engine/Animation/MotionPresets.h>

#include <imgui.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace SIMPEngine
{
    class Scene;
    class Framebuffer;
}

float MotionViewHeight(const std::string &category);

class MotionPreview
{
public:
    MotionPreview();
    ~MotionPreview();

    void ShowMotion(const std::string &name, const SIMPEngine::MotionOptions &options = {});
    void ShowEffect(const std::string &name);
    void ShowLua(const std::string &code, const std::string &fallbackMotion = {}, const std::string &fallbackEffect = {});
    void Restart();
    void Release();

    void Update(float dt);
    ImTextureID Render(int width, int height, float viewHeight);

    const std::string &Key() const { return m_Key; }
    const std::string &Error() const { return m_Error; }
    float Progress() const { return m_Length > 0.0f ? m_Time / m_Length : 0.0f; }

private:
    enum class Mode
    {
        None,
        Motion,
        Effect,
        Lua
    };

    void Build();
    bool RunLua();

    Mode m_Mode = Mode::None;
    std::string m_Key;
    std::string m_Name;
    std::string m_Code;
    std::string m_FallbackMotion;
    std::string m_FallbackEffect;
    std::string m_Error;
    SIMPEngine::MotionOptions m_Options;
    std::unique_ptr<SIMPEngine::Scene> m_Scene;
    std::unique_ptr<SIMPEngine::Framebuffer> m_Target;
    float m_Time = 0.0f;
    float m_Length = 1.0f;
    bool m_UntilIdle = false;
    bool m_AutoFit = false;
    bool m_HasBounds = false;
    glm::vec2 m_BoundsMin{0.0f};
    glm::vec2 m_BoundsMax{0.0f};
    glm::vec2 m_ViewCenter{0.0f};
    float m_ViewScale = 1.0f;
};

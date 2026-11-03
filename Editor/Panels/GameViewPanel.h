#pragma once

#include <EditorPanel.h>
#include <../EditorContext.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/RenderingAPI.h>

#include <imgui.h>
#include <memory>

class GameViewPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Game";

    explicit GameViewPanel(EditorContext *context) : m_Context(context) {}

    void OnImGuiRender() override;

    bool IsFocused() const { return m_Focused; }

private:
    EditorContext *m_Context;
    std::unique_ptr<SIMPEngine::Framebuffer> m_Framebuffer;
    SIMPEngine::RenderStats m_Stats;
    bool m_Focused = false;
};

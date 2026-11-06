#pragma once

#include <EditorPanel.h>
#include <../EditorContext.h>

#include <glm/glm.hpp>

class TilePalettePanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###TilePalette";

    explicit TilePalettePanel(EditorContext *context) : m_Context(context) {}

    void OnImGuiRender() override;

private:
    void DrawPalette(TilemapComponent &tilemap);

    EditorContext *m_Context;
    float m_Zoom = 2.0f;
    bool m_Selecting = false;
    glm::ivec2 m_SelectStart{0};
};

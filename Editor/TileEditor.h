#pragma once

#include "EditorContext.h"

#include <Engine/Scene/Component.h>

#include <imgui.h>
#include <glm/glm.hpp>

#include <functional>

class TileEditor
{
public:
    explicit TileEditor(EditorContext *context) : m_Context(context) {}

    bool IsActive() const;
    bool IsInteracting() const { return m_Stroke != Stroke::None; }

    void Update(const glm::vec2 &mouseWorld, const glm::vec2 &viewMin, const glm::vec2 &viewMax, bool hovered, bool panning);
    void HandleShortcuts();
    void RenderWorld(float worldPerPixel);
    void DrawScreen(ImDrawList *drawList, const std::function<ImVec2(const glm::vec2 &)> &toScreen) const;

private:
    enum class Stroke
    {
        None,
        Paint,
        Erase,
        Rect,
        RectErase
    };

    TilemapComponent *Map() const;
    TransformComponent *Transform() const;

    void PaintStamp(const glm::ivec2 &cell);
    void EraseFootprint(const glm::ivec2 &cell);
    void StrokeLine(const glm::ivec2 &from, const glm::ivec2 &to);
    void ApplyRect(const glm::ivec2 &a, const glm::ivec2 &b, bool erase);
    void ApplyFill(const glm::ivec2 &cell, bool erase);
    void Pick(const glm::ivec2 &cell);
    void Begin(Stroke stroke);
    void End();

    EditorContext *m_Context;
    Stroke m_Stroke = Stroke::None;
    glm::ivec2 m_Hover{0};
    glm::ivec2 m_Last{0};
    glm::ivec2 m_RectStart{0};
    bool m_HoverValid = false;
    glm::vec2 m_ViewMin{0.0f};
    glm::vec2 m_ViewMax{0.0f};
};

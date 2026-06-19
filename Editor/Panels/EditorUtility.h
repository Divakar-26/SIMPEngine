#pragma once

#include <imgui.h>
#include <Engine/Rendering/Texture.h>
#include <algorithm>

namespace EditorUI
{

    constexpr float Padding = 4.0f;

    inline ImVec2 CalculateFitSize(const SIMPEngine::Texture &tex, float boxSize)
    {
        float w = (float)tex.GetWidth();
        float h = (float)tex.GetHeight();

        if (w <= 0.0f || h <= 0.0f || w > 10000.0f || h > 10000.0f)
            return ImVec2(boxSize, boxSize);

        float scale = std::min(boxSize / w, boxSize / h);
        return ImVec2(w * scale, h * scale);
    }

    inline bool DrawThumbnailButton(
        const char *id,
        const SIMPEngine::Texture &tex,
        float boxSize,
        bool selected = false,
        const ImVec4 &tintCol = ImVec4(1, 1, 1, 1))
    {

        ImVec2 cellSize(boxSize, boxSize);

        ImVec2 start = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, cellSize);

        bool clicked = ImGui::IsItemClicked();
        bool hovered = ImGui::IsItemHovered();

        ImDrawList *dl = ImGui::GetWindowDrawList();

        ImU32 bgCol =
            selected ? IM_COL32(70, 110, 180, 180) : hovered ? IM_COL32(255, 255, 255, 25)
                                                             : IM_COL32(0, 0, 0, 0);

        ImU32 borderCol =
            selected ? IM_COL32(120, 170, 255, 255) : hovered ? IM_COL32(255, 255, 255, 90)
                                                              : IM_COL32(255, 255, 255, 20);

        dl->AddRectFilled(
            start,
            ImVec2(start.x + boxSize, start.y + boxSize),
            bgCol,
            4.0f);

        dl->AddRect(
            start,
            ImVec2(start.x + boxSize, start.y + boxSize),
            borderCol,
            4.0f,
            0,
            selected ? 2.0f : 1.0f);

        if (tex.GetID())
        {
            ImVec2 thumbSize = CalculateFitSize(tex, boxSize - Padding * 2.0f);

            ImVec2 drawMin(
                start.x + Padding + ((boxSize - Padding * 2.0f) - thumbSize.x) * 0.5f,
                start.y + Padding + ((boxSize - Padding * 2.0f) - thumbSize.y) * 0.5f);

            ImVec2 drawMax(
                drawMin.x + thumbSize.x,
                drawMin.y + thumbSize.y);

            dl->AddImage(
                (ImTextureID)(intptr_t)tex.GetID(),
                drawMin,
                drawMax,
                ImVec2(0, 1),
                ImVec2(1, 0),
                IM_COL32(
                    (int)(tintCol.x * 255),
                    (int)(tintCol.y * 255),
                    (int)(tintCol.z * 255),
                    (int)(tintCol.w * 255)));
        }

        return clicked;
    }

    inline void DrawThumbnailImage(
        const SIMPEngine::Texture &tex,
        float boxSize,
        const ImVec4 &tintCol = ImVec4(1, 1, 1, 1))
    {
        if (!tex.GetID())
        {
            ImGui::Dummy(ImVec2(boxSize, boxSize));
            return;
        }

        ImVec2 thumbSize = CalculateFitSize(tex, boxSize - Padding * 2.0f);
        ImVec2 startPos = ImGui::GetCursorPos();

        ImGui::SetCursorPos(ImVec2(
            startPos.x + (boxSize - thumbSize.x) * 0.5f,
            startPos.y + (boxSize - thumbSize.y) * 0.5f));

        ImGui::Image(
            (ImTextureID)(intptr_t)tex.GetID(),
            thumbSize,
            ImVec2(0, 1),
            ImVec2(1, 0),
            tintCol,
            ImVec4(0, 0, 0, 0));

        ImGui::SetCursorPos(startPos);
        ImGui::Dummy(ImVec2(boxSize, boxSize));
    }
}
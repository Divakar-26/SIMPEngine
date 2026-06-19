#include "AnimationMakerPanel.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/Animation.h>
#include <Engine/Scene/Component.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace
{
    static ImVec2 FitSizeInside(float srcW, float srcH, float maxW, float maxH)
    {
        if (srcW <= 0.0f || srcH <= 0.0f || maxW <= 0.0f | maxH <= 0.0f)
            return ImVec2(0.0f, 0.0f);
        float scale = std::min(maxW / srcW, maxH / srcH);
        return ImVec2(srcW * scale, srcH * scale);
    }

    static void DrawCheckerBackground(ImDrawList* dl, ImVec2 min, ImVec2 max, float cellSize = 8.0f)
    {
        const ImU32 dark  = IM_COL32(45,  45,  50,  255);
        const ImU32 light = IM_COL32(62,  62,  68,  255);

        for (float y = min.y; y < max.y; y += cellSize)
        {
            int row = (int)((y - min.y) / cellSize);
            for (float x = min.x; x < max.x; x += cellSize)
            {
                int col = (int)((x - min.x) / cellSize);
                ImU32 col32 = ((row + col) % 2 == 0) ? dark : light;
                ImVec2 cellMin(x, y);
                ImVec2 cellMax(std::min(x + cellSize, max.x), std::min(y + cellSize, max.y));
                dl->AddRectFilled(cellMin, cellMax, col32);
            }
        }
    }

    static void DrawGridOverlay(
        ImDrawList* drawList,
        const ImVec2& min, const ImVec2& max,
        int texW, int texH,
        int frameW, int frameH,
        int highlightFrame = -1)
    {
        if (!drawList || texW <= 0 || texH <= 0 || frameW <= 0 || frameH <= 0)
            return;

        const ImU32 gridCol    = IM_COL32(255, 255, 255, 55);
        const ImU32 borderCol  = IM_COL32(200, 200, 220, 180);
        const ImU32 highlightC = IM_COL32(255, 210,  60, 200);

        drawList->AddRect(min, max, borderCol, 0.0f, 0, 1.5f);

        float scaleX = (max.x - min.x) / (float)texW;
        float scaleY = (max.y - min.y) / (float)texH;

        int cols = std::max(1, texW / frameW);
        int rows = std::max(1, texH / frameH);

        // Vertical lines
        for (int c = 1; c < cols; ++c)
        {
            float x = min.x + c * frameW * scaleX;
            drawList->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), gridCol, 1.0f);
        }

        // Horizontal lines
        for (int r = 1; r < rows; ++r)
        {
            float y = min.y + r * frameH * scaleY;
            drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), gridCol, 1.0f);
        }

        // Highlight the current animated frame
        if (highlightFrame >= 0)
        {
            int fc = highlightFrame % cols;
            int fr = highlightFrame / cols;

            ImVec2 hMin(min.x + fc * frameW * scaleX,
                        min.y + fr * frameH * scaleY);
            ImVec2 hMax(hMin.x + frameW * scaleX,
                        hMin.y + frameH * scaleY);

            drawList->AddRect(hMin, hMax, highlightC, 0.0f, 0, 2.0f);
            drawList->AddRectFilled(hMin, hMax, IM_COL32(255, 210, 60, 28));
        }
    }
}

void AnimationMakerPanel::Open(SIMPEngine::Entity targetEntity,
                               const std::string& spriteSheetVPath)
{
    m_TargetEntity    = targetEntity;
    m_SpriteSheetVPath = spriteSheetVPath;

    m_FrameWidth    = 32;
    m_FrameHeight   = 32;
    m_FrameCount    = 1;
    m_FrameDuration = 0.1f;
    m_Loop          = true;

    m_PreviewFrame    = 0;
    m_PreviewElapsed  = 0.0f;
    m_PreviewPlaying  = true;

    LoadSpriteSheet(spriteSheetVPath);
    m_Open = true;
}

bool AnimationMakerPanel::LoadSpriteSheet(const std::string& vpath)
{
    auto real = SIMPEngine::VFS::Resolve(vpath);
    if (!real)
    {
        CORE_ERROR("Could not resolve sprite sheet: {}", vpath);
        m_SpriteSheetTexture = nullptr;
        return false;
    }

    auto texture = std::make_shared<SIMPEngine::Texture>();
    if (!texture->LoadFromFile(real->c_str()))
    {
        CORE_ERROR("Could not load sprite sheet: {}", *real);
        m_SpriteSheetTexture = nullptr;
        return false;
    }

    texture->SetVFSPath(vpath);
    m_SpriteSheetTexture = texture;
    return true;
}

void AnimationMakerPanel::OnImGuiRender()
{
    if (!m_Open)
        return;

    ImGui::OpenPopup("Animation Maker");

    // Large enough to be useful; user can resize
    ImGui::SetNextWindowSizeConstraints(ImVec2(740, 480), ImVec2(1400, 960));
    ImGui::SetNextWindowSize(ImVec2(900, 580), ImGuiCond_FirstUseEver);

    if (!ImGui::BeginPopupModal("Animation Maker", nullptr, ImGuiWindowFlags_None))
        return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.9f, 1.0f));
    ImGui::TextUnformatted("Animation Maker");
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
    ImGui::TextUnformatted(m_SpriteSheetVPath.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    const float  controlsWidth = 270.0f;
    const ImVec2 avail         = ImGui::GetContentRegionAvail();
    const float  childHeight   = avail.y - 4.0f;

    //  LEFT — Control
    ImGui::BeginChild("##anim_controls", ImVec2(controlsWidth, childHeight), true);
    {
        ImGui::TextUnformatted("Frame Settings");
        ImGui::Separator();
        ImGui::Spacing();

        if (m_SpriteSheetTexture)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 0.5f, 1.0f));
            ImGui::Text("Sheet  %d × %d px",
                        m_SpriteSheetTexture->GetWidth(),
                        m_SpriteSheetTexture->GetHeight());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // Clamp after drag
        ImGui::DragInt("Frame Width",    &m_FrameWidth,    1.0f, 1, 4096);
        ImGui::DragInt("Frame Height",   &m_FrameHeight,   1.0f, 1, 4096);
        ImGui::DragInt("Frame Count",    &m_FrameCount,    1.0f, 1, 100000);
        ImGui::DragFloat("Frame Duration", &m_FrameDuration, 0.005f, 0.005f, 10.0f, "%.3f s");
        ImGui::Checkbox("Loop", &m_Loop);

        m_FrameWidth    = std::max(1, m_FrameWidth);
        m_FrameHeight   = std::max(1, m_FrameHeight);
        m_FrameCount    = std::max(1, m_FrameCount);
        m_FrameDuration = std::max(0.005f, m_FrameDuration);

        // Derived info
        if (m_SpriteSheetTexture && m_FrameWidth > 0 && m_FrameHeight > 0)
        {
            int cols = std::max(1, m_SpriteSheetTexture->GetWidth()  / m_FrameWidth);
            int rows = std::max(1, m_SpriteSheetTexture->GetHeight() / m_FrameHeight);
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.65f, 1.0f));
            ImGui::Text("Grid  %d cols × %d rows  (%d cells)", cols, rows, cols * rows);
            ImGui::Text("Total  %.2f s", m_FrameCount * m_FrameDuration);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Preview");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(m_PreviewPlaying ? "  ⏸  Pause" : "  ▶  Play ", ImVec2(-1, 28)))
            m_PreviewPlaying = !m_PreviewPlaying;

        if (ImGui::Button("  ⏮  Reset", ImVec2(-1, 28)))
        {
            m_PreviewFrame   = 0;
            m_PreviewElapsed = 0.0f;
        }

        ImGui::Spacing();

        // Manual frame scrubber
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.8f, 1.0f));
        ImGui::Text("Frame  %d / %d", m_PreviewFrame, std::max(0, m_FrameCount - 1));
        ImGui::PopStyleColor();

        int scrub = m_PreviewFrame;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderInt("##scrub", &scrub, 0, std::max(0, m_FrameCount - 1)))
        {
            m_PreviewFrame   = scrub;
            m_PreviewElapsed = 0.0f;
            m_PreviewPlaying = false;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const bool canBuild = (m_TargetEntity && m_SpriteSheetTexture);

        if (!canBuild)
            ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.22f, 0.55f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.68f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f, 0.45f, 0.22f, 1.0f));

        if (ImGui::Button("Build Animation", ImVec2(-1, 36)))
        {
            if (!m_TargetEntity.HasComponent<AnimatedSpriteComponent>())
                m_TargetEntity.AddComponent<AnimatedSpriteComponent>();

            auto& ac = m_TargetEntity.GetComponent<AnimatedSpriteComponent>();
            ac.animation = std::make_unique<SIMPEngine::Animation>(
                m_SpriteSheetTexture,
                m_FrameWidth,
                m_FrameHeight,
                m_FrameCount,
                m_FrameDuration,
                m_Loop);

            Close();
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleColor(3);

        if (!canBuild)
            ImGui::EndDisabled();

        ImGui::Spacing();

        if (ImGui::Button("Cancel", ImVec2(-1, 32)))
        {
            Close();
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 6);

    ImGui::BeginChild("##anim_right", ImVec2(0, childHeight), false);
    {
        if (!m_SpriteSheetTexture)
        {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "No sprite sheet loaded.");
            ImGui::EndChild();
            ImGui::EndPopup();
            return;
        }

        TickPreview(ImGui::GetIO().DeltaTime);

        const int texW = m_SpriteSheetTexture->GetWidth();
        const int texH = m_SpriteSheetTexture->GetHeight();

        ImVec2 rightAvail = ImGui::GetContentRegionAvail();

        // Split vertically: sheet gets ~65 %, live preview ~35 %
        const float previewPanelH = std::max(120.0f, rightAvail.y * 0.32f);
        const float sheetPanelH   = rightAvail.y - previewPanelH - 10.0f;

        ImGui::BeginChild("##sheet_view", ImVec2(0, sheetPanelH), true);
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.85f, 1.0f));
            ImGui::TextUnformatted("Sprite Sheet");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.55f, 1.0f));
            ImGui::Text("(%d × %d)", texW, texH);
            ImGui::PopStyleColor();
            ImGui::Separator();

            ImVec2 area = ImGui::GetContentRegionAvail();
            if (area.x < 10.0f) area.x = 10.0f;
            if (area.y < 10.0f) area.y = 10.0f;

            ImVec2 imgSize = FitSizeInside((float)texW, (float)texH, area.x, area.y);

            ImVec2 cursor  = ImGui::GetCursorScreenPos();
            ImVec2 imgMin(cursor.x + (area.x - imgSize.x) * 0.5f,
                          cursor.y + (area.y - imgSize.y) * 0.5f);
            ImVec2 imgMax(imgMin.x + imgSize.x, imgMin.y + imgSize.y);

            ImDrawList* dl = ImGui::GetWindowDrawList();

            DrawCheckerBackground(dl, imgMin, imgMax, 8.0f);

            dl->AddImage(
                (ImTextureID)(intptr_t)m_SpriteSheetTexture->GetID(),
                imgMin, imgMax,
                ImVec2(0, 1), ImVec2(1, 0));

            DrawGridOverlay(dl, imgMin, imgMax,
                            texW, texH,
                            m_FrameWidth, m_FrameHeight,
                            m_PreviewFrame);

            ImGui::Dummy(area);
        }
        ImGui::EndChild();

        ImGui::Spacing();

        ImGui::BeginChild("##frame_preview", ImVec2(0, previewPanelH), true);
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.85f, 1.0f));
            ImGui::TextUnformatted("Live Preview");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.55f, 1.0f));
            ImGui::Text("— frame %d", m_PreviewFrame);
            ImGui::PopStyleColor();
            ImGui::Separator();

            ImVec2 area = ImGui::GetContentRegionAvail();
            if (area.x < 4.0f) area.x = 4.0f;
            if (area.y < 4.0f) area.y = 4.0f;

            // Show only the current frame, centred and aspect-correct
            ImVec2 frameDispSize = FitSizeInside(
                (float)m_FrameWidth, (float)m_FrameHeight,
                area.x, area.y);

            // Clamp to a reasonable max so it doesn't look blurry on tiny frames
            const float maxFrameDisp = std::min(area.y - 4.0f, 200.0f);
            if (frameDispSize.x > maxFrameDisp || frameDispSize.y > maxFrameDisp)
            {
                float s = maxFrameDisp / std::max(frameDispSize.x, frameDispSize.y);
                frameDispSize.x *= s;
                frameDispSize.y *= s;
            }

            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImVec2 fMin(cursor.x + (area.x - frameDispSize.x) * 0.5f,
                        cursor.y + (area.y - frameDispSize.y) * 0.5f);
            ImVec2 fMax(fMin.x + frameDispSize.x, fMin.y + frameDispSize.y);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            DrawCheckerBackground(dl, fMin, fMax, 6.0f);

            // Compute UV for the current frame
            if (texW > 0 && texH > 0 && m_FrameWidth > 0 && m_FrameHeight > 0)
            {
                int framesPerRow = std::max(1, texW / m_FrameWidth);
                int fx = m_PreviewFrame % framesPerRow;
                int fy = m_PreviewFrame / framesPerRow;

                float u0 =  fx      * m_FrameWidth  / (float)texW;
                float u1 = (fx + 1) * m_FrameWidth  / (float)texW;
                // Flip V for OpenGL convention (origin at bottom-left)
                float v0 = 1.0f - (fy + 1) * m_FrameHeight / (float)texH;
                float v1 = 1.0f -  fy      * m_FrameHeight / (float)texH;

                dl->AddImage(
                    (ImTextureID)(intptr_t)m_SpriteSheetTexture->GetID(),
                    fMin, fMax,
                    ImVec2(u0, v0), ImVec2(u1, v1));
            }

            // Thin border
            dl->AddRect(fMin, fMax, IM_COL32(255, 210, 60, 160), 0.0f, 0, 1.5f);

            ImGui::Dummy(area);
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::EndPopup();
}

void AnimationMakerPanel::TickPreview(float dt)
{
    if (!m_PreviewPlaying || m_FrameCount <= 1 || m_FrameDuration <= 0.0f)
        return;

    m_PreviewElapsed += dt;
    while (m_PreviewElapsed >= m_FrameDuration)
    {
        m_PreviewElapsed -= m_FrameDuration;
        m_PreviewFrame++;
        if (m_PreviewFrame >= m_FrameCount)
            m_PreviewFrame = m_Loop ? 0 : m_FrameCount - 1;
    }
}

void AnimationMakerPanel::Close()
{
    m_Open = false;
    m_TargetEntity = {};
    m_SpriteSheetVPath.clear();
    m_SpriteSheetTexture = nullptr;
    m_PreviewFrame       = 0;
    m_PreviewElapsed     = 0.0f;
    m_PreviewPlaying     = false;
}
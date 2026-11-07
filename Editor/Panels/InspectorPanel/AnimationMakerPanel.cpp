#include "AnimationMakerPanel.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/Animation.h>
#include <Engine/Scene/Component.h>
#include <imgui.h>
#include "UI/Widgets.h"
#include "../../EditorCommands.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    static ImVec2 FitSizeInside(float srcW, float srcH, float maxW, float maxH)
    {
        if (srcW <= 0.0f || srcH <= 0.0f || maxW <= 0.0f || maxH <= 0.0f)
            return ImVec2(0.0f, 0.0f);
        float scale = std::min(maxW / srcW, maxH / srcH);
        return ImVec2(srcW * scale, srcH * scale);
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

        const ImU32 gridCol    = EditorUI::U32(EditorUI::Colors::Text, 0.18f);
        const ImU32 borderCol  = EditorUI::U32(EditorUI::Colors::BorderStrong);
        const ImU32 highlightC = EditorUI::U32(EditorUI::Colors::Accent);

        drawList->AddRect(min, max, borderCol, 0.0f, 0, 1.5f);

        float scaleX = (max.x - min.x) / static_cast<float>(texW);
        float scaleY = (max.y - min.y) / static_cast<float>(texH);

        int cols = std::max(1, texW / frameW);
        int rows = std::max(1, texH / frameH);

        for (int c = 1; c < cols; ++c)
        {
            float x = min.x + static_cast<float>(c * frameW) * scaleX;
            drawList->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), gridCol, 1.0f);
        }

        for (int r = 1; r < rows; ++r)
        {
            float y = min.y + static_cast<float>(r * frameH) * scaleY;
            drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), gridCol, 1.0f);
        }

        if (highlightFrame >= 0)
        {
            int fc = highlightFrame % cols;
            int fr = highlightFrame / cols;

            ImVec2 hMin(min.x + static_cast<float>(fc * frameW) * scaleX,
                        min.y + static_cast<float>(fr * frameH) * scaleY);
            ImVec2 hMax(hMin.x + static_cast<float>(frameW) * scaleX,
                        hMin.y + static_cast<float>(frameH) * scaleY);

            drawList->AddRect(hMin, hMax, highlightC, 0.0f, 0, 2.0f);
            drawList->AddRectFilled(hMin, hMax, EditorUI::U32(EditorUI::Colors::Accent, 0.15f));
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

    if (LoadSpriteSheet(spriteSheetVPath))
        AutoSlice();
    m_Open = true;
}

void AnimationMakerPanel::AutoSlice()
{
    m_Slice = SIMPEngine::SpriteSheetSlicer::Detect(m_SpriteSheetVPath);
    if (m_Slice.FrameWidth <= 0 || m_Slice.FrameHeight <= 0)
        return;
    m_FrameWidth = m_Slice.FrameWidth;
    m_FrameHeight = m_Slice.FrameHeight;
    m_FrameCount = std::max(1, m_Slice.FrameCount);
    m_PreviewFrame = 0;
    m_PreviewElapsed = 0.0f;
    m_PreviewPlaying = true;
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
    using namespace EditorUI;

    if (!m_Open)
        return;

    ImGui::SetNextWindowSizeConstraints(ImVec2(760, 500), ImVec2(1600, 1000));
    if (!BeginModal("Animation Maker", &m_Open, ImVec2(980, 620)))
        return;

    struct SuppressScope
    {
        SuppressScope() { PushEditSuppression(); }
        ~SuppressScope() { PopEditSuppression(); }
    } suppress;

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextUnformatted(m_SpriteSheetVPath.c_str());
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 2));

    const float  controlsWidth = 290.0f;
    const ImVec2 avail         = ImGui::GetContentRegionAvail();
    const float  childHeight   = avail.y - 4.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Metrics::FrameRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Header);
    ImGui::BeginChild("##anim_controls", ImVec2(controlsWidth, childHeight), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    {
        SectionLabel("Frame Settings");

        if (m_SpriteSheetTexture)
        {
            char sheet[48];
            std::snprintf(sheet, sizeof(sheet), "%d x %d px",
                          m_SpriteSheetTexture->GetWidth(),
                          m_SpriteSheetTexture->GetHeight());
            PropertyValue("Sheet", sheet);
        }

        if (m_SpriteSheetTexture)
        {
            char detected[96];
            if (m_Slice.Found)
                std::snprintf(detected, sizeof(detected), "%d frames, %d x %d", m_Slice.FrameCount, m_Slice.FrameWidth, m_Slice.FrameHeight);
            else
                std::snprintf(detected, sizeof(detected), "No grid found");
            PropertyValue("Detected", detected, m_Slice.Found ? &Colors::Success : &Colors::Warning);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && !m_Slice.Method.empty())
                ImGui::SetTooltip("Found by %s", m_Slice.Method.c_str());
            PropertyLabel("");
            if (Button("Detect Again", ButtonKind::Ghost, ImVec2(-FLT_MIN, 0)))
                AutoSlice();
        }

        int size[2] = {m_FrameWidth, m_FrameHeight};
        if (PropertyInt2("Frame Size", size, 1.0f, 1, 4096))
        {
            m_FrameWidth  = size[0];
            m_FrameHeight = size[1];
        }
        PropertyInt("Count", &m_FrameCount, 1.0f, 1, 100000);
        PropertyFloat("Duration", &m_FrameDuration, 0.005f, 0.005f, 10.0f, "%.3f s");
        PropertyBool("Loop", &m_Loop);

        m_FrameWidth    = std::max(1, m_FrameWidth);
        m_FrameHeight   = std::max(1, m_FrameHeight);
        m_FrameCount    = std::max(1, m_FrameCount);
        m_FrameDuration = std::max(0.005f, m_FrameDuration);

        if (m_SpriteSheetTexture && m_FrameWidth > 0 && m_FrameHeight > 0)
        {
            int cols = std::max(1, m_SpriteSheetTexture->GetWidth()  / m_FrameWidth);
            int rows = std::max(1, m_SpriteSheetTexture->GetHeight() / m_FrameHeight);
            char grid[64];
            std::snprintf(grid, sizeof(grid), "%d x %d cells", cols, rows);
            PropertyValue("Grid", grid, &Colors::TextDim);
            std::snprintf(grid, sizeof(grid), "%.2f s", static_cast<float>(m_FrameCount) * m_FrameDuration);
            PropertyValue("Length", grid, &Colors::TextDim);
        }

        SectionLabel("Preview");

        if (IconButton("##restart", ICON_LC_SKIP_BACK, "Restart"))
        {
            m_PreviewFrame   = 0;
            m_PreviewElapsed = 0.0f;
        }
        ImGui::SameLine(0, 4);
        if (IconButton("##play", m_PreviewPlaying ? ICON_LC_PAUSE : ICON_LC_PLAY, m_PreviewPlaying ? "Pause" : "Play", m_PreviewPlaying))
            m_PreviewPlaying = !m_PreviewPlaying;
        ImGui::SameLine(0, 8);

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Frame %d / %d", m_PreviewFrame, std::max(0, m_FrameCount - 1));

        int scrub = m_PreviewFrame;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##scrub", &scrub, 0, std::max(0, m_FrameCount - 1), ""))
        {
            m_PreviewFrame   = scrub;
            m_PreviewElapsed = 0.0f;
            m_PreviewPlaying = false;
        }

        float footer = ImGui::GetFrameHeight() * 2.0f + 18.0f;
        float remaining = ImGui::GetContentRegionAvail().y - footer;
        if (remaining > 0.0f)
            ImGui::Dummy(ImVec2(0, remaining));

        const bool canBuild = (m_TargetEntity && m_SpriteSheetTexture);

        ImGui::BeginDisabled(!canBuild);
        if (Button("Build Animation", ButtonKind::Primary, ImVec2(-FLT_MIN, 0)))
        {
            if (m_Context)
                EditorCommands::RecordEdit(*m_Context);
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
        ImGui::EndDisabled();

        if (Button("Cancel", ButtonKind::Ghost, ImVec2(-FLT_MIN, 0)))
        {
            Close();
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 10);

    ImGui::BeginChild("##anim_right", ImVec2(0, childHeight), ImGuiChildFlags_None);
    {
        if (!m_SpriteSheetTexture)
        {
            EmptyState("No sprite sheet loaded", "The selected file could not be loaded as a texture.");
            ImGui::EndChild();
            ImGui::PopStyleVar();
            EndModal();
            return;
        }

        TickPreview(ImGui::GetIO().DeltaTime);

        const int texW = m_SpriteSheetTexture->GetWidth();
        const int texH = m_SpriteSheetTexture->GetHeight();

        ImVec2 rightAvail = ImGui::GetContentRegionAvail();

        const float previewPanelH = std::max(140.0f, rightAvail.y * 0.34f);
        const float sheetPanelH   = rightAvail.y - previewPanelH - 10.0f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Canvas);
        ImGui::BeginChild("##sheet_view", ImVec2(0, sheetPanelH), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        {
            SectionLabel("Sprite Sheet");

            ImVec2 area = ImGui::GetContentRegionAvail();
            if (area.x < 10.0f) area.x = 10.0f;
            if (area.y < 10.0f) area.y = 10.0f;

            ImVec2 imgSize = FitSizeInside(static_cast<float>(texW), static_cast<float>(texH), area.x, area.y);

            ImVec2 cursor  = ImGui::GetCursorScreenPos();
            ImVec2 imgMin(std::floor(cursor.x + (area.x - imgSize.x) * 0.5f),
                          std::floor(cursor.y + (area.y - imgSize.y) * 0.5f));
            ImVec2 imgMax(imgMin.x + imgSize.x, imgMin.y + imgSize.y);

            ImDrawList* dl = ImGui::GetWindowDrawList();

            DrawCheckerboard(dl, imgMin, imgMax, 8.0f);

            dl->AddImage(
                EditorUI::TextureId(m_SpriteSheetTexture->GetID()),
                imgMin, imgMax,
                ImVec2(0, 1), ImVec2(1, 0));

            DrawGridOverlay(dl, imgMin, imgMax,
                            texW, texH,
                            m_FrameWidth, m_FrameHeight,
                            m_PreviewFrame);

            ImGui::Dummy(area);
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0, 2));

        ImGui::BeginChild("##frame_preview", ImVec2(0, previewPanelH), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        {
            SectionLabel("Live Preview");

            ImVec2 area = ImGui::GetContentRegionAvail();
            if (area.x < 4.0f) area.x = 4.0f;
            if (area.y < 4.0f) area.y = 4.0f;

            ImVec2 frameDispSize = FitSizeInside(
                static_cast<float>(m_FrameWidth), static_cast<float>(m_FrameHeight),
                area.x, area.y);

            const float maxFrameDisp = std::min(area.y - 4.0f, 220.0f);
            if (frameDispSize.x > maxFrameDisp || frameDispSize.y > maxFrameDisp)
            {
                float s = maxFrameDisp / std::max(frameDispSize.x, frameDispSize.y);
                frameDispSize.x *= s;
                frameDispSize.y *= s;
            }

            ImVec2 cursor = ImGui::GetCursorScreenPos();
            ImVec2 fMin(std::floor(cursor.x + (area.x - frameDispSize.x) * 0.5f),
                        std::floor(cursor.y + (area.y - frameDispSize.y) * 0.5f));
            ImVec2 fMax(fMin.x + frameDispSize.x, fMin.y + frameDispSize.y);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            DrawCheckerboard(dl, fMin, fMax, 6.0f);

            if (texW > 0 && texH > 0 && m_FrameWidth > 0 && m_FrameHeight > 0)
            {
                int framesPerRow = std::max(1, texW / m_FrameWidth);
                int fx = m_PreviewFrame % framesPerRow;
                int fy = m_PreviewFrame / framesPerRow;

                float u0 = static_cast<float>(fx * m_FrameWidth) / static_cast<float>(texW);
                float u1 = static_cast<float>((fx + 1) * m_FrameWidth) / static_cast<float>(texW);
                float v0 = 1.0f - static_cast<float>(fy * m_FrameHeight) / static_cast<float>(texH);
                float v1 = 1.0f - static_cast<float>((fy + 1) * m_FrameHeight) / static_cast<float>(texH);

                dl->AddImage(
                    EditorUI::TextureId(m_SpriteSheetTexture->GetID()),
                    fMin, fMax,
                    ImVec2(u0, v0), ImVec2(u1, v1));
            }

            dl->AddRect(fMin, fMax, U32(Colors::Accent, 0.8f), 0.0f, 0, 1.5f);

            ImGui::Dummy(area);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    EndModal();
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

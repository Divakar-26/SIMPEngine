#include "InspectorPanel.h"
#include "AnimationMakerPanel.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/Animation.h>
#include <Engine/Scene/Component.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    static void DrawFrameStrip(
        SIMPEngine::Animation *anim,
        float stripHeight)
    {
        using namespace EditorUI;

        if (!anim || !anim->GetTexture())
            return;

        auto tex = anim->GetTexture();
        int texW = tex->GetWidth();
        int texH = tex->GetHeight();
        int fw = anim->GetFrameWidth();
        int fh = anim->GetFrameHeight();
        int fc = anim->GetFrameNumber();

        if (fw <= 0 || fh <= 0 || fc <= 0 || texW <= 0 || texH <= 0)
            return;

        int framesPerRow = std::max(1, texW / fw);

        float cellW = stripHeight * (float)fw / (float)fh;
        float gap = 2.0f;
        float panelW = ImGui::GetContentRegionAvail().x;

        int visible = std::max(1, (int)((panelW - 24.0f) / (cellW + gap)));
        visible = std::min(visible, fc);
        int current = anim->GetCurrentFrame();

        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        for (int i = 0; i < visible; ++i)
        {
            int fx = i % framesPerRow;
            int fy = i / framesPerRow;

            float u0 = fx * fw / (float)texW;
            float u1 = (fx + 1) * fw / (float)texW;
            float v0 = 1.0f - (fy + 1) * fh / (float)texH;
            float v1 = 1.0f - fy * fh / (float)texH;

            ImVec2 cellMin(cursor.x + i * (cellW + gap), cursor.y);
            ImVec2 cellMax(cellMin.x + cellW, cellMin.y + stripHeight);

            DrawCheckerboard(dl, cellMin, cellMax, 5.0f);
            dl->AddImage((ImTextureID)(intptr_t)tex->GetID(), cellMin, cellMax, ImVec2(u0, v1), ImVec2(u1, v0));
            dl->AddRect(cellMin, cellMax, U32(i == current ? Colors::Accent : Colors::Border), 0.0f, 0, i == current ? 1.5f : 1.0f);
        }

        if (fc > visible)
        {
            char more[16];
            std::snprintf(more, sizeof(more), "+%d", fc - visible);
            ImVec2 ts = ImGui::CalcTextSize(more);
            dl->AddText(ImVec2(cursor.x + visible * (cellW + gap) + 4.0f, cursor.y + (stripHeight - ts.y) * 0.5f), U32(Colors::TextFaint), more);
        }

        ImGui::Dummy(ImVec2(panelW, stripHeight));
    }
}

void InspectorPanel::DrawAnimationSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    SIMPEngine::Scene *const selectedScene = m_Context->Scene;
    const entt::entity selectedHandle = m_SelectedEntity.GetHandle();
    EditorContext &ctx = *m_Context;

    auto openPicker = [this, selectedScene, selectedHandle](const char *title)
    {
        m_AssetPicker->Open(
            title,
            "assets://",
            [this, selectedScene, selectedHandle](const std::string &vpath)
            {
                if (m_Context->Scene != selectedScene)
                    return;
                SIMPEngine::Entity selected(selectedHandle, selectedScene);
                if (!selected || !selected.HasComponent<AnimatedSpriteComponent>())
                    return;
                m_AnimationMaker->Open(selected, vpath);
            },
            {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".webp"});
    };

    DrawComponentSection<AnimatedSpriteComponent>(
        ctx, "Animated Sprite", m_SelectedEntity,
        [this, &ctx, &openPicker, &m_SelectedEntity](AnimatedSpriteComponent &ac)
        {
            SIMPEngine::Animation *anim = ac.animation.get();

            if (!anim || !anim->GetTexture())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
                ImGui::TextWrapped("No animation. Pick a sprite sheet and slice it into frames.");
                ImGui::PopStyleColor();
                if (Button("Create Animation...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
                    openPicker("Select Sprite Sheet");
                return;
            }

            auto tex = anim->GetTexture();
            float dur = anim->GetFrameDuration();
            int fc = anim->GetFrameNumber();

            PropertyValue("Sheet", tex->GetVFSPath().c_str(), &Colors::TextDim);

            {
                int size[2] = {anim->GetFrameWidth(), anim->GetFrameHeight()};
                int fn = anim->GetFrameNumber();
                float fd = anim->GetFrameDurationRef();
                bool lp = anim->IsLooping();

                bool changed = false;
                changed |= PropertyInt2("Frame Size", size, 1.0f, 1, 4096);
                changed |= PropertyInt("Frames", &fn, 1.0f, 1, 100000);
                changed |= PropertyFloat("Frame Time", &fd, 0.005f, 0.005f, 10.0f, "%.3f s");
                changed |= PropertyBool("Loop", &lp);

                if (changed)
                {
                    ac.animation = std::make_unique<SIMPEngine::Animation>(
                        tex,
                        std::max(1, size[0]),
                        std::max(1, size[1]),
                        std::max(1, fn),
                        std::max(0.005f, fd),
                        lp);
                    anim = ac.animation.get();
                }
            }

            char total[48];
            std::snprintf(total, sizeof(total), "%.2f s  ·  %s", fc * dur, anim->isFinished() ? "finished" : (anim->IsLooping() ? "looping" : "playing"));
            PropertyValue("Length", total, &Colors::TextDim);

            PropertyLabel("Frames");
            DrawFrameStrip(anim, 40.0f);

            PropertyLabel("");
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float third = std::floor((ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f);
            if (Button("Restart", ButtonKind::Secondary, ImVec2(third, 0)))
                anim->Reset();
            ImGui::SameLine();
            if (Button("Edit...", ButtonKind::Secondary, ImVec2(third, 0)))
                m_AnimationMaker->Open(m_SelectedEntity, tex->GetVFSPath());
            ImGui::SameLine();
            if (Button("Replace...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
                openPicker("Replace Sprite Sheet");

            PropertyLabel("");
            if (Button("Remove animation", ButtonKind::Danger, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                ac.animation.reset();
            }
        });
}

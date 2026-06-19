#include "InspectorPanel.h"
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
    static void DrawFrameStrip(
        SIMPEngine::Animation* anim,
        float stripHeight = 48.0f)
    {
        if (!anim || !anim->GetTexture())
            return;

        auto tex   = anim->GetTexture();
        int  texW  = tex->GetWidth();
        int  texH  = tex->GetHeight();
        int  fw    = anim->GetFrameWidth();
        int  fh    = anim->GetFrameHeight();
        int  fc    = anim->GetFrameNumber();

        if (fw <= 0 || fh <= 0 || fc <= 0)
            return;

        int framesPerRow = std::max(1, texW / fw);

        float aspect   = (float)fw / (float)fh;
        float cellW    = stripHeight * aspect;
        float panelW   = ImGui::GetContentRegionAvail().x;

        int visible = std::max(1, (int)(panelW / (cellW + 2.0f)));
        visible     = std::min(visible, fc);

        ImDrawList* dl     = ImGui::GetWindowDrawList();
        ImVec2      cursor = ImGui::GetCursorScreenPos();

        for (int i = 0; i < visible; ++i)
        {
            int fx = i % framesPerRow;
            int fy = i / framesPerRow;

            float u0 =  fx      * fw / (float)texW;
            float u1 = (fx + 1) * fw / (float)texW;
            float v0 = 1.0f - (fy + 1) * fh / (float)texH;
            float v1 = 1.0f -  fy      * fh / (float)texH;

            ImVec2 cellMin(cursor.x + i * (cellW + 2.0f), cursor.y);
            ImVec2 cellMax(cellMin.x + cellW, cellMin.y + stripHeight);

            dl->AddRectFilled(cellMin, cellMax, IM_COL32(30, 30, 35, 255));
            dl->AddImage(
                (ImTextureID)(intptr_t)tex->GetID(),
                cellMin, cellMax,
                ImVec2(u0, v0), ImVec2(u1, v1));
            dl->AddRect(cellMin, cellMax, IM_COL32(80, 80, 90, 180));
        }

        if (fc > visible)
        {
            // Ellipsis indicator
            ImVec2 dotMin(cursor.x + visible * (cellW + 2.0f), cursor.y);
            ImVec2 dotMax(dotMin.x + 20.0f, dotMin.y + stripHeight);
            dl->AddRectFilled(dotMin, dotMax, IM_COL32(30, 30, 35, 180));
            dl->AddText(
                ImVec2(dotMin.x + 3.0f, dotMin.y + stripHeight * 0.5f - 6.0f),
                IM_COL32(160, 160, 170, 255), "...");
        }

        ImGui::Dummy(ImVec2(panelW, stripHeight));
    }
}

void InspectorPanel::DrawAnimationSection(SIMPEngine::Entity& m_SelectedEntity)
{
    DrawComponentCard<AnimatedSpriteComponent>(
        "Animated Sprite", m_SelectedEntity,
        [this, &m_SelectedEntity](AnimatedSpriteComponent& ac)
        {
            SIMPEngine::Animation* anim = ac.animation.get();

            if (!anim || !anim->GetTexture())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
                ImGui::TextWrapped("No animation assigned. Use the picker below "
                                   "or drag a sprite sheet onto the entity.");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                if (ImGui::Button("  +  Create Animation", ImVec2(-1, 32)))
                {
                    m_AssetPicker->Open(
                        "Select Sprite Sheet",
                        "assets://",
                        [this, &m_SelectedEntity](const std::string& vpath)
                        {
                            m_AnimationMaker->Open(m_SelectedEntity, vpath);
                        });
                }
                return;
            }

            auto  tex  = anim->GetTexture();
            float dur  = anim->GetFrameDuration();
            int   fc   = anim->GetFrameNumber();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 0.5f, 1.0f));
            ImGui::Text("Sheet   %s", tex->GetVFSPath().c_str());
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.75f, 1.0f));
            ImGui::Text("Frames  %d   |   %d × %d px   |   %.3f s/frame   |   %.2f s total",
                        fc,
                        anim->GetFrameWidth(),
                        anim->GetFrameHeight(),
                        dur,
                        fc * dur);
            ImGui::PopStyleColor();

            ImGui::Spacing();

            {
                int fw = anim->GetFrameWidth();
                int fh = anim->GetFrameHeight();
                int fn = anim->GetFrameNumber();
                float fd = anim->GetFrameDurationRef();
                bool  lp = anim->IsLooping();

                bool changed = false;

                changed |= ImGui::DragInt("Frame Width",  &fw, 1.0f, 1, 4096);
                changed |= ImGui::DragInt("Frame Height", &fh, 1.0f, 1, 4096);
                changed |= ImGui::DragInt("Frame Count",  &fn, 1.0f, 1, 100000);
                changed |= ImGui::DragFloat("Frame Duration", &fd, 0.005f, 0.005f, 10.0f, "%.3f s");
                changed |= ImGui::Checkbox("Loop", &lp);

                if (changed)
                {
                    // Rebuild animation in-place with new parameters
                    ac.animation = std::make_unique<SIMPEngine::Animation>(
                        tex,
                        std::max(1, fw),
                        std::max(1, fh),
                        std::max(1, fn),
                        std::max(0.005f, fd),
                        lp);
                    anim = ac.animation.get();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextUnformatted("Playback");
            ImGui::Spacing();

            if (ImGui::Button("Play", ImVec2(80, 26)))
                anim->Reset();

            ImGui::SameLine();

            if (anim->isFinished())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.3f, 1.0f));
                ImGui::TextUnformatted("Finished");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
                ImGui::TextUnformatted(anim->IsLooping() ? "Looping" : "Playing");
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            ImGui::TextUnformatted("Frame Strip");
            ImGui::Spacing();
            DrawFrameStrip(anim, 52.0f);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Edit in Animation Maker", ImVec2(-1, 30)))
                m_AnimationMaker->Open(m_SelectedEntity, tex->GetVFSPath());

            ImGui::Spacing();

            if (ImGui::Button("Replace Sprite Sheet", ImVec2(-1, 30)))
            {
                m_AssetPicker->Open(
                    "Replace Sprite Sheet",
                    "assets://",
                    [this, &m_SelectedEntity](const std::string& vpath)
                    {
                        m_AnimationMaker->Open(m_SelectedEntity, vpath);
                    });
            }

            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.42f, 0.14f, 0.14f, 1.0f));

            if (ImGui::Button("Remove Animation", ImVec2(-1, 28)))
                ac.animation.reset();

            ImGui::PopStyleColor(3);
        });
}
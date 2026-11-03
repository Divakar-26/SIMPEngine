#include "InspectorPanel.h"

#include <cmath>
#include <cstdio>

namespace
{
    bool AcceptTexturePayload(EditorContext &ctx, SpriteComponent &sc)
    {
        bool accepted = false;
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ASSET"))
            {
                if (auto texture = EditorCommands::LoadTexture(static_cast<const char *>(payload->Data)))
                {
                    EditorCommands::RecordEdit(ctx);
                    sc.texture = texture;
                    accepted = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return accepted;
    }

    bool DrawTextureSlot(EditorContext &ctx, SpriteComponent &sc)
    {
        using namespace EditorUI;

        EditorUI::PropertyLabel("Texture");
        float width = ImGui::GetContentRegionAvail().x;
        float height = std::min(120.0f, std::max(64.0f, width * 0.6f));
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 max(pos.x + width, pos.y + height);

        ImGui::InvisibleButton("##slot", ImVec2(width, height));
        bool hovered = ImGui::IsItemHovered();
        bool open = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        AcceptTexturePayload(ctx, sc);

        ImDrawList *dl = ImGui::GetWindowDrawList();

        if (sc.texture && sc.texture->GetID())
        {
            DrawCheckerboard(dl, pos, max, 8.0f);
            DrawImageFit(dl, EditorUI::TextureId(sc.texture->GetID()),
                         ImVec2(static_cast<float>(sc.texture->GetWidth()), static_cast<float>(sc.texture->GetHeight())),
                         ImVec2(pos.x + 4, pos.y + 4), ImVec2(max.x - 4, max.y - 4));
        }
        else
        {
            dl->AddRectFilled(pos, max, U32(Colors::Frame), Metrics::FrameRounding);
            const char *hint = "Drop a texture";
            ImVec2 ts = ImGui::CalcTextSize(hint);
            dl->AddText(ImVec2(std::floor((pos.x + max.x - ts.x) * 0.5f), std::floor((pos.y + max.y - ts.y) * 0.5f)), U32(Colors::TextFaint), hint);
        }
        dl->AddRect(pos, max, U32(hovered ? Colors::BorderStrong : Colors::Border), Metrics::FrameRounding);
        return open;
    }
}

void InspectorPanel::DrawSpriteSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    SIMPEngine::Scene *const selectedScene = m_Context->Scene;
    const entt::entity selectedHandle = m_SelectedEntity.GetHandle();
    EditorContext &ctx = *m_Context;

    DrawComponentSection<SpriteComponent>(ctx, "Sprite", m_SelectedEntity, [this, &ctx, selectedScene, selectedHandle](SpriteComponent &sc)
                                          {
        bool openPicker = DrawTextureSlot(ctx, sc);

        if (sc.texture)
        {
            char info[160];
            std::snprintf(info, sizeof(info), "%s  (%d x %d)", sc.texture->GetVFSPath().c_str(), sc.texture->GetWidth(), sc.texture->GetHeight());
            PropertyValue("", info, &Colors::TextDim);
        }

        PropertyLabel("");
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float half = std::floor((ImGui::GetContentRegionAvail().x - spacing) * 0.5f);
        openPicker |= Button("Browse...", ButtonKind::Secondary, ImVec2(sc.texture ? half : -FLT_MIN, 0));
        if (sc.texture)
        {
            ImGui::SameLine();
            if (Button("Clear", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                sc.texture = nullptr;
            }
        }

        float size[2] = {sc.width, sc.height};
        if (PropertyVec2("Size", size, 0.5f, "%.1f"))
        {
            sc.width = size[0];
            sc.height = size[1];
        }
        if (sc.texture)
        {
            PropertyLabel("");
            if (Button("Use texture size", ButtonKind::Ghost, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                sc.width = static_cast<float>(sc.texture->GetWidth());
                sc.height = static_cast<float>(sc.texture->GetHeight());
            }
        }

        PropertyColor("Tint", sc.tint);
        PropertyBool2("Flip", "X", &sc.flipX, "Y", &sc.flipY);

        if (sc.texture)
        {
            static const char *filters[] = {"Nearest", "Linear"};
            int filter = sc.texture->GetFilter() == SIMPEngine::TextureFilter::Nearest ? 0 : 1;
            if (PropertySegmented("Filter", &filter, filters, 2))
                sc.texture->SetFilter(filter == 0 ? SIMPEngine::TextureFilter::Nearest : SIMPEngine::TextureFilter::Linear);
        }

        if (openPicker)
        {
            m_AssetPicker->Open(
                "Select Texture",
                "assets://",
                [this, selectedScene, selectedHandle](const std::string &vpath)
                {
                    if (m_Context->Scene != selectedScene)
                        return;
                    SIMPEngine::Entity selected(selectedHandle, selectedScene);
                    if (!selected || !selected.HasComponent<SpriteComponent>())
                        return;

                    if (auto texture = EditorCommands::LoadTexture(vpath))
                    {
                        EditorCommands::RecordEdit(*m_Context);
                        selected.GetComponent<SpriteComponent>().texture = texture;
                    }
                },
                {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".webp"});
        } });
}

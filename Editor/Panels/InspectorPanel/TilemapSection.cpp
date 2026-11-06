#include "InspectorPanel.h"

#include <Engine/Rendering/Tileset.h>
#include <Engine/Scene/TilemapUtils.h>

#include <cmath>
#include <cstdio>

namespace
{
    const std::vector<std::string> TilesetExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".webp", ".json", ".tsj"};

    void AcceptTilesetDrop(EditorContext &ctx, SIMPEngine::Entity entity)
    {
        if (!ImGui::BeginDragDropTarget())
            return;

        const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ASSET");
        if (!payload)
            payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM");
        if (payload)
            EditorCommands::AssignTileset(ctx, entity, static_cast<const char *>(payload->Data));
        ImGui::EndDragDropTarget();
    }

    void DrawTilesetSlot(EditorContext &ctx, SIMPEngine::Entity entity, TilemapComponent &tm)
    {
        using namespace EditorUI;

        PropertyLabel("Tileset");
        float width = ImGui::GetContentRegionAvail().x;
        float height = 64.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 max(pos.x + width, pos.y + height);

        ImGui::InvisibleButton("##tileset", ImVec2(width, height));
        bool hovered = ImGui::IsItemHovered();
        AcceptTilesetDrop(ctx, entity);

        ImDrawList *dl = ImGui::GetWindowDrawList();
        auto texture = tm.tileset ? tm.tileset->GetTexture() : nullptr;
        if (texture && texture->GetID())
        {
            DrawCheckerboard(dl, pos, max, 8.0f);
            DrawImageFit(dl, TextureId(texture->GetID()),
                         ImVec2(static_cast<float>(texture->GetWidth()), static_cast<float>(texture->GetHeight())),
                         ImVec2(pos.x + 3, pos.y + 3), ImVec2(max.x - 3, max.y - 3));
        }
        else
        {
            dl->AddRectFilled(pos, max, U32(Colors::Frame), Metrics::FrameRounding);
            const char *hint = "Drop an image or .tsj";
            ImVec2 ts = ImGui::CalcTextSize(hint);
            dl->AddText(ImVec2(std::floor((pos.x + max.x - ts.x) * 0.5f), std::floor((pos.y + max.y - ts.y) * 0.5f)), U32(Colors::TextFaint), hint);
        }
        dl->AddRect(pos, max, U32(hovered ? Colors::BorderStrong : Colors::Border), Metrics::FrameRounding);

        if (tm.tileset && texture)
        {
            const std::string &source = tm.tileset->GetSourcePath();
            char info[200];
            std::snprintf(info, sizeof(info), "%s  (%d tiles)", source.empty() ? texture->GetVFSPath().c_str() : source.c_str(), tm.tileset->GetTileCount());
            PropertyValue("", info, &Colors::TextDim);
        }
    }
}

void InspectorPanel::DrawTilemapSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    SIMPEngine::Scene *const selectedScene = m_Context->Scene;
    const entt::entity selectedHandle = m_SelectedEntity.GetHandle();
    EditorContext &ctx = *m_Context;
    SIMPEngine::Entity entity = m_SelectedEntity;

    DrawComponentSection<TilemapComponent>(ctx, "Tilemap", m_SelectedEntity, [this, &ctx, entity, selectedScene, selectedHandle](TilemapComponent &tm)
                                           {
        DrawTilesetSlot(ctx, entity, tm);

        PropertyLabel("");
        if (Button("Browse...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            m_AssetPicker->Open(
                "Select Tileset",
                "assets://",
                [this, selectedScene, selectedHandle](const std::string &vpath)
                {
                    if (m_Context->Scene != selectedScene)
                        return;
                    SIMPEngine::Entity selected(selectedHandle, selectedScene);
                    if (selected)
                        EditorCommands::AssignTileset(*m_Context, selected, vpath);
                },
                TilesetExtensions);
        }

        if (tm.tileset && tm.tileset->GetTexture())
        {
            int size[2] = {tm.tileset->GetTileWidth(), tm.tileset->GetTileHeight()};
            int margin = tm.tileset->GetMargin();
            int spacing = tm.tileset->GetSpacing();
            bool changed = PropertyInt2("Tile Pixels", size, 0.25f, 1, 4096);
            changed |= PropertyInt("Margin", &margin, 0.1f, 0, 1024);
            changed |= PropertyInt("Spacing", &spacing, 0.1f, 0, 1024);
            if (changed)
            {
                auto rebuilt = std::make_shared<SIMPEngine::Tileset>(tm.tileset->GetTexture(), std::max(1, size[0]), std::max(1, size[1]), std::max(0, margin), std::max(0, spacing));
                tm.tileset = rebuilt;
            }
        }

        PropertyFloat("Cell Size", &tm.tileSize, 0.5f, 1.0f, 4096.0f, "%.1f");
        if (tm.tileset)
        {
            PropertyLabel("");
            if (Button("Match tile pixels", ButtonKind::Ghost, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                tm.tileSize = static_cast<float>(tm.tileset->GetTileWidth());
            }
        }

        PropertyColor("Tint", tm.tint);
        PropertyBool("Visible", &tm.visible);
        PropertyBool("Solid", &tm.solid);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Solid tiles are reported by IsSolidAt / OverlapsSolid for gameplay collision");

        char stats[64];
        std::snprintf(stats, sizeof(stats), "%d", SIMPEngine::CountTiles(tm));
        PropertyValue("Tiles", stats);
        glm::ivec2 lo, hi;
        if (SIMPEngine::GetUsedBounds(tm, lo, hi))
        {
            std::snprintf(stats, sizeof(stats), "%d x %d  from (%d, %d)", hi.x - lo.x + 1, hi.y - lo.y + 1, lo.x, lo.y);
            PropertyValue("Bounds", stats, &Colors::TextDim);
        }

        PropertyLabel("");
        float spacingX = ImGui::GetStyle().ItemSpacing.x;
        float half = std::floor((ImGui::GetContentRegionAvail().x - spacingX) * 0.5f);
        bool painting = ctx.Tiles.Enabled;
        ImGui::BeginDisabled(!ctx.IsEditing());
        if (Button(painting ? "Stop Painting" : "Paint Tiles", painting ? ButtonKind::Primary : ButtonKind::Secondary, ImVec2(half, 0)))
        {
            ctx.Tiles.Enabled = !painting;
            if (ctx.Tiles.Enabled)
                ctx.Tiles.FocusPalette = true;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (Button("Clear", ButtonKind::Danger, ImVec2(-FLT_MIN, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            tm.chunks.clear();
        } });
}

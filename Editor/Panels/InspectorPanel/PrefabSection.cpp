#include "InspectorPanel.h"

#include <Engine/Scene/Prefab.h>

void InspectorPanel::DrawPrefabSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;
    if (!m_SelectedEntity.HasComponent<PrefabInstanceComponent>())
        return;

    entt::entity updated = entt::null;
    bool changed = false;
    DrawComponentSection<PrefabInstanceComponent>(ctx, "Prefab", m_SelectedEntity, [&](PrefabInstanceComponent &link)
                                                  {
        PropertyValue("Source", link.path.c_str());
        PropertyLabel("");
        if (Button("Apply to Prefab", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            EditorCommands::ApplyToPrefab(ctx, m_SelectedEntity);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Write this entity and its children back to the prefab file");
        PropertyLabel("");
        if (Button("Update from Prefab", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            updated = m_SelectedEntity.GetHandle();
            changed = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Replace this instance with the current prefab file, keeping its position, rotation and scale");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("An instance is a copy. Edit it and press Apply to Prefab, then use Update from Prefab on the other instances");
        ImGui::PopStyleColor(); });

    if (changed && updated != entt::null)
        EditorCommands::UpdateFromPrefab(ctx, SIMPEngine::Entity(updated, ctx.Scene));
}

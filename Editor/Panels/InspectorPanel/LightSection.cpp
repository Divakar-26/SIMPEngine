#include "InspectorPanel.h"

#include <Engine/Scene/Systems/LightingSystem.h>

#include <cmath>

void InspectorPanel::DrawLightSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;
    SIMPEngine::Scene *scene = m_Context->Scene;

    DrawComponentSection<LightComponent>(ctx, "Light", m_SelectedEntity, [&ctx, scene](LightComponent &light)
                                         {
        if (!scene->GetLighting().Enabled)
        {
            PropertyLabel("");
            if (Button("Enable Scene Lighting", ButtonKind::Primary, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                scene->GetLighting().Enabled = true;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Lights only show when lighting is on for the scene");
        }

        static const char *types[] = {"Point", "Spot"};
        int type = static_cast<int>(light.type);
        if (PropertySegmented("Type", &type, types, 2))
            light.type = static_cast<LightType>(type);

        PropertyBool("Enabled", &light.enabled);
        PropertyColor("Color", light.color);
        PropertyFloat("Intensity", &light.intensity, 0.01f, 0.0f, 8.0f, "%.2f");
        PropertyFloat("Radius", &light.radius, 1.0f, 1.0f, 100000.0f, "%.0f");
        PropertyFloat("Falloff", &light.falloff, 0.01f, 0.1f, 10.0f, "%.2f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Below 1 keeps light bright to the edge, above 1 concentrates it at the center");

        if (light.type == LightType::Spot)
        {
            PropertyFloat("Cone", &light.outerAngle, 0.25f, 1.0f, 180.0f, "± %.0f°");
            light.innerAngle = std::min(light.innerAngle, light.outerAngle);
            PropertyFloat("Hard Core", &light.innerAngle, 0.25f, 0.0f, light.outerAngle, "± %.0f°");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Angle that stays fully lit before the cone edge softens. The entity rotation aims the cone");
        }

        SectionLabel("Flicker");
        PropertyFloat("Amount", &light.flicker, 0.005f, 0.0f, 1.0f, "%.2f");
        if (light.flicker > 0.0f)
            PropertyFloat("Speed", &light.flickerSpeed, 0.1f, 0.0f, 100.0f, "%.1f");

        PropertyLabel("");
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        float h = std::floor(w * 0.45f);
        ImDrawList *dl = ImGui::GetWindowDrawList();
        auto texture = SIMPEngine::LightingSystem::GetLightTexture(light);
        ImVec2 max(pos.x + w, pos.y + h);
        dl->AddRectFilled(pos, max, IM_COL32(12, 12, 16, 255), Metrics::FrameRounding);
        if (texture && texture->GetID())
        {
            float side = h - 8.0f;
            ImVec2 c(std::floor(pos.x + w * 0.5f), std::floor(pos.y + h * 0.5f));
            ImU32 tint = IM_COL32(light.color.r, light.color.g, light.color.b, 255);
            dl->AddImage(TextureId(texture->GetID()), ImVec2(c.x - side * 0.5f, c.y - side * 0.5f), ImVec2(c.x + side * 0.5f, c.y + side * 0.5f), ImVec2(0, 1), ImVec2(1, 0), tint);
        }
        dl->AddRect(pos, max, U32(Colors::Border), Metrics::FrameRounding);
        ImGui::Dummy(ImVec2(w, h)); });

    DrawComponentSection<GlowComponent>(ctx, "Glow", m_SelectedEntity, [](GlowComponent &glow)
                                        {
        PropertyBool("Enabled", &glow.enabled);
        PropertyColor("Color", glow.color);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Multiplies the entity's own colors; white keeps them");
        PropertyFloat("Intensity", &glow.intensity, 0.01f, 0.0f, 20.0f, "%.2f");
        PropertyFloat("Spread", &glow.spread, 0.01f, 0.1f, 4.0f, "%.2f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Halo size. Glowing entities share one blur, so the largest spread in view is used"); });

    DrawComponentSection<ParallaxComponent>(ctx, "Parallax", m_SelectedEntity, [](ParallaxComponent &parallax)
                                            {
        PropertyVec2("Factor", &parallax.factor.x, 0.01f, "%.2f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("How fast this layer follows the camera: 0 = fixed to the screen, 1 = normal world, below 1 = far background, above 1 = foreground");
        static const char *presets[] = {"Sky", "Far", "Mid", "Near"};
        int current = -1;
        PropertyLabel("Presets");
        if (SegmentedControl("##parallaxpresets", &current, presets, 4))
        {
            NotifyEdit();
            const float factors[] = {0.0f, 0.25f, 0.5f, 0.8f};
            parallax.factor = glm::vec2(factors[current]);
        } });
}

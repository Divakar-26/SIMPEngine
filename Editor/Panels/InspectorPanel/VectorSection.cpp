#include "InspectorPanel.h"
#include "../AssetPickerPanel.h"

#include <Engine/Vector/VectorPath.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
    enum class Preset
    {
        Circle,
        Rect,
        Rounded,
        Hexagon,
        Star,
        Heart
    };

    const char *PresetNames[] = {"Circle", "Rect", "Rounded", "Hexagon", "Star", "Heart"};

    SIMPEngine::VectorPath MakePreset(Preset preset, float size)
    {
        using SIMPEngine::VectorPath;
        const float r = size * 0.5f;
        switch (preset)
        {
        case Preset::Circle:
            return VectorPath::Circle(r);
        case Preset::Rect:
            return VectorPath::Rect(size, size);
        case Preset::Rounded:
            return VectorPath::Rect(size, size, size * 0.2f);
        case Preset::Hexagon:
            return VectorPath::Polygon(6, r);
        case Preset::Star:
            return VectorPath::Star(5, r, r * 0.45f);
        case Preset::Heart:
            return VectorPath::Heart(size);
        }
        return VectorPath::Circle(r);
    }

    float PathSize(const SIMPEngine::VectorPath &path)
    {
        glm::vec2 min, max;
        if (!path.GetBounds(min, max))
            return 100.0f;
        return std::max({max.x - min.x, max.y - min.y, 1.0f});
    }

    int PresetRow(const char *id)
    {
        int chosen = -1;
        ImGui::PushID(id);
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float width = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;
        for (int i = 0; i < 6; ++i)
        {
            if (i % 3 != 0)
                ImGui::SameLine();
            if (EditorUI::Button(PresetNames[i], EditorUI::ButtonKind::Secondary, ImVec2(width, 0)))
                chosen = i;
        }
        ImGui::PopID();
        return chosen;
    }

    size_t NodeCount(const SIMPEngine::VectorPath &path)
    {
        size_t count = 0;
        for (const auto &contour : path.Contours)
            count += contour.Nodes.size();
        return count;
    }
}

void InspectorPanel::DrawVectorSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;
    EditorContext &ctx = *m_Context;
    SIMPEngine::Scene *scene = ctx.Scene;
    const entt::entity handle = m_SelectedEntity.GetHandle();

    DrawComponentSection<VectorShapeComponent>(ctx, "Vector Shape", m_SelectedEntity, [this, &ctx, scene, handle](VectorShapeComponent &v)
                                                {
        SectionLabel("Shape");
        const int preset = PresetRow("shape");
        if (preset >= 0)
        {
            EditorCommands::RecordEdit(ctx);
            v.path = MakePreset(static_cast<Preset>(preset), PathSize(v.path));
        }
        PropertyLabel("");
        if (Button("Load SVG...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_AssetPicker)
        {
            m_AssetPicker->Open("Select SVG", "assets://", [&ctx, scene, handle](const std::string &vpath)
                                {
                if (!scene->GetRegistry().valid(handle))
                    return;
                auto *target = scene->GetRegistry().try_get<VectorShapeComponent>(handle);
                if (!target)
                    return;
                SIMPEngine::VectorPath path;
                SIMPEngine::VectorStyleHint hint;
                if (!path.LoadSvgFile(vpath, PathSize(target->path), &hint))
                {
                    LOG_ERROR("Could not read SVG {}", vpath);
                    return;
                }
                EditorCommands::RecordEdit(ctx);
                target->path = std::move(path);
                if (hint.HasFill)
                {
                    target->fill = true;
                    target->fillColor = hint.Fill;
                }
                if (hint.HasStroke)
                {
                    target->stroke = true;
                    target->strokeColor = hint.Stroke;
                    target->strokeWidth = hint.StrokeWidth;
                } }, {".svg"});
        }
        char info[64];
        std::snprintf(info, sizeof(info), "%zu contours, %zu points", v.path.Contours.size(), NodeCount(v.path));
        PropertyValue("Path", info, &Colors::TextDim);

        SectionLabel("Fill");
        PropertyBool("Fill", &v.fill);
        if (v.fill)
        {
            PropertyBool("Gradient", &v.gradient);
            if (v.gradient)
            {
                PropertyColor("From Color", v.gradientFromColor);
                PropertyColor("To Color", v.gradientToColor);
                PropertyVec2("From", &v.gradientFrom.x, 1.0f, "%.0f");
                PropertyVec2("To", &v.gradientTo.x, 1.0f, "%.0f");
            }
            else
                PropertyColor("Color", v.fillColor);
        }

        SectionLabel("Stroke");
        PropertyBool("Stroke", &v.stroke);
        if (v.stroke)
        {
            PropertyColor("Stroke Color", v.strokeColor);
            PropertyFloat("Width", &v.strokeWidth, 0.1f, 0.0f, 500.0f, "%.1f");
            static const char *caps[] = {"Butt", "Round", "Square"};
            int cap = static_cast<int>(v.cap);
            if (PropertySegmented("Cap", &cap, caps, 3))
                v.cap = static_cast<SIMPEngine::StrokeCap>(cap);
            static const char *joins[] = {"Miter", "Round", "Bevel"};
            int join = static_cast<int>(v.join);
            if (PropertySegmented("Join", &join, joins, 3))
                v.join = static_cast<SIMPEngine::StrokeJoin>(join);
            float trim[2] = {v.trimStart, v.trimEnd};
            if (PropertyRange("Trim", trim, 0.005f, "%.2f", 0.0f, 1.0f))
            {
                v.trimStart = trim[0];
                v.trimEnd = trim[1];
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Draws only this part of the outline. Animate Trim End from 0 to 1 to make the line draw itself");
            PropertyFloat("Trim Offset", &v.trimOffset, 0.005f, -1.0f, 1.0f, "%.2f");
            PropertyFloat("Dash", &v.dashLength, 0.2f, 0.0f, 1000.0f, "%.1f");
            if (v.dashLength > 0.0f)
            {
                PropertyFloat("Gap", &v.dashGap, 0.2f, 0.0f, 1000.0f, "%.1f");
                PropertyFloat("Dash Offset", &v.dashOffset, 0.5f, 0.0f, 0.0f, "%.1f");
            }
        }

        SectionLabel("Morph");
        if (v.morphTarget.Empty())
            PropertyValue("Target", "None", &Colors::TextFaint);
        else
        {
            PropertyFloat("Amount", &v.morph, 0.005f, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("0 = this shape, 1 = the target. Animate it with Tween.To(e, {morph = 1}, ...) or vector:MorphTo(path, seconds)");
        }
        const int target = PresetRow("target");
        if (target >= 0)
        {
            EditorCommands::RecordEdit(ctx);
            v.morphTarget = MakePreset(static_cast<Preset>(target), PathSize(v.path));
            if (v.morph == 0.0f)
                v.morph = 0.5f;
        }
        if (!v.morphTarget.Empty())
        {
            PropertyLabel("");
            if (Button("Clear Target", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                v.morphTarget.Clear();
                v.morph = 0.0f;
            }
        } });
}

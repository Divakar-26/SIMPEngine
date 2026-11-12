#include "InspectorPanel.h"

#include <cmath>
#include <cstring>

namespace
{
    struct AnchorPreset
    {
        const char *Name;
        glm::vec2 Min;
        glm::vec2 Max;
        glm::vec2 Pivot;
    };

    const AnchorPreset AnchorPresets[] = {
        {"Center", {0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}},
        {"Top Left", {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}},
        {"Top", {0.5f, 1.0f}, {0.5f, 1.0f}, {0.5f, 1.0f}},
        {"Top Right", {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}},
        {"Left", {0.0f, 0.5f}, {0.0f, 0.5f}, {0.0f, 0.5f}},
        {"Right", {1.0f, 0.5f}, {1.0f, 0.5f}, {1.0f, 0.5f}},
        {"Bottom Left", {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
        {"Bottom", {0.5f, 0.0f}, {0.5f, 0.0f}, {0.5f, 0.0f}},
        {"Bottom Right", {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}},
        {"Stretch Width", {0.0f, 0.5f}, {1.0f, 0.5f}, {0.5f, 0.5f}},
        {"Stretch Height", {0.5f, 0.0f}, {0.5f, 1.0f}, {0.5f, 0.5f}},
        {"Stretch All", {0.0f, 0.0f}, {1.0f, 1.0f}, {0.5f, 0.5f}}};

    int CurrentPreset(const UIRectComponent &rect)
    {
        for (int i = 0; i < static_cast<int>(sizeof(AnchorPresets) / sizeof(AnchorPresets[0])); ++i)
            if (rect.anchorMin == AnchorPresets[i].Min && rect.anchorMax == AnchorPresets[i].Max && rect.pivot == AnchorPresets[i].Pivot)
                return i;
        return -1;
    }

    bool AcceptTexture(EditorContext &ctx, UIImageComponent &image)
    {
        bool accepted = false;
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ASSET"))
            {
                if (auto texture = EditorCommands::LoadTexture(static_cast<const char *>(payload->Data)))
                {
                    EditorCommands::RecordEdit(ctx);
                    image.texture = texture;
                    accepted = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return accepted;
    }
}

void InspectorPanel::DrawUISection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;

    DrawComponentSection<CanvasComponent>(ctx, "Canvas", m_SelectedEntity, [](CanvasComponent &canvas)
                                          {
        PropertyVec2("Reference Size", &canvas.referenceSize.x, 1.0f, "%.0f", 16.0f, 16384.0f);
        static const char *modes[] = {"Constant Pixel", "Scale With Screen"};
        PropertySegmented("Scale Mode", &canvas.scaleMode, modes, 2);
        if (canvas.scaleMode == 1)
            PropertyFloat("Match Width/Height", &canvas.match, 0.01f, 0.0f, 1.0f, "%.2f");
        PropertyFloat("Safe Area", &canvas.safeArea, 0.005f, 0.0f, 0.4f, "%.3f");
        PropertyInt("Sort Order", &canvas.sortOrder, 1.0f, -100, 100);
        PropertyBool("Visible", &canvas.visible);
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("UI nodes under this entity are drawn on the screen after the world. In the Viewport the canvas is shown at its reference size beside the origin; play to see it on screen");
        ImGui::PopStyleColor(); });

    DrawComponentSection<UIRectComponent>(ctx, "UI Rect", m_SelectedEntity, [](UIRectComponent &rect)
                                          {
        const int preset = CurrentPreset(rect);
        PropertyLabel("Anchor Preset");
        if (ImGui::BeginCombo("##anchorPreset", preset >= 0 ? AnchorPresets[preset].Name : "Custom"))
        {
            for (int i = 0; i < static_cast<int>(sizeof(AnchorPresets) / sizeof(AnchorPresets[0])); ++i)
            {
                if (ImGui::Selectable(AnchorPresets[i].Name, i == preset))
                {
                    EditorUI::NotifyEdit();
                    rect.anchorMin = AnchorPresets[i].Min;
                    rect.anchorMax = AnchorPresets[i].Max;
                    rect.pivot = AnchorPresets[i].Pivot;
                    rect.position = {0.0f, 0.0f};
                    rect.margin = {0.0f, 0.0f, 0.0f, 0.0f};
                }
            }
            ImGui::EndCombo();
        }
        PropertyVec2("Anchor Min", &rect.anchorMin.x, 0.01f, "%.2f", 0.0f, 1.0f);
        PropertyVec2("Anchor Max", &rect.anchorMax.x, 0.01f, "%.2f", 0.0f, 1.0f);
        PropertyVec2("Pivot", &rect.pivot.x, 0.01f, "%.2f", 0.0f, 1.0f);
        const bool stretchX = rect.anchorMin.x != rect.anchorMax.x;
        const bool stretchY = rect.anchorMin.y != rect.anchorMax.y;
        if (!stretchX || !stretchY)
            PropertyVec2("Position", &rect.position.x, 0.5f, "%.1f");
        if (!stretchX || !stretchY)
            PropertyVec2("Size", &rect.size.x, 0.5f, "%.1f", 0.0f, 16384.0f);
        if (stretchX)
        {
            PropertyFloat("Margin Left", &rect.margin.x, 0.5f, -16384.0f, 16384.0f, "%.1f");
            PropertyFloat("Margin Right", &rect.margin.z, 0.5f, -16384.0f, 16384.0f, "%.1f");
        }
        if (stretchY)
        {
            PropertyFloat("Margin Bottom", &rect.margin.y, 0.5f, -16384.0f, 16384.0f, "%.1f");
            PropertyFloat("Margin Top", &rect.margin.w, 0.5f, -16384.0f, 16384.0f, "%.1f");
        }
        PropertyBool("Visible", &rect.visible);
        PropertyBool("Blocks Input", &rect.blocksInput);
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("Canvas units, y up. A parent with a UI Layout positions its children and ignores their anchors");
        ImGui::PopStyleColor(); });

    DrawComponentSection<UIPanelComponent>(ctx, "UI Panel", m_SelectedEntity, [](UIPanelComponent &panel)
                                           {
        PropertyColor("Color", panel.color);
        PropertyFloat("Corner Radius", &panel.cornerRadius, 0.25f, 0.0f, 512.0f, "%.1f");
        PropertyFloat("Border Width", &panel.borderWidth, 0.1f, 0.0f, 64.0f, "%.1f");
        if (panel.borderWidth > 0.0f)
            PropertyColor("Border Color", panel.borderColor); });

    DrawComponentSection<UILabelComponent>(ctx, "UI Label", m_SelectedEntity, [](UILabelComponent &label)
                                           {
        PropertyText("Text", label.text, true);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("{name} is replaced by the game variable with that name");
        PropertyFloat("Font Size", &label.size, 0.5f, 1.0f, 512.0f, "%.1f");
        PropertyColor("Color", label.color);
        static const char *horizontal[] = {"Left", "Center", "Right"};
        PropertySegmented("Align", &label.align, horizontal, 3);
        static const char *vertical[] = {"Top", "Middle", "Bottom"};
        PropertySegmented("Vertical", &label.verticalAlign, vertical, 3);
        PropertyBool("Shadow", &label.shadow);
        PropertyText("Font", label.fontPath); });

    DrawComponentSection<UIImageComponent>(ctx, "UI Image", m_SelectedEntity, [&ctx](UIImageComponent &image)
                                           {
        std::string path = image.texture ? image.texture->GetVFSPath() : std::string();
        if (PropertyText("Texture", path))
            image.texture = path.empty() ? nullptr : EditorCommands::LoadTexture(path);
        AcceptTexture(ctx, image);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Drag an image here from the FileSystem panel");
        PropertyColor("Tint", image.tint);
        PropertyBool("Preserve Aspect", &image.preserveAspect);
        PropertyBool("Use Region", &image.useRegion);
        if (image.useRegion)
        {
            PropertyVec2("Region Origin", &image.region.x, 1.0f, "%.0f");
            PropertyVec2("Region Size", &image.region.w, 1.0f, "%.0f");
        } });

    DrawComponentSection<UIButtonComponent>(ctx, "UI Button", m_SelectedEntity, [&m_SelectedEntity](UIButtonComponent &button)
                                            {
        PropertyBool("Enabled", &button.enabled);
        PropertyBool("Focusable", &button.focusable);
        PropertyColor("Hover Tint", button.hoverTint);
        PropertyColor("Press Tint", button.pressTint);
        PropertyColor("Disabled Tint", button.disabledTint);
        PropertyFloat("Hover Scale", &button.hoverScale, 0.005f, 0.1f, 4.0f, "%.2f");
        PropertyFloat("Press Scale", &button.pressScale, 0.005f, 0.1f, 4.0f, "%.2f");
        PropertyText("Hover Sound", button.hoverSound);
        PropertyText("Click Sound", button.clickSound);
        PropertyFloat("Volume", &button.volume, 0.01f, 0.0f, 4.0f, "%.2f");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped(m_SelectedEntity.HasComponent<UIPanelComponent>() || m_SelectedEntity.HasComponent<UIImageComponent>()
                               ? "Scripts receive OnClick, OnHoverEnter and OnHoverExit. Tab (or pad Back) shows the focus ring, arrows or the pad move it, the UIConfirm action clicks"
                               : "Add a UI Panel or UI Image so there is something to click");
        ImGui::PopStyleColor(); });

    DrawComponentSection<UILayoutComponent>(ctx, "UI Layout", m_SelectedEntity, [](UILayoutComponent &layout)
                                            {
        static const char *directions[] = {"Row", "Column"};
        PropertySegmented("Direction", &layout.direction, directions, 2);
        PropertyFloat("Spacing", &layout.spacing, 0.5f, 0.0f, 1024.0f, "%.1f");
        PropertyFloat("Padding Left", &layout.padding.x, 0.5f, 0.0f, 1024.0f, "%.1f");
        PropertyFloat("Padding Bottom", &layout.padding.y, 0.5f, 0.0f, 1024.0f, "%.1f");
        PropertyFloat("Padding Right", &layout.padding.z, 0.5f, 0.0f, 1024.0f, "%.1f");
        PropertyFloat("Padding Top", &layout.padding.w, 0.5f, 0.0f, 1024.0f, "%.1f");
        static const char *justify[] = {"Start", "Center", "End"};
        PropertySegmented("Justify", &layout.justify, justify, 3);
        PropertySegmented("Align", &layout.align, justify, 3);
        PropertyBool("Stretch", &layout.stretch);
        PropertyBool("Enabled", &layout.enabled); });
}

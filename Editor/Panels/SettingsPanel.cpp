#include "SettingsPanel.h"
#include "UI/Widgets.h"

#include <imgui_internal.h>

void SettingsPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Open)
        return;

    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin("Editor Settings", &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }

    PushEditSuppression();
    EditorSettings &s = Settings();
    bool apply = false;
    bool dirty = false;

    if (BeginSection("appearance", "Appearance"))
    {
        static const char *themes[] = {"Graphite", "Godot", "Light"};
        int theme = (int)s.Theme;
        if (PropertySegmented("Theme", &theme, themes, 3))
        {
            s.Theme = (ThemeId)theme;
            apply = true;
        }

        static const char *fonts[] = {"Inter", "JetBrains Mono"};
        int font = (int)s.Font;
        if (PropertySegmented("Interface font", &font, fonts, 2))
        {
            s.Font = (UIFont)font;
            apply = true;
        }

        PropertyLabel("Scale");
        if (ImGui::SliderFloat("##scale", &s.Scale, 0.8f, 1.6f, "%.2fx"))
            dirty = true;
        if (ImGui::IsItemDeactivatedAfterEdit())
            apply = true;
    }
    EndSection();

    if (BeginSection("viewport", "Viewport"))
    {
        dirty |= PropertyBool("Show grid", &s.ShowGrid);
        dirty |= PropertyFloat("Grid size", &s.GridSize, 1.0f, 1.0f, 1024.0f, "%.0f");
        dirty |= PropertyBool("Snap by default", &s.Snap);
        dirty |= PropertyFloat("Snap step", &s.SnapMove, 0.5f, 0.5f, 1024.0f, "%.1f");
        dirty |= PropertyFloat("Rotation step", &s.SnapRotate, 0.5f, 1.0f, 90.0f, "%.0f°");
        dirty |= PropertyBool("Render stats", &s.ShowStats);
    }
    EndSection();

    if (BeginSection("performance", "Performance"))
    {
        if (PropertyBool("VSync", &s.VSync))
            apply = true;
    }
    EndSection();

    if (apply || dirty)
        ImGui::MarkIniSettingsDirty();
    if (apply)
        m_ApplyRequested = true;

    PopEditSuppression();
    ImGui::End();
}

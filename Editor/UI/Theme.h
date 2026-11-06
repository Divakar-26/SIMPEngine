#pragma once

#include <imgui.h>

namespace EditorUI
{
    constexpr ImVec4 Hex(unsigned int rgb, float a = 1.0f)
    {
        return ImVec4(static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f, static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f, static_cast<float>(rgb & 0xFFu) / 255.0f, a);
    }

    namespace Colors
    {
        inline ImVec4 Canvas;
        inline ImVec4 Window;
        inline ImVec4 Header;
        inline ImVec4 HeaderHover;
        inline ImVec4 Frame;
        inline ImVec4 FrameHover;
        inline ImVec4 Border;
        inline ImVec4 BorderStrong;

        inline ImVec4 Text;
        inline ImVec4 TextDim;
        inline ImVec4 TextFaint;
        inline ImVec4 TextOnAccent;

        inline ImVec4 Accent;
        inline ImVec4 AccentHover;
        inline ImVec4 Selection;
        inline ImVec4 Hover;

        inline ImVec4 AxisX;
        inline ImVec4 AxisY;
        inline ImVec4 Warning;
        inline ImVec4 Error;
        inline ImVec4 Success;
        inline ImVec4 NodeTint;
        inline ImVec4 FolderTint;
        inline ImVec4 CameraFrame;
    }

    namespace Metrics
    {
        inline constexpr float FontSize = 14.0f;
        inline constexpr float FrameRounding = 3.0f;
        inline constexpr float RowHeight = 22.0f;
        inline constexpr float StatusBarHeight = 22.0f;
    }

    enum class ThemeId
    {
        Graphite,
        Godot,
        Light
    };

    enum class UIFont
    {
        Inter,
        JetBrainsMono
    };

    enum class FontRole
    {
        Regular,
        Medium,
        Strong,
        Mono
    };

    struct EditorSettings
    {
        ThemeId Theme = ThemeId::Graphite;
        UIFont Font = UIFont::Inter;
        float Scale = 1.0f;
        bool VSync = true;
        bool ShowGrid = true;
        bool ShowStats = false;
        float GridSize = 32.0f;
        bool Snap = false;
        float SnapMove = 8.0f;
        float SnapRotate = 15.0f;
        bool PreviewLighting = true;
        bool ShowColliders = false;
    };

    EditorSettings &Settings();

    void LoadFonts();
    void ApplyTheme(ThemeId theme);
    void ApplySettings();
    void RegisterSettingsHandler();
    const char *ThemeName(ThemeId theme);

    ImFont *GetFont(FontRole role);

    inline ImU32 U32(const ImVec4 &c) { return ImGui::GetColorU32(c); }
    inline ImU32 U32(const ImVec4 &c, float alpha) { return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, c.w * alpha)); }
    inline ImVec4 WithAlpha(const ImVec4 &c, float alpha) { return ImVec4(c.x, c.y, c.z, alpha); }
}

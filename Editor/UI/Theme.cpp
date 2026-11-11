#include "Theme.h"
#include "IconsLucide.h"

#include <imgui_internal.h>

#include <Engine/Application/Application.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace EditorUI
{
    namespace
    {
        EditorSettings s_Settings;

        ImFont *s_Inter[3] = {nullptr, nullptr, nullptr};
        ImFont *s_Mono[2] = {nullptr, nullptr};

        const ImWchar s_IconRange[] = {ICON_MIN_LC, ICON_MAX_LC, 0};

        struct Palette
        {
            unsigned int Canvas, Window, Header, HeaderHover, Frame, FrameHover, Border, BorderStrong;
            unsigned int Text, TextDim, TextFaint, TextOnAccent;
            unsigned int Accent, AccentHover;
            float SelectionAlpha, HoverAlpha;
            unsigned int AxisX, AxisY, Warning, Error, Success, NodeTint, FolderTint, CameraFrame;
            bool Light;
        };

        const Palette s_Graphite = {
            0x181818, 0x232323, 0x2b2b2b, 0x323232, 0x1a1a1a, 0x202020, 0x313131, 0x3d3d3d,
            0xdddddd, 0x9b9b9b, 0x6a6a6a, 0xffffff,
            0x4a8be0, 0x5b9aea,
            0.32f, 0.05f,
            0xe0655f, 0x7cc36e, 0xe6b450, 0xe0655f, 0x7cc36e, 0x8fb3e8, 0x8fa8c8, 0xc39ae6,
            false};

        const Palette s_Godot = {
            0x1b1f26, 0x252b34, 0x2c333e, 0x343c49, 0x1d2229, 0x222831, 0x323a46, 0x3e4756,
            0xe0e3e8, 0xa3abb8, 0x6c7583, 0x10141a,
            0x70bafa, 0x8cc8fb,
            0.28f, 0.05f,
            0xf2716b, 0x8fdf8a, 0xf5c45d, 0xf2716b, 0x8fdf8a, 0x8da5f3, 0x8fa4c4, 0xd59cf5,
            false};

        const Palette s_Light = {
            0xdedede, 0xf4f4f4, 0xe9e9e9, 0xe1e1e1, 0xffffff, 0xfbfbfb, 0xd4d4d4, 0xbdbdbd,
            0x1f1f1f, 0x5c5c5c, 0x9a9a9a, 0xffffff,
            0x0d8ce9, 0x2b9cf0,
            0.20f, 0.06f,
            0xd2463f, 0x3e9d3a, 0xb07a0c, 0xd2463f, 0x3e9d3a, 0x4a6fd4, 0x6b84a8, 0x9b5fc9,
            true};

        const Palette &PaletteFor(ThemeId id)
        {
            switch (id)
            {
            case ThemeId::Godot:
                return s_Godot;
            case ThemeId::Light:
                return s_Light;
            default:
                return s_Graphite;
            }
        }

        ImFont *LoadFamily(const char *textVPath, float size)
        {
            ImGuiIO &io = ImGui::GetIO();

            auto textPath = SIMPEngine::VFS::Resolve(textVPath);
            if (!textPath || !SIMPEngine::FileSystem::Exists(*textPath))
            {
                CORE_WARN("EditorUI: font not found {}", textVPath);
                return nullptr;
            }

            ImFontConfig textCfg;
            textCfg.GlyphExcludeRanges = s_IconRange;
            ImFont *font = io.Fonts->AddFontFromFileTTF(textPath->c_str(), size, &textCfg);
            if (!font)
                return nullptr;

            auto iconPath = SIMPEngine::VFS::Resolve("engine://fonts/lucide.ttf");
            if (iconPath && SIMPEngine::FileSystem::Exists(*iconPath))
            {
                ImFontConfig iconCfg;
                iconCfg.MergeMode = true;
                iconCfg.PixelSnapH = true;
                iconCfg.GlyphOffset = ImVec2(0.0f, 2.0f);
                iconCfg.GlyphMinAdvanceX = size;
                io.Fonts->AddFontFromFileTTF(iconPath->c_str(), size * 0.95f, &iconCfg, s_IconRange);
            }

            return font;
        }

        void *SettingsReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
        {
            return std::strcmp(name, "Editor") == 0 ? static_cast<void *>(&s_Settings) : nullptr;
        }

        void SettingsReadLine(ImGuiContext *, ImGuiSettingsHandler *, void *, const char *line)
        {
            int i = 0;
            float f = 0.0f;
            if (std::strncmp(line, "ScriptTheme=", 12) == 0)
                s_Settings.ScriptTheme = line + 12;
            else if (std::sscanf(line, "ScriptFontSize=%f", &f) == 1)
                s_Settings.ScriptFontSize = std::clamp(f, 9.0f, 32.0f);
            else if (std::sscanf(line, "Theme=%d", &i) == 1)
                s_Settings.Theme = (ThemeId)std::clamp(i, 0, 2);
            else if (std::sscanf(line, "Font=%d", &i) == 1)
                s_Settings.Font = (UIFont)std::clamp(i, 0, 1);
            else if (std::sscanf(line, "Scale=%f", &f) == 1)
                s_Settings.Scale = std::clamp(f, 0.75f, 2.0f);
            else if (std::sscanf(line, "VSync=%d", &i) == 1)
                s_Settings.VSync = i != 0;
            else if (std::sscanf(line, "Grid=%d", &i) == 1)
                s_Settings.ShowGrid = i != 0;
            else if (std::sscanf(line, "Stats=%d", &i) == 1)
                s_Settings.ShowStats = i != 0;
            else if (std::sscanf(line, "GridSize=%f", &f) == 1)
                s_Settings.GridSize = std::clamp(f, 1.0f, 4096.0f);
            else if (std::sscanf(line, "Snap=%d", &i) == 1)
                s_Settings.Snap = i != 0;
            else if (std::sscanf(line, "SnapMove=%f", &f) == 1)
                s_Settings.SnapMove = std::clamp(f, 0.01f, 4096.0f);
            else if (std::sscanf(line, "SnapRotate=%f", &f) == 1)
                s_Settings.SnapRotate = std::clamp(f, 0.1f, 180.0f);
            else if (std::sscanf(line, "Lighting=%d", &i) == 1)
                s_Settings.PreviewLighting = i != 0;
            else if (std::sscanf(line, "Colliders=%d", &i) == 1)
                s_Settings.ShowColliders = i != 0;
            else if (std::sscanf(line, "PlayFullscreen=%d", &i) == 1)
                s_Settings.PlayFullscreen = i != 0;
        }

        void SettingsApplyAll(ImGuiContext *, ImGuiSettingsHandler *)
        {
            ApplySettings();
        }

        void SettingsWriteAll(ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
        {
            const EditorSettings &s = s_Settings;
            buf->appendf("[%s][Editor]\n", handler->TypeName);
            buf->appendf("Theme=%d\nFont=%d\nScale=%.2f\nVSync=%d\nGrid=%d\nStats=%d\nGridSize=%.2f\nSnap=%d\nSnapMove=%.2f\nSnapRotate=%.2f\nLighting=%d\nColliders=%d\n",
                         static_cast<int>(s.Theme), static_cast<int>(s.Font), s.Scale, s.VSync ? 1 : 0, s.ShowGrid ? 1 : 0, s.ShowStats ? 1 : 0,
                         s.GridSize, s.Snap ? 1 : 0, s.SnapMove, s.SnapRotate, s.PreviewLighting ? 1 : 0, s.ShowColliders ? 1 : 0);
            buf->appendf("PlayFullscreen=%d\n", s.PlayFullscreen ? 1 : 0);
            buf->appendf("ScriptTheme=%s\nScriptFontSize=%.1f\n\n", s.ScriptTheme.c_str(), s.ScriptFontSize);
        }
    }

    EditorSettings &Settings()
    {
        return s_Settings;
    }

    const char *ThemeName(ThemeId theme)
    {
        switch (theme)
        {
        case ThemeId::Godot:
            return "Godot";
        case ThemeId::Light:
            return "Light";
        default:
            return "Graphite";
        }
    }

    void RegisterSettingsHandler()
    {
        if (ImGui::FindSettingsHandler("SIMPEditor"))
            return;
        ImGuiSettingsHandler handler;
        handler.TypeName = "SIMPEditor";
        handler.TypeHash = ImHashStr("SIMPEditor");
        handler.ReadOpenFn = SettingsReadOpen;
        handler.ReadLineFn = SettingsReadLine;
        handler.ApplyAllFn = SettingsApplyAll;
        handler.WriteAllFn = SettingsWriteAll;
        ImGui::AddSettingsHandler(&handler);
    }

    void LoadFonts()
    {
        ImGuiIO &io = ImGui::GetIO();
        if (s_Inter[0])
        {
            io.FontDefault = GetFont(FontRole::Regular);
            return;
        }

        s_Inter[0] = LoadFamily("engine://fonts/Inter-Regular.ttf", Metrics::FontSize);
        s_Inter[1] = LoadFamily("engine://fonts/Inter-Medium.ttf", Metrics::FontSize);
        s_Inter[2] = LoadFamily("engine://fonts/Inter-SemiBold.ttf", Metrics::FontSize);
        s_Mono[0] = LoadFamily("engine://fonts/JetBrainsMonoNerdFont-Regular.ttf", Metrics::FontSize - 1.0f);
        s_Mono[1] = LoadFamily("engine://fonts/JetBrainsMonoNerdFont-Bold.ttf", Metrics::FontSize - 1.0f);

        ImFont *fallback = io.FontDefault ? io.FontDefault : io.Fonts->AddFontDefault();
        for (auto &f : s_Inter)
            if (!f)
                f = s_Mono[0] ? s_Mono[0] : fallback;
        for (auto &f : s_Mono)
            if (!f)
                f = s_Inter[0];

        io.FontDefault = GetFont(FontRole::Regular);
    }

    ImFont *GetFont(FontRole role)
    {
        bool mono = s_Settings.Font == UIFont::JetBrainsMono;
        ImFont *font = nullptr;
        switch (role)
        {
        case FontRole::Regular:
            font = mono ? s_Mono[0] : s_Inter[0];
            break;
        case FontRole::Medium:
            font = mono ? s_Mono[0] : s_Inter[1];
            break;
        case FontRole::Strong:
            font = mono ? s_Mono[1] : s_Inter[2];
            break;
        case FontRole::Mono:
            font = s_Mono[0];
            break;
        }
        return font ? font : ImGui::GetFont();
    }

    void ApplySettings()
    {
        ApplyTheme(s_Settings.Theme);

        ImGuiIO &io = ImGui::GetIO();
        io.FontDefault = GetFont(FontRole::Regular);
        ImGui::GetStyle().FontScaleMain = s_Settings.Scale;

        SIMPEngine::Application::Get().GetWindow().SetVSync(s_Settings.VSync);
    }

    void ApplyTheme(ThemeId theme)
    {
        const Palette &p = PaletteFor(theme);

        Colors::Canvas = Hex(p.Canvas);
        Colors::Window = Hex(p.Window);
        Colors::Header = Hex(p.Header);
        Colors::HeaderHover = Hex(p.HeaderHover);
        Colors::Frame = Hex(p.Frame);
        Colors::FrameHover = Hex(p.FrameHover);
        Colors::Border = Hex(p.Border);
        Colors::BorderStrong = Hex(p.BorderStrong);
        Colors::Text = Hex(p.Text);
        Colors::TextDim = Hex(p.TextDim);
        Colors::TextFaint = Hex(p.TextFaint);
        Colors::TextOnAccent = Hex(p.TextOnAccent);
        Colors::Accent = Hex(p.Accent);
        Colors::AccentHover = Hex(p.AccentHover);
        Colors::Selection = Hex(p.Accent, p.SelectionAlpha);
        Colors::Hover = p.Light ? ImVec4(0, 0, 0, p.HoverAlpha) : ImVec4(1, 1, 1, p.HoverAlpha);
        Colors::AxisX = Hex(p.AxisX);
        Colors::AxisY = Hex(p.AxisY);
        Colors::Warning = Hex(p.Warning);
        Colors::Error = Hex(p.Error);
        Colors::Success = Hex(p.Success);
        Colors::NodeTint = Hex(p.NodeTint);
        Colors::FolderTint = Hex(p.FolderTint);
        Colors::CameraFrame = Hex(p.CameraFrame);

        ImGuiStyle &s = ImGui::GetStyle();
        float scale = s.FontScaleMain;
        s = ImGuiStyle();
        if (p.Light)
            ImGui::StyleColorsLight(&s);
        else
            ImGui::StyleColorsDark(&s);
        s.FontScaleMain = scale;

        s.FontSizeBase = Metrics::FontSize;
        s._NextFrameFontSizeBase = Metrics::FontSize;

        s.WindowRounding = 0.0f;
        s.ChildRounding = 0.0f;
        s.PopupRounding = 4.0f;
        s.FrameRounding = Metrics::FrameRounding;
        s.GrabRounding = 2.0f;
        s.TabRounding = 3.0f;
        s.ScrollbarRounding = 3.0f;

        s.WindowBorderSize = 1.0f;
        s.ChildBorderSize = 1.0f;
        s.PopupBorderSize = 1.0f;
        s.FrameBorderSize = 0.0f;
        s.TabBorderSize = 0.0f;
        s.TabBarBorderSize = 1.0f;
        s.TabBarOverlineSize = 1.0f;
        s.SeparatorTextBorderSize = 1.0f;

        s.WindowPadding = ImVec2(8, 8);
        s.FramePadding = ImVec2(6, 3);
        s.CellPadding = ImVec2(4, 2);
        s.ItemSpacing = ImVec2(6, 4);
        s.ItemInnerSpacing = ImVec2(4, 4);
        s.IndentSpacing = 14.0f;
        s.ScrollbarSize = 10.0f;
        s.GrabMinSize = 8.0f;
        s.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        s.WindowMenuButtonPosition = ImGuiDir_None;
        s.SelectableTextAlign = ImVec2(0.0f, 0.5f);

        s.DockingSeparatorSize = 2.0f;
        s.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesToNodes;
        s.TreeLinesSize = 1.0f;

        s.AntiAliasedLines = true;
        s.AntiAliasedFill = true;
        s.CircleTessellationMaxError = 0.10f;

        ImVec4 *c = s.Colors;

        c[ImGuiCol_Text] = Colors::Text;
        c[ImGuiCol_TextDisabled] = Colors::TextFaint;
        c[ImGuiCol_TextLink] = Colors::Accent;

        c[ImGuiCol_WindowBg] = Colors::Window;
        c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg] = Colors::Window;
        c[ImGuiCol_Border] = Colors::Border;
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_FrameBg] = Colors::Frame;
        c[ImGuiCol_FrameBgHovered] = Colors::FrameHover;
        c[ImGuiCol_FrameBgActive] = Colors::FrameHover;

        c[ImGuiCol_TitleBg] = Colors::Canvas;
        c[ImGuiCol_TitleBgActive] = Colors::Canvas;
        c[ImGuiCol_TitleBgCollapsed] = Colors::Canvas;
        c[ImGuiCol_MenuBarBg] = Colors::Canvas;

        c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab] = Colors::BorderStrong;
        c[ImGuiCol_ScrollbarGrabHovered] = Colors::TextFaint;
        c[ImGuiCol_ScrollbarGrabActive] = Colors::TextDim;

        c[ImGuiCol_CheckMark] = Colors::Accent;
        c[ImGuiCol_SliderGrab] = Colors::Accent;
        c[ImGuiCol_SliderGrabActive] = Colors::AccentHover;

        c[ImGuiCol_Button] = Colors::Header;
        c[ImGuiCol_ButtonHovered] = Colors::HeaderHover;
        c[ImGuiCol_ButtonActive] = Colors::BorderStrong;

        c[ImGuiCol_Header] = Colors::Selection;
        c[ImGuiCol_HeaderHovered] = Colors::Hover;
        c[ImGuiCol_HeaderActive] = Colors::Selection;

        c[ImGuiCol_Separator] = Colors::Border;
        c[ImGuiCol_SeparatorHovered] = Colors::Accent;
        c[ImGuiCol_SeparatorActive] = Colors::Accent;

        c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ResizeGripHovered] = WithAlpha(Colors::Accent, 0.5f);
        c[ImGuiCol_ResizeGripActive] = Colors::Accent;

        c[ImGuiCol_InputTextCursor] = Colors::Text;

        c[ImGuiCol_Tab] = Colors::Canvas;
        c[ImGuiCol_TabHovered] = Colors::Header;
        c[ImGuiCol_TabSelected] = Colors::Window;
        c[ImGuiCol_TabSelectedOverline] = Colors::Accent;
        c[ImGuiCol_TabDimmed] = Colors::Canvas;
        c[ImGuiCol_TabDimmedSelected] = Colors::Window;
        c[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_DockingPreview] = WithAlpha(Colors::Accent, 0.30f);
        c[ImGuiCol_DockingEmptyBg] = Colors::Canvas;

        c[ImGuiCol_PlotLines] = Colors::Accent;
        c[ImGuiCol_PlotLinesHovered] = Colors::AccentHover;
        c[ImGuiCol_PlotHistogram] = Colors::Accent;
        c[ImGuiCol_PlotHistogramHovered] = Colors::AccentHover;

        c[ImGuiCol_TableHeaderBg] = Colors::Header;
        c[ImGuiCol_TableBorderStrong] = Colors::Border;
        c[ImGuiCol_TableBorderLight] = Colors::Border;
        c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt] = Colors::Hover;

        c[ImGuiCol_TreeLines] = Colors::Border;
        c[ImGuiCol_TextSelectedBg] = WithAlpha(Colors::Accent, 0.35f);
        c[ImGuiCol_DragDropTarget] = Colors::Accent;
        c[ImGuiCol_NavCursor] = Colors::Accent;
        c[ImGuiCol_NavWindowingHighlight] = WithAlpha(Colors::Text, 0.70f);
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0.35f);
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, p.Light ? 0.25f : 0.45f);
    }
}

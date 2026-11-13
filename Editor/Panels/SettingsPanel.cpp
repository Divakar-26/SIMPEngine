#include "SettingsPanel.h"

#include "ProjectHub/RecentProjects.h"
#include "UI/FileDialogs.h"
#include "UI/Widgets.h"

#include <Engine/Application/Application.h>
#include <Engine/Core/CrashHandler.h>
#include <Engine/Core/Version.h>
#include <Engine/Rendering/GL.h>

#include <SDL3/SDL.h>

#include <imgui_internal.h>

#include <string>

namespace
{
    SDL_Window *Window()
    {
        return SIMPEngine::Application::Get().GetWindow().GetNativeWindow();
    }

    void Hint(const char *text)
    {
        using namespace EditorUI;
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("%s", text);
        ImGui::PopStyleColor();
    }
}

void SettingsPanel::DrawGeneral(bool &apply, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("window", "Window"))
    {
        dirty |= PropertyBool("Remember size", &s.RememberWindow);
        Hint("The editor reopens with the window size (or maximized state) it was closed with.");

        SDL_Window *window = Window();
        int width = 0, height = 0;
        if (window)
            SDL_GetWindowSize(window, &width, &height);
        const std::string size = std::to_string(width) + " x " + std::to_string(height);
        PropertyValue("Current size", size.c_str());

        PropertyLabel("Size presets");
        static const struct
        {
            const char *Name;
            int W, H;
        } presets[] = {{"1280x720", 1280, 720}, {"1600x900", 1600, 900}, {"1920x1080", 1920, 1080}};
        for (int i = 0; i < 3; ++i)
        {
            if (i)
                ImGui::SameLine();
            if (Button(presets[i].Name, ButtonKind::Secondary) && window)
            {
                SDL_RestoreWindow(window);
                SDL_SetWindowSize(window, presets[i].W, presets[i].H);
                SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            }
        }

        PropertyLabel("");
        const bool maximized = window && (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED);
        if (Button(maximized ? "Restore" : "Maximize", ButtonKind::Secondary) && window)
        {
            if (maximized)
                SDL_RestoreWindow(window);
            else
                SDL_MaximizeWindow(window);
        }
        ImGui::SameLine();
        const bool fullscreen = window && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
        if (Button(fullscreen ? "Leave Fullscreen" : "Fullscreen", ButtonKind::Secondary) && window)
            SDL_SetWindowFullscreen(window, !fullscreen);
    }
    EndSection();

    if (BeginSection("performance", "Performance"))
    {
        if (PropertyBool("VSync", &s.VSync))
            apply = true;
        dirty |= PropertyBool("Stats overlay", &s.ShowStats);
    }
    EndSection();

    if (BeginSection("notifications", "Notifications"))
        dirty |= PropertyBool("Status messages", &s.ShowToasts);
    EndSection();
}

void SettingsPanel::DrawAppearance(bool &apply, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("theme", "Theme"))
    {
        static const char *themes[] = {"Graphite", "Godot", "Light"};
        int theme = static_cast<int>(s.Theme);
        if (PropertySegmented("Theme", &theme, themes, 3))
        {
            s.Theme = static_cast<ThemeId>(theme);
            apply = true;
        }

        static const char *fonts[] = {"Inter", "JetBrains Mono"};
        int font = static_cast<int>(s.Font);
        if (PropertySegmented("UI font", &font, fonts, 2))
        {
            s.Font = static_cast<UIFont>(font);
            apply = true;
        }

        PropertyLabel("UI scale");
        if (ImGui::SliderFloat("##scale", &s.Scale, 0.8f, 1.6f, "%.2fx"))
            dirty = true;
        if (ImGui::IsItemDeactivatedAfterEdit())
            apply = true;
        PropertyLabel("");
        if (Button("100%", ButtonKind::Secondary))
        {
            s.Scale = 1.0f;
            apply = true;
        }
        ImGui::SameLine();
        if (Button("125%", ButtonKind::Secondary))
        {
            s.Scale = 1.25f;
            apply = true;
        }
        ImGui::SameLine();
        if (Button("150%", ButtonKind::Secondary))
        {
            s.Scale = 1.5f;
            apply = true;
        }
    }
    EndSection();
}

void SettingsPanel::DrawViewport(bool &, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("grid", "Grid and snapping"))
    {
        dirty |= PropertyBool("Show grid", &s.ShowGrid);
        dirty |= PropertyFloat("Grid size", &s.GridSize, 1.0f, 1.0f, 1024.0f, "%.0f");
        dirty |= PropertyBool("Snap on", &s.Snap);
        dirty |= PropertyFloat("Snap step", &s.SnapMove, 0.5f, 0.5f, 1024.0f, "%.1f");
        dirty |= PropertyFloat("Rotate step", &s.SnapRotate, 0.5f, 1.0f, 90.0f, "%.0f deg");
    }
    EndSection();

    if (BeginSection("camera", "Camera"))
    {
        float percent = (s.ZoomStep - 1.0f) * 100.0f;
        if (PropertyFloat("Zoom step", &percent, 0.5f, 2.0f, 100.0f, "%.0f%%"))
        {
            s.ZoomStep = 1.0f + percent / 100.0f;
            dirty = true;
        }
        dirty |= PropertyBool("Camera frame", &s.ShowCameraFrame);
    }
    EndSection();

    if (BeginSection("overlays", "Overlays"))
    {
        dirty |= PropertyBool("Preview lighting", &s.PreviewLighting);
        dirty |= PropertyBool("Show colliders", &s.ShowColliders);
    }
    EndSection();
}

void SettingsPanel::DrawEditing(bool &, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("recovery", "Recovery"))
    {
        PropertyLabel("Autosave");
        float seconds = s.AutosaveSeconds;
        if (ImGui::DragFloat("##autosave", &seconds, 1.0f, 0.0f, 600.0f, seconds < 1.0f ? "off" : "%.0f s"))
        {
            s.AutosaveSeconds = seconds;
            dirty = true;
        }
        Hint("Unsaved changes are written to a recovery file at this interval and offered back after a crash. 0 turns it off.");
    }
    EndSection();

    if (BeginSection("history", "Undo history"))
    {
        dirty |= PropertyInt("Undo steps", &s.UndoSteps, 1.0f, 5, 1000);
        Hint("Each step stores the whole scene, so very large scenes use more memory with a high number.");
    }
    EndSection();
}

void SettingsPanel::DrawPlay(bool &, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("play", "Play mode"))
    {
        dirty |= PropertyBool("Play fullscreen", &s.PlayFullscreen);
        dirty |= PropertyBool("Pause on error", &s.PausePlayOnError);
        Hint("When off, errors are only logged and the game keeps running.");
    }
    EndSection();
}

void SettingsPanel::DrawScripts(bool &, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("scripts", "Script editor"))
    {
        dirty |= PropertyFloat("Font size", &s.ScriptFontSize, 0.25f, 9.0f, 32.0f, "%.0f");
        dirty |= PropertyInt("Tab width", &s.TabSize, 1.0f, 2, 8);
        PropertyValue("Color theme", s.ScriptTheme.c_str());
        Hint("The color theme is chosen in the Script Editor panel itself.");
    }
    EndSection();
}

void SettingsPanel::DrawAndroid(bool &apply, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();
    (void)apply;

    if (auto folder = TakePickedFolder("androidSdk"))
    {
        s.AndroidSdkPath = *folder;
        dirty = true;
        m_AndroidRescan = true;
    }

    if (BeginSection("androidEnable", "Android"))
    {
        if (PropertyBool("Android tools", &s.AndroidTools))
        {
            dirty = true;
            m_AndroidRescan = true;
        }
        Hint("Optional. Detects the Android SDK on this computer and adds a Run on Device button to the toolbar when adb is available. Turn it off to hide everything Android.");
    }
    EndSection();

    if (BeginSection("androidTools", "Detected tools"))
    {
        const AndroidTools::Toolchain empty;
        const AndroidTools::Toolchain &tools = m_Android ? m_Android->Tools() : empty;
        auto row = [&](const char *label, const std::string &value)
        {
            const bool found = !value.empty();
            const ImVec4 color = found ? Colors::Success : Colors::TextDim;
            PropertyValue(label, found ? value.c_str() : "Not found", &color);
        };
        row("SDK", tools.Sdk.string());
        row("adb", tools.Adb.string());
        row("Emulator", tools.Emulator.string());
        row("NDK", tools.Ndk.empty() ? std::string() : tools.NdkVersion + "  " + tools.Ndk.string());
        row("Java (JDK)", tools.JavaHome.string());
        row("CMake", tools.Cmake.string());
        row("Android project", tools.AndroidProject.string());

        PropertyLabel("");
        std::string summary;
        if (!s.AndroidTools)
            summary = "Android tools are turned off.";
        else if (!tools.CanDeploy())
            summary = "Run on Device needs adb (SDK Platform-Tools).";
        else if (!tools.CanBuild())
        {
            summary = "Run on Device can install; building needs";
            if (tools.Sdk.empty())
                summary += " the SDK,";
            if (tools.JavaHome.empty())
                summary += " a JDK,";
            if (tools.AndroidProject.empty())
                summary += " the engine's Android folder,";
            summary.back() = '.';
        }
        else
            summary = "Ready: build, install and run on devices from the toolbar.";
        ImGui::PushStyleColor(ImGuiCol_Text, tools.CanBuild() && s.AndroidTools ? Colors::Success : Colors::Warning);
        ImGui::TextWrapped("%s", summary.c_str());
        ImGui::PopStyleColor();
    }
    EndSection();

    if (BeginSection("androidLocation", "SDK location"))
    {
        if (PropertyText("SDK folder", s.AndroidSdkPath))
            dirty = true;
        PropertyLabel("");
        if (Button("Browse...", ButtonKind::Secondary))
            PickFolder("androidSdk", s.AndroidSdkPath);
        ImGui::SameLine();
        if (Button("Rescan", ButtonKind::Primary))
            m_AndroidRescan = true;
        ImGui::SameLine();
        if (Button("Clear", ButtonKind::Secondary))
        {
            s.AndroidSdkPath.clear();
            dirty = true;
            m_AndroidRescan = true;
        }
        Hint("Leave empty to search the usual places: ANDROID_HOME, ANDROID_SDK_ROOT, Android Studio's default folders on Windows, Linux and macOS, the Android folder's local.properties and adb on PATH.");
    }
    EndSection();

    if (BeginSection("androidGet", "Get the tools"))
    {
        Hint("To run on a device you only need Platform-Tools (adb). To build APKs from the editor also install an Android SDK with the NDK, CMake and a JDK; Android Studio installs all of them.");
        PropertyLabel("");
        if (Button("Android Studio", ButtonKind::Secondary))
            SDL_OpenURL("https://developer.android.com/studio");
        ImGui::SameLine();
        if (Button("Command-line tools", ButtonKind::Secondary))
            SDL_OpenURL("https://developer.android.com/studio#command-line-tools-only");
        ImGui::SameLine();
        if (Button("Platform-Tools", ButtonKind::Secondary))
            SDL_OpenURL("https://developer.android.com/tools/releases/platform-tools");
    }
    EndSection();
}

void SettingsPanel::DrawMaintenance(bool &apply, bool &dirty)
{
    using namespace EditorUI;
    EditorSettings &s = Settings();

    if (BeginSection("folders", "Folders"))
    {
        const std::string preferences = RecentProjects::PreferenceDirectory().string();
        PropertyValue("Preferences", preferences.c_str());
        PropertyLabel("");
        if (Button("Open Preferences", ButtonKind::Secondary))
            OpenInFileManager(preferences);
        ImGui::SameLine();
        if (Button("Open Crash Reports", ButtonKind::Secondary) && !SIMPEngine::CrashHandler::ReportDirectory().empty())
            OpenInFileManager(SIMPEngine::CrashHandler::ReportDirectory());
        ImGui::SameLine();
        if (Button("Open Recovery", ButtonKind::Secondary))
            OpenInFileManager((RecentProjects::PreferenceDirectory() / "recovery").string());
    }
    EndSection();

    if (BeginSection("reset", "Reset"))
    {
        PropertyLabel("");
        if (Button("Reset Window Layout", ButtonKind::Secondary) && m_ResetLayout)
            m_ResetLayout();
        PropertyLabel("");
        if (!m_ConfirmReset)
        {
            if (Button("Reset All Settings", ButtonKind::Secondary))
                m_ConfirmReset = true;
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
            ImGui::TextUnformatted("Restore every editor setting to its default?");
            ImGui::PopStyleColor();
            PropertyLabel("");
            if (Button("Yes, Reset", ButtonKind::Primary))
            {
                const std::string scriptTheme = s.ScriptTheme;
                s = DefaultSettings();
                s.ScriptTheme = scriptTheme;
                m_AndroidRescan = true;
                apply = true;
                dirty = true;
                m_ConfirmReset = false;
            }
            ImGui::SameLine();
            if (Button("Cancel", ButtonKind::Secondary))
                m_ConfirmReset = false;
        }
    }
    EndSection();

    if (BeginSection("about", "About"))
    {
        const std::string version = std::string(SIMPEngine::EngineName) + " " + SIMPEngine::EngineVersion;
        PropertyValue("Engine", version.c_str());
        const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        const char *glVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
        PropertyValue("Renderer", renderer ? renderer : "unknown");
        PropertyValue("OpenGL", glVersion ? glVersion : "unknown");
        PropertyValue("Video driver", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "unknown");
    }
    EndSection();
}

void SettingsPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Open)
        return;

    ImGui::SetNextWindowSize(ImVec2(820, 560), ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(ImVec2(640, 420), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (!ImGui::Begin("Editor Settings", &m_Open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }

    PushEditSuppression();
    bool apply = false;
    bool dirty = false;

    static const char *categories[] = {"General", "Appearance", "Viewport", "Editing", "Play", "Script Editor", "Android", "Maintenance"};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 12.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Frame);
    ImGui::BeginChild("##settingsNav", ImVec2(190.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    for (int i = 0; i < 8; ++i)
        if (ImGui::Selectable(categories[i], m_Category == i, 0, ImVec2(0, 28.0f)))
            m_Category = i;
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 8.0f));
    ImGui::BeginChild("##settingsBody", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    switch (m_Category)
    {
    case 0:
        DrawGeneral(apply, dirty);
        break;
    case 1:
        DrawAppearance(apply, dirty);
        break;
    case 2:
        DrawViewport(apply, dirty);
        break;
    case 3:
        DrawEditing(apply, dirty);
        break;
    case 4:
        DrawPlay(apply, dirty);
        break;
    case 5:
        DrawScripts(apply, dirty);
        break;
    case 6:
        DrawAndroid(apply, dirty);
        break;
    default:
        DrawMaintenance(apply, dirty);
        break;
    }
    ImGui::EndChild();

    if (apply || dirty)
        ImGui::MarkIniSettingsDirty();
    if (apply)
        m_ApplyRequested = true;

    PopEditSuppression();
    ImGui::End();
}

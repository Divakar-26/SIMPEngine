#include <Engine/UI/ImGuiLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Log.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include "style.h"
#include "Core/FileSystem.h"
#include "Core/VFS.h"

namespace SIMPEngine
{
    ImGuiLayer::ImGuiLayer()
        : Layer("ImGuiLayer")
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }

    void ImGuiLayer::OnAttach()
    {
        CORE_INFO("ImGuiLayer Attached");

        auto &window = Application::Get().GetWindow();
        m_Window = window.GetNativeWindow();
        m_Renderer = window.GetRenderer();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        io.ConfigWindowsMoveFromTitleBarOnly = true;

        io.MouseDrawCursor = false;

        LoadFonts();
        ApplyModernTheme();

        ImGui_ImplSDL3_InitForOpenGL(m_Window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void ImGuiLayer::LoadFonts()
    {
        ImGuiIO &io = ImGui::GetIO();

        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;

        auto fontPath = VFS::Resolve("assets://fonts/JetBrainsMonoNerdFont-Regular.ttf");

        if (fontPath && FileSystem::Exists(*fontPath))
        {
            m_FontRegular = io.Fonts->AddFontFromFileTTF(fontPath->c_str(), EditorStyleMetrics::FontSize, &cfg);
        }

        if (!m_FontRegular)
        {
            CORE_ERROR("ImGuiLayer: font not found at {}, falling back to default font.",
                       fontPath.value_or("<unresolved>"));
            m_FontRegular = io.Fonts->AddFontDefault();
        }

        io.FontDefault = m_FontRegular;
    }

    void ImGuiLayer::ApplyModernTheme()
    {
        ImGuiStyle &s = ImGui::GetStyle();
        ImGui::StyleColorsDark();

        s.WindowRounding    = EditorStyleMetrics::WindowRounding;
        s.ChildRounding     = EditorStyleMetrics::ChildRounding;
        s.PopupRounding     = EditorStyleMetrics::PopupRounding;
        s.FrameRounding     = EditorStyleMetrics::FrameRounding;
        s.GrabRounding      = EditorStyleMetrics::GrabRounding;
        s.TabRounding       = EditorStyleMetrics::TabRounding;
        s.ScrollbarRounding = EditorStyleMetrics::ScrollbarRounding;

        s.WindowBorderSize = 0.0f;
        s.ChildBorderSize  = EditorStyleMetrics::ChildBorder;
        s.PopupBorderSize  = EditorStyleMetrics::PopupBorder;
        s.FrameBorderSize  = EditorStyleMetrics::FrameBorder;
        s.TabBorderSize    = 0.0f;

        s.WindowPadding    = ImVec2(8, 8);
        s.FramePadding     = ImVec2(8, 4);
        s.CellPadding      = ImVec2(6, 4);
        s.ItemSpacing      = ImVec2(8, 6);
        s.ItemInnerSpacing = ImVec2(6, 4);
        s.IndentSpacing    = 18.0f;
        s.ScrollbarSize    = 12.0f;
        s.GrabMinSize      = 10.0f;

        s.WindowMenuButtonPosition = ImGuiDir_None;
        s.AntiAliasedLines = true;
        s.AntiAliasedFill  = true;
        s.CircleTessellationMaxError = 0.10f;

        s.DockingSeparatorSize = 1.0f;

        ImVec4 *c = s.Colors;

        c[ImGuiCol_Text]         = TEXT;
        c[ImGuiCol_TextDisabled] = TEXT_DISABLED;

        c[ImGuiCol_WindowBg] = BG0;
        c[ImGuiCol_ChildBg]  = BG1;
        c[ImGuiCol_PopupBg]  = BG1;
        c[ImGuiCol_Border]   = BORDER;
        c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        c[ImGuiCol_FrameBg]        = BG2;
        c[ImGuiCol_FrameBgHovered] = BG3;
        c[ImGuiCol_FrameBgActive]  = ACCENT_MUTED;

        c[ImGuiCol_TitleBg]          = BG1;
        c[ImGuiCol_TitleBgActive]    = BG1;
        c[ImGuiCol_TitleBgCollapsed] = BG1;
        c[ImGuiCol_MenuBarBg]        = BG1;

        c[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab]        = BG3;
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(TEXT_DIM.x, TEXT_DIM.y, TEXT_DIM.z, 0.5f);
        c[ImGuiCol_ScrollbarGrabActive]  = TEXT_DIM;

        c[ImGuiCol_CheckMark] = ACCENT;

        c[ImGuiCol_SliderGrab]       = ACCENT;
        c[ImGuiCol_SliderGrabActive] = ACCENT_HOVER;

        c[ImGuiCol_Button]        = BG2;
        c[ImGuiCol_ButtonHovered] = BG3;
        c[ImGuiCol_ButtonActive]  = ACCENT_ACTIVE;

        c[ImGuiCol_Header]        = ACCENT_MUTED;
        c[ImGuiCol_HeaderHovered] = ImVec4(1, 1, 1, 0.06f);
        c[ImGuiCol_HeaderActive]  = ACCENT_MUTED;

        c[ImGuiCol_Separator]        = BORDER;
        c[ImGuiCol_SeparatorHovered] = ACCENT;
        c[ImGuiCol_SeparatorActive]  = ACCENT;

        c[ImGuiCol_ResizeGrip]        = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ResizeGripHovered] = ImVec4(TEXT_DIM.x, TEXT_DIM.y, TEXT_DIM.z, 0.4f);
        c[ImGuiCol_ResizeGripActive]  = ACCENT;

        c[ImGuiCol_Tab]                = BG_TAB_INACTIVE;
        c[ImGuiCol_TabHovered]         = BG3;
        c[ImGuiCol_TabActive]          = BG0;
        c[ImGuiCol_TabUnfocused]       = BG_TAB_INACTIVE;
        c[ImGuiCol_TabUnfocusedActive] = BG1;

        c[ImGuiCol_DockingPreview] = ACCENT_MUTED;
        c[ImGuiCol_DockingEmptyBg] = BG0;

        c[ImGuiCol_PlotLines]            = ACCENT;
        c[ImGuiCol_PlotLinesHovered]     = ACCENT_HOVER;
        c[ImGuiCol_PlotHistogram]        = ACCENT;
        c[ImGuiCol_PlotHistogramHovered] = ACCENT_HOVER;

        c[ImGuiCol_TableHeaderBg]     = BG1;
        c[ImGuiCol_TableBorderStrong] = BORDER;
        c[ImGuiCol_TableBorderLight]  = BORDER_SOFT;
        c[ImGuiCol_TableRowBg]        = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TableRowBgAlt]     = ImVec4(1, 1, 1, 0.02f);

        c[ImGuiCol_TextSelectedBg] = ACCENT_MUTED;
        c[ImGuiCol_DragDropTarget] = ACCENT;

        c[ImGuiCol_NavHighlight]          = ACCENT;
        c[ImGuiCol_NavWindowingHighlight] = ImVec4(1, 1, 1, 0.70f);
        c[ImGuiCol_NavWindowingDimBg]     = ImVec4(0, 0, 0, 0.35f);
        c[ImGuiCol_ModalWindowDimBg]      = ImVec4(0, 0, 0, 0.45f);
    }

    void ImGuiLayer::OnDetach()
    {
        CORE_INFO("ImGuiLayer Detached");
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void ImGuiLayer::OnEvent(Event &e)
    {
        if (m_BlockEvent)
        {
            if ((ImGui::GetIO().WantCaptureMouse &&
                 (e.GetEventType() == EventType::MouseButtonPressed ||
                  e.GetEventType() == EventType::MouseButtonReleased ||
                  e.GetEventType() == EventType::MouseMoved ||
                  e.GetEventType() == EventType::MouseScrolled)) ||
                (ImGui::GetIO().WantCaptureKeyboard &&
                 (e.GetEventType() == EventType::KeyPressed ||
                  e.GetEventType() == EventType::KeyReleased ||
                  e.GetEventType() == EventType::KeyTyped)))
            {
                e.Handled = true;
            }
        }
    }

    void ImGuiLayer::OnRender()
    {
    }

    void ImGuiLayer::OnUpdate(TimeStep ts)
    {
    }

    void ImGuiLayer::OnSDLEvent(SDL_Event &e)
    {
        ImGui_ImplSDL3_ProcessEvent(&e);
    }
}

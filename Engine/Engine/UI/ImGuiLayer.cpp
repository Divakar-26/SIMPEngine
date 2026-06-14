#include <Engine/UI/ImGuiLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include "style.h"

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

        ImGui::StyleColorsDark();

        io.FontGlobalScale = 0.2f;

        float oldScale = io.FontGlobalScale;
        io.FontGlobalScale = 1.0f;

        ImGuiStyle &s = ImGui::GetStyle();

        s.WindowRounding = 8;
        s.ChildRounding = 8;
        s.PopupRounding = 8;
        s.FrameRounding = 6;
        s.GrabRounding = 6;
        s.TabRounding = 6;

        s.WindowBorderSize = 0;
        s.ChildBorderSize = 1;
        s.FrameBorderSize = 0;

        s.WindowPadding = ImVec2(10, 10);
        s.FramePadding = ImVec2(8, 4);
        s.ItemSpacing = ImVec2(8, 6);
        s.ItemInnerSpacing = ImVec2(6, 4);

        s.ScrollbarSize = 12;
        s.IndentSpacing = 18;

        ImVec4 *c = s.Colors;

        c[ImGuiCol_Text] = TEXT;
        c[ImGuiCol_TextDisabled] = TEXT_DIM;

        c[ImGuiCol_WindowBg] = BG0;
        c[ImGuiCol_ChildBg] = BG1;
        c[ImGuiCol_PopupBg] = BG1;

        c[ImGuiCol_Border] =
            ImVec4(0.18f, 0.18f, 0.20f, 1.0f);

        c[ImGuiCol_FrameBg] = BG2;
        c[ImGuiCol_FrameBgHovered] =
            ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
        c[ImGuiCol_FrameBgActive] =
            ImVec4(0.22f, 0.22f, 0.27f, 1.0f);

        c[ImGuiCol_Button] = BG2;
        c[ImGuiCol_ButtonHovered] = ACCENT_HOVER;
        c[ImGuiCol_ButtonActive] = ACCENT_ACTIVE;

        c[ImGuiCol_Header] =
            ImVec4(0.14f, 0.14f, 0.17f, 1.0f);
        c[ImGuiCol_HeaderHovered] = ACCENT_HOVER;
        c[ImGuiCol_HeaderActive] = ACCENT_ACTIVE;

        c[ImGuiCol_Tab] = BG1;
        c[ImGuiCol_TabHovered] = ACCENT_HOVER;
        c[ImGuiCol_TabActive] = ACCENT;
        c[ImGuiCol_TabUnfocused] = BG1;
        c[ImGuiCol_TabUnfocusedActive] = BG2;

        c[ImGuiCol_CheckMark] = ACCENT;

        c[ImGuiCol_SliderGrab] = ACCENT;
        c[ImGuiCol_SliderGrabActive] = ACCENT_HOVER;

        c[ImGuiCol_ResizeGrip] = ACCENT;
        c[ImGuiCol_ResizeGripHovered] = ACCENT_HOVER;
        c[ImGuiCol_ResizeGripActive] = ACCENT_ACTIVE;

        if (io.Fonts->AddFontFromFileTTF("../assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 18.0f))
        {
            CORE_ERROR("Could not load the fonts");
        }

        // ImFontConfig config;
        // config.MergeMode = true;
        // config.PixelSnapH = true;

        // static const ImWchar icon_ranges[] = {0xE000, 0xE7BF, 0};
        // io.Fonts->AddFontFromFileTTF("../assets/myicons.ttf", 18.0f, &config, icon_ranges);

        // Init backends
        ImGui_ImplSDL3_InitForOpenGL(m_Window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init("#version 330 core");
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
        ImGui::ShowDemoWindow();
    }

    void ImGuiLayer::End()
    {
        ImGuiIO &io = ImGui::GetIO();

        // Rendering
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

        // ImGui::ShowDemoWindow();
    }

    void ImGuiLayer::OnUpdate(TimeStep ts)
    {
    }

    void ImGuiLayer::OnSDLEvent(SDL_Event &e)
    {
        ImGui_ImplSDL3_ProcessEvent(&e);
    }
}

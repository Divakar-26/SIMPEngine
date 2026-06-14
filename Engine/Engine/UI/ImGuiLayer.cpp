#include <Engine/UI/ImGuiLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Core/Window.h>
#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>

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
        s.WindowRounding = 6.f;
        s.FrameRounding = 4.f;
        s.GrabRounding = 3.f;
        s.TabRounding = 4.f;
        s.FramePadding = ImVec2(6, 3);
        s.ItemSpacing = ImVec2(6, 4);
        s.IndentSpacing = 14.f;
        s.WindowBorderSize = 1.f;
        s.FrameBorderSize = 0.f; // remove frame borders, use bg contrast instead

        ImVec4 *c = ImGui::GetStyle().Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.086f, 0.086f, 0.094f, 1.f); // #3131bf
        c[ImGuiCol_FrameBg] = ImVec4(0.051f, 0.051f, 0.059f, 1.f);  // #0d0d0f
        c[ImGuiCol_Header] = ImVec4(0.118f, 0.118f, 0.176f, 1.f);   // active row
        c[ImGuiCol_HeaderHovered] = ImVec4(0.122f, 0.122f, 0.157f, 1.f);
        c[ImGuiCol_TitleBg] = ImVec4(0.078f, 0.078f, 0.086f, 1.f);
        // Accent: a single purple
        c[ImGuiCol_CheckMark] = ImVec4(0.486f, 0.416f, 0.961f, 1.f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.486f, 0.416f, 0.961f, 1.f);

        s.Colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);

        if(io.Fonts->AddFontFromFileTTF("../assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 18.0f)){
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

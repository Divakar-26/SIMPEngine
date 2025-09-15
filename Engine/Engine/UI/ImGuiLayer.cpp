#include "UI/ImGuiLayer.h"
#include "Application/Application.h"
#include "Core/Window.h"
#include "Core/Log.h"

#include "Input/Input.h"
#include <ImGuizmo.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

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
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        io.FontGlobalScale = 0.2f; // main UI

        float oldScale = io.FontGlobalScale;
        io.FontGlobalScale = 1.0f;

        io.IniFilename = "../assets/ImGUIPanels/imgui.ini";

        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.129f, 0.149f, 0.180f, 1.0f); // RGBA
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.129f, 0.149f, 0.180f, 1.0f);  // RGBA
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        style.FrameBorderSize = 1.0f; // default is 0.0f (no border)

        style.Colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);

        io.Fonts->AddFontFromFileTTF("../assets/JetBrainsMonoNerdFont-Regular.ttf", 18.0f);

        ImFontConfig config;
        config.MergeMode = true;
        config.PixelSnapH = true;

        static const ImWchar icon_ranges[] = {0xE000, 0xE7BF, 0};
        io.Fonts->AddFontFromFileTTF("../assets/myicons.ttf", 18.0f, &config, icon_ranges);

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

#include "Engine/ImGuiLayer.h"
#include "Engine/Application.h"
#include "Engine/Window.h"
#include "Engine/Log.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/EventDispatcher.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"



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

        // Grab native SDL_Window and SDL_Renderer pointers from your Window class
        m_Window = window.GetNativeWindow();
        m_Renderer = window.GetRenderer();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
        ImGui_ImplSDLRenderer3_Init(m_Renderer);
    }

    void ImGuiLayer::OnDetach()
    {
        CORE_INFO("ImGuiLayer Detached");

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
    }

    void ImGuiLayer::OnUpdate()
    {
        Begin();

        // Here you can create ImGui windows, UI elements etc.
        ImGui::Begin("Hello, ImGui!");

        if (ImGui::Button("Click Me"))
        {
            CORE_INFO("Button Clicked!");
        }

        ImGui::Text("This is some text.");

        ImGui::End();

        End();
    }

    void ImGuiLayer::OnEvent(Event &e)
    {
    }

    void ImGuiLayer::OnSDLEvent(SDL_Event &e)
    {
        ImGui_ImplSDL3_ProcessEvent(&e);
    }
}
#include "UI/ImGuiLayer.h"
#include "Application/Application.h"
#include "Window.h"
#include "Log.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

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

    void ImGuiLayer::OnUpdate(class TimeStep ts)
    {
    }

    void ImGuiLayer::OnRender()
    {

        ImGui::Begin("Hello, ImGui!");

        if (ImGui::Button("Click Me"))
        {
            CORE_INFO("Button Clicked!");
        }
        // ImGui::Text("Delta Time: %.3f ms", ts.GetMilliseconds());
        // ImGui::Text("FPS: %.1f", 1.0f / ts.GetSeconds());
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);


        ImGui::Text("This is some text.");

        ImGui::End();

    }

    void ImGuiLayer::OnEvent(Event &e)
    {
        // CORE_INFO("Event: {} | WantCaptureMouse = {}", e.ToString(), ImGui::GetIO().WantCaptureMouse);

        if (ImGui::GetIO().WantCaptureMouse &&
            (e.GetEventType() == EventType::MouseButtonPressed ||
             e.GetEventType() == EventType::MouseButtonReleased ||
             e.GetEventType() == EventType::MouseMoved))
        {
            e.Handled = true;
            // CORE_INFO("ImGui handled this mouse event");
        }
    }

    void ImGuiLayer::OnSDLEvent(SDL_Event &e)
    {
        ImGui_ImplSDL3_ProcessEvent(&e);
    }
}
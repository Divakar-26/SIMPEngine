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

#include "Rendering/Renderer.h"

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
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

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
        if (viewportFocused)
        {
            CORE_INFO("View port focues");
        }
    }

    void ImGuiLayer::OnRender()
    {
        // 1. Start a full-screen window to host the dockspace
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("DockSpace Demo", nullptr, window_flags);
        ImGui::PopStyleVar(2);

        // 2. Create the dockspace
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // --- Console panel ---
        ImGui::Begin("Console");
        ImGui::Text("This is the console window");
        ImGui::End();

        // --- Scene / Viewport panel ---
        ImGui::Begin("Scene");

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        // Resize and render your viewport as before
        Renderer::GetAPI()->ResizeViewport((int)viewportSize.x, (int)viewportSize.y);
        SDL_SetRenderTarget(Renderer::GetSDLRenderer(), Renderer::GetAPI()->GetViewportTexture());
        Renderer::Clear();
        Application::Get().GetRenderingLayer()->OnRender();
        SDL_SetRenderTarget(Renderer::GetSDLRenderer(), nullptr);

        SDL_Texture *tex = Renderer::GetAPI()->GetViewportTexture();
        ImGui::Image((void *)tex, viewportSize, ImVec2(0, 0), ImVec2(1, 1));

        ImGui::End();

        // --- Inspector panel ---
        ImGui::Begin("Inspector");
        ImGui::Text("Inspector panel");
        ImGui::End();

        ImGui::End();
    }

    void ImGuiLayer::OnEvent(Event &e)
    {
        // CORE_INFO("Event: {} | WantCaptureMouse = {}", e.ToString(), ImGui::GetIO().WantCaptureMouse);

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

        // if (ImGui::GetIO().WantCaptureKeyboard)
        // {
        //     if (viewportFocused)
        //     {
        //         e.Handled = false;
        //     }
        // }
    }

    void ImGuiLayer::OnSDLEvent(SDL_Event &e)
    {
        ImGui_ImplSDL3_ProcessEvent(&e);
    }
}
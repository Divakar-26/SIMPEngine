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
#include "ImGuiSink.h"

namespace SIMPEngine
{

    ImGuiLayer::ImGuiLayer()
        : Layer("ImGuiLayer"), viewportFocused(false)
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

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Log", nullptr, &showLogWindow))
                {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Scene Hierarchy");

        // Get the rendering layer
        auto &renderingLayer = *Application::Get().GetRenderingLayer();
        auto &scene = renderingLayer.GetScene();
        auto &registry = scene.GetRegistry();

        // Iterate entities with a view
        auto view = registry.view<TagComponent>();
        for (auto entityID : view)
        {
            SIMPEngine::Entity entity(entityID, &scene);
            std::string name = entity.HasComponent<TagComponent>() ? entity.GetComponent<TagComponent>().Tag : "Unnamed";

            // Select entity
            if (ImGui::Selectable(name.c_str(), selectedEntity.GetHandle() == entity.GetHandle()))
                selectedEntity = entity;
        }

        // Queue entity creation instead of direct call
        if (ImGui::Button("Create Entity"))
        {
            renderingLayer.QueueCommand([](Scene &scene)
                                        {
        auto e = scene.CreateEntity("New Entity");
        e.AddComponent<TagComponent>("New Entity"); });
        }

        ImGui::End();

        // Inspector for selected entity
        if (selectedEntity)
        {
            ImGui::Begin("Inspector");

            // Transform component
            if (selectedEntity.HasComponent<TransformComponent>())
            {
                auto &t = selectedEntity.GetComponent<TransformComponent>();
                float x = t.x, y = t.y, rotation = t.rotation, scaleX = t.scaleX, scaleY = t.scaleY;

                bool changed = false;
                changed |= ImGui::DragFloat("X", &x, 1.0f);
                changed |= ImGui::DragFloat("Y", &y, 1.0f);
                changed |= ImGui::DragFloat("Rotation", &rotation, 1.0f);
                changed |= ImGui::DragFloat("Scale X", &scaleX, 0.1f);
                changed |= ImGui::DragFloat("Scale Y", &scaleY, 0.1f);

                if (changed)
                {
                    // Queue transform update
                    renderingLayer.QueueCommand([entityHandle = selectedEntity.GetHandle(), x, y, rotation, scaleX, scaleY](Scene &scene)
                                                {
            Entity e(entityHandle, &scene);
            if (e.HasComponent<TransformComponent>())
            {
                auto &t = e.GetComponent<TransformComponent>();
                t.x = x;
                t.y = y;
                t.rotation = rotation;
                t.scaleX = scaleX;
                t.scaleY = scaleY;
            } });
                }
            }
            else
            {
                if (ImGui::Button("Add Transform Component"))
                {
                    auto entityHandle = selectedEntity.GetHandle();
                    renderingLayer.QueueCommand([entityHandle](Scene &scene)
                                                {
            Entity e(entityHandle, &scene);
            e.AddComponent<TransformComponent>(0.0f, 0.0f, 0.0f, 1.0f, 1.0f); });
                }
            }

            // Render component
            if (selectedEntity.HasComponent<RenderComponent>())
            {
                auto &r = selectedEntity.GetComponent<RenderComponent>();
                float color[4] = {r.color.r / 255.0f, r.color.g / 255.0f, r.color.b / 255.0f, r.color.a / 255.0f};

                if (ImGui::ColorEdit4("Color", color))
                {
                    renderingLayer.QueueCommand([entityHandle = selectedEntity.GetHandle(), color](Scene &scene)
                                                {
                Entity e(entityHandle, &scene);
                if (e.HasComponent<RenderComponent>())
                {
                    auto &r = e.GetComponent<RenderComponent>();
                    r.color.r = (Uint8)(color[0] * 255);
                    r.color.g = (Uint8)(color[1] * 255);
                    r.color.b = (Uint8)(color[2] * 255);
                    r.color.a = (Uint8)(color[3] * 255);
                } });
                }
            }

            if (selectedEntity.HasComponent<VelocityComponent>())
            {
                auto &vel = selectedEntity.GetComponent<VelocityComponent>();
                ImGui::DragFloat("Velocity X", &vel.vx, 1.0f);
                ImGui::DragFloat("Velocity Y", &vel.vy, 1.0f);
            }
            else
            {
                // Add Velocity Component button
                if (ImGui::Button("Add Velocity Component"))
                {
                    auto entityHandle = selectedEntity.GetHandle();
                    Application::Get().GetRenderingLayer()->QueueCommand([entityHandle](Scene &scene)
                                                                         {
                                                                             Entity e(entityHandle, &scene);
                                                                             e.AddComponent<VelocityComponent>(0.0f, 0.0f); // default velocity
                                                                         });
                }
            }

            ImGui::End();
        }

        ImGui::Begin("Scene");

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        Renderer::GetAPI()->ResizeViewport((int)viewportSize.x, (int)viewportSize.y);
        SDL_SetRenderTarget(Renderer::GetSDLRenderer(), Renderer::GetAPI()->GetViewportTexture());

        Application::Get().GetRenderingLayer()->GetCamera().SetViewportSize(viewportSize.x, viewportSize.y);

        Renderer::Clear();
        Application::Get().GetRenderingLayer()->OnRender();
        SDL_SetRenderTarget(Renderer::GetSDLRenderer(), nullptr);

        SDL_Texture *tex = Renderer::GetAPI()->GetViewportTexture();
        ImGui::Image((void *)tex, viewportSize, ImVec2(0, 0), ImVec2(1, 1));

        ImGui::End();

        ImGui::Begin("Inspector");
        ImGui::Text("Inspector panel");
        ImGui::End();

        if (showLogWindow)
        {
            ImGui::Begin("Log", &showLogWindow);

            auto sink = SIMPEngine::Log::GetImGuiSink();
            if (sink)
            {
                for (auto &line : sink->Logs)
                {
                    ImVec4 color;
                    if (line.find("[error]") != std::string::npos)
                        color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    else if (line.find("[info]") != std::string::npos)
                        color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    else
                        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                    ImGui::TextColored(color, "%s", line.c_str());
                }

                ImGui::SetScrollHereY(1.0f);
            }

            ImGui::End();
        }

        ImGui::End();
    }

    void ImGuiLayer::OnEvent(Event &e)
    {
        // CORE_INFO("Event: {} | WantCaptureMouse = {}", e.ToString(), ImGui::GetIO().WantCaptureMouse);
        if (!viewportFocused)
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
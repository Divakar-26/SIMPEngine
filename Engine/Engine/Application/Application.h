#pragma once
#include <Engine/Core/Window.h>
#include <Engine/Events/Event.h>
#include <Engine/Layer/LayerStack.h>
#include <Engine/UI/ImGuiLayer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Math/Camera2D.h>
#include <Engine/Scene/SceneManager.h>


#include <functional>

namespace SIMPEngine
{

    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void OnEvent(Event &e);
        void SDLEventToEngine(SDL_Event &event);

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *overlay);

        Window &GetWindow() { return m_Window; }

        static Application &Get() { return *s_Instance; }
        // RenderingLayer* GetRenderingLayer() { return m_RenderingLayer; }

        ImGuiLayer *GetImGuiLayer() { return m_ImGuiLayer; }

    private:
        Window m_Window;
        bool m_Running = true;

        LayerStack m_LayerStack;
        ImGuiLayer *m_ImGuiLayer;

        static Application *s_Instance;
    };

    // To be defined by the client (game)
    Application *CreateApplication();
}

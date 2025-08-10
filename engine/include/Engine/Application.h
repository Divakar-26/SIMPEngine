#pragma once
#include "Engine/Window.h"
#include "Engine/Events/Event.h"
#include "Engine/LayerStack.h"
#include "Engine/ImGuiLayer.h"
#include "Engine/RenderingLayer.h"
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
        void SDLEventToEngine(SDL_Event & event);

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *overlay);

        Window &GetWindow() { return m_Window; }

        // Singleton access
        static Application &Get() { return *s_Instance; }

    private:
        Window m_Window;
        bool m_Running = true;

        LayerStack m_LayerStack;
        ImGuiLayer *m_ImGuiLayer;
        RenderingLayer *m_RenderingLayer;
       
        static Application *s_Instance;
    };

    // To be defined by the client (game)
    Application *CreateApplication();
}

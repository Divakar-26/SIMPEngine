#pragma once
#include "Window.h"
#include "Events/Event.h"
#include "Layer/LayerStack.h"
#include "UI/ImGuiLayer.h"
#include "Layer/RenderingLayer.h"
#include <functional>
#include "Math/Camera2D.h"
#include "Scene/SceneManager.h"

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

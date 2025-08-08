#pragma once
#include "Engine/Window.h"
#include "Engine/Events/Event.h"
#include <functional>

namespace SIMPEngine {

    class Application {
    public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);

    private:
        Window m_Window;
        bool m_Running = true;
    };

    // To be defined by the client (game)
    Application* CreateApplication();
}

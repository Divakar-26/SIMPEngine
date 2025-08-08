#include "Engine/Application.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/EventDispatcher.h"
#include <iostream>

namespace SIMPEngine
{

    Application::Application()
    {
        m_Window.Init("My Game", 800, 600);
    }

    Application::~Application()
    {
        m_Window.ShutDown();
    }

    void Application::Run()
    {
        while (m_Running)
        {
            SDL_Event sdlEvent;
            while (SDL_PollEvent(&sdlEvent))
            {
                // Map SDL events to engine events (WindowCloseEvent, etc.)
                switch (sdlEvent.type)
                {
                case SDL_EVENT_QUIT:
                {
                    WindowCloseEvent e;
                    OnEvent(e);
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED:
                {
                    WindowResizeEvent e(sdlEvent.window.data1, sdlEvent.window.data2);
                    OnEvent(e);
                    break;
                }
                default:
                    break;
                }
            }

            // Game update logic goes here
            m_Window.OnUpdate();
        }
    }

    void Application::OnEvent(Event &e)
    {

        std::cout << "[EVENT] " << e.ToString() << "\n";

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &ev)
                                              {
            m_Running = false;
            return true; });
    }

}

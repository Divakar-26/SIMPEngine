#include "Engine/Application.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/EventDispatcher.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"

#include "Engine/Log.h"

#include <iostream>

namespace SIMPEngine
{
    Application *Application::s_Instance = nullptr;

    Application::Application()
    {
        if (s_Instance)
        {
            // You can handle this better, maybe throw or assert
        }
        s_Instance = this;
        m_Window.Init("My Game", 800, 600);
    }

    Application::~Application()
    {
        m_Window.ShutDown();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        while (m_Running)
        {
            SDL_Event sdlEvent;
            while (SDL_PollEvent(&sdlEvent))
            {
                // Map SDL events to engine events (WindowCloseEvent, etc.)
                m_ImGuiLayer->OnSDLEvent(sdlEvent);
                SDLEventToEngine(sdlEvent);
            }

            SDL_SetRenderDrawColor(m_Window.GetRenderer(), 100, 0, 0, 255);
            SDL_RenderClear(m_Window.GetRenderer());

            for (Layer *layer : m_LayerStack)
                layer->OnUpdate();

            // Game update logic goes here
            SDL_RenderPresent(m_Window.GetRenderer());
        }
    }

    void Application::OnEvent(Event &e)
    {

        CORE_TRACE("{}", e.ToString());

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &ev)
                                              {
            m_Running = false;
            return true; });

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
                break;
        }
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer *overlay)
    {
        m_LayerStack.PushOverlay(overlay);
    }

    void Application::SDLEventToEngine(SDL_Event & sdlEvent)
    {
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
        case SDL_EVENT_KEY_DOWN:
        {
            bool repeat = (sdlEvent.key.repeat != 0);
            KeyPressedEvent e(sdlEvent.key.key, repeat);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            KeyReleasedEvent e(sdlEvent.key.key);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_TEXT_INPUT:
        {
            KeyTypedEvent e(sdlEvent.text.text[0]); // first char for simplicity
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            MouseMovedEvent e(sdlEvent.motion.x, sdlEvent.motion.y);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            MouseButtonPressedEvent e(sdlEvent.button.button);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            MouseButtonReleasedEvent e(sdlEvent.button.button);
            OnEvent(e);
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            MouseScrolledEvent e(sdlEvent.wheel.x, sdlEvent.wheel.y);
            OnEvent(e);
            break;
        }
        default:
            break;
        }
    }
}

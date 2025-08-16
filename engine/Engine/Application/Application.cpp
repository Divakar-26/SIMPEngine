#include "Application/Application.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include <entt/entt.hpp>

#include "Log.h"

#include "Rendering/SDLRenderingAPI.h"
#include "Rendering/Renderer.h"
#include "Input/Input.h"

#include "Rendering/Texture.h"

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
        m_Window.Init("My Game", 1024, 720);

        Renderer::Init(std::make_unique<SDLRenderingAPI>(), m_Window.GetRenderer());

        m_ImGuiLayer = new ImGuiLayer();
        m_RenderingLayer = new RenderingLayer();

        PushOverlay(m_RenderingLayer);
        PushOverlay(m_ImGuiLayer);

        CORE_INFO("Entt version {}", ENTT_VERSION);
        entt::registry m_Registry;
        CORE_ERROR("Registry is {}", typeid(m_Registry).name());
    }

    Application::~Application()
    {
        m_Window.ShutDown();
        s_Instance = nullptr;
    }

    void Application::Run()
    {
        uint64_t lastFrameTime = SDL_GetPerformanceCounter();
        const double freq = (double)SDL_GetPerformanceFrequency();

        while (m_Running)
        {
            uint64_t currentFrameTime = SDL_GetPerformanceCounter();
            float deltaTime = (float)((currentFrameTime - lastFrameTime) / (float)freq);
            lastFrameTime = currentFrameTime;

            SDL_Event sdlEvent;
            while (SDL_PollEvent(&sdlEvent))
            {
                // if(m_ImGuiLayer)
                m_ImGuiLayer->OnSDLEvent(sdlEvent);
                SDLEventToEngine(sdlEvent);
            }

            for (Layer *layer : m_LayerStack)
                layer->OnUpdate(deltaTime);

            SDL_SetRenderDrawColor(m_Window.GetRenderer(), 100, 100, 100, 255);
            SDL_RenderClear(m_Window.GetRenderer());

            m_ImGuiLayer->Begin();
            for (Layer *layer : m_LayerStack)
                layer->OnRender();


            m_ImGuiLayer->End();

            Renderer::Present();
        }
    }

    void Application::OnEvent(Event &e)
    {

        m_ImGuiLayer->OnEvent(e); // ImGui first

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent &ev)
                                              {
        m_Running = false;
        return true; });

        if (m_ImGuiLayer->viewportFocused)
        {
            dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent &ev)
                                                 {
                                                 CORE_INFO("KeyPressedEvent keycode = {}", ev.GetKeyCode());
                                                 Input::OnKeyPressed(ev.GetKeyCode());
                                                 return false; });

            dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent &ev)
                                                  {
                                                 CORE_INFO("{}", ev.ToString());

        Input::OnKeyReleased(ev.GetKeyCode());
        return false; });

            dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent &ev)
                                                         {
                                                 CORE_INFO("{}", ev.ToString());

        Input::OnMouseButtonPressed(ev.GetMouseButton());
        return false; });

            dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent &ev)
                                                          {
                                                 CORE_INFO("{}", ev.ToString());

        Input::OnMouseButtonReleased(ev.GetMouseButton());
        return false; });

            dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent &ev)
                                                 {
                                                //  CORE_INFO("{}", ev.ToString());

        Input::OnMouseMoved(ev.GetX(), ev.GetY());
        return false; });

            dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent &ev)
                                                    {
                                                    CORE_INFO("{}", ev.ToString());
                                                    return false; });
        }

        else
        {
            Input::ResetAllKeys();
        }

        if (!e.Handled)
        {
            for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
            {
                (*--it)->OnEvent(e);
                if (e.Handled)
                    break;
            }
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

    void Application::SDLEventToEngine(SDL_Event &sdlEvent)
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

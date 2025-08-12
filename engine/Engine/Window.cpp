    #include "Window.h"
    #include "Log.h"
    #include <SDL3/SDL.h>
    #include <iostream>

    namespace SIMPEngine
    {
        Window::Window() {}

        Window::~Window()
        {
            ShutDown();
        }

        bool Window::Init(const char *name, int width, int height)
        {
            if (SDL_Init(SDL_INIT_VIDEO) == 0)
            {
                CORE_ERROR("SDL_Init failed: {}", SDL_GetError());
                return false;
            }

            m_Window = SDL_CreateWindow(name, width, height, SDL_WINDOW_RESIZABLE);
            if (!m_Window)
            {
                CORE_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
                SDL_Quit();
                return false;
            }

            SDL_PropertiesID props = SDL_CreateProperties();
            SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, "opengl");
            SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, m_Window);
            SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 0);

            m_Renderer = SDL_CreateRendererWithProperties(props);

            SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_NONE);
            SDL_DestroyProperties(props);

            if (!m_Renderer)
            {
                CORE_ERROR("SDL_CreateRendererWithProperties failed: {}", SDL_GetError());
                SDL_DestroyWindow(m_Window);
                SDL_Quit();
                return false;
            }

            SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_BLEND);

            CORE_INFO("Window '{}' created successfully ({} x {})", name, width, height);
            m_Initialized = true;
            return true;
        }

        void Window::OnUpdate()
        {

        }

        void Window::ShutDown()
        {
            if (!m_Initialized)
                return;

            if (m_Window)
            {
                CORE_INFO("Destroying window...");
                SDL_DestroyWindow(m_Window);
                m_Window = nullptr;
            }

            SDL_Quit();
            m_Initialized = false;

            CORE_INFO("Window shutdown complete.");
        }
    }
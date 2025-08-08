#include "Engine/Window.h"
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
            std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
            return false;
        }

        m_Window = SDL_CreateWindow(name, width, height, SDL_WINDOW_RESIZABLE);
        if (!m_Window)
        {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
            SDL_Quit();
            return false;
        }

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, "opengl"); // could be "metal", "vulkan", etc.
        SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, m_Window);
        SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 1); // enable vsync

        m_Renderer = SDL_CreateRendererWithProperties(props);
        SDL_DestroyProperties(props);

        if (!m_Renderer)
        {
            std::cerr << "SDL_CreateRendererWithProperties failed: " << SDL_GetError() << "\n";
            SDL_DestroyWindow(m_Window);
            SDL_Quit();
            return false;
        }

        m_Initialized = true;
        return true;
    }

    void Window::OnUpdate()
    {
        SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_Renderer);

        // Draw something here...

        SDL_RenderPresent(m_Renderer);
    }

    void Window::ShutDown()
    {
        if (!m_Initialized)
            return;

        if (m_Window)
        {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }

        SDL_Quit();
        m_Initialized = false;
    }
}
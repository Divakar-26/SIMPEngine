#include <Engine/Core/Window.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/EngineDimensions.h>

#include <SDL3/SDL.h>
#include <iostream>

    namespace SIMPEngine
    {
        Window::Window() {
        }

        Window::~Window()
        {
            ShutDown();
        }

        bool Window::Init(const char *name, int width, int height, SDL_WindowFlags flags)
        {
            if (SDL_Init(SDL_INIT_VIDEO) == 0)
            {
                CORE_ERROR("SDL_Init failed: {}", SDL_GetError());
                return false;
            }

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            m_Window = SDL_CreateWindow(name, width, height, SDL_WINDOW_OPENGL | flags);
            if (!m_Window)
            {
                CORE_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
                SDL_Quit();
                return false;
            }

            m_GLContext = SDL_GL_CreateContext(m_Window);
            if(!m_GLContext){
                CORE_ERROR("SDL_GL_CreateContext Failed: {}", SDL_GetError());
                SDL_DestroyWindow(m_Window);
                SDL_Quit();
                return false;
            }

            SDL_GL_MakeCurrent(m_Window, m_GLContext);
            auto glLoader = [](const char *procName) -> void *
            { return reinterpret_cast<void *>(SDL_GL_GetProcAddress(procName)); };
            if(!gladLoadGLLoader(glLoader)){
                CORE_ERROR("Failed to intilaize glad");
                SDL_GL_DestroyContext(m_GLContext);
                SDL_DestroyWindow(m_Window);
                SDL_Quit();
                return false;
            }

            SDL_GL_SetSwapInterval(m_VSync ? 1 : 0);

            CORE_INFO("Window '{}' created successfully ({} x {})", name, width, height);

            m_Width = width;
            m_Height = height;
            EngineDimensions::SetDimensions(width, height);

            m_Initialized = true;
            return true;
        }

        void Window::OnUpdate()
        {
            SDL_GL_SwapWindow(m_Window);
        }

        void Window::ShutDown()
        {
            if (!m_Initialized)
                return;

            if(m_GLContext){
                SDL_GL_DestroyContext(m_GLContext);
                m_GLContext = nullptr;
            }

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
    
    void Window::SetVSync(bool enabled)
    {
        m_VSync = enabled;
        if (m_GLContext)
            SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    }
}

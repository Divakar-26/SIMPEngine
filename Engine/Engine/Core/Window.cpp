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

#if defined(SIMP_GLES)
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef __APPLE__
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

            m_Window = SDL_CreateWindow(name, width, height, SDL_WINDOW_OPENGL | flags);
            if (!m_Window)
            {
                CORE_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
                SDL_Quit();
                return false;
            }

#if defined(SIMP_GLES)
            static constexpr int Versions[][2] = {{3, 2}, {3, 1}, {3, 0}};
#else
            static constexpr int Versions[][2] = {{4, 6}, {4, 5}, {4, 3}, {4, 1}, {4, 0}};
#endif
            for (const auto &version : Versions)
            {
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version[0]);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version[1]);
                m_GLContext = SDL_GL_CreateContext(m_Window);
                if (m_GLContext)
                {
                    CORE_INFO("OpenGL {}.{} context created", version[0], version[1]);
                    break;
                }
            }
            if (!m_GLContext)
            {
                CORE_ERROR("SDL_GL_CreateContext failed: {}", SDL_GetError());
                SDL_SetError("This computer's graphics driver does not support OpenGL 4.0, which SIMPEngine needs. Updating the graphics driver usually fixes this.");
                SDL_DestroyWindow(m_Window);
                m_Window = nullptr;
                SDL_Quit();
                return false;
            }

            SDL_GL_MakeCurrent(m_Window, m_GLContext);
#if !defined(SIMP_GLES)
            auto glLoader = [](const char *procName) -> void *
            { return reinterpret_cast<void *>(SDL_GL_GetProcAddress(procName)); };
            if(!gladLoadGLLoader(glLoader)){
                CORE_ERROR("Failed to intilaize glad");
                SDL_GL_DestroyContext(m_GLContext);
                SDL_DestroyWindow(m_Window);
                SDL_Quit();
                return false;
            }
#endif

            if (const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION)))
                CORE_INFO("GL_VERSION: {}", version);

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
    
    void Window::SetFullscreen(bool fullscreen)
    {
        if (!m_Window || IsFullscreen() == fullscreen)
            return;
        SDL_SetWindowFullscreen(m_Window, fullscreen);
        SDL_SyncWindow(m_Window);
    }

    bool Window::IsFullscreen() const
    {
        return m_Window && (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_FULLSCREEN) != 0;
    }

    void Window::SetVSync(bool enabled)
    {
        m_VSync = enabled;
        if (m_GLContext)
            SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    }
}

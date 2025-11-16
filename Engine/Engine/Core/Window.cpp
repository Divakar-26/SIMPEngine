#include <Engine/Core/Window.h>
#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>
#include <iostream>

    namespace SIMPEngine
    {
        Window::Window() {
            //set some init values
        }

        Window::~Window()
        {
            //automatically call ShutDown on destructor
            ShutDown();
        }

        bool Window::Init(const char *name, int width, int height)
        {
            //Initialize the SDL
            if (SDL_Init(SDL_INIT_VIDEO) == 0)
            {
                CORE_ERROR("SDL_Init failed: {}", SDL_GetError());
                return false;
            }

            // Set opengl parameters
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            //Create the window using name, width, height, the last argument is windowFlag. if it fails to make the window return false;
            m_Window = SDL_CreateWindow(name, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            if (!m_Window)
            {
                CORE_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
                SDL_Quit();
                return false;
            }

            //create the opengl context
            m_GLContext = SDL_GL_CreateContext(m_Window);
            if(!m_GLContext){
                CORE_ERROR("SDL_GL_CreateContext Failed: {}", SDL_GetError());
                SDL_DestroyWindow(m_Window);
                SDL_Quit();
                return false;
            }

            SDL_GL_MakeCurrent(m_Window, m_GLContext);  
            if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)){
                CORE_ERROR("Failed to intilaize glad");
                SDL_GL_DestroyContext(m_GLContext);
                SDL_DestroyWindow(m_Window);
                SDL_Quit();
                return false;
            }

            // Enable V-Sync ()
            SDL_GL_SetSwapInterval(1);

            //Prepare renderer
            // SDL_PropertiesID props = SDL_CreateProperties();
            // SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, "opengl");
            // SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, m_Window);
            // SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, 0);

            // //create renderer using those properties
            // m_Renderer = SDL_CreateRendererWithProperties(props);

            
            // SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_NONE);

            // //now the renderer is here, so delete the props
            // SDL_DestroyProperties(props);

            // //return false if creation of renderer is failed
            // if (!m_Renderer)
            // {
            //     CORE_ERROR("SDL_CreateRendererWithProperties failed: {}", SDL_GetError());
            //     SDL_DestroyWindow(m_Window);
            //     SDL_Quit();
            //     return false;
            // }

            //Log this
            CORE_INFO("Window '{}' created successfully ({} x {})", name, width, height);

            m_Initialized = true;
            return true;
        }

        void Window::OnUpdate()
        {   
            //window update code is here
            SDL_GL_SwapWindow(m_Window);
            // like delta time etc etc
        }   

        void Window::ShutDown()
        {
            //if window is not initilized then return;
            if (!m_Initialized)
                return;

            //if window exists then delete it and 
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

            //call SDL_Quit
            SDL_Quit();
            m_Initialized = false;

            //log it
            CORE_INFO("Window shutdown complete.");
        }
    }
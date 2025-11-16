#pragma once

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace SIMPEngine
{
    class Window
    {
    public:
        Window();
        ~Window();

        bool Init(const char *name, int width, int height);
        void ShutDown();
        void OnUpdate();

        //return the window for others to use like appplication
        SDL_Window *GetNativeWindow() { return m_Window; }

        //same as GetNativeWindow
        SDL_Renderer *GetRenderer() { return m_Renderer; }
        SDL_GLContext GetGLContetext(){return m_GLContext;}

    private:
        bool m_Initialized = false;
        SDL_Window *m_Window = nullptr;
        SDL_Renderer *m_Renderer = nullptr;
        SDL_GLContext m_GLContext = nullptr;
    };
}

#pragma once
#include <SDL3/SDL.h>
#include<SDL3_image/SDL_image.h>
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

        SDL_Window *GetNativeWindow() { return m_Window; }
        SDL_Renderer *GetRenderer() { return m_Renderer; }

    private:
        bool m_Initialized = false;
        SDL_Window *m_Window = nullptr;
        SDL_Renderer *m_Renderer = nullptr;
    };
}

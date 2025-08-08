#include<SDL3/SDL.h>

namespace SIMPEngine
{
    class Window{
        public:
            Window();
            ~Window();

            bool Init(const char * name, int width, int height);
            void ShutDown();
            void OnUpdate();

            SDL_Window* GetNativeWindow() const{
                return m_Window;
            }


        private:
            bool m_Initialized = false;
            SDL_Window * m_Window = nullptr;
            SDL_Renderer * m_Renderer = nullptr;
    };
}

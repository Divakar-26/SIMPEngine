#pragma once

#include<SDL3/SDL.h>

namespace SIMPEngine
{
    class Texture
    {
    public:
        Texture();
        ~Texture();

        bool LoadFromFile(SDL_Renderer * renderer, const char * path);
        void Destroy();

        SDL_Texture * GetSDLTexture() const {return m_Texture;}
        int GetWidth() const {return m_Width;}
        int GetHeight() const {return m_Height;}

    private:
        SDL_Texture * m_Texture;
        int m_Width;
        int m_Height;
    };
}
#pragma once

#include<SDL3/SDL.h>

namespace SIMPEngine
{
    class Texture
    {
    public:
        Texture();
        ~Texture();

        //loading texture from a image file(png, jpg etc)
        bool LoadFromFile(SDL_Renderer * renderer, const char * path);
        // destorys the texture
        void Destroy();

        //return the texture
        SDL_Texture * GetSDLTexture() const {return m_Texture;}

        //return the width and height
        int GetWidth() const {return m_Width;}
        int GetHeight() const {return m_Height;}

    private:
        SDL_Texture * m_Texture = nullptr;
        int m_Width;
        int m_Height;
    };
}
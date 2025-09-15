#pragma once

#include<glad/glad.h>
#include <string>

namespace SIMPEngine
{
    class Texture
    {
    public:
        Texture();
        ~Texture();

        //loading texture from a image file(png, jpg etc)
        bool LoadFromFile(const char * path);
        // destorys the texture
        void Destroy();

        //return the texture
        GLuint GetID() const {return m_TextureID;}

        //return the width and height
        int GetWidth() const {return m_Width;}
        int GetHeight() const {return m_Height;}

    private:
        GLuint m_TextureID = 0;
        int m_Width;
        int m_Height;
    };
}
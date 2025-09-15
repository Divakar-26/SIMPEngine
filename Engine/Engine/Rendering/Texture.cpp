#include "Texture.h"
#include "Core/Log.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace SIMPEngine
{

    Texture::Texture() {}

    Texture::~Texture()
    {
        Destroy();
    }

    bool Texture::LoadFromFile(const char *path)
    {
        Destroy();

        int channel;
        unsigned char *data = stbi_load(path, &m_Width, &m_Height, &channel, 0);
        if (!data)
        {
            CORE_ERROR("Failed to load image : {}", path);
            return false;
        }

        GLenum format = (channel == 4) ? GL_RGBA : GL_RGB;
        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);


        //setting up the properties of the texture, like wrapping and filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
        return true;
    }

    void Texture::Destroy()
    {
        if(m_TextureID){
            glDeleteTextures(1, &m_TextureID);
            m_TextureID = 0;
            m_Width = m_Height = 0;
        }
    }

}

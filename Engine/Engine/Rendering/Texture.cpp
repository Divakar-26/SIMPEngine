#include <Engine/Rendering/Texture.h>
#include <Engine/Core/Log.h>
#include <iostream>

#define STBI_NO_SIMD
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

        m_Path = (std::string)path;

        stbi_set_flip_vertically_on_load(true);
        int channel;
        unsigned char *data = stbi_load(path, &m_Width, &m_Height, &channel, 4);
        if (!data)
        {
            CORE_ERROR("Failed to load image : {}", path);
            return false;
        }

        if (m_Width <= 0 || m_Height <= 0)
        {
            CORE_ERROR("Invalid image dimensions: {}x{} ch={}", m_Width, m_Height, channel);
            stbi_image_free(data);
            return false;
        }

        const GLenum format = GL_RGBA;
        glGenTextures(1, &m_TextureID);
        if (m_TextureID == 0)
        {
            CORE_ERROR("Failed to generate OpenGL texture for: {}", path);
            stbi_image_free(data);
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            CORE_ERROR("OpenGL texture upload failed ({}): {}", error, path);
            glDeleteTextures(1, &m_TextureID);
            m_TextureID = 0;
            stbi_image_free(data);
            return false;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_Filter = TextureFilter::Nearest;

        stbi_image_free(data);
        return true;
    }

    void Texture::SetFilter(TextureFilter filter)
    {
        m_Filter = filter;
        if (!m_TextureID)
            return;

        GLint mode = filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::Destroy()
    {
        if (m_TextureID)
        {
            glDeleteTextures(1, &m_TextureID);
            m_TextureID = 0;
            m_Width = m_Height = 0;
        }
    }

    bool Texture::LoadFromMemory(unsigned char *data, size_t size)
    {
        Destroy();
        stbi_set_flip_vertically_on_load(true);

        int width, height, channels;
        unsigned char *pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
        if (!pixels)
            return false;

        UploadToGPU(pixels, width, height, 4);

        stbi_image_free(pixels);
        return true;
    }

    void Texture::UploadToGPU(unsigned char *pixels, int width, int height, int channels)
    {
        (void)channels;
        Destroy();
        m_Width = width;
        m_Height = height;
        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_Filter = TextureFilter::Linear;
    }
}

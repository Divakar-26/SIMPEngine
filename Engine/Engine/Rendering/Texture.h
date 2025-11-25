#pragma once

#include <glad/glad.h>
#include <string>

namespace SIMPEngine
{
    class Texture
    {
    public:
        Texture();
        ~Texture();

        // loading texture from a image file(png, jpg etc)
        bool LoadFromFile(const char *path);
        // destorys the texture
        void Destroy();

        // return the texture
        GLuint GetID() const { return m_TextureID; }

        // return the width and height
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        std::string GetPath() { return m_Path; }
        std::string GetVFSPath() {return m_VFSPath;}

        void SetVFSPath(std::string vfsPath) { m_VFSPath = vfsPath ;}

        bool LoadFromMemory(unsigned char *data, size_t size);
        void UploadToGPU(unsigned char *pixels, int width, int height, int channels);

    private:
        GLuint m_TextureID = 0;
        int m_Width;
        int m_Height;

        std::string m_Path = "";
        std::string m_VFSPath = "";
    };
}
#pragma once

#include <glad/glad.h>
#include <string>

namespace SIMPEngine
{
    enum class TextureFilter
    {
        Nearest,
        Linear
    };

    class Texture
    {
    public:
        Texture();
        ~Texture();

        bool LoadFromFile(const char *path);
        void Destroy();

        GLuint GetID() const { return m_TextureID; }

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        std::string GetPath() { return m_Path; }
        std::string GetVFSPath() {return m_VFSPath;}

        void SetVFSPath(std::string vfsPath) { m_VFSPath = vfsPath ;}

        void SetFilter(TextureFilter filter);
        TextureFilter GetFilter() const { return m_Filter; }

        bool LoadFromMemory(unsigned char *data, size_t size);
        void UploadToGPU(unsigned char *pixels, int width, int height, int channels);

    private:
        GLuint m_TextureID = 0;
        int m_Width = 0;
        int m_Height = 0;
        TextureFilter m_Filter = TextureFilter::Nearest;

        std::string m_Path = "";
        std::string m_VFSPath = "";
    };
}

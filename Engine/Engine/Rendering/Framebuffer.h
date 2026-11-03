#pragma once

namespace SIMPEngine
{
    class Framebuffer
    {
    public:
        Framebuffer(int width = 1, int height = 1);
        ~Framebuffer();

        Framebuffer(const Framebuffer &) = delete;
        Framebuffer &operator=(const Framebuffer &) = delete;

        void Resize(int width, int height);

        unsigned int GetID() const { return m_ID; }
        unsigned int GetColorAttachment() const { return m_ColorAttachment; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

    private:
        void Invalidate();
        void Release();

        unsigned int m_ID = 0;
        unsigned int m_ColorAttachment = 0;
        int m_Width = 0;
        int m_Height = 0;
    };
}

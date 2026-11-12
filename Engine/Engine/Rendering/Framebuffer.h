#pragma once

#include <cstdint>
#include <vector>

namespace SIMPEngine
{
    class Framebuffer
    {
    public:
        Framebuffer(int width = 1, int height = 1, bool hdr = false, int samples = 1);
        ~Framebuffer();

        Framebuffer(const Framebuffer &) = delete;
        Framebuffer &operator=(const Framebuffer &) = delete;

        void Resize(int width, int height);
        void SetSamples(int samples);

        void Resolve();
        bool ReadPixels(std::vector<uint8_t> &rgba, bool topDown = true);

        unsigned int GetID() const { return m_ID; }
        unsigned int GetResolveID() const { return m_ResolveID ? m_ResolveID : m_ID; }
        unsigned int GetColorAttachment() const { return m_ColorAttachment; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        int GetSamples() const { return m_Samples; }
        bool IsHDR() const { return m_HDR; }
        bool IsMultisampled() const { return m_Samples > 1; }

        static int MaxSamples();
        static bool HdrSupported();

    private:
        void Invalidate();
        void Release();

        unsigned int m_ID = 0;
        unsigned int m_ResolveID = 0;
        unsigned int m_ColorAttachment = 0;
        unsigned int m_ColorRenderbuffer = 0;
        int m_Width = 0;
        int m_Height = 0;
        int m_Samples = 1;
        bool m_HDR = false;
    };
}

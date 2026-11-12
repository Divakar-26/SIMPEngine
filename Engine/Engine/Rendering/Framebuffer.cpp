#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Core/Log.h>

#include <Engine/Rendering/GL.h>
#include <algorithm>
#include <cstring>

namespace SIMPEngine
{
    namespace
    {
        struct BindingGuard
        {
            GLint Read = 0;
            GLint Draw = 0;

            BindingGuard()
            {
                glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &Read);
                glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &Draw);
            }

            ~BindingGuard()
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(Read));
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(Draw));
            }
        };

        void FlushPending()
        {
            if (Renderer::GetAPI())
                Renderer::Flush();
        }
    }

    Framebuffer::Framebuffer(int width, int height, bool hdr, int samples)
        : m_Width(std::max(1, width)), m_Height(std::max(1, height)), m_Samples(std::max(1, samples)), m_HDR(hdr)
    {
        Invalidate();
    }

    Framebuffer::~Framebuffer()
    {
        Release();
    }

    bool Framebuffer::HdrSupported()
    {
#if defined(SIMP_GLES)
        static int supported = -1;
        if (supported < 0)
        {
            supported = 0;
            GLint count = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &count);
            for (GLint i = 0; i < count; ++i)
            {
                const char *name = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
                if (name && (std::strcmp(name, "GL_EXT_color_buffer_float") == 0 || std::strcmp(name, "GL_EXT_color_buffer_half_float") == 0))
                    supported = 1;
            }
            if (!supported)
                CORE_WARN("GLES: no float render targets (EXT_color_buffer_float); HDR effects fall back to 8 bit");
        }
        return supported == 1;
#else
        return true;
#endif
    }

    int Framebuffer::MaxSamples()
    {
        GLint value = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &value);
        return std::max(1, static_cast<int>(value));
    }

    void Framebuffer::Release()
    {
        if (m_ColorAttachment)
            glDeleteTextures(1, &m_ColorAttachment);
        if (m_ColorRenderbuffer)
            glDeleteRenderbuffers(1, &m_ColorRenderbuffer);
        if (m_ResolveID)
            glDeleteFramebuffers(1, &m_ResolveID);
        if (m_ID)
            glDeleteFramebuffers(1, &m_ID);
        m_ColorAttachment = 0;
        m_ColorRenderbuffer = 0;
        m_ResolveID = 0;
        m_ID = 0;
    }

    void Framebuffer::Resize(int width, int height)
    {
        width = std::max(1, width);
        height = std::max(1, height);
        if (width == m_Width && height == m_Height && m_ID)
            return;

        m_Width = width;
        m_Height = height;
        Invalidate();
    }

    void Framebuffer::SetSamples(int samples)
    {
        samples = std::max(1, samples);
        if (samples == m_Samples && m_ID)
            return;
        m_Samples = samples;
        Invalidate();
    }

    void Framebuffer::Invalidate()
    {
        Release();

        m_Samples = std::min(m_Samples, MaxSamples());
        if (m_HDR && !HdrSupported())
            m_HDR = false;
        const GLenum internalFormat = m_HDR ? GL_RGBA16F : GL_RGBA8;

        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), m_Width, m_Height, 0, GL_RGBA, m_HDR ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (m_Samples > 1)
        {
            glGenFramebuffers(1, &m_ResolveID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                CORE_ERROR("Framebuffer resolve target {}x{} is incomplete", m_Width, m_Height);

            glGenRenderbuffers(1, &m_ColorRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, m_ColorRenderbuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, internalFormat, m_Width, m_Height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glGenFramebuffers(1, &m_ID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_ColorRenderbuffer);
        }
        else
        {
            glGenFramebuffers(1, &m_ID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            CORE_ERROR("Framebuffer {}x{} ({}x MSAA) is incomplete", m_Width, m_Height, m_Samples);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::Resolve()
    {
        if (m_Samples <= 1 || !m_ID || !m_ResolveID)
            return;

        FlushPending();
        BindingGuard guard;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveID);
        glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    bool Framebuffer::ReadPixels(std::vector<uint8_t> &rgba, bool topDown)
    {
        if (!m_ID)
            return false;

        FlushPending();
        Resolve();

        const size_t stride = static_cast<size_t>(m_Width) * 4;
        rgba.resize(stride * static_cast<size_t>(m_Height));
        {
            BindingGuard guard;
            glBindFramebuffer(GL_READ_FRAMEBUFFER, GetResolveID());
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
        }

        if (topDown)
        {
            std::vector<uint8_t> row(stride);
            for (int y = 0; y < m_Height / 2; ++y)
            {
                uint8_t *a = rgba.data() + static_cast<size_t>(y) * stride;
                uint8_t *b = rgba.data() + static_cast<size_t>(m_Height - 1 - y) * stride;
                std::memcpy(row.data(), a, stride);
                std::memcpy(a, b, stride);
                std::memcpy(b, row.data(), stride);
            }
        }
        return true;
    }
}

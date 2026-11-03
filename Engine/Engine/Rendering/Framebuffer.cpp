#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Core/Log.h>

#include <glad/glad.h>
#include <algorithm>

namespace SIMPEngine
{
    Framebuffer::Framebuffer(int width, int height)
        : m_Width(std::max(1, width)), m_Height(std::max(1, height))
    {
        Invalidate();
    }

    Framebuffer::~Framebuffer()
    {
        Release();
    }

    void Framebuffer::Release()
    {
        if (m_ColorAttachment)
            glDeleteTextures(1, &m_ColorAttachment);
        if (m_ID)
            glDeleteFramebuffers(1, &m_ID);
        m_ColorAttachment = 0;
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

    void Framebuffer::Invalidate()
    {
        Release();

        glGenFramebuffers(1, &m_ID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            CORE_ERROR("Framebuffer {}x{} is incomplete", m_Width, m_Height);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

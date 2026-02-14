#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/Shaders/SpriteShader.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace SIMPEngine
{

    GLRenderingAPI::GLRenderingAPI() {}
    GLRenderingAPI::~GLRenderingAPI()
    {
        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
        if (m_EBO)
            glDeleteBuffers(1, &m_EBO);
        if (m_LineVAO)
            glDeleteVertexArrays(1, &m_LineVAO);
        if (m_LineVBO)
            glDeleteBuffers(1, &m_LineVBO);
    }

    void GLRenderingAPI::Init()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);

        // Shader Program
        m_Shader = std::make_unique<Shader>(
            Shaders::SPRITE_VERT,
            Shaders::SPRITE_FRAG);

        // ------ QUAD ------
        float vertices[] = {
            // pos       //uv
            -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f};

        unsigned int indices[6] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ------ LINE -------

        glGenVertexArrays(1, &m_LineVAO);
        glGenBuffers(1, &m_LineVBO);

        glBindVertexArray(m_LineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // InitFramebuffer(m_ViewportWidth, m_ViewportHeight);
    }

    void GLRenderingAPI::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        float vertices[] = {x1, y1, x2, y2};

        m_Shader->Bind();

        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f("uColor",
                               color.r / 255.0f,
                               color.g / 255.0f,
                               color.b / 255.0f,
                               color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", 0);

        glBindVertexArray(m_LineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_LINES, 0, 2);

        glBindVertexArray(0);

        m_Shader->Unbind();
    }

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, float rotation, SDL_Color color, bool fill, float zIndex)
    {
        ApplyCommonSpriteState(x, y, width, height, rotation, zIndex, color, false);

        glBindVertexArray(m_VAO);
        if (fill)
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        glBindVertexArray(0);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect *srcRect)
    {
        if (!texture)
        {
            return;
        }

        if (srcRect)
        {
            float texWidth = texture->GetWidth();
            float texHeight = texture->GetHeight();

            float u1 = srcRect->x / texWidth;
            float v1 = srcRect->y / texHeight;
            float u2 = (srcRect->x + srcRect->w) / texWidth;
            float v2 = (srcRect->y + srcRect->h) / texHeight;

            float newVertices[] = {
                -0.5f, 0.5f, 0.0f, u1, v2, // Top-left
                0.5f, 0.5f, 0.0f, u2, v2,  // Top-right
                0.5f, -0.5f, 0.0f, u2, v1, // Bottom-right
                -0.5f, -0.5f, 0.0f, u1, v1 // Bottom-left
            };

            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newVertices), newVertices);

            glBindVertexArray(m_VAO);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }

        ApplyCommonSpriteState(x, y, width, height, rotation, zIndex, color, true);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->GetID());
        m_Shader->SetUniform1i("uTexture", 0);

        glDepthMask(GL_FALSE);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        glBindTexture(GL_TEXTURE_2D, 0);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::ApplyCommonSpriteState(
        float x, float y, float width, float height,
        float rotation, float zIndex,
        SDL_Color color, bool useTexture)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f(
            "uColor",
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", useTexture ? 1 : 0);
    }

    void GLRenderingAPI::SetViewMatrix(const glm::mat4 &view) { m_ViewMatrix = view; }

    // to be only used by editor later
    void GLRenderingAPI::BeginFrame()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
    }

    // same to be used by editor later do not touch it
    void GLRenderingAPI::EndFrame() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    void GLRenderingAPI::InitFramebuffer(int width, int height)
    {
        if (m_Framebuffer)
        {
            glDeleteFramebuffers(1, &m_Framebuffer);
            glDeleteTextures(1, &m_ColorAttachment);
            glDeleteRenderbuffers(1, &m_RBO);
        }
        glGenFramebuffers(1, &m_Framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        glGenRenderbuffers(1, &m_RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderingAPI::ResizeViewport(int width, int height)
    {
        glViewport(0, 0, width, height);
        std::cout << "Viewport changed" << std::endl;
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        InitFramebuffer(width, height);
    }

    unsigned int GLRenderingAPI::GetViewportTexture() { return m_ColorAttachment; }

    void GLRenderingAPI::SetClearColor(float r, float g, float b, float a)
    {
        m_ClearColor[0] = r;
        m_ClearColor[1] = g;
        m_ClearColor[2] = b;
        m_ClearColor[3] = a;
    }

    void GLRenderingAPI::Clear()
    {
        glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLRenderingAPI::Present() {}

    glm::vec2 GLRenderingAPI::TransformPosition(float x, float y) const { return {x, y}; }
    glm::vec2 GLRenderingAPI::TransformSize(float w, float h) const { return {w, h}; }

    void GLRenderingAPI::SetProjectionMatrix(const glm::mat4 &proj)
    {
        m_Projection = proj;
    }

} // namespace SIMPEngine
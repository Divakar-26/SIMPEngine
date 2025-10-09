#include "GLRenderingAPI.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace SIMPEngine
{
    static const char *vertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTex;
        uniform mat4 uMVP;
        out vec2 TexCoord;
        void main() {
            gl_Position = uMVP * vec4(aPos, 1.0);
            TexCoord = aTex;
        }
    )";

    static const char *fragmentSrc = R"(    
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform vec4 uColor;
        uniform sampler2D uTexture;
        uniform bool uUseTexture;
        void main() {
            if(uUseTexture) {
                vec4 tex = texture(uTexture, TexCoord);
                FragColor = vec4(tex.rgb * uColor.rgb, tex.a * uColor.a);
            } else {
                FragColor = uColor;
            }
        }
    )";

    GLRenderingAPI::GLRenderingAPI() {}
    GLRenderingAPI::~GLRenderingAPI()
    {
        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
        if (m_EBO)
            glDeleteBuffers(1, &m_EBO);
    }

    void GLRenderingAPI::Init()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
        m_Shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
        float vertices[] = {
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

        SetProjection(m_ViewportWidth, m_ViewportHeight);
        InitFramebuffer(m_ViewportWidth, m_ViewportHeight);
        SetProjection(m_ViewportWidth, m_ViewportHeight);
    }

    void GLRenderingAPI::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        m_Shader->Bind();
        float vertices[] = {x1, y1, x2, y2};
        GLuint lineVAO, lineVBO;
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f("uColor", color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", 0);
        glDrawArrays(GL_LINES, 0, 2);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glDeleteBuffers(1, &lineVBO);
        glDeleteVertexArrays(1, &lineVAO);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::SetProjection(float width, float height)
    {
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        // Origin at center
        m_Projection = glm::ortho(-halfWidth, halfWidth, halfHeight, -halfHeight, -1000.0f, 1000.0f);
    }

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

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, SDL_Color color, bool fill, float zIndex)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));
        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;
        m_Shader->Bind();
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f("uColor", color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", 0);
        glBindVertexArray(m_VAO);
        if (fill)
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        glBindVertexArray(0);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::DrawTexture(GLuint texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        if (!texture)
            return;
        // glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        // model = glm::translate(model, glm::vec3(width * 0.5f, height * 0.5f, 0.0f));
        // model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        // model = glm::translate(model, glm::vec3(-width * 0.5f, -height * 0.5f, 0.0f));
        // model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;
        m_Shader->Bind();
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f("uColor", color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        m_Shader->SetUniform1i("uTexture", 0);
        glDepthMask(GL_FALSE);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::SetViewMatrix(const glm::mat4 &view) { m_ViewMatrix = view; }

    void GLRenderingAPI::BeginFrame()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
    }

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
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        InitFramebuffer(width, height);
        SetProjection(width, height);
    }

    unsigned int GLRenderingAPI::GetViewportTexture() { return m_ColorAttachment; }

} // namespace SIMPEngine

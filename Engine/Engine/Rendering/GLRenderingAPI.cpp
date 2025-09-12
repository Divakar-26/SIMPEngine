#include "GLRenderingAPI.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace SIMPEngine
{
    static const char *vertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 uMVP;
        uniform vec4 uColor;
        out vec4 fragColor;
        void main() { gl_Position = uMVP * vec4(aPos,1.0); fragColor = uColor; }
    )";

    static const char *fragmentSrc = R"(
        #version 330 core
        in vec4 fragColor;
        out vec4 FragColor;
        void main() { FragColor = fragColor; }
    )";

    GLRenderingAPI::GLRenderingAPI() {}
    GLRenderingAPI::~GLRenderingAPI()
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void GLRenderingAPI::Init()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);

        m_Shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);

        float vertices[12] = {
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f};

        unsigned int indices[6] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        SetProjection(m_ViewportWidth, m_ViewportHeight);
    }

    void GLRenderingAPI::SetProjection(float width, float height)
    {
        m_Projection = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    }

    void GLRenderingAPI::ResizeViewport(int width, int height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        glViewport(0, 0, width, height);
        SetProjection(width, height);
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

    glm::vec2 GLRenderingAPI::TransformPosition(float x, float y) const
    {
        return {x, y}; 
    }

    glm::vec2 GLRenderingAPI::TransformSize(float w, float h) const
    {
        return {w, h}; 
    }

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, SDL_Color color, bool fill)
    {
        glm::vec2 pos = {x, y};
        glm::vec2 size = {width, height};

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f));
        model = glm::scale(model, glm::vec3(size, 1.0f));
        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f("uColor",
                               color.r / 255.0f,
                               color.g / 255.0f,
                               color.b / 255.0f,
                               color.a / 255.0f);

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_Shader->Unbind();
    }

    void GLRenderingAPI::SetViewMatrix(const glm::mat4 &view)
    {
        m_ViewMatrix = view;
    }

    unsigned int GLRenderingAPI::GetViewportTexture()
    {
        return 0;
    }

    void GLRenderingAPI::BeginFrame()
    {
        Clear();
    }

    void GLRenderingAPI::EndFrame()
    {
    }

}

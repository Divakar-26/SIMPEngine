#include "GLRenderingAPI.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace SIMPEngine
{
    // Vertex shader now passes texcoords and supports toggling texture usage
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

    // Fragment shader samples texture only when uUseTexture == 1
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
                FragColor = tex * uColor;
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

        // positions (x,y,z) + texcoords (u,v)
        float vertices[] = {
            // pos            // tex
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // top-left
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // bottom-right
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f  // bottom-left
        };

        unsigned int indices[6] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        // texcoord attribute (location = 1)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        // No texture: set uUseTexture = false
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
        m_Shader->SetUniform1i("uUseTexture", 0);

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_Shader->Unbind();
    }

    // Implemented DrawTexture using texture id and rotation (degrees).
    // rotation: clockwise degrees (but glm::rotate uses radians counter-clockwise, so we convert)
    void GLRenderingAPI::DrawTexture(GLuint texture, float x, float y, float width, float height, SDL_Color color, float rotation)
    {
        // Build model matrix: translate -> rotate around center -> scale
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));

        // move origin to center, rotate, move back
        model = glm::translate(model, glm::vec3(width * 0.5f, height * 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(-width * 0.5f, -height * 0.5f, 0.0f));

        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4("uMVP", mvp);
        m_Shader->SetUniform4f("uColor",
                               color.r / 255.0f,
                               color.g / 255.0f,
                               color.b / 255.0f,
                               color.a / 255.0f);

        // Tell shader to use texture
        m_Shader->SetUniform1i("uUseTexture", 1);

        // Bind the texture to unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        m_Shader->SetUniform1i("uTexture", 0);

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Unbind texture for cleanliness
        glBindTexture(GL_TEXTURE_2D, 0);

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

} // namespace SIMPEngine

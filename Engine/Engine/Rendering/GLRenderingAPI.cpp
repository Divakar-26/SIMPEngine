// GLRenderingAPI.cpp
#include "GLRenderingAPI.h"
#include <glad/glad.h>
#include <iostream>

namespace SIMPEngine
{
    static const char *vertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform vec4 uColor;
        out vec4 fragColor;
        void main()
        {
            gl_Position = vec4(aPos, 1.0);
            fragColor = uColor;
        }
    )";

    static const char *fragmentSrc = R"(
        #version 330 core
        in vec4 fragColor;
        out vec4 FragColor;
        void main()
        {
            FragColor = fragColor;
        }
    )";

    GLRenderingAPI::GLRenderingAPI() {}

    GLRenderingAPI::~GLRenderingAPI()
    {
        glDeleteProgram(m_ShaderProgram);
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void GLRenderingAPI::Init()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glViewport(0, 0, 1920, 1080);

        CreateShaders();

        // Quad vertices (NDC space, will update in DrawQuad)
        float vertices[12] = {
            -0.5f, 0.5f, 0.0f, // top-left
            0.5f, 0.5f, 0.0f,  // top-right
            0.5f, -0.5f, 0.0f, // bottom-right
            -0.5f, -0.5f, 0.0f // bottom-left
        };

        unsigned int indices[6] = {
            0, 1, 2,
            2, 3, 0};

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

    void GLRenderingAPI::Present()
    {
        // SDL_GL_SwapWindow(window) outside
    }

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, SDL_Color color, bool fill)
    {
        float viewportWidth = 1920.0f;
        float viewportHeight = 1080.0f;

        float ndcX = (x / viewportWidth) * 2.0f - 1.0f;
        float ndcY = 1.0f - (y / viewportHeight) * 2.0f;
        float ndcWidth = (width / viewportWidth) * 2.0f;
        float ndcHeight = (height / viewportHeight) * 2.0f;

        float vertices[] = {
            ndcX, ndcY, 0.0f,
            ndcX + ndcWidth, ndcY, 0.0f,
            ndcX + ndcWidth, ndcY - ndcHeight, 0.0f,
            ndcX, ndcY - ndcHeight, 0.0f};

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glUseProgram(m_ShaderProgram);
        int colorLoc = glGetUniformLocation(m_ShaderProgram, "uColor");
        glUniform4f(colorLoc,
                    color.r / 255.0f,
                    color.g / 255.0f,
                    color.b / 255.0f,
                    color.a / 255.0f);

        glBindVertexArray(m_VAO);
        if (fill)
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_LINE_LOOP, 0, 4);

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void GLRenderingAPI::CreateShaders()
    {
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexSrc, NULL);
        glCompileShader(vertex);

        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentSrc, NULL);
        glCompileShader(fragment);

        m_ShaderProgram = glCreateProgram();
        glAttachShader(m_ShaderProgram, vertex);
        glAttachShader(m_ShaderProgram, fragment);
        glLinkProgram(m_ShaderProgram);

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        int success;
        char infoLog[512];
        glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(m_ShaderProgram, 512, NULL, infoLog);
            std::cerr << "Shader Linking Failed: " << infoLog << std::endl;
        }
    }


}

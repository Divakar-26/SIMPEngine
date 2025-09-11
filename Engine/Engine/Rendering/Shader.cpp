#include "Shader.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace SIMPEngine
{
    Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        unsigned int vertex = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        unsigned int fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

        m_ID = glCreateProgram();
        glAttachShader(m_ID, vertex);
        glAttachShader(m_ID, fragment);
        glLinkProgram(m_ID);

        int success;
        char infoLog[512];
        glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(m_ID, 512, NULL, infoLog);
            std::cerr << "Shader Linking Failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader::~Shader() { glDeleteProgram(m_ID); }

    void Shader::Bind() const { glUseProgram(m_ID); }
    void Shader::Unbind() const { glUseProgram(0); }

    void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
    {
        glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
    }

    void Shader::SetUniformMat4(const std::string &name, const glm::mat4 &matrix)
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
    {
        unsigned int shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Shader Compilation Failed: " << infoLog << std::endl;
        }

        return shader;
    }

    int Shader::GetUniformLocation(const std::string &name) const
    {
        return glGetUniformLocation(m_ID, name.c_str());
    }
}

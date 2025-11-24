#include <Engine/Rendering/Shader.h>
#include <Engine/Core/Log.h>

#include <fstream>
#include <sstream>

namespace SIMPEngine
{
    Shader::Shader(const std::string &vertexSource, const std::string &fragmentSource)
    {
        unsigned int vert = CompileShader(GL_VERTEX_SHADER, vertexSource);
        unsigned int frag = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

        if (!vert || !frag)
        {
            CORE_ERROR("Shader compilation failed!");
            m_ID = 0;
            return;
        }

        m_ID = glCreateProgram();
        glAttachShader(m_ID, vert);
        glAttachShader(m_ID, frag);
        glLinkProgram(m_ID);

        int success;
        glGetProgramiv(m_ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetProgramInfoLog(m_ID, 1024, nullptr, infoLog);
            CORE_ERROR("Shader linking failed: {}", infoLog);
            m_ID = 0;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    Shader::~Shader()
    {
        glDeleteProgram(m_ID);
    }

    unsigned int Shader::CompileShader(unsigned int type, const std::string &source)
    {
        unsigned int shaderID = glCreateShader(type);
        const char *src = source.c_str();
        glShaderSource(shaderID, 1, &src, nullptr);
        glCompileShader(shaderID);

        int compiled;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char infoLog[1024];
            glGetShaderInfoLog(shaderID, 1024, nullptr, infoLog);
            CORE_ERROR("Shader compilation failed: {}", infoLog);
            return 0;
        }

        return shaderID;
    }

    void Shader::Bind() const
    {
        glUseProgram(m_ID);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }

    void Shader::SetUniform1i(const std::string &name, int value)
    {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3)
    {
        glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
    }

    void Shader::SetUniformMat4(const std::string &name, const glm::mat4 &matrix)
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
    }

    int Shader::GetUniformLocation(const std::string &name) const
    {
        int location = glGetUniformLocation(m_ID, name.c_str());
        if (location == -1)
            CORE_WARN("Uniform '{}' not found in shader!", name);

        return location;
    }
}

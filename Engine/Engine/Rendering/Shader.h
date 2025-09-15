#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace SIMPEngine
{
    class Shader
    {
    public:
        Shader(const std::string &vertexSrc, const std::string &fragmentSrc);
        ~Shader();

        void Bind() const;
        void Unbind() const;
        void SetUniform1i(const std::string &name, int value); // <-- NEW
        void SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3);
        void SetUniformMat4(const std::string &name, const glm::mat4 &matrix);
        void SetMat4(const std::string &name, const glm::mat4 &matrix);
        unsigned int GetID() const { return m_ID; }

    private:
        unsigned int m_ID;

        unsigned int CompileShader(unsigned int type, const std::string &source);
        int GetUniformLocation(const std::string &name) const;
    };
}

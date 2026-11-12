#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    enum class ShaderUniformType : int
    {
        Float = 0,
        Vec2 = 1,
        Vec3 = 2,
        Vec4 = 3,
        Color = 4,
        Texture = 5,
        Int = 6,
        Bool = 7
    };

    struct ShaderUniformDecl
    {
        std::string Name;
        ShaderUniformType Type = ShaderUniformType::Float;
        float Default[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float Min = 0.0f;
        float Max = 1.0f;
    };

    class ShaderEffect
    {
    public:
        ~ShaderEffect();

        static std::shared_ptr<ShaderEffect> Load(const std::string &path);
        static std::shared_ptr<ShaderEffect> FromSource(const std::string &name, const std::string &source);
        static void ReleaseAll();

        static std::string BuildSource(const std::string &userSource);
        static const char *VertexSource();
        static std::vector<ShaderUniformDecl> ParseUniforms(const std::string &userSource);
        static bool IsBuiltinUniform(const std::string &name);

        bool IsValid() const { return m_Program != 0; }
        const std::string &GetError() const { return m_Error; }
        const std::string &GetPath() const { return m_Path; }
        unsigned int GetProgram() const { return m_Program; }
        uint32_t GetVersion() const { return m_Version; }
        const std::vector<ShaderUniformDecl> &GetDeclared() const { return m_Declared; }
        int Location(const std::string &name) const;

        void Reload(const std::string &source);

    private:
        ShaderEffect() = default;
        void Compile(const std::string &source);
        void Release();

        std::string m_Path;
        std::string m_Error;
        std::string m_Source;
        unsigned int m_Program = 0;
        uint32_t m_Version = 0;
        long long m_Stamp = 0;
        unsigned int m_LastCheck = 0;
        std::vector<ShaderUniformDecl> m_Declared;
        mutable std::unordered_map<std::string, int> m_Locations;
    };
}

#include <Engine/Rendering/ShaderEffect.h>

#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/Shaders/GLSLHeader.h>

#include <Engine/Rendering/GL.h>

#include <SDL3/SDL_timer.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <sstream>

namespace SIMPEngine
{
    namespace
    {
#if defined(SIMP_GLES)
        constexpr const char *VersionLine = "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n";
#else
        constexpr const char *VersionLine = "#version 400 core\n";
#endif

        constexpr const char *FragmentHeader =
            "in vec2 uv;\n"
            "uniform sampler2D uTexture;\n"
            "uniform float uTime;\n"
            "uniform vec2 uResolution;\n"
            "uniform vec2 uMouse;\n"
            "uniform float uProgress;\n"
            "uniform vec4 uRegion;\n"
            "out vec4 color;\n"
            "#line 1\n";

        std::unordered_map<std::string, std::shared_ptr<ShaderEffect>> s_Cache;
        bool s_ContextAlive = true;

        std::string Trim(const std::string &text)
        {
            size_t begin = 0, end = text.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
                ++begin;
            while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
                --end;
            return text.substr(begin, end - begin);
        }

        std::vector<float> ParseNumbers(const std::string &text)
        {
            std::vector<float> values;
            std::stringstream stream(text);
            std::string item;
            while (std::getline(stream, item, ','))
            {
                const std::string trimmed = Trim(item);
                if (trimmed.empty())
                    continue;
                char *end = nullptr;
                const float value = std::strtof(trimmed.c_str(), &end);
                if (end != trimmed.c_str())
                    values.push_back(value);
            }
            return values;
        }

        unsigned int CompileStage(unsigned int type, const std::string &source, std::string &error, const std::string &label)
        {
            unsigned int shader = glCreateShader(type);
            const char *text = source.c_str();
            glShaderSource(shader, 1, &text, nullptr);
            glCompileShader(shader);
            int compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled)
                return shader;
            char log[2048];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            error = label + ": " + log;
            glDeleteShader(shader);
            return 0;
        }

        long long Stamp(const std::string &real)
        {
            std::error_code ec;
            const auto time = std::filesystem::last_write_time(std::filesystem::path(real), ec);
            return ec ? 0 : static_cast<long long>(time.time_since_epoch().count());
        }
    }

    ShaderEffect::~ShaderEffect()
    {
        Release();
    }

    void ShaderEffect::Release()
    {
        if (m_Program && s_ContextAlive)
            glDeleteProgram(m_Program);
        m_Program = 0;
        m_Locations.clear();
    }

    bool ShaderEffect::IsBuiltinUniform(const std::string &name)
    {
        return name == "uTexture" || name == "uTime" || name == "uResolution" || name == "uMouse" || name == "uProgress" || name == "uRegion";
    }

    const char *ShaderEffect::VertexSource()
    {
        static const std::string source = std::string(VersionLine) +
                                          "out vec2 uv;\n"
                                          "void main()\n"
                                          "{\n"
                                          "    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
                                          "    uv = p;\n"
                                          "    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
                                          "}\n";
        return source.c_str();
    }

    std::string ShaderEffect::BuildSource(const std::string &userSource)
    {
        if (userSource.find("#version") != std::string::npos)
            return userSource;
        return std::string(VersionLine) + FragmentHeader + userSource;
    }

    std::vector<ShaderUniformDecl> ShaderEffect::ParseUniforms(const std::string &userSource)
    {
        static const std::regex declaration(R"(^\s*uniform\s+(float|vec2|vec3|vec4|int|bool|sampler2D)\s+([A-Za-z_]\w*)\s*;(.*)$)");
        static const std::regex range(R"(range\s*\(([^)]*)\))");
        static const std::regex defaults(R"(default\s*\(([^)]*)\))");

        std::vector<ShaderUniformDecl> result;
        std::stringstream stream(userSource);
        std::string line;
        while (std::getline(stream, line))
        {
            std::smatch match;
            if (!std::regex_match(line, match, declaration))
                continue;
            ShaderUniformDecl decl;
            decl.Name = match[2].str();
            if (IsBuiltinUniform(decl.Name))
                continue;

            const std::string type = match[1].str();
            const std::string comment = match[3].str();
            int components = 1;
            if (type == "float")
                decl.Type = ShaderUniformType::Float;
            else if (type == "vec2")
            {
                decl.Type = ShaderUniformType::Vec2;
                components = 2;
            }
            else if (type == "vec3")
            {
                decl.Type = ShaderUniformType::Vec3;
                components = 3;
            }
            else if (type == "vec4")
            {
                decl.Type = ShaderUniformType::Vec4;
                components = 4;
            }
            else if (type == "int")
                decl.Type = ShaderUniformType::Int;
            else if (type == "bool")
                decl.Type = ShaderUniformType::Bool;
            else
                decl.Type = ShaderUniformType::Texture;

            if ((decl.Type == ShaderUniformType::Vec3 || decl.Type == ShaderUniformType::Vec4) && comment.find("color") != std::string::npos)
            {
                decl.Type = ShaderUniformType::Color;
                decl.Default[0] = decl.Default[1] = decl.Default[2] = decl.Default[3] = 1.0f;
            }

            std::smatch found;
            if (std::regex_search(comment, found, range))
            {
                const std::vector<float> values = ParseNumbers(found[1].str());
                if (values.size() >= 2)
                {
                    decl.Min = values[0];
                    decl.Max = values[1];
                }
            }
            if (std::regex_search(comment, found, defaults))
            {
                const std::vector<float> values = ParseNumbers(found[1].str());
                for (int i = 0; i < 4 && i < static_cast<int>(values.size()); ++i)
                    decl.Default[i] = values[i];
                if (values.size() == 1 && components > 1 && decl.Type != ShaderUniformType::Color)
                    for (int i = 1; i < components; ++i)
                        decl.Default[i] = values[0];
            }
            result.push_back(decl);
        }
        return result;
    }

    void ShaderEffect::Compile(const std::string &userSource)
    {
        Release();
        m_Error.clear();
        m_Declared = ParseUniforms(userSource);
        ++m_Version;

        std::string error;
        const std::string label = m_Path.empty() ? std::string("shader") : m_Path;
        unsigned int vertex = CompileStage(GL_VERTEX_SHADER, VertexSource(), error, label + " (vertex)");
        unsigned int fragment = vertex ? CompileStage(GL_FRAGMENT_SHADER, BuildSource(userSource), error, label) : 0;
        if (!vertex || !fragment)
        {
            if (vertex)
                glDeleteShader(vertex);
            m_Error = error;
            CORE_ERROR("Shader error in {}", m_Error);
            return;
        }

        unsigned int program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        int linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char log[2048];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            m_Error = label + ": " + log;
            CORE_ERROR("Shader link error in {}", m_Error);
            glDeleteProgram(program);
            return;
        }
        m_Program = program;
    }

    void ShaderEffect::Reload(const std::string &source)
    {
        m_Source = source;
        Compile(source);
    }

    int ShaderEffect::Location(const std::string &name) const
    {
        if (!m_Program)
            return -1;
        auto it = m_Locations.find(name);
        if (it != m_Locations.end())
            return it->second;
        const int location = glGetUniformLocation(m_Program, name.c_str());
        m_Locations[name] = location;
        return location;
    }

    std::shared_ptr<ShaderEffect> ShaderEffect::FromSource(const std::string &name, const std::string &source)
    {
        std::shared_ptr<ShaderEffect> effect(new ShaderEffect());
        effect->m_Path = name;
        effect->Reload(source);
        return effect;
    }

    std::shared_ptr<ShaderEffect> ShaderEffect::Load(const std::string &path)
    {
        const unsigned int now = SDL_GetTicks();
        auto it = s_Cache.find(path);
        if (it != s_Cache.end())
        {
            ShaderEffect &effect = *it->second;
            if (now - effect.m_LastCheck < 250)
                return it->second;
            effect.m_LastCheck = now;
            if (const std::optional<std::string> real = VFS::Resolve(path))
            {
                const long long stamp = Stamp(*real);
                if (stamp != effect.m_Stamp)
                {
                    effect.m_Stamp = stamp;
                    if (std::optional<std::string> text = FileSystem::ReadText(*real))
                    {
                        CORE_INFO("Reloading shader {}", path);
                        effect.Reload(*text);
                    }
                }
            }
            return it->second;
        }

        std::shared_ptr<ShaderEffect> effect(new ShaderEffect());
        effect->m_Path = path;
        effect->m_LastCheck = now;
        const std::optional<std::string> real = VFS::Resolve(path);
        std::optional<std::string> text = real ? FileSystem::ReadText(*real) : std::nullopt;
        if (!text)
        {
            effect->m_Error = "Cannot read shader file " + path;
            CORE_ERROR("{}", effect->m_Error);
            effect->m_Stamp = real ? Stamp(*real) : 0;
        }
        else
        {
            effect->m_Stamp = Stamp(*real);
            effect->Reload(*text);
        }
        s_Cache[path] = effect;
        return effect;
    }

    void ShaderEffect::ReleaseAll()
    {
        s_Cache.clear();
        s_ContextAlive = false;
    }
}

#include <Engine/Particles/ParticleSerializer.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

#include <array>
#include <filesystem>
#include <fstream>

namespace SIMPEngine
{
    namespace
    {
        constexpr std::array<const char *, 8> ShapeNames = {"soft", "circle", "square", "spark", "star", "smoke", "ring", "texture"};
        constexpr std::array<const char *, 4> EmitterNames = {"point", "circle", "box", "line"};
        constexpr std::array<const char *, 3> BlendNames = {"alpha", "additive", "multiply"};

        template <size_t N>
        int IndexOf(const std::array<const char *, N> &names, const std::string &value, int fallback)
        {
            for (size_t i = 0; i < N; ++i)
                if (value == names[i])
                    return static_cast<int>(i);
            return fallback;
        }

        void EmitVec2(YAML::Emitter &out, const char *key, const glm::vec2 &v)
        {
            out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        }

        void EmitVec4(YAML::Emitter &out, const char *key, const glm::vec4 &v)
        {
            out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << v.r << v.g << v.b << v.a << YAML::EndSeq;
        }

        template <typename T>
        void Read(const YAML::Node &node, const char *key, T &out)
        {
            if (auto value = node[key])
            {
                try
                {
                    out = value.as<T>();
                }
                catch (const YAML::Exception &e)
                {
                    CORE_WARN("Particles: bad value for '{}': {}", key, e.what());
                }
            }
        }

        void ReadVec2(const YAML::Node &node, const char *key, glm::vec2 &out)
        {
            auto value = node[key];
            if (value && value.IsSequence() && value.size() >= 2)
                out = {value[0].as<float>(), value[1].as<float>()};
        }

        void ReadVec4(const YAML::Node &node, const char *key, glm::vec4 &out)
        {
            auto value = node[key];
            if (value && value.IsSequence() && value.size() >= 4)
                out = {value[0].as<float>(), value[1].as<float>(), value[2].as<float>(), value[3].as<float>()};
        }

        std::string Real(const std::string &path)
        {
            if (auto resolved = VFS::Resolve(path))
                return *resolved;
            return path;
        }
    }

    const char *ParticleSerializer::ShapeName(ParticleShape shape)
    {
        return ShapeNames[static_cast<size_t>(shape)];
    }

    const char *ParticleSerializer::EmitterName(EmitterShape shape)
    {
        return EmitterNames[static_cast<size_t>(shape)];
    }

    void ParticleSerializer::Emit(YAML::Emitter &out, const ParticleEffect &e)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << e.Name;
        out << YAML::Key << "looping" << YAML::Value << e.Looping;
        out << YAML::Key << "duration" << YAML::Value << e.Duration;
        out << YAML::Key << "rate" << YAML::Value << e.Rate;
        out << YAML::Key << "burst" << YAML::Value << e.Burst;
        out << YAML::Key << "max_particles" << YAML::Value << e.MaxParticles;
        out << YAML::Key << "prewarm" << YAML::Value << e.Prewarm;
        out << YAML::Key << "world_space" << YAML::Value << e.WorldSpace;

        out << YAML::Key << "emitter" << YAML::Value << EmitterName(e.Emitter);
        out << YAML::Key << "radius" << YAML::Value << e.Radius;
        EmitVec2(out, "box", e.BoxSize);
        out << YAML::Key << "edge" << YAML::Value << e.EmitFromEdge;

        EmitVec2(out, "lifetime", e.Lifetime);
        out << YAML::Key << "direction" << YAML::Value << e.Direction;
        out << YAML::Key << "spread" << YAML::Value << e.Spread;
        EmitVec2(out, "speed", e.Speed);
        EmitVec2(out, "gravity", e.Gravity);
        out << YAML::Key << "drag" << YAML::Value << e.Drag;
        out << YAML::Key << "radial" << YAML::Value << e.Radial;
        out << YAML::Key << "orbital" << YAML::Value << e.Orbital;
        out << YAML::Key << "turbulence" << YAML::Value << e.Turbulence;
        out << YAML::Key << "turbulence_frequency" << YAML::Value << e.TurbulenceFrequency;

        EmitVec2(out, "size", e.Size);
        out << YAML::Key << "end_size" << YAML::Value << e.EndSize;
        out << YAML::Key << "stretch" << YAML::Value << e.Stretch;
        EmitVec2(out, "start_rotation", e.StartRotation);
        EmitVec2(out, "angular_velocity", e.AngularVelocity);
        out << YAML::Key << "align_to_velocity" << YAML::Value << e.AlignToVelocity;

        EmitVec4(out, "start_color", e.StartColor);
        EmitVec4(out, "mid_color", e.MidColor);
        EmitVec4(out, "end_color", e.EndColor);
        out << YAML::Key << "use_mid_color" << YAML::Value << e.UseMidColor;
        out << YAML::Key << "hue_variation" << YAML::Value << e.HueVariation;
        out << YAML::Key << "fade_in" << YAML::Value << e.FadeIn;

        out << YAML::Key << "shape" << YAML::Value << ShapeName(e.Shape);
        if (e.CustomTexture && !e.CustomTexture->GetVFSPath().empty())
            out << YAML::Key << "texture" << YAML::Value << e.CustomTexture->GetVFSPath();
        out << YAML::Key << "blend" << YAML::Value << BlendNames[static_cast<size_t>(e.Blend)];
        out << YAML::Key << "emissive" << YAML::Value << e.Emissive;
        out << YAML::EndMap;
    }

    bool ParticleSerializer::Load(const YAML::Node &node, ParticleEffect &e)
    {
        if (!node || !node.IsMap())
            return false;

        Read(node, "name", e.Name);
        Read(node, "looping", e.Looping);
        Read(node, "duration", e.Duration);
        Read(node, "rate", e.Rate);
        Read(node, "burst", e.Burst);
        Read(node, "max_particles", e.MaxParticles);
        Read(node, "prewarm", e.Prewarm);
        Read(node, "world_space", e.WorldSpace);

        std::string text;
        Read(node, "emitter", text);
        e.Emitter = static_cast<EmitterShape>(IndexOf(EmitterNames, text, static_cast<int>(e.Emitter)));
        Read(node, "radius", e.Radius);
        ReadVec2(node, "box", e.BoxSize);
        Read(node, "edge", e.EmitFromEdge);

        ReadVec2(node, "lifetime", e.Lifetime);
        Read(node, "direction", e.Direction);
        Read(node, "spread", e.Spread);
        ReadVec2(node, "speed", e.Speed);
        ReadVec2(node, "gravity", e.Gravity);
        Read(node, "drag", e.Drag);
        Read(node, "radial", e.Radial);
        Read(node, "orbital", e.Orbital);
        Read(node, "turbulence", e.Turbulence);
        Read(node, "turbulence_frequency", e.TurbulenceFrequency);

        ReadVec2(node, "size", e.Size);
        Read(node, "end_size", e.EndSize);
        Read(node, "stretch", e.Stretch);
        ReadVec2(node, "start_rotation", e.StartRotation);
        ReadVec2(node, "angular_velocity", e.AngularVelocity);
        Read(node, "align_to_velocity", e.AlignToVelocity);

        ReadVec4(node, "start_color", e.StartColor);
        ReadVec4(node, "mid_color", e.MidColor);
        ReadVec4(node, "end_color", e.EndColor);
        Read(node, "use_mid_color", e.UseMidColor);
        Read(node, "hue_variation", e.HueVariation);
        Read(node, "fade_in", e.FadeIn);

        text.clear();
        Read(node, "shape", text);
        e.Shape = static_cast<ParticleShape>(IndexOf(ShapeNames, text, static_cast<int>(e.Shape)));
        std::string texture;
        Read(node, "texture", texture);
        if (!texture.empty())
        {
            e.CustomTexture = TextureManager::Get().LoadTexture(texture, texture);
            if (e.CustomTexture)
                e.CustomTexture->SetVFSPath(texture);
        }
        text.clear();
        Read(node, "blend", text);
        Read(node, "emissive", e.Emissive);
        e.Blend = static_cast<BlendMode>(IndexOf(BlendNames, text, static_cast<int>(e.Blend)));
        return true;
    }

    bool ParticleSerializer::SaveFile(const std::string &path, const ParticleEffect &effect)
    {
        std::string real = Real(path);
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(real).parent_path(), ec);

        std::ofstream file(real);
        if (!file.is_open())
        {
            CORE_ERROR("Particles: cannot write '{}'", real);
            return false;
        }

        YAML::Emitter out;
        out << YAML::BeginMap << YAML::Key << "ParticleEffect" << YAML::Value;
        Emit(out, effect);
        out << YAML::EndMap;
        file << out.c_str();
        return true;
    }

    bool ParticleSerializer::LoadFile(const std::string &path, ParticleEffect &effect)
    {
        try
        {
            YAML::Node root = YAML::LoadFile(Real(path));
            return Load(root["ParticleEffect"], effect);
        }
        catch (const YAML::Exception &e)
        {
            CORE_ERROR("Particles: cannot read '{}': {}", path, e.what());
            return false;
        }
    }
}

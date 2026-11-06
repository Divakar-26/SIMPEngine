#pragma once

#include <Engine/Particles/ParticleEffect.h>

#include <yaml-cpp/yaml.h>
#include <string>

namespace SIMPEngine
{
    class ParticleSerializer
    {
    public:
        static constexpr const char *Extension = ".particle";

        static void Emit(YAML::Emitter &out, const ParticleEffect &effect);
        static bool Load(const YAML::Node &node, ParticleEffect &effect);

        static bool SaveFile(const std::string &path, const ParticleEffect &effect);
        static bool LoadFile(const std::string &path, ParticleEffect &effect);

        static const char *ShapeName(ParticleShape shape);
        static const char *EmitterName(EmitterShape shape);
    };
}

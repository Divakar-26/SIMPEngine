#pragma once

#include <Engine/Particles/ParticleEffect.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace SIMPEngine
{
    struct ParticlePreset
    {
        std::string Name;
        std::string Category;
        std::string Description;
        ParticleEffect Effect;
        float ViewHeight = 260.0f;
        glm::vec2 Anchor{0.0f, 0.0f};
    };

    class ParticlePresets
    {
    public:
        static const std::vector<ParticlePreset> &All();
        static const std::vector<std::string> &Categories();
        static const ParticlePreset *Find(const std::string &name);
    };
}

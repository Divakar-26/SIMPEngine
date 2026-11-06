#pragma once

#include <Engine/Particles/ParticleEmitter.h>

#include <entt/entt.hpp>

struct TransformComponent;

namespace SIMPEngine
{
    class ParticleSystem
    {
    public:
        void Update(entt::registry &registry, float dt);
        void Render(entt::registry &registry);

        static EmitterFrame FrameOf(const TransformComponent &transform);
    };
}

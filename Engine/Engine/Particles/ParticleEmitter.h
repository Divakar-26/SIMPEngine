#pragma once

#include <Engine/Particles/ParticleEffect.h>

#include <glm/glm.hpp>

namespace SIMPEngine
{
    struct EmitterFrame
    {
        glm::vec2 Origin{0.0f};
        float Rotation = 0.0f;
        float Scale = 1.0f;
    };

    class ParticleEmitter
    {
    public:
        static void Simulate(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame, float dt, bool emitting = true);
        static void Emit(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame, int count);
        static void Render(const ParticleEffect &effect, const ParticleState &state, const EmitterFrame &frame, float z);
        static void Restart(ParticleState &state);
        static void Prewarm(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame, float seconds);
        static bool IsAlive(const ParticleEffect &effect, const ParticleState &state);
    };

    class ParticleTextures
    {
    public:
        static std::shared_ptr<Texture> Get(ParticleShape shape);
        static void Release();
    };
}

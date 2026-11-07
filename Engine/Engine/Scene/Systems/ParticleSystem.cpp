#include "ParticleSystem.h"
#include <Engine/Scene/Component.h>
#include <Engine/Math/EntityUtility.h>

#include <cmath>

namespace SIMPEngine
{
    EmitterFrame ParticleSystem::FrameOf(const TransformComponent &transform)
    {
        auto world = DecomposeWorld(transform);
        EmitterFrame frame;
        frame.Origin = world.Position;
        frame.Rotation = world.Rotation;
        frame.Scale = (std::abs(world.Scale.x) + std::abs(world.Scale.y)) * 0.5f;
        return frame;
    }

    void ParticleSystem::Update(entt::registry &registry, float dt)
    {
        auto view = registry.view<ParticleEmitterComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto &emitter = view.get<ParticleEmitterComponent>(entity);
            if (!emitter.playing)
                continue;
            ParticleEmitter::Simulate(emitter.effect, emitter.state, FrameOf(view.get<TransformComponent>(entity)), dt);
        }
    }

    void ParticleSystem::Render(entt::registry &registry, int emissiveFilter)
    {
        auto view = registry.view<ParticleEmitterComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto &emitter = view.get<ParticleEmitterComponent>(entity);
            if (emissiveFilter >= 0 && emitter.effect.Emissive != (emissiveFilter == 1))
                continue;
            auto &transform = view.get<TransformComponent>(entity);
            ParticleEmitter::Render(emitter.effect, emitter.state, FrameOf(transform), transform.zIndex);
        }
    }
}

#include "OscillatorSystem.h"

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    void OscillatorSystem::Update(entt::registry &registry, float dt)
    {
        constexpr float TwoPi = 6.28318530718f;

        auto view = registry.view<OscillatorComponent, TransformComponent>();
        for (entt::entity handle : view)
        {
            OscillatorComponent &osc = view.get<OscillatorComponent>(handle);
            TransformComponent &transform = view.get<TransformComponent>(handle);
            if (!osc.playing)
                continue;

            osc.time += dt;
            const float wave = std::sin((osc.time * std::max(osc.frequency, 0.0f) + osc.phase) * TwoPi);

            const glm::vec2 move = osc.move * wave;
            transform.position += move - osc.appliedMove;
            osc.appliedMove = move;

            const float swing = osc.swing * wave;
            transform.rotation += swing - osc.appliedSwing + osc.spin * dt;
            transform.rotation = std::fmod(transform.rotation, 360.0f);
            osc.appliedSwing = swing;

            const float pulse = std::clamp(osc.pulse, 0.0f, 0.95f) * wave;
            transform.scale *= (1.0f + pulse) / (1.0f + osc.appliedPulse);
            osc.appliedPulse = pulse;

            transform.dirty = true;
        }
    }
}

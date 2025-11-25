#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/SIMPEngine.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Rendering/Renderer.h>
#include <random>

class BallScript : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override
    {
    }

#include <random>

    void OnUpdate(float dt) override
    {
        auto &t = GetComponent<TransformComponent>();
        auto &render = GetComponent<RenderComponent>();
        auto &phys = GetComponent<PhysicsComponent>();
        int width = SIMPEngine::Renderer::GetViewportSize().x;
        int height = SIMPEngine::Renderer::GetViewportSize().y;

        int radius = render.width;

        // Random variation generator
        static std::default_random_engine rng(std::random_device{}());
        static std::uniform_real_distribution<float> variation(-0.1f, 0.1f); // ±10%

        auto addVariation = [&](float &v)
        {
            v *= -1.0f;              // bounce
            v *= 0.98f;              // slight damping
            v += v * variation(rng); // random inconsistency
        };

        if (t.position.y + radius > height / 2)
            addVariation(phys.body->velocity.y);

        if (t.position.y - radius < -height / 2)
            addVariation(phys.body->velocity.y);

        if (t.position.x - radius < -width / 2)
            addVariation(phys.body->velocity.x);

        if (t.position.x + radius > width / 2)
            addVariation(phys.body->velocity.x);
    }

public:
};
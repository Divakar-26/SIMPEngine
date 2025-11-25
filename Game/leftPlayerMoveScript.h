#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/SIMPEngine.h>
#include <Engine/Scene/ScriptableEntity.h>

class LeftPlayerMove : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override
    {
    }

    void OnUpdate(float dt) override
    {
        int width = SIMPEngine::Renderer::GetViewportSize().x;
        int height = SIMPEngine::Renderer::GetViewportSize().y;

        auto &physics = GetComponent<PhysicsComponent>();

        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_I))
        {
            if (!(physics.body->position.y + physics.body->getHeigt() / 2 > height / 2))
            {
                physics.body->position.y += 5;
            }
        }
        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_K))
        {
            if (!(physics.body->position.y - physics.body->getHeigt() / 2 < -height / 2))
            {
                physics.body->position.y -= 5;
            }

        }

        // physics.body->velocity = {0, 0};
    }

public:
};
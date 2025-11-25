#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/SIMPEngine.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Rendering/Renderer.h>

class RightPlayerMove : public SIMPEngine::ScriptableEntity
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

        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_UP))
        {
            if (!(physics.body->position.y + physics.body->getHeigt() / 2 > height / 2))
            {
                physics.body->position.y += 5;
            }
        }
        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_DOWN))
        {
            if (!(physics.body->position.y - physics.body->getHeigt() / 2 < -height / 2))
            {
                physics.body->position.y -= 5;
            }
        }
    }

public:
};
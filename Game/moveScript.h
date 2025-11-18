#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>

class MoveRightScript : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override
    {
        
    }   

    void OnUpdate(float dt) override
    {
        auto & t = GetComponent<TransformComponent>();
        t.position.x += 1000.0f * dt;
    }
};
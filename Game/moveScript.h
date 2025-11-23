#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/SIMPEngine.h>
#include <Engine/Scene/ScriptableEntity.h>

class MoveRightScript : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override
    {
        
    }   

    void OnUpdate(float dt) override
    {
        if(SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_UP)){
            rateKyaHai += 3;
        }

        auto & t = GetComponent<TransformComponent>();
        // t.position.x += 1000.0f * dt;
        t.rotation += rateKyaHai * dt;
    }
public:
    int rateKyaHai = 3;
};
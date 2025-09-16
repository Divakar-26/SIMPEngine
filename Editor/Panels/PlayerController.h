#include "Scene/ScriptableEntity.h"
#include "Input/SIMP_Keys.h"
#include "Input/Input.h"

class PlayerController : public SIMPEngine::ScriptableEntity
{
private:
    float time = 0.0f;

protected:
    void OnCreate() override
    {
        // Start with some random offset so multiple entities bounce differently
        time = static_cast<float>(rand() % 10000) / 1000.0f;
    }

    void OnUpdate(float dt) override
    {
        auto &transform = GetComponent<TransformComponent>();

        time += dt;

        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_UP))
        {
            transform.position.y -= 1000 * dt;
        }
        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_DOWN))
        {
            transform.position.y += 1000 * dt;
        }
        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_LEFT))
        {
            transform.position.x -= 1000 * dt;
        }
        if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_RIGHT))
        {
            transform.position.x += 1000 * dt;
        }

    }
};
    
#include "Scene/ScriptableEntity.h"
#include <cmath>   // for sin()

class PlayerController : public SIMPEngine::ScriptableEntity
{
private:
    float time = 0.0f;

protected:
    void OnCreate() override
    {
        // Start with some random offset so multiple entities bounce differently
        time = static_cast<float>(rand() % 1000) / 100.0f;
    }

    void OnUpdate(float dt) override
    {
        auto& transform = GetComponent<TransformComponent>();

        time += dt;

        // Bounce effect using sine wave
        transform.position.y = 200.0f + std::sin(time * 5.0f) * 50.0f;

        // Slowly rotate for extra spice
        transform.rotation += 90.0f * dt;
    }
};

#include "Scene/ScriptableEntity.h"

class PlayerController : public SIMPEngine::ScriptableEntity
{
protected:
    void OnCreate() override
    {
        // initialization
    }

    void OnUpdate(float dt) override
    {
        auto& transform = GetComponent<TransformComponent>();
        transform.position.x += 100.0f * dt; // example
    }
};

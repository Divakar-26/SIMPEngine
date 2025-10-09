#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include "Scene/Component.h"

class PhysicsSystem
{
public:
    PhysicsSystem() : world(b2Vec2(0.0f, 980.0f)) {}

    void Update(entt::registry &registry, float dt)
    {
        world.Step(dt, velocityIterations, positionIterations);

        auto view = registry.view<TransformComponent, PhysicsComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &physics = view.get<PhysicsComponent>(entity);

            if (physics.body)
            {
                b2Vec2 pos = physics.body->GetPosition();
                transform.position.x = pos.x;
                transform.position.y = pos.y;
                transform.rotation = physics.body->GetAngle() * 180.0f / 3.14159f;
            }
        }
    }

    b2World world;

private:
    int velocityIterations = 8;
    int positionIterations = 3;
};
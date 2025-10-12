#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include "Scene/Component.h"

class PhysicsSystem
{
public:
    PhysicsSystem() : world(b2Vec2(0.0f, 980.0f)) {}

    void Update(entt::registry &registry, float dt)
    {
        // 1️⃣ Apply scripted velocity to Box2D bodies
        auto controlView = registry.view<PhysicsComponent, VelocityComponent>();
        for (auto entity : controlView)
        {
            auto &physics = controlView.get<PhysicsComponent>(entity);
            auto &velocity = controlView.get<VelocityComponent>(entity);

            if (physics.body)
            {
                // Apply velocity (convert to Box2D scale if needed)
                physics.body->SetLinearVelocity(b2Vec2(velocity.vx * 100.0f, velocity.vy * 100.0f));
            }
        }

        // 2️⃣ Step simulation
        world.Step(dt, velocityIterations, positionIterations);

        // 3️⃣ Sync positions back to transforms
        auto view = registry.view<TransformComponent, PhysicsComponent, VelocityComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &physics = view.get<PhysicsComponent>(entity);
            auto &velocity = view.get<VelocityComponent>(entity);

            if (physics.body)
            {
                b2Vec2 pos = physics.body->GetPosition();
                transform.position.x = pos.x;
                transform.position.y = pos.y;
                transform.rotation = physics.body->GetAngle() * 180.0f / 3.14159f;

                // Sync Box2D → VelocityComponent
                b2Vec2 vel = physics.body->GetLinearVelocity();
                velocity.vx = vel.x;
                velocity.vy = vel.y;
            }
        }
    }

    b2World world;

private:
    int velocityIterations = 8;
    int positionIterations = 3;
};
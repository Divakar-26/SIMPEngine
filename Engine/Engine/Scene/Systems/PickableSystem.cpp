#include "PickableSystem.h"

#include "SystemUtil.h"

#include <Engine/Input/Input.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Scene.h>

#include <AccelEngine/body.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace SIMPEngine
{
    namespace
    {
        constexpr float MaxThrowSpeed = 4000.0f;

        int ButtonCode(const PickableComponent &pickable)
        {
            return pickable.button == 1 ? 3 : pickable.button == 2 ? 2 : 1;
        }

        void Release(Scene &scene, entt::entity entity, bool throwIt)
        {
            entt::registry &registry = scene.GetRegistry();
            PickableComponent &pickable = registry.get<PickableComponent>(entity);
            pickable.held = false;

            if (auto *transform = registry.try_get<TransformComponent>(entity))
            {
                transform->scale = pickable.baseScale;
                transform->zIndex = pickable.baseZ;
                transform->dirty = true;
            }

            glm::vec2 velocity = throwIt ? pickable.velocity * pickable.throwStrength : glm::vec2(0.0f);
            const float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            if (speed > MaxThrowSpeed)
                velocity *= MaxThrowSpeed / speed;
            pickable.velocity = {0.0f, 0.0f};

            if (auto *physics = registry.try_get<PhysicsComponent>(entity); physics && physics->body)
                physics->body->setVelocity({velocity.x, velocity.y}, 0.0f);
            if (auto *controller = registry.try_get<CharacterControllerComponent>(entity))
                controller->velocity = velocity;
            if (auto *mover = registry.try_get<VelocityComponent>(entity))
            {
                mover->vx = velocity.x;
                mover->vy = velocity.y;
            }
            scene.SendEvent(entity, ScriptEvent::Dropped);
        }
    }

    void PickableSystem::Drop(Scene &scene, entt::entity entity)
    {
        entt::registry &registry = scene.GetRegistry();
        if (registry.valid(entity) && registry.all_of<PickableComponent>(entity) && registry.get<PickableComponent>(entity).held)
            Release(scene, entity, false);
    }

    void PickableSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();
        auto view = registry.view<PickableComponent, TransformComponent>();

        const bool inside = Input::IsMouseInViewport() && !scene.IsMouseOverUI();
        const glm::vec2 mouse = scene.ScreenToWorld(Input::GetMouseViewportPosition());

        bool anyHeld = false;
        std::vector<entt::entity> dropped;
        for (entt::entity handle : view)
        {
            PickableComponent &pickable = view.get<PickableComponent>(handle);
            if (!pickable.held)
                continue;

            if (!pickable.enabled || !Input::IsMouseButtonPressed(ButtonCode(pickable)))
            {
                dropped.push_back(handle);
                continue;
            }
            anyHeld = true;

            TransformComponent &transform = view.get<TransformComponent>(handle);
            const glm::vec2 current = DecomposeWorld(transform).Position;
            const glm::vec2 goal = mouse + pickable.grabOffset;
            const float blend = pickable.smoothing <= 0.0f ? 1.0f : 1.0f - std::exp(-pickable.smoothing * dt);
            const glm::vec2 next = current + (goal - current) * blend;

            transform.position += next - current;
            transform.dirty = true;
            if (dt > 0.0f)
                pickable.velocity += ((next - current) / dt - pickable.velocity) * 0.4f;

            if (auto *physics = registry.try_get<PhysicsComponent>(handle); physics && physics->body)
            {
                PhysicsSystem::SyncBodyToTransform(registry, handle);
                physics->body->setVelocity({0.0f, 0.0f}, 0.0f);
            }
            if (auto *controller = registry.try_get<CharacterControllerComponent>(handle))
                controller->velocity = {0.0f, 0.0f};
            if (auto *mover = registry.try_get<VelocityComponent>(handle))
                mover->vx = mover->vy = 0.0f;
        }

        for (entt::entity handle : dropped)
            if (registry.valid(handle))
                Release(scene, handle, true);

        if (anyHeld || !inside)
            return;

        entt::entity best = entt::null;
        float bestZ = 0.0f;
        for (entt::entity handle : view)
        {
            const PickableComponent &pickable = view.get<PickableComponent>(handle);
            if (!pickable.enabled || !Input::IsMouseButtonJustPressed(ButtonCode(pickable)))
                continue;
            if (!PointHitsEntity(registry, handle, mouse, pickable.padding))
                continue;
            const float z = view.get<TransformComponent>(handle).zIndex;
            if (best == entt::null || z >= bestZ)
            {
                best = handle;
                bestZ = z;
            }
        }
        if (best == entt::null)
            return;

        PickableComponent &pickable = view.get<PickableComponent>(best);
        TransformComponent &transform = view.get<TransformComponent>(best);
        pickable.held = true;
        pickable.velocity = {0.0f, 0.0f};
        pickable.baseScale = transform.scale;
        pickable.baseZ = transform.zIndex;
        pickable.grabOffset = pickable.keepOffset ? DecomposeWorld(transform).Position - mouse : glm::vec2(0.0f);
        transform.scale = pickable.baseScale * std::max(pickable.liftScale, 0.01f);
        if (pickable.bringToFront)
            transform.zIndex = pickable.baseZ + 100.0f;
        transform.dirty = true;
        scene.SendEvent(best, ScriptEvent::Grabbed);
    }
}

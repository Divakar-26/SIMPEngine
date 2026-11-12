#include "FollowSystem.h"

#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Scene.h>

#include <cmath>

namespace SIMPEngine
{
    void FollowSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();
        auto view = registry.view<FollowComponent, TransformComponent>();
        for (entt::entity handle : view)
        {
            FollowComponent &follow = view.get<FollowComponent>(handle);
            if (!follow.enabled || follow.target.empty())
                continue;

            entt::entity target = follow.handle;
            const TagComponent *tag = registry.valid(target) ? registry.try_get<TagComponent>(target) : nullptr;
            if (!tag || tag->Tag != follow.target)
            {
                target = scene.GetEntityByName(follow.target).GetHandle();
                follow.handle = target;
            }
            if (target == entt::null || target == handle || !registry.valid(target) || !registry.all_of<TransformComponent>(target))
                continue;

            TransformComponent &transform = view.get<TransformComponent>(handle);
            const glm::vec2 current = DecomposeWorld(transform).Position;
            glm::vec2 goal = DecomposeWorld(registry.get<TransformComponent>(target)).Position + follow.offset;
            if (!follow.followX)
                goal.x = current.x;
            if (!follow.followY)
                goal.y = current.y;

            if (follow.stopDistance > 0.0f)
            {
                const glm::vec2 away = goal - current;
                const float distance = std::sqrt(away.x * away.x + away.y * away.y);
                if (distance <= follow.stopDistance)
                    continue;
                goal = current + away * ((distance - follow.stopDistance) / distance);
            }

            const float blend = follow.smoothing <= 0.0f ? 1.0f : 1.0f - std::exp(-follow.smoothing * dt);
            transform.position += (goal - current) * blend;
            transform.dirty = true;
        }
    }
}

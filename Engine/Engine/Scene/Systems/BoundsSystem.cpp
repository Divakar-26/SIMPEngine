#include "BoundsSystem.h"
#include "SystemUtil.h"

#include <Engine/Scene/Scene.h>

#include <cmath>
#include <algorithm>

namespace SIMPEngine
{
    void BoundsSystem::Update(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        auto view = registry.view<WorldBoundsComponent, TransformComponent>();
        for (entt::entity handle : view)
        {
            const WorldBoundsComponent &bounds = view.get<WorldBoundsComponent>(handle);
            if (!bounds.enabled)
                continue;

            const glm::vec2 position = DecomposeWorld(view.get<TransformComponent>(handle)).Position;
            const bool inward = bounds.mode == BoundsMode::Clamp || bounds.mode == BoundsMode::Bounce;
            const float margin = inward ? bounds.margin : -bounds.margin;
            const glm::vec2 low = glm::min(bounds.min, bounds.max) + glm::vec2(margin);
            const glm::vec2 high = glm::max(bounds.min, bounds.max) - glm::vec2(margin);

            glm::vec2 next = position;
            bool outside = false;
            for (int axis = 0; axis < 2; ++axis)
            {
                if (position[axis] >= low[axis] && position[axis] <= high[axis])
                    continue;
                outside = true;
                switch (bounds.mode)
                {
                case BoundsMode::Clamp:
                    next[axis] = std::clamp(position[axis], low[axis], high[axis]);
                    break;
                case BoundsMode::Bounce:
                    next[axis] = std::clamp(position[axis], low[axis], high[axis]);
                    BounceEntity(registry, handle, axis, position[axis] < low[axis] ? 1.0f : -1.0f);
                    break;
                case BoundsMode::Wrap:
                {
                    const float range = high[axis] - low[axis];
                    if (range > 0.0f)
                        next[axis] = low[axis] + std::fmod(std::fmod(position[axis] - low[axis], range) + range, range);
                    break;
                }
                case BoundsMode::Destroy:
                    break;
                }
            }

            if (!outside)
                continue;
            if (bounds.mode == BoundsMode::Destroy)
                registry.emplace_or_replace<LifetimeComponent>(handle, LifetimeComponent{0.0f});
            else
                MoveEntityBy(registry, handle, next - position);
        }
    }
}

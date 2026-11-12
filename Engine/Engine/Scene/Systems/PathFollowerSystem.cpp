#include "PhysicsSystem.h"
#include "PathFollowerSystem.h"

#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    void PathFollowerSystem::Restart(PathFollowerComponent &path)
    {
        path.originSet = false;
        path.target = 0;
        path.direction = 1;
        path.waiting = 0.0f;
        path.playing = true;
    }

    void PathFollowerSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();
        m_Pending.clear();

        auto view = registry.view<PathFollowerComponent, TransformComponent>();
        for (entt::entity handle : view)
        {
            PathFollowerComponent &path = view.get<PathFollowerComponent>(handle);
            TransformComponent &transform = view.get<TransformComponent>(handle);

            if (!path.originSet)
            {
                path.origin = transform.position;
                path.originSet = true;
            }

            if (!path.playing || path.points.empty())
                continue;

            if (path.waiting > 0.0f)
            {
                path.waiting -= dt;
                continue;
            }

            const int count = static_cast<int>(path.points.size());
            path.target = std::clamp(path.target, 0, count - 1);

            float remaining = std::max(path.speed, 0.0f) * dt;
            int guard = count + 1;
            bool moved = false;

            while (remaining > 0.0f && path.playing && guard-- > 0)
            {
                const glm::vec2 goal = path.relative ? path.origin + path.points[static_cast<size_t>(path.target)]
                                                     : path.points[static_cast<size_t>(path.target)];
                const glm::vec2 delta = goal - transform.position;
                const float distance = glm::length(delta);

                if (distance > remaining)
                {
                    transform.position += delta / distance * remaining;
                    if (path.faceDirection)
                        transform.rotation = glm::degrees(std::atan2(delta.y, delta.x));
                    moved = true;
                    break;
                }

                if (distance > 0.0001f)
                {
                    if (path.faceDirection)
                        transform.rotation = glm::degrees(std::atan2(delta.y, delta.x));
                    remaining -= distance;
                }
                transform.position = goal;
                moved = true;

                m_Pending.push_back({handle, ScriptEvent::PathPoint, static_cast<float>(path.target)});

                bool finished = false;
                if (count < 2)
                {
                    finished = true;
                }
                else if (path.mode == PathMode::Once)
                {
                    if (path.target + 1 >= count)
                        finished = true;
                    else
                        path.target += 1;
                }
                else if (path.mode == PathMode::Loop)
                {
                    path.target = (path.target + 1) % count;
                }
                else
                {
                    int next = path.target + path.direction;
                    if (next >= count || next < 0)
                    {
                        path.direction = -path.direction;
                        next = path.target + path.direction;
                    }
                    path.target = next;
                }

                if (finished)
                {
                    path.playing = false;
                    m_Pending.push_back({handle, ScriptEvent::PathEnd, 0.0f});
                    break;
                }

                if (path.waitTime > 0.0f)
                {
                    path.waiting = path.waitTime;
                    break;
                }
            }

            if (moved)
            {
                transform.dirty = true;
                PhysicsSystem::SyncBodyToTransform(registry, handle);
            }
        }

        for (const Pending &pending : m_Pending)
            scene.SendEvent(pending.Target, pending.Type, entt::null, pending.Value);
    }
}

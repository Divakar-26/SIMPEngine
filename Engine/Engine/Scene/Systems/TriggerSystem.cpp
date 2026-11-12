#include "TriggerSystem.h"

#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL_rect.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace SIMPEngine
{
    void TriggerSystem::Update(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        m_Pending.clear();

        auto triggers = registry.view<TriggerComponent, TransformComponent>();
        auto bodies = registry.view<CollisionComponent, TransformComponent>();
        auto controllers = registry.view<CharacterControllerComponent, TransformComponent>(entt::exclude<CollisionComponent>);

        for (entt::entity triggerHandle : triggers)
        {
            TriggerComponent &trigger = triggers.get<TriggerComponent>(triggerHandle);
            std::vector<entt::entity> current;

            if (trigger.enabled)
            {
                const WorldTransform2D world = DecomposeWorld(triggers.get<TransformComponent>(triggerHandle));
                const SDL_FRect zone{world.Position.x + trigger.offsetX,
                                     world.Position.y + trigger.offsetY,
                                     trigger.width * std::abs(world.Scale.x),
                                     trigger.height * std::abs(world.Scale.y)};

                for (entt::entity other : bodies)
                {
                    if (other == triggerHandle)
                        continue;

                    if (!trigger.filter.empty())
                    {
                        const TagComponent *tag = registry.try_get<TagComponent>(other);
                        if (!tag || tag->Tag != trigger.filter)
                            continue;
                    }

                    const WorldTransform2D otherWorld = DecomposeWorld(bodies.get<TransformComponent>(other));
                    const CollisionComponent &collision = bodies.get<CollisionComponent>(other);
                    const SDL_FRect box{otherWorld.Position.x + collision.offsetX,
                                        otherWorld.Position.y + collision.offsetY,
                                        collision.width * std::abs(otherWorld.Scale.x),
                                        collision.height * std::abs(otherWorld.Scale.y)};

                    if (SDL_HasRectIntersectionFloat(&zone, &box))
                        current.push_back(other);
                }

                for (entt::entity other : controllers)
                {
                    if (other == triggerHandle)
                        continue;
                    if (!trigger.filter.empty())
                    {
                        const TagComponent *tag = registry.try_get<TagComponent>(other);
                        if (!tag || tag->Tag != trigger.filter)
                            continue;
                    }

                    const glm::vec2 center = DecomposeWorld(controllers.get<TransformComponent>(other)).Position;
                    const CharacterControllerComponent &controller = controllers.get<CharacterControllerComponent>(other);
                    const SDL_FRect box{center.x + controller.offset.x - controller.size.x * 0.5f,
                                        center.y + controller.offset.y - controller.size.y * 0.5f,
                                        controller.size.x, controller.size.y};

                    if (SDL_HasRectIntersectionFloat(&zone, &box))
                        current.push_back(other);
                }
            }

            for (entt::entity other : current)
            {
                const bool known = std::find(trigger.inside.begin(), trigger.inside.end(), other) != trigger.inside.end();
                m_Pending.push_back({triggerHandle, other, known ? ScriptEvent::TriggerStay : ScriptEvent::TriggerEnter});
            }

            for (entt::entity other : trigger.inside)
            {
                if (std::find(current.begin(), current.end(), other) == current.end())
                    m_Pending.push_back({triggerHandle, other, ScriptEvent::TriggerExit});
            }

            trigger.inside = std::move(current);
        }

        for (const Pending &pending : m_Pending)
        {
            if (!registry.valid(pending.Trigger))
                continue;

            scene.SendEvent(pending.Trigger, pending.Type, pending.Other);
            scene.SendEvent(pending.Other, pending.Type, pending.Trigger);
        }
    }
}

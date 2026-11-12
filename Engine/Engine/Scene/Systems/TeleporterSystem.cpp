#include "TeleporterSystem.h"
#include "SystemUtil.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Scene/Scene.h>

#include <vector>
#include <algorithm>
#include <iterator>

namespace SIMPEngine
{
    void TeleporterSystem::Update(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        auto view = registry.view<TeleporterComponent, TriggerComponent, TransformComponent>();

        std::unordered_set<entt::entity> insideAny;
        for (entt::entity handle : view)
            for (entt::entity other : view.get<TriggerComponent>(handle).inside)
                insideAny.insert(other);

        for (auto it = m_Blocked.begin(); it != m_Blocked.end();)
            it = (!registry.valid(*it) || insideAny.find(*it) == insideAny.end()) ? m_Blocked.erase(it) : std::next(it);

        struct Jump
        {
            entt::entity Who;
            glm::vec2 Delta;
            entt::entity Portal;
        };
        std::vector<Jump> jumps;

        for (entt::entity handle : view)
        {
            const TeleporterComponent &portal = view.get<TeleporterComponent>(handle);
            const TriggerComponent &trigger = view.get<TriggerComponent>(handle);
            if (!portal.enabled || portal.target.empty() || trigger.inside.empty())
                continue;

            const Entity destination = scene.GetEntityByName(portal.target);
            if (!destination || destination.GetHandle() == handle || !registry.all_of<TransformComponent>(destination.GetHandle()))
                continue;
            const glm::vec2 goal = DecomposeWorld(registry.get<TransformComponent>(destination.GetHandle())).Position + portal.offset;

            for (entt::entity other : trigger.inside)
            {
                if (!registry.valid(other) || other == destination.GetHandle() || m_Blocked.count(other) || !registry.all_of<TransformComponent>(other))
                    continue;
                const bool duplicate = std::any_of(jumps.begin(), jumps.end(), [other](const Jump &jump)
                                                   { return jump.Who == other; });
                if (duplicate)
                    continue;
                jumps.push_back({other, goal - DecomposeWorld(registry.get<TransformComponent>(other)).Position, handle});
            }
        }

        for (const Jump &jump : jumps)
        {
            if (!registry.valid(jump.Who) || !registry.valid(jump.Portal))
                continue;
            const TeleporterComponent &portal = registry.get<TeleporterComponent>(jump.Portal);
            MoveEntityBy(registry, jump.Who, jump.Delta);
            if (!portal.keepVelocity)
                StopEntity(registry, jump.Who);
            if (!portal.sound.empty())
                AudioEngine::Play(portal.sound);
            m_Blocked.insert(jump.Who);
        }
    }
}

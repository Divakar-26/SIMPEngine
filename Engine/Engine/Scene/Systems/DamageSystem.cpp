#include "DamageSystem.h"
#include "HealthSystem.h"

#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace SIMPEngine
{
    namespace
    {
        void Knockback(entt::registry &registry, entt::entity source, entt::entity target, float strength)
        {
            auto *controller = registry.try_get<CharacterControllerComponent>(target);
            const auto *sourceTransform = registry.try_get<TransformComponent>(source);
            const auto *targetTransform = registry.try_get<TransformComponent>(target);
            if (!controller || !sourceTransform || !targetTransform || strength <= 0.0f)
                return;

            const glm::vec2 from = DecomposeWorld(*sourceTransform).Position;
            const glm::vec2 to = DecomposeWorld(*targetTransform).Position;
            glm::vec2 direction = to - from;

            if (controller->mode == ControllerMode::Platformer)
            {
                const float side = direction.x >= 0.0f ? 1.0f : -1.0f;
                controller->velocity = {side * strength, strength * 0.6f};
                controller->grounded = false;
                return;
            }

            const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            direction = length > 0.001f ? direction / length : glm::vec2(1.0f, 0.0f);
            controller->velocity = direction * strength;
        }
    }

    void DamageSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();
        m_Hits.clear();

        auto view = registry.view<DamageComponent, TriggerComponent>();
        for (entt::entity handle : view)
        {
            DamageComponent &damage = view.get<DamageComponent>(handle);
            const TriggerComponent &trigger = view.get<TriggerComponent>(handle);

            auto &cooldowns = damage.cooldowns;
            cooldowns.erase(std::remove_if(cooldowns.begin(), cooldowns.end(), [&](std::pair<entt::entity, float> &entry)
                                           {
                if (std::find(trigger.inside.begin(), trigger.inside.end(), entry.first) == trigger.inside.end())
                    return true;
                entry.second -= dt;
                return damage.interval > 0.0f && entry.second <= 0.0f; }),
                            cooldowns.end());

            if (!damage.enabled || damage.amount == 0.0f)
                continue;

            for (entt::entity other : trigger.inside)
            {
                if (other == handle || !registry.valid(other) || !registry.all_of<HealthComponent>(other))
                    continue;
                if (!damage.filter.empty())
                {
                    const TagComponent *tag = registry.try_get<TagComponent>(other);
                    if (!tag || tag->Tag != damage.filter)
                        continue;
                }
                const bool coolingDown = std::any_of(cooldowns.begin(), cooldowns.end(), [other](const auto &entry)
                                                     { return entry.first == other; });
                if (coolingDown)
                    continue;

                cooldowns.emplace_back(other, damage.interval);
                m_Hits.push_back({handle, other});
            }
        }

        for (const Hit &hit : m_Hits)
        {
            if (!registry.valid(hit.Source) || !registry.valid(hit.Target))
                continue;
            const DamageComponent *damage = registry.try_get<DamageComponent>(hit.Source);
            if (!damage)
                continue;
            const float amount = damage->amount;
            const float knockback = damage->knockback;
            const bool destroyOnHit = damage->destroyOnHit;

            const bool applied = amount > 0.0f ? HealthSystem::Damage(scene, hit.Target, amount, hit.Source)
                                               : HealthSystem::Heal(scene, hit.Target, -amount) > 0.0f;
            if (!applied || !registry.valid(hit.Source))
                continue;

            if (registry.valid(hit.Target) && amount > 0.0f)
                Knockback(registry, hit.Source, hit.Target, knockback);
            if (destroyOnHit)
                registry.emplace_or_replace<LifetimeComponent>(hit.Source, LifetimeComponent{0.0f});
        }
    }
}

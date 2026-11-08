#include "HealthSystem.h"

#include <Engine/Scene/Scene.h>

#include <algorithm>

namespace SIMPEngine
{
    void HealthSystem::Update(Scene &scene, float dt)
    {
        auto view = scene.GetRegistry().view<HealthComponent>();
        for (entt::entity handle : view)
        {
            HealthComponent &health = view.get<HealthComponent>(handle);
            health.max = std::max(health.max, 0.0f);
            health.current = std::clamp(health.current, 0.0f, health.max);
            if (health.invulnerableRemaining > 0.0f)
                health.invulnerableRemaining = std::max(0.0f, health.invulnerableRemaining - dt);
            if (health.dead && health.current > 0.0f)
                health.dead = false;
        }
    }

    bool HealthSystem::Damage(Scene &scene, entt::entity target, float amount, entt::entity source)
    {
        entt::registry &registry = scene.GetRegistry();
        if (amount <= 0.0f || target == entt::null || !registry.valid(target))
            return false;

        HealthComponent *health = registry.try_get<HealthComponent>(target);
        if (!health || health->dead || health->invulnerableRemaining > 0.0f)
            return false;

        health->current = std::max(0.0f, health->current - amount);
        health->invulnerableRemaining = health->invulnerability;
        scene.SendEvent(target, ScriptEvent::Damaged, source, amount);

        if (!registry.valid(target))
            return true;
        health = registry.try_get<HealthComponent>(target);
        if (!health || health->dead || health->current > 0.0f)
            return true;

        health->dead = true;
        scene.SendEvent(target, ScriptEvent::Death, source);

        if (!registry.valid(target))
            return true;
        health = registry.try_get<HealthComponent>(target);
        if (health && health->dead && health->destroyOnDeath)
            registry.emplace_or_replace<LifetimeComponent>(target, LifetimeComponent{0.0f});
        return true;
    }

    float HealthSystem::Heal(Scene &scene, entt::entity target, float amount)
    {
        entt::registry &registry = scene.GetRegistry();
        if (amount <= 0.0f || target == entt::null || !registry.valid(target))
            return 0.0f;

        HealthComponent *health = registry.try_get<HealthComponent>(target);
        if (!health || health->dead)
            return 0.0f;

        const float before = health->current;
        health->current = std::min(health->max, health->current + amount);
        return health->current - before;
    }

    bool HealthSystem::Kill(Scene &scene, entt::entity target, entt::entity source)
    {
        entt::registry &registry = scene.GetRegistry();
        if (target == entt::null || !registry.valid(target))
            return false;

        HealthComponent *health = registry.try_get<HealthComponent>(target);
        if (!health || health->dead)
            return false;

        health->invulnerableRemaining = 0.0f;
        return Damage(scene, target, std::max(health->current, 0.001f), source);
    }

    bool HealthSystem::Revive(Scene &scene, entt::entity target)
    {
        entt::registry &registry = scene.GetRegistry();
        if (target == entt::null || !registry.valid(target))
            return false;

        HealthComponent *health = registry.try_get<HealthComponent>(target);
        if (!health)
            return false;

        health->dead = false;
        health->current = health->max;
        health->invulnerableRemaining = 0.0f;
        return true;
    }
}

#include "PickupSystem.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Core/GameVariables.h>
#include <Engine/Scene/Scene.h>

namespace SIMPEngine
{
    void PickupSystem::Update(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        auto view = registry.view<PickupComponent, TriggerComponent>();
        for (entt::entity handle : view)
        {
            PickupComponent &pickup = view.get<PickupComponent>(handle);
            const bool touched = !view.get<TriggerComponent>(handle).inside.empty();
            if (!touched)
            {
                pickup.collected = false;
                continue;
            }
            if (!pickup.enabled || pickup.collected)
                continue;

            pickup.collected = true;
            GameVariables::Add(pickup.key, pickup.amount);
            if (!pickup.sound.empty())
                AudioEngine::Play(pickup.sound, pickup.volume);
            if (pickup.destroyOnPickup)
                registry.emplace_or_replace<LifetimeComponent>(handle, LifetimeComponent{0.0f});
        }
    }
}

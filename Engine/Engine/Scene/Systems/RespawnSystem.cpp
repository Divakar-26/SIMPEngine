#include "RespawnSystem.h"
#include "HealthSystem.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Scene.h>
#include <vector>

namespace SIMPEngine
{
    void RespawnSystem::Respawn(Scene &scene, entt::entity entity)
    {
        entt::registry &registry = scene.GetRegistry();
        if (!registry.valid(entity) || !registry.all_of<RespawnComponent, TransformComponent>(entity))
            return;

        RespawnComponent &respawn = registry.get<RespawnComponent>(entity);
        TransformComponent &transform = registry.get<TransformComponent>(entity);
        const glm::vec2 current = DecomposeWorld(transform).Position;
        transform.position += respawn.point - current;
        transform.dirty = true;
        respawn.timer = -1.0f;

        if (auto *controller = registry.try_get<CharacterControllerComponent>(entity))
            controller->velocity = {0.0f, 0.0f};
        if (auto *velocity = registry.try_get<VelocityComponent>(entity))
            velocity->vx = velocity->vy = 0.0f;
        if (respawn.restoreHealth && registry.all_of<HealthComponent>(entity))
            HealthSystem::Revive(scene, entity);
        scene.MarkPhysicsDirty();
    }

    void RespawnSystem::Update(Scene &scene, float dt)
    {
        entt::registry &registry = scene.GetRegistry();

        auto checkpoints = registry.view<CheckpointComponent, TriggerComponent, TransformComponent>();
        for (entt::entity handle : checkpoints)
        {
            const CheckpointComponent &checkpoint = checkpoints.get<CheckpointComponent>(handle);
            if (!checkpoint.enabled)
                continue;
            const glm::vec2 place = DecomposeWorld(checkpoints.get<TransformComponent>(handle)).Position + checkpoint.offset;
            bool reached = false;
            for (entt::entity other : checkpoints.get<TriggerComponent>(handle).inside)
            {
                if (!registry.valid(other) || !registry.all_of<RespawnComponent>(other))
                    continue;
                RespawnComponent &respawn = registry.get<RespawnComponent>(other);
                if (!respawn.pointSet || respawn.point != place)
                    reached = true;
                respawn.point = place;
                respawn.pointSet = true;
            }
            if (reached && !checkpoint.sound.empty())
                AudioEngine::Play(checkpoint.sound);
        }

        std::vector<entt::entity> due;
        auto view = registry.view<RespawnComponent, TransformComponent>();
        for (entt::entity handle : view)
        {
            RespawnComponent &respawn = view.get<RespawnComponent>(handle);
            const glm::vec2 position = DecomposeWorld(view.get<TransformComponent>(handle)).Position;
            if (!respawn.pointSet)
            {
                respawn.point = position;
                respawn.pointSet = true;
            }

            const auto *health = registry.try_get<HealthComponent>(handle);
            const bool dead = (health && health->dead) || (respawn.useKillY && position.y < respawn.killY);
            if (!dead)
            {
                respawn.timer = -1.0f;
                continue;
            }
            if (respawn.timer < 0.0f)
                respawn.timer = respawn.delay;
            respawn.timer -= dt;
            if (respawn.timer <= 0.0f)
                due.push_back(handle);
        }

        for (entt::entity handle : due)
            Respawn(scene, handle);
    }
}

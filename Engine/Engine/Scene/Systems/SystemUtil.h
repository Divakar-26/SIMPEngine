#pragma once
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Systems/PhysicsSystem.h>

#include <AccelEngine/body.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    inline bool PointHitsEntity(entt::registry &registry, entt::entity entity, const glm::vec2 &point, float padding)
    {
        const TransformComponent &transform = registry.get<TransformComponent>(entity);

        if (!registry.all_of<SpriteComponent>(entity) && !registry.all_of<RenderComponent>(entity))
            if (const auto *collision = registry.try_get<CollisionComponent>(entity))
            {
                const glm::vec2 position = DecomposeWorld(transform).Position;
                return point.x >= position.x + collision->offsetX - padding && point.x <= position.x + collision->offsetX + collision->width + padding &&
                       point.y >= position.y + collision->offsetY - padding && point.y <= position.y + collision->offsetY + collision->height + padding;
            }

        glm::vec2 size{32.0f, 32.0f};
        if (const auto *sprite = registry.try_get<SpriteComponent>(entity))
            size = {sprite->width, sprite->height};
        else if (const auto *render = registry.try_get<RenderComponent>(entity))
            size = {render->width, render->height};

        if (std::abs(glm::determinant(glm::mat3(transform.worldTransform))) < 1e-12f)
            return false;
        const glm::vec4 local = glm::inverse(transform.worldTransform) * glm::vec4(point, 0.0f, 1.0f);
        const glm::vec2 scale = glm::max(DecomposeWorld(transform).Scale, glm::vec2(1e-4f));
        const glm::vec2 half = size * 0.5f + glm::vec2(padding) / scale;
        return std::abs(local.x) <= half.x && std::abs(local.y) <= half.y;
    }

    inline void MoveEntityBy(entt::registry &registry, entt::entity entity, const glm::vec2 &delta)
    {
        TransformComponent &transform = registry.get<TransformComponent>(entity);
        transform.position += delta;
        transform.dirty = true;
        PhysicsSystem::SyncBodyToTransform(registry, entity);
    }

    inline void StopEntity(entt::registry &registry, entt::entity entity)
    {
        if (auto *physics = registry.try_get<PhysicsComponent>(entity); physics && physics->body)
            physics->body->setVelocity({0.0f, 0.0f}, 0.0f);
        if (auto *controller = registry.try_get<CharacterControllerComponent>(entity))
            controller->velocity = {0.0f, 0.0f};
        if (auto *mover = registry.try_get<VelocityComponent>(entity))
            mover->vx = mover->vy = 0.0f;
    }

    inline void BounceEntity(entt::registry &registry, entt::entity entity, int axis, float direction)
    {
        auto fix = [direction](float &value)
        { value = direction * std::abs(value); };
        if (auto *physics = registry.try_get<PhysicsComponent>(entity); physics && physics->body)
        {
            if (axis == 0)
                fix(physics->body->velocity.x);
            else
                fix(physics->body->velocity.y);
            physics->body->wakeUp();
        }
        if (auto *controller = registry.try_get<CharacterControllerComponent>(entity))
            fix(controller->velocity[axis]);
        if (auto *mover = registry.try_get<VelocityComponent>(entity))
            fix(axis == 0 ? mover->vx : mover->vy);
    }
}

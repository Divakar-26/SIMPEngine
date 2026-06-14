#include "EntityUtility.h"
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    std::optional<AABB2D> GetEntityBounds2D(Entity entity)
    {
        if (!entity)
            return std::nullopt;

        if (!entity.HasComponent<TransformComponent>())
            return std::nullopt;

        auto &tc = entity.GetComponent<TransformComponent>();

        float width = 0.0f;
        float height = 0.0f;

        if (entity.HasComponent<RenderComponent>())
        {
            auto &rc = entity.GetComponent<RenderComponent>();
            width = rc.width;
            height = rc.height;
        }
        else if (entity.HasComponent<SpriteComponent>())
        {
            auto &sc = entity.GetComponent<SpriteComponent>();
            width = sc.width;
            height = sc.height;
        }
        else if (entity.HasComponent<AnimatedSpriteComponent>())
        {
            auto &ac = entity.GetComponent<AnimatedSpriteComponent>();
            if (ac.animation && ac.animation->GetTexture())
            {
                width = (float)ac.animation->GetFrameWidth();
                height = (float)ac.animation->GetFrameHeight();
            }
        }
        else if (entity.HasComponent<CollisionComponent>())
        {
            auto &cc = entity.GetComponent<CollisionComponent>();
            width = cc.width;
            height = cc.height;
        }
        else
        {
            return std::nullopt;
        }

        float halfW = width * tc.scale.x * 0.5f;
        float halfH = height * tc.scale.y * 0.5f;

        AABB2D bounds;
        bounds.min = {tc.position.x - halfW, tc.position.y - halfH};
        bounds.max = {tc.position.x + halfW, tc.position.y + halfH};

        return bounds;
    }

    bool PointInEntityBounds(Entity entity, const glm::vec2 &point)
    {
        auto bounds = GetEntityBounds2D(entity);
        if (!bounds)
            return false;

        return bounds->Contains(point);
    }

    glm::vec2 GetEntityCenter(Entity entity)
    {
        auto bounds = GetEntityBounds2D(entity);

        if (!bounds)
            return {0.0f, 0.0f};

        return {
            (bounds->min.x + bounds->max.x) * 0.5f,
            (bounds->min.y + bounds->max.y) * 0.5f};
    }
}
#pragma once

#include <Engine/Scene/Entity.h>
#include <glm/glm.hpp>
#include <array>
#include <optional>

struct TransformComponent;

namespace SIMPEngine
{
    struct AABB2D
    {
        glm::vec2 min{0.0f, 0.0f};
        glm::vec2 max{0.0f, 0.0f};

        bool Contains(const glm::vec2 &p) const
        {
            return p.x >= min.x && p.x <= max.x &&
                   p.y >= min.y && p.y <= max.y;
        }
    };

    struct WorldTransform2D
    {
        glm::vec2 Position{0.0f};
        float Rotation = 0.0f;
        glm::vec2 Scale{1.0f};
    };

    WorldTransform2D DecomposeWorld(const TransformComponent &tc);

    std::optional<glm::vec2> GetEntityLocalSize(Entity entity);
    std::optional<std::array<glm::vec2, 4>> GetEntityCorners(Entity entity);
    std::optional<AABB2D> GetEntityBounds2D(Entity entity);
    bool PointInEntityBounds(Entity entity, const glm::vec2 &point);
    glm::vec2 GetEntityCenter(Entity entity);
}

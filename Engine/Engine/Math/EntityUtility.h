#pragma once

#include <optional>
#include <glm/glm.hpp>
#include <Engine/Scene/Entity.h>

namespace SIMPEngine
{
    struct AABB2D
    {
        glm::vec2 min{0.0f, 0.0f};
        glm::vec2 max{0.0f, 0.0f};

        bool Contains(const glm::vec2& p) const
        {
            return p.x >= min.x && p.x <= max.x &&
                   p.y >= min.y && p.y <= max.y;
        }
    };

    std::optional<AABB2D> GetEntityBounds2D(Entity entity);
    bool PointInEntityBounds(Entity entity, const glm::vec2& point);

    glm::vec2 GetEntityCenter(Entity entity);
}
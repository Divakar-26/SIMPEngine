#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>

struct TilemapComponent;
struct TransformComponent;

namespace SIMPEngine
{
    class TilemapSystem
    {
    public:
        void Render(entt::registry &registry);

        static void VisibleWorldBounds(glm::vec2 &min, glm::vec2 &max);
        static void DrawTilemap(const TilemapComponent &tilemap, const TransformComponent &transform, const glm::vec2 &viewMin, const glm::vec2 &viewMax);
    };
}

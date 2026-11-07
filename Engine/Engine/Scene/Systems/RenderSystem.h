#pragma once

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class RenderSystem
    {
    public:
        void RenderQuads(entt::registry &registry);
        void RenderSprites(entt::registry &registry);
        void RenderText(entt::registry &registry);

        static void DrawQuad(entt::registry &registry, entt::entity entity);
        static void DrawSprite(entt::registry &registry, entt::entity entity);
        static void DrawText(entt::registry &registry, entt::entity entity);
        static void DrawEntity(entt::registry &registry, entt::entity entity);
    };
}

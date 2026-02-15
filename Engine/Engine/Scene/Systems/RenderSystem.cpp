#include <Engine/Scene/Systems/RenderSystem.h>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    void RenderSystem::RenderQuads(entt::registry &m_Registry)
    {
        std::vector<std::pair<entt::entity, float>> entitiesWithZIndex;
        auto view = m_Registry.view<TransformComponent, RenderComponent>();

        for (auto entity : view)
        {
            if (m_Registry.any_of<SpriteComponent>(entity) ||
                m_Registry.any_of<AnimatedSpriteComponent>(entity))
            {
                // auto &sprite = m_Registry.get<SpriteComponent>(entity);
                // if (sprite.texture && sprite.texture->GetID() != 0)
                continue;
            }

            auto &transform = view.get<TransformComponent>(entity);
            entitiesWithZIndex.emplace_back(entity, transform.zIndex);
        }

        std::sort(entitiesWithZIndex.begin(), entitiesWithZIndex.end(),
                  [](const auto &a, const auto &b)
                  { return a.second < b.second; });

        for (const auto &[entity, zIndex] : entitiesWithZIndex)
        {
            auto &transform = m_Registry.get<TransformComponent>(entity);
            auto &render = m_Registry.get<RenderComponent>(entity);

            glm::vec3 worldPos = transform.worldTransform[3];

            Renderer::DrawQuad(worldPos.x, worldPos.y, render.width * transform.scale.x,
                               render.height * transform.scale.y, transform.rotation,
                               render.color, true, zIndex);
        }
    }

    void RenderSystem::RenderSprites(entt::registry &m_Registry)
    {
        std::vector<std::pair<entt::entity, float>> entitiesWithZIndex;

        // Collect all entities that have a Sprite or Animation
        auto view = m_Registry.view<TransformComponent>();

        for (auto entity : view)
        {
            // Must have Transform + (Sprite OR Animation)
            if (m_Registry.any_of<SpriteComponent>(entity) ||
                m_Registry.any_of<AnimatedSpriteComponent>(entity))
            {
                auto &transform = m_Registry.get<TransformComponent>(entity);
                entitiesWithZIndex.emplace_back(entity, transform.zIndex);
            }
        }

        // Sort by zIndex (lower first → back to front)
        std::sort(entitiesWithZIndex.begin(), entitiesWithZIndex.end(),
                  [](const auto &a, const auto &b)
                  { return a.second < b.second; });

        // Render in correct order
        for (const auto &[entity, zIndex] : entitiesWithZIndex)
        {
            auto &transform = m_Registry.get<TransformComponent>(entity);
            float width = 1.0f;
            float height = 1.0f;

            if (m_Registry.any_of<RenderComponent>(entity))
            {
                auto &render = m_Registry.get<RenderComponent>(entity);
                width = render.width * transform.scale.x;
                height = render.height * transform.scale.y;
            }

            if (m_Registry.any_of<AnimatedSpriteComponent>(entity))
            {
                auto &anim = m_Registry.get<AnimatedSpriteComponent>(entity);
                if (anim.animation)
                {
                    anim.animation->Draw(transform.position.x, transform.position.y, width,
                                         height, SDL_Color{255, 255, 255, 255},
                                         transform.rotation);

                    continue;
                }
            }

            if (m_Registry.any_of<SpriteComponent>(entity))
            {
                auto &spriteComp = m_Registry.get<SpriteComponent>(entity);
                if (spriteComp.texture)
                {
                    Renderer::DrawTexture(spriteComp.texture, transform.position.x,
                                          transform.position.y, width, height,
                                          SDL_Color{255, 255, 255, 255}, transform.rotation,
                                          zIndex);
                }
            }
        }
    }
}

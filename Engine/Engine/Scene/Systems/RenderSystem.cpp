#include <Engine/Scene/Systems/RenderSystem.h>
#include <Engine/Scene/Component.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/Font.h>

#include <cmath>

namespace SIMPEngine
{
    void RenderSystem::RenderQuads(entt::registry &m_Registry)
    {
        auto view = m_Registry.view<TransformComponent, RenderComponent>();

        for (auto entity : view)
        {
            if (m_Registry.any_of<SpriteComponent, AnimatedSpriteComponent>(entity))
                continue;

            auto &transform = view.get<TransformComponent>(entity);
            auto &render = view.get<RenderComponent>(entity);
            WorldTransform2D w = DecomposeWorld(transform);

            Renderer::DrawQuad(w.Position.x, w.Position.y, render.width * w.Scale.x,
                               render.height * w.Scale.y, w.Rotation,
                               render.color, true, transform.zIndex);
        }
    }

    void RenderSystem::RenderSprites(entt::registry &m_Registry)
    {
        auto view = m_Registry.view<TransformComponent>();

        for (auto entity : view)
        {
            bool hasSprite = m_Registry.any_of<SpriteComponent>(entity);
            bool hasAnim = m_Registry.any_of<AnimatedSpriteComponent>(entity);
            if (!hasSprite && !hasAnim)
                continue;

            auto &transform = view.get<TransformComponent>(entity);
            WorldTransform2D w = DecomposeWorld(transform);

            float width = w.Scale.x;
            float height = w.Scale.y;

            if (auto *render = m_Registry.try_get<RenderComponent>(entity))
            {
                width = render->width * w.Scale.x;
                height = render->height * w.Scale.y;
            }

            SpriteComponent *sprite = m_Registry.try_get<SpriteComponent>(entity);
            if (sprite)
            {
                width = sprite->width * w.Scale.x;
                height = sprite->height * w.Scale.y;
            }

            if (hasAnim)
            {
                auto &anim = m_Registry.get<AnimatedSpriteComponent>(entity);
                if (anim.animation)
                {
                    SDL_Color tint = sprite ? sprite->tint : SDL_Color{255, 255, 255, 255};
                    anim.animation->Draw(w.Position.x, w.Position.y, width, height, tint, w.Rotation, transform.zIndex);
                    continue;
                }
            }

            if (sprite && sprite->texture)
            {
                SpriteDesc desc;
                desc.TextureRef = sprite->texture;
                desc.Position = w.Position;
                desc.Size = {width, height};
                desc.Rotation = w.Rotation;
                desc.Z = transform.zIndex;
                desc.Tint = sprite->tint;
                desc.FlipX = sprite->flipX;
                desc.FlipY = sprite->flipY;
                Renderer::DrawSprite(desc);
            }
        }
    }

    void RenderSystem::RenderText(entt::registry &m_Registry)
    {
        auto view = m_Registry.view<TransformComponent, TextComponent>();

        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &text = view.get<TextComponent>(entity);
            if (text.Text.empty())
                continue;

            auto font = text.FontPath.empty() ? Font::GetDefault() : Font::Load(text.FontPath);
            if (!font)
                continue;

            WorldTransform2D w = DecomposeWorld(transform);
            float size = text.Size * std::abs(w.Scale.y);
            glm::vec2 extent = font->MeasureText(text.Text, size);

            float r = glm::radians(w.Rotation);
            glm::vec2 up(-std::sin(r), std::cos(r));
            glm::vec2 anchor = w.Position + up * (extent.y * 0.5f);

            TextAlign align = text.Align == 0 ? TextAlign::Left : text.Align == 2 ? TextAlign::Right : TextAlign::Center;
            Renderer::DrawText(text.Text, anchor.x, anchor.y, size, text.Color, align, w.Rotation, transform.zIndex, font.get());
        }
    }
}

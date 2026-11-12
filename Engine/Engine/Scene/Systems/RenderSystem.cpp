#include <Engine/Scene/Systems/RenderSystem.h>
#include <Engine/Scene/Component.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Core/GameVariables.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Scene/Systems/TilemapSystem.h>
#include <Engine/Scene/Systems/ParticleSystem.h>
#include <Engine/Scene/Systems/VectorSystem.h>

#include <cmath>

namespace SIMPEngine
{
    void RenderSystem::DrawQuad(entt::registry &registry, entt::entity entity)
    {
        if (registry.any_of<SpriteComponent, AnimatedSpriteComponent>(entity) || !registry.all_of<TransformComponent, RenderComponent>(entity))
            return;

        auto &transform = registry.get<TransformComponent>(entity);
        auto &render = registry.get<RenderComponent>(entity);
        WorldTransform2D w = DecomposeWorld(transform);
        Renderer::DrawQuad(w.Position.x, w.Position.y, render.width * w.Scale.x,
                           render.height * w.Scale.y, w.Rotation,
                           render.color, true, transform.zIndex);
    }

    void RenderSystem::DrawSprite(entt::registry &registry, entt::entity entity)
    {
        bool hasSprite = registry.any_of<SpriteComponent>(entity);
        bool hasAnim = registry.any_of<AnimatedSpriteComponent>(entity);
        if ((!hasSprite && !hasAnim) || !registry.all_of<TransformComponent>(entity))
            return;

        auto &transform = registry.get<TransformComponent>(entity);
        WorldTransform2D w = DecomposeWorld(transform);

        float width = w.Scale.x;
        float height = w.Scale.y;

        if (auto *render = registry.try_get<RenderComponent>(entity))
        {
            width = render->width * w.Scale.x;
            height = render->height * w.Scale.y;
        }

        SpriteComponent *sprite = registry.try_get<SpriteComponent>(entity);
        if (sprite)
        {
            width = sprite->width * w.Scale.x;
            height = sprite->height * w.Scale.y;
        }

        if (hasAnim)
        {
            auto &anim = registry.get<AnimatedSpriteComponent>(entity);
            if (anim.animation && anim.animation->GetTexture())
            {
                const SDL_FRect frame = anim.animation->GetFrameRect();
                SpriteDesc desc;
                desc.TextureRef = anim.animation->GetTexture();
                desc.Position = w.Position;
                desc.Size = {width, height};
                desc.Rotation = w.Rotation;
                desc.Z = transform.zIndex;
                desc.Tint = sprite ? sprite->tint : SDL_Color{255, 255, 255, 255};
                desc.FlipX = sprite && sprite->flipX;
                desc.FlipY = sprite && sprite->flipY;
                desc.SourceRect = &frame;
                Renderer::DrawSprite(desc);
                return;
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
            desc.SourceRect = sprite->useRegion ? &sprite->region : nullptr;
            Renderer::DrawSprite(desc);
        }
    }

    void RenderSystem::DrawText(entt::registry &registry, entt::entity entity)
    {
        if (!registry.all_of<TransformComponent, TextComponent>(entity))
            return;

        auto &transform = registry.get<TransformComponent>(entity);
        auto &text = registry.get<TextComponent>(entity);
        if (text.Text.empty())
            return;
        const std::string shown = GameVariables::Format(text.Text);

        auto font = text.FontPath.empty() ? Font::GetDefault() : Font::Load(text.FontPath);
        if (!font)
            return;

        WorldTransform2D w = DecomposeWorld(transform);
        float size = text.Size * std::abs(w.Scale.y);
        glm::vec2 extent = font->MeasureText(shown, size);

        float r = glm::radians(w.Rotation);
        glm::vec2 up(-std::sin(r), std::cos(r));
        glm::vec2 anchor = w.Position + up * (extent.y * 0.5f);

        TextAlign align = text.Align == 0 ? TextAlign::Left : text.Align == 2 ? TextAlign::Right : TextAlign::Center;
        Renderer::DrawText(shown, anchor.x, anchor.y, size, text.Color, align, w.Rotation, transform.zIndex, font.get());
    }

    void RenderSystem::DrawEntity(entt::registry &registry, entt::entity entity)
    {
        if (!registry.valid(entity) || !registry.all_of<TransformComponent>(entity))
            return;

        if (registry.all_of<TilemapComponent>(entity))
        {
            glm::vec2 viewMin, viewMax;
            TilemapSystem::VisibleWorldBounds(viewMin, viewMax);
            TilemapSystem::DrawTilemap(registry.get<TilemapComponent>(entity), registry.get<TransformComponent>(entity), viewMin, viewMax);
        }
        DrawQuad(registry, entity);
        DrawSprite(registry, entity);
        VectorSystem::DrawVector(registry, entity);
        if (auto *emitter = registry.try_get<ParticleEmitterComponent>(entity))
        {
            auto &transform = registry.get<TransformComponent>(entity);
            ParticleEmitter::Render(emitter->effect, emitter->state, ParticleSystem::FrameOf(transform), transform.zIndex);
        }
        DrawText(registry, entity);
    }

    void RenderSystem::RenderQuads(entt::registry &registry)
    {
        for (auto entity : registry.view<TransformComponent, RenderComponent>(entt::exclude<ShaderGroupMemberTag>))
            DrawQuad(registry, entity);
    }

    void RenderSystem::RenderSprites(entt::registry &registry)
    {
        for (auto entity : registry.view<TransformComponent>(entt::exclude<ShaderGroupMemberTag>))
            DrawSprite(registry, entity);
    }

    void RenderSystem::RenderText(entt::registry &registry)
    {
        for (auto entity : registry.view<TransformComponent, TextComponent>(entt::exclude<ShaderGroupMemberTag>))
            DrawText(registry, entity);
    }
}

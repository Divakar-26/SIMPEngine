#include "EntityUtility.h"
#include <Engine/Scene/Component.h>
#include <Engine/Rendering/Font.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    WorldTransform2D DecomposeWorld(const TransformComponent &tc)
    {
        const glm::mat4 &m = tc.worldTransform;
        WorldTransform2D w;
        w.Position = {m[3][0], m[3][1]};

        glm::vec2 col0(m[0][0], m[0][1]);
        glm::vec2 col1(m[1][0], m[1][1]);
        w.Scale = {glm::length(col0), glm::length(col1)};
        if (w.Scale.x <= 0.0f && w.Scale.y <= 0.0f)
        {
            w.Position = tc.position;
            w.Rotation = tc.rotation;
            w.Scale = tc.scale;
            return w;
        }

        w.Rotation = glm::degrees(std::atan2(col0.y, col0.x));
        float det = col0.x * col1.y - col0.y * col1.x;
        if (det < 0.0f)
            w.Scale.y = -w.Scale.y;
        return w;
    }

    std::optional<glm::vec2> GetEntityLocalSize(Entity entity)
    {
        if (!entity || !entity.HasComponent<TransformComponent>())
            return std::nullopt;

        if (entity.HasComponent<SpriteComponent>())
        {
            auto &sc = entity.GetComponent<SpriteComponent>();
            return glm::vec2(sc.width, sc.height);
        }
        if (entity.HasComponent<RenderComponent>())
        {
            auto &rc = entity.GetComponent<RenderComponent>();
            return glm::vec2(rc.width, rc.height);
        }
        if (entity.HasComponent<AnimatedSpriteComponent>())
        {
            auto &ac = entity.GetComponent<AnimatedSpriteComponent>();
            if (ac.animation && ac.animation->GetTexture())
                return glm::vec2(static_cast<float>(ac.animation->GetFrameWidth()), static_cast<float>(ac.animation->GetFrameHeight()));
        }
        if (entity.HasComponent<TextComponent>())
        {
            auto &tx = entity.GetComponent<TextComponent>();
            auto font = tx.FontPath.empty() ? Font::GetDefault() : Font::Load(tx.FontPath);
            if (font)
            {
                glm::vec2 size = font->MeasureText(tx.Text.empty() ? " " : tx.Text, tx.Size);
                return glm::vec2(std::max(size.x, tx.Size * 0.5f), std::max(size.y, tx.Size));
            }
        }
        if (entity.HasComponent<CollisionComponent>())
        {
            auto &cc = entity.GetComponent<CollisionComponent>();
            return glm::vec2(cc.width, cc.height);
        }
        if (entity.HasComponent<CameraComponent>())
            return glm::vec2(48.0f, 48.0f);

        return std::nullopt;
    }

    std::optional<std::array<glm::vec2, 4>> GetEntityCorners(Entity entity)
    {
        auto size = GetEntityLocalSize(entity);
        if (!size)
            return std::nullopt;

        auto &tc = entity.GetComponent<TransformComponent>();
        WorldTransform2D w = DecomposeWorld(tc);

        glm::vec2 half = *size * 0.5f * glm::abs(w.Scale);
        glm::vec2 offset(0.0f);
        if (entity.HasComponent<TextComponent>())
        {
            auto &tx = entity.GetComponent<TextComponent>();
            if (tx.Align == 0)
                offset.x = half.x;
            else if (tx.Align == 2)
                offset.x = -half.x;
        }

        float r = glm::radians(w.Rotation);
        float c = std::cos(r), s = std::sin(r);
        glm::vec2 local[4] = {
            {-half.x, half.y}, {half.x, half.y}, {half.x, -half.y}, {-half.x, -half.y}};

        std::array<glm::vec2, 4> corners;
        for (size_t i = 0; i < 4; ++i)
        {
            glm::vec2 p = local[i] + offset;
            corners[i] = w.Position + glm::vec2(p.x * c - p.y * s, p.x * s + p.y * c);
        }
        return corners;
    }

    std::optional<AABB2D> GetEntityBounds2D(Entity entity)
    {
        auto corners = GetEntityCorners(entity);
        if (!corners)
            return std::nullopt;

        AABB2D bounds;
        bounds.min = bounds.max = (*corners)[0];
        for (const auto &c : *corners)
        {
            bounds.min = glm::min(bounds.min, c);
            bounds.max = glm::max(bounds.max, c);
        }
        return bounds;
    }

    bool PointInEntityBounds(Entity entity, const glm::vec2 &point)
    {
        auto corners = GetEntityCorners(entity);
        if (!corners)
            return false;

        const auto &c = *corners;
        bool sign = false;
        for (size_t i = 0; i < 4; ++i)
        {
            glm::vec2 a = c[i], b = c[(i + 1) % 4];
            float cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
            if (i == 0)
                sign = cross >= 0.0f;
            else if ((cross >= 0.0f) != sign)
                return false;
        }
        return true;
    }

    glm::vec2 GetEntityCenter(Entity entity)
    {
        auto corners = GetEntityCorners(entity);
        if (!corners)
        {
            if (entity && entity.HasComponent<TransformComponent>())
                return DecomposeWorld(entity.GetComponent<TransformComponent>()).Position;
            return {0.0f, 0.0f};
        }

        glm::vec2 sum(0.0f);
        for (const auto &c : *corners)
            sum += c;
        return sum * 0.25f;
    }
}

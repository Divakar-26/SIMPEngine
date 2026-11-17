#pragma once

#include <Engine/Math/EntityUtility.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>

#include <cmath>

namespace SIMPEngine
{
    inline void ApplyEntityClip(entt::registry &registry, entt::entity entity)
    {
        const ClipTag *tag = registry.try_get<ClipTag>(entity);
        if (!tag || !registry.valid(tag->owner))
        {
            Renderer::ClearClip();
            return;
        }
        const auto *clip = registry.try_get<ClipComponent>(tag->owner);
        const auto *transform = registry.try_get<TransformComponent>(tag->owner);
        if (!clip || !transform)
        {
            Renderer::ClearClip();
            return;
        }
        const WorldTransform2D world = DecomposeWorld(*transform);
        Renderer::SetClipWorld(world.Position.x, world.Position.y, clip->size.x * std::abs(world.Scale.x), clip->size.y * std::abs(world.Scale.y));
    }
}

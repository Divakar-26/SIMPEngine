#include "VectorSystem.h"

#include <Engine/Core/Profiler/EngineProfiler.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Component.h>
#include <Engine/Vector/VectorShape.h>

namespace SIMPEngine
{
    void VectorSystem::Render(entt::registry &registry)
    {
        PROFILE_SCOPE("VectorSystem::Render");
        for (auto entity : registry.view<TransformComponent, VectorShapeComponent>(entt::exclude<ShaderGroupMemberTag>))
            DrawVector(registry, entity);
    }

    void VectorSystem::DrawVector(entt::registry &registry, entt::entity entity)
    {
        const auto *shape = registry.try_get<VectorShapeComponent>(entity);
        const auto *transform = registry.try_get<TransformComponent>(entity);
        if (!shape || !transform)
            return;
        const WorldTransform2D world = DecomposeWorld(*transform);
        VectorShapes::Draw(*shape, world.Position, world.Rotation, world.Scale, transform->zIndex);
    }
}

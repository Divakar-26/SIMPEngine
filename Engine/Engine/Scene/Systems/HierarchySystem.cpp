//
// Created by divakar on 9/14/26.
//

#include "HierarchySystem.h"

void SIMPEngine::HierarchySystem::Update(entt::registry &registry, float deltaTime)
{
    PROFILE_FUNCTION();

    auto view = registry.view<TransformComponent>();

    for (auto entity : view)
    {
        auto &tf = view.get<TransformComponent>(entity);

        glm::mat4 local = tf.GetLocalTransform();

        if (!registry.any_of<HierarchyComponent>(entity))
        {
            tf.worldTransform = local;
            continue;
        }

        auto &h = registry.get<HierarchyComponent>(entity);

        if (h.parent == entt::null || !registry.valid(h.parent))
        {
            tf.worldTransform = local;
            continue;
        }

        auto &parentTf =
            registry.get<TransformComponent>(h.parent);

        tf.worldTransform =
            parentTf.worldTransform * local;
    }
}
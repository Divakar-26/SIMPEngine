#include "HierarchySystem.h"

namespace
{
    void Propagate(entt::registry &registry, entt::entity entity, const glm::mat4 &parent, int depth)
    {
        auto &tf = registry.get<TransformComponent>(entity);
        tf.worldTransform = parent * tf.GetLocalTransform();

        if (depth > 64)
            return;

        auto *h = registry.try_get<HierarchyComponent>(entity);
        if (!h)
            return;

        for (auto child : h->children)
        {
            if (registry.valid(child) && registry.all_of<TransformComponent>(child))
                Propagate(registry, child, tf.worldTransform, depth + 1);
        }
    }
}

void SIMPEngine::HierarchySystem::Update(entt::registry &registry, float deltaTime)
{
    PROFILE_FUNCTION();
    (void)deltaTime;

    auto view = registry.view<TransformComponent>();

    for (auto entity : view)
    {
        auto *h = registry.try_get<HierarchyComponent>(entity);
        bool isRoot = !h || h->parent == entt::null || !registry.valid(h->parent) ||
                      !registry.all_of<TransformComponent>(h->parent);
        if (isRoot)
            Propagate(registry, entity, glm::mat4(1.0f), 0);
    }
}

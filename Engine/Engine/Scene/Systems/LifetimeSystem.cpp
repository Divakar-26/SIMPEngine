#include "LifetimeSystem.h"

void SIMPEngine::LifetimeSystem::DestroyRecursive(entt::registry &reg, entt::entity e)
{
    if (reg.any_of<HierarchyComponent>(e))
    {
        const auto children = reg.get<HierarchyComponent>(e).children;

        for (auto child : children)
        {
            if (reg.valid(child))
                DestroyRecursive(reg, child);
        }
    }

    reg.destroy(e);
}

void SIMPEngine::LifetimeSystem::Update(entt::registry &reg, float dt)
{
    std::vector<entt::entity> toDestroy;

    auto view = reg.view<LifetimeComponent>();

    for (auto e : view)
    {
        auto &lf = view.get<LifetimeComponent>(e);

        lf.remaining -= dt;

        if (lf.remaining <= 0.0f)
            toDestroy.push_back(e);
    }

    for (auto e : toDestroy)
    {
        if (reg.valid(e))
            DestroyRecursive(reg, e);
    }
}

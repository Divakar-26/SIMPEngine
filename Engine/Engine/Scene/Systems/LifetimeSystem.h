#pragma once
#include <entt/entt.hpp>
#include <Scene/Component.h>

namespace SIMPEngine
{
    class LifetimeSystem
    {
    public:
        void DestroyRecursive(entt::registry &reg, entt::entity e)
        {
            if (reg.any_of<HierarchyComponent>(e))
            {
                auto &h = reg.get<HierarchyComponent>(e);

                for (auto child : h.children)
                {
                    if (reg.valid(child))
                        DestroyRecursive(reg, child);
                }
            }

            reg.destroy(e);
        }

        void Update(entt::registry &reg, float dt)
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
                DestroyRecursive(reg, e);
        }
    };
}
#pragma once
#include <Scene/Component.h>
#include <entt/entt.hpp>
#include <Engine/Core/Profiler/EngineProfiler.h>

namespace SIMPEngine
{

    class HierarchySystem
    {

    public:
        void Update(entt::registry &registry, float deltaTime = 0.0f)
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
    };
}
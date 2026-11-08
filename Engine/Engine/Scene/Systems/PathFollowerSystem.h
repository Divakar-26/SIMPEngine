#pragma once
#include <Engine/Scene/Component.h>
#include <Engine/Scene/ScriptEvent.h>

#include <entt/entt.hpp>
#include <vector>

namespace SIMPEngine
{
    class Scene;

    class PathFollowerSystem
    {
    public:
        void Update(Scene &scene, float dt);

        static void Restart(PathFollowerComponent &path);

    private:
        struct Pending
        {
            entt::entity Target = entt::null;
            ScriptEvent Type = ScriptEvent::PathPoint;
            float Value = 0.0f;
        };

        std::vector<Pending> m_Pending;
    };
}

#pragma once
#include <Engine/Scene/Component.h>
#include <Engine/Scene/ScriptEvent.h>

#include <entt/entt.hpp>
#include <vector>

namespace SIMPEngine
{
    class Scene;

    class TriggerSystem
    {
    public:
        void Update(Scene &scene);

    private:
        struct Pending
        {
            entt::entity Trigger = entt::null;
            entt::entity Other = entt::null;
            ScriptEvent Type = ScriptEvent::TriggerEnter;
        };

        std::vector<Pending> m_Pending;
    };
}

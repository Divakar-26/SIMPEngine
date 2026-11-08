#pragma once
#include <Engine/Scene/Component.h>

#include <entt/entt.hpp>

namespace SIMPEngine
{
    class Scene;

    class HealthSystem
    {
    public:
        void Update(Scene &scene, float dt);

        static bool Damage(Scene &scene, entt::entity target, float amount, entt::entity source = entt::null);
        static float Heal(Scene &scene, entt::entity target, float amount);
        static bool Kill(Scene &scene, entt::entity target, entt::entity source = entt::null);
        static bool Revive(Scene &scene, entt::entity target);
    };
}

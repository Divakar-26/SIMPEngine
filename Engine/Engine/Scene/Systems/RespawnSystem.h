#pragma once
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class Scene;

    class RespawnSystem
    {
    public:
        void Update(Scene &scene, float dt);
        static void Respawn(Scene &scene, entt::entity entity);
    };
}

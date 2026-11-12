#pragma once
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class Scene;

    class PickableSystem
    {
    public:
        void Update(Scene &scene, float dt);
        static void Drop(Scene &scene, entt::entity entity);
    };
}

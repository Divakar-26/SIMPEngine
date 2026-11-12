#pragma once
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class Scene;

    class FollowSystem
    {
    public:
        void Update(Scene &scene, float dt);
    };
}

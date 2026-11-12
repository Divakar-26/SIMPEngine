#pragma once
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class Scene;

    class PickupSystem
    {
    public:
        void Update(Scene &scene);
    };
}

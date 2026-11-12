#pragma once
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class Scene;

    class BoundsSystem
    {
    public:
        void Update(Scene &scene);
    };
}

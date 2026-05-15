#pragma once
#include <string>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    class TiledLoader
    {
    public:
        static bool Load(
            const std::string& mapPath,
            TilemapComponent& tilemap,
            TransformComponent& transform
        );
    };
}
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <Engine/Scene/Component.h>
#include <entt/entt.hpp>

namespace SIMPEngine
{
    class Scene;
    class Tileset;

    class TiledLoader
    {
    public:
        static std::shared_ptr<Tileset> LoadTileset(const std::string &tilesetPath);

        static bool IsTiledMap(const std::string &path);

        static bool Load(
            const std::string& mapPath,
            TilemapComponent& tilemap,
            TransformComponent& transform
        );

        static std::vector<entt::entity> Import(const std::string &mapPath, Scene &scene);
    };
}

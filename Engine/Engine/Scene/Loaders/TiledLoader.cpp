#include "TiledLoader.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

using json = nlohmann::json;

namespace SIMPEngine
{

bool TiledLoader::Load(
    const std::string& mapPath,
    TilemapComponent& tilemap,
    TransformComponent& transform)
{

    std::string realMapPath;

    // get the actual path from the VFS path
    if (auto resolved = VFS::Resolve(mapPath))
        realMapPath = *resolved; 
    else
    {
        CORE_ERROR("Failed to resolve VFS path '{}'", mapPath);
        return false;
    }

    // open the JSON file
    std::ifstream mapFile(realMapPath);
    if (!mapFile.is_open())
    {
        CORE_ERROR("Failed to open map file '{}'", realMapPath);
        return false;
    }

    // get the map data from the json
    json mapData;
    mapFile >> mapData;
    mapFile.close();


    if (!mapData.contains("width") ||
        !mapData.contains("height") ||
        !mapData.contains("tilewidth") ||
        !mapData.contains("tileheight"))
    {
        CORE_ERROR("Invalid Tiled map format.");
        return false;
    }

    int mapWidth   = mapData["width"];
    int mapHeight  = mapData["height"];
    int tileWidth  = mapData["tilewidth"];
    int tileHeight = mapData["tileheight"];

    if (tileWidth != tileHeight)
    {
        CORE_ERROR("Non-square tiles not supported yet."); 
        return false;
    }
    
    tilemap.tileSize = tileWidth;


    if (!mapData.contains("tilesets") || mapData["tilesets"].empty())
    {
        CORE_ERROR("No tilesets found in map.");
        return false;
    }

    auto tilesetInfo = mapData["tilesets"][0];

    if (!tilesetInfo.contains("source"))
    {
        CORE_ERROR("Embedded tilesets not supported yet.");
        return false;
    }

    std::string tilesetRelative = tilesetInfo["source"];

    // Extract map directory
    std::string mapDir =
        realMapPath.substr(0, realMapPath.find_last_of("/\\"));

    // Build full tileset path
    std::string fullTilesetPath = mapDir + "/" + tilesetRelative;

    std::ifstream tsFile(fullTilesetPath);
    if (!tsFile.is_open())
    {
        CORE_ERROR("Failed to open tileset file '{}'", fullTilesetPath);
        return false;
    }

    json tilesetData;
    tsFile >> tilesetData;
    tsFile.close();

    if (!tilesetData.contains("image"))
    {
        CORE_ERROR("Tileset missing image field.");
        return false;
    }

    std::string tilesetDir =
        fullTilesetPath.substr(0, fullTilesetPath.find_last_of("/\\"));

    std::string imageRelative = tilesetData["image"];
    std::string fullImagePath = tilesetDir + "/" + imageRelative;

    auto texture = TextureManager::Get().LoadTexture(
        fullImagePath,
        fullImagePath
    );

    if (!texture)
    {
        CORE_ERROR("Failed to load texture '{}'", fullImagePath);
        return false;
    }

    int tsTileWidth  = tilesetData["tilewidth"];
    int tsTileHeight = tilesetData["tileheight"];

    tilemap.tileset = std::make_shared<Tileset>(
        texture,
        tsTileWidth,
        tsTileHeight
    );

    if (!mapData.contains("layers") || mapData["layers"].empty())
    {
        CORE_ERROR("No layers found.");
        return false;
    }

    auto layer = mapData["layers"][0];

    if (!layer.contains("data"))
    {
        CORE_ERROR("Layer has no tile data.");
        return false;
    }

    auto tileArray = layer["data"];

    for (int y = 0; y < mapHeight; y++)
    {
        for (int x = 0; x < mapWidth; x++)
        {
            int index  = y * mapWidth + x;
            int tileID = tileArray[index];

            if (tileID == 0)
                continue;

            // Tiled starts at 1
            tileID -= 1;

            // Flip Y because Tiled = top-left origin
            int flippedY = mapHeight - 1 - y;

            SetTile(tilemap, x, flippedY, tileID);
        }
    }


    float worldWidth  = mapWidth  * tilemap.tileSize;
    float worldHeight = mapHeight * tilemap.tileSize;

    transform.position.x = -worldWidth  * 0.5f;
    transform.position.y = -worldHeight * 0.5f;

    CORE_INFO("Tiled map loaded successfully.");

    return true;
}

}
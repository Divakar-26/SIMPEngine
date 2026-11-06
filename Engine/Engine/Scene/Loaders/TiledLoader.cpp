#include "TiledLoader.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>

using json = nlohmann::json;

namespace SIMPEngine
{
namespace
{
    constexpr uint32_t FlippedHorizontally = 0x80000000u;
    constexpr uint32_t FlippedVertically = 0x40000000u;
    constexpr uint32_t GidMask = 0x0FFFFFFFu;

    std::string DirectoryOf(const std::string &path)
    {
        size_t slash = path.find_last_of("/\\");
        if (slash == std::string::npos)
            return {};
        return path.substr(0, slash);
    }

    std::string JoinPath(const std::string &dir, const std::string &relative)
    {
        size_t scheme = dir.find("://");
        std::string prefix = scheme == std::string::npos ? std::string() : dir.substr(0, scheme + 3);
        std::string body = scheme == std::string::npos ? dir : dir.substr(scheme + 3);
        bool absolute = !body.empty() && body[0] == '/';

        std::vector<std::string> parts;
        auto push = [&](const std::string &text)
        {
            std::stringstream ss(text);
            std::string part;
            while (std::getline(ss, part, '/'))
            {
                if (part.empty() || part == ".")
                    continue;
                if (part == "..")
                {
                    if (!parts.empty())
                        parts.pop_back();
                    continue;
                }
                parts.push_back(part);
            }
        };
        push(body);
        push(relative);

        std::string out = prefix + (absolute ? "/" : "");
        for (size_t i = 0; i < parts.size(); ++i)
            out += (i ? "/" : "") + parts[i];
        return out;
    }

    bool ReadJson(const std::string &path, json &out)
    {
        std::string real = path;
        if (auto resolved = VFS::Resolve(path))
            real = *resolved;

        std::ifstream file(real);
        if (!file.is_open())
        {
            CORE_ERROR("Tiled: cannot open '{}'", real);
            return false;
        }

        try
        {
            file >> out;
        }
        catch (const json::exception &e)
        {
            CORE_ERROR("Tiled: invalid JSON in '{}': {}", real, e.what());
            return false;
        }
        return true;
    }

    std::shared_ptr<Tileset> TilesetFromJson(const json &data, const std::string &baseDir)
    {
        if (!data.contains("image") || !data.contains("tilewidth") || !data.contains("tileheight"))
        {
            CORE_ERROR("Tiled: tileset needs image, tilewidth and tileheight (image collections are not supported)");
            return nullptr;
        }

        std::string imagePath = JoinPath(baseDir, data["image"].get<std::string>());
        auto texture = TextureManager::Get().LoadTexture(imagePath, imagePath);
        if (!texture)
        {
            CORE_ERROR("Tiled: failed to load tileset image '{}'", imagePath);
            return nullptr;
        }

        return std::make_shared<Tileset>(texture,
                                         data["tilewidth"].get<int>(),
                                         data["tileheight"].get<int>(),
                                         data.value("margin", 0),
                                         data.value("spacing", 0));
    }

    struct MapTileset
    {
        uint32_t FirstGid = 1;
        std::shared_ptr<Tileset> Set;
    };

    struct LayerTiles
    {
        std::string Name;
        bool Visible = true;
        float Opacity = 1.0f;
        std::vector<std::pair<glm::ivec2, uint32_t>> Cells;
    };

    bool ReadMap(const std::string &mapPath, json &map, std::vector<MapTileset> &tilesets, std::vector<LayerTiles> &layers)
    {
        if (!ReadJson(mapPath, map))
            return false;

        if (!map.contains("tilewidth") || !map.contains("tileheight") || !map.contains("layers"))
        {
            CORE_ERROR("Tiled: '{}' is not a map", mapPath);
            return false;
        }

        if (map.value("orientation", std::string("orthogonal")) != "orthogonal")
            CORE_WARN("Tiled: only orthogonal maps are supported, '{}' may look wrong", mapPath);

        std::string mapDir = DirectoryOf(mapPath);
        for (const auto &entry : map.value("tilesets", json::array()))
        {
            MapTileset ts;
            ts.FirstGid = entry.value("firstgid", 1u);
            if (entry.contains("source"))
            {
                std::string source = JoinPath(mapDir, entry["source"].get<std::string>());
                json tsData;
                if (ReadJson(source, tsData))
                    ts.Set = TilesetFromJson(tsData, DirectoryOf(source));
                if (ts.Set)
                    ts.Set->SetSourcePath(source);
            }
            else
            {
                ts.Set = TilesetFromJson(entry, mapDir);
            }
            if (ts.Set)
                tilesets.push_back(ts);
        }

        if (tilesets.empty())
        {
            CORE_ERROR("Tiled: '{}' has no usable tileset", mapPath);
            return false;
        }
        if (tilesets.size() > 1)
            CORE_WARN("Tiled: '{}' uses {} tilesets; only tiles from the first one are imported", mapPath, tilesets.size());

        int mapHeight = map.value("height", 0);
        bool infinite = map.value("infinite", false);

        auto readData = [&](const json &data, int x0, int y0, int width, LayerTiles &layer)
        {
            if (!data.is_array())
            {
                CORE_WARN("Tiled: layer '{}' uses base64/compressed data; export the map with CSV encoding", layer.Name);
                return;
            }
            for (size_t i = 0; i < data.size(); ++i)
            {
                uint32_t gid = data[i].get<uint32_t>();
                if ((gid & GidMask) == 0)
                    continue;
                int x = x0 + static_cast<int>(i) % width;
                int y = y0 + static_cast<int>(i) / width;
                layer.Cells.push_back({{x, y}, gid});
            }
        };

        for (const auto &l : map["layers"])
        {
            if (l.value("type", std::string()) != "tilelayer")
                continue;

            LayerTiles layer;
            layer.Name = l.value("name", std::string("Layer"));
            layer.Visible = l.value("visible", true);
            layer.Opacity = l.value("opacity", 1.0f);

            if (l.contains("chunks"))
            {
                for (const auto &chunk : l["chunks"])
                    readData(chunk["data"], chunk.value("x", 0), chunk.value("y", 0), std::max(1, chunk.value("width", 1)), layer);
            }
            else if (l.contains("data"))
            {
                readData(l["data"], 0, 0, std::max(1, l.value("width", map.value("width", 1))), layer);
            }

            for (auto &[cell, gid] : layer.Cells)
                cell.y = (infinite ? 0 : mapHeight) - 1 - cell.y;

            layers.push_back(std::move(layer));
        }

        return true;
    }

    void FillTilemap(TilemapComponent &tilemap, const LayerTiles &layer, const MapTileset &tileset, float tileSize)
    {
        tilemap.tileset = tileset.Set;
        tilemap.tileSize = tileSize;
        tilemap.visible = layer.Visible;
        tilemap.tint.a = static_cast<Uint8>(std::clamp(layer.Opacity, 0.0f, 1.0f) * 255.0f);
        tilemap.chunks.clear();

        int count = tileset.Set->GetTileCount();
        for (const auto &[cell, gid] : layer.Cells)
        {
            int id = static_cast<int>((gid & GidMask) - tileset.FirstGid);
            if (id < 0 || id >= count)
                continue;
            uint8_t flags = 0;
            if (gid & FlippedHorizontally)
                flags |= TileFlipX;
            if (gid & FlippedVertically)
                flags |= TileFlipY;
            SetTile(tilemap, cell.x, cell.y, id, flags);
        }
    }
}

std::shared_ptr<Tileset> TiledLoader::LoadTileset(const std::string &tilesetPath)
{
    json data;
    if (!ReadJson(tilesetPath, data))
        return nullptr;

    auto tileset = TilesetFromJson(data, DirectoryOf(tilesetPath));
    if (tileset)
        tileset->SetSourcePath(tilesetPath);
    return tileset;
}

bool TiledLoader::IsTiledMap(const std::string &path)
{
    json data;
    if (!ReadJson(path, data))
        return false;
    return data.contains("layers") && data.contains("tilewidth");
}

bool TiledLoader::Load(
    const std::string& mapPath,
    TilemapComponent& tilemap,
    TransformComponent& transform)
{
    json map;
    std::vector<MapTileset> tilesets;
    std::vector<LayerTiles> layers;
    if (!ReadMap(mapPath, map, tilesets, layers) || layers.empty())
        return false;

    float tileSize = static_cast<float>(map["tilewidth"].get<int>());
    FillTilemap(tilemap, layers.front(), tilesets.front(), tileSize);

    transform.position.x = -static_cast<float>(map.value("width", 0)) * tileSize * 0.5f;
    transform.position.y = -static_cast<float>(map.value("height", 0)) * tileSize * 0.5f;
    transform.dirty = true;
    return true;
}

std::vector<entt::entity> TiledLoader::Import(const std::string &mapPath, Scene &scene)
{
    std::vector<entt::entity> created;

    json map;
    std::vector<MapTileset> tilesets;
    std::vector<LayerTiles> layers;
    if (!ReadMap(mapPath, map, tilesets, layers))
        return created;

    if (layers.empty())
    {
        CORE_WARN("Tiled: '{}' has no tile layers", mapPath);
        return created;
    }

    float tileSize = static_cast<float>(map["tilewidth"].get<int>());
    std::string name = std::filesystem::path(mapPath).stem().string();

    Entity root = scene.CreateEntity(name.empty() ? "Tiled Map" : name);
    auto &rootTransform = root.GetComponent<TransformComponent>();
    rootTransform.position.x = -static_cast<float>(map.value("width", 0)) * tileSize * 0.5f;
    rootTransform.position.y = -static_cast<float>(map.value("height", 0)) * tileSize * 0.5f;
    created.push_back(root.GetHandle());

    for (size_t i = 0; i < layers.size(); ++i)
    {
        Entity layer = scene.CreateEntity(layers[i].Name);
        layer.GetComponent<TransformComponent>().zIndex = static_cast<float>(i) * 0.1f;
        auto &tilemap = layer.AddComponent<TilemapComponent>();
        FillTilemap(tilemap, layers[i], tilesets.front(), tileSize);
        scene.SetParent(layer.GetHandle(), root.GetHandle());
        created.push_back(layer.GetHandle());
    }

    scene.UpdateTransforms();
    CORE_INFO("Tiled: imported '{}' ({} layers)", mapPath, layers.size());
    return created;
}
}

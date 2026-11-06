#pragma once
#include "Component.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace SIMPEngine
{
inline int floor_div(int a, int b)
{
    int q = a / b;
    int r = a % b;
    if ((r != 0) && ((r < 0) != (b < 0)))
        --q;
    return q;
}

inline int mod(int a, int b)
{
    int r = a % b;
    if (r < 0)
        r += b;
    return r;
}

inline int64_t ChunkKey(int cx, int cy)
{
    return (static_cast<int64_t>(cx) << 32) | static_cast<int64_t>(static_cast<uint32_t>(cy));
}

inline glm::ivec2 ChunkCoord(int64_t key)
{
    return {static_cast<int>(key >> 32), static_cast<int>(static_cast<uint32_t>(key))};
}

inline void SetTile(TilemapComponent &tm, int tileX, int tileY, int id, uint8_t flags = 0)
{
    int chunkX = floor_div(tileX, CHUNK_SIZE);
    int chunkY = floor_div(tileY, CHUNK_SIZE);

    int localX = mod(tileX, CHUNK_SIZE);
    int localY = mod(tileY, CHUNK_SIZE);

    int64_t key = ChunkKey(chunkX, chunkY);

    if (id < 0 && tm.chunks.find(key) == tm.chunks.end())
        return;

    TileChunk &chunk = tm.chunks[key];

    Tile &tile = chunk.tiles[static_cast<size_t>(localY * CHUNK_SIZE + localX)];
    tile.id = id;
    tile.flags = id < 0 ? 0 : flags;
}

inline Tile* GetTile(TilemapComponent &tm, int tileX, int tileY)
{
    int chunkX = floor_div(tileX, CHUNK_SIZE);
    int chunkY = floor_div(tileY, CHUNK_SIZE);

    int localX = mod(tileX, CHUNK_SIZE);
    int localY = mod(tileY, CHUNK_SIZE);

    int64_t key = ChunkKey(chunkX, chunkY);

    auto it = tm.chunks.find(key);
    if (it == tm.chunks.end())
        return nullptr;

    return &it->second.tiles[static_cast<size_t>(localY * CHUNK_SIZE + localX)];
}

inline int GetTileId(TilemapComponent &tm, int tileX, int tileY)
{
    Tile *tile = GetTile(tm, tileX, tileY);
    return tile ? tile->id : -1;
}

inline void EraseTile(TilemapComponent &tm, int tileX, int tileY)
{
    SetTile(tm, tileX, tileY, -1);
}

void PruneEmptyChunks(TilemapComponent &tm);
int CountTiles(const TilemapComponent &tm);
bool GetUsedBounds(const TilemapComponent &tm, glm::ivec2 &min, glm::ivec2 &max);

glm::vec2 TilemapOrigin(const TransformComponent &transform);
glm::vec2 TilemapCellSize(const TilemapComponent &tm, const TransformComponent &transform);
glm::ivec2 WorldToTile(const TilemapComponent &tm, const TransformComponent &transform, const glm::vec2 &world);
glm::vec2 TileToWorld(const TilemapComponent &tm, const TransformComponent &transform, const glm::ivec2 &cell);

int FloodFill(TilemapComponent &tm, const glm::ivec2 &start, int id, uint8_t flags, const glm::ivec2 &boundsMin, const glm::ivec2 &boundsMax);

bool IsSolidAt(entt::registry &registry, const glm::vec2 &world);
bool OverlapsSolid(entt::registry &registry, const glm::vec2 &min, const glm::vec2 &max);
}

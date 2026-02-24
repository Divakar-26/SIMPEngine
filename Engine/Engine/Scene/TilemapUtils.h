#pragma once
#include "Component.h"

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
    return ((int64_t)cx << 32) | (uint32_t)cy;
}

inline void SetTile(TilemapComponent &tm, int tileX, int tileY, int id)
{
    int chunkX = floor_div(tileX, CHUNK_SIZE);
    int chunkY = floor_div(tileY, CHUNK_SIZE);

    int localX = mod(tileX, CHUNK_SIZE);
    int localY = mod(tileY, CHUNK_SIZE);

    int64_t key = ChunkKey(chunkX, chunkY);

    TileChunk &chunk = tm.chunks[key]; // auto-create chunk

    chunk.tiles[localY * CHUNK_SIZE + localX].id = id;
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

    return &it->second.tiles[localY * CHUNK_SIZE + localX];
}

}
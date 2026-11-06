#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Math/EntityUtility.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace SIMPEngine
{
void PruneEmptyChunks(TilemapComponent &tm)
{
    for (auto it = tm.chunks.begin(); it != tm.chunks.end();)
    {
        bool empty = std::all_of(it->second.tiles.begin(), it->second.tiles.end(), [](const Tile &t)
                                 { return t.id < 0; });
        it = empty ? tm.chunks.erase(it) : std::next(it);
    }
}

int CountTiles(const TilemapComponent &tm)
{
    int count = 0;
    for (const auto &[key, chunk] : tm.chunks)
        for (const Tile &tile : chunk.tiles)
            if (tile.id >= 0)
                ++count;
    return count;
}

bool GetUsedBounds(const TilemapComponent &tm, glm::ivec2 &min, glm::ivec2 &max)
{
    bool any = false;
    for (const auto &[key, chunk] : tm.chunks)
    {
        glm::ivec2 base = ChunkCoord(key) * CHUNK_SIZE;
        for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i)
        {
            if (chunk.tiles[static_cast<size_t>(i)].id < 0)
                continue;
            glm::ivec2 cell = base + glm::ivec2(i % CHUNK_SIZE, i / CHUNK_SIZE);
            min = any ? glm::min(min, cell) : cell;
            max = any ? glm::max(max, cell) : cell;
            any = true;
        }
    }
    return any;
}

glm::vec2 TilemapOrigin(const TransformComponent &transform)
{
    return DecomposeWorld(transform).Position;
}

glm::vec2 TilemapCellSize(const TilemapComponent &tm, const TransformComponent &transform)
{
    glm::vec2 scale = glm::abs(DecomposeWorld(transform).Scale);
    return glm::max(glm::vec2(tm.tileSize) * scale, glm::vec2(0.0001f));
}

glm::ivec2 WorldToTile(const TilemapComponent &tm, const TransformComponent &transform, const glm::vec2 &world)
{
    glm::vec2 local = (world - TilemapOrigin(transform)) / TilemapCellSize(tm, transform);
    return {static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y))};
}

glm::vec2 TileToWorld(const TilemapComponent &tm, const TransformComponent &transform, const glm::ivec2 &cell)
{
    return TilemapOrigin(transform) + glm::vec2(cell) * TilemapCellSize(tm, transform);
}

int FloodFill(TilemapComponent &tm, const glm::ivec2 &start, int id, uint8_t flags, const glm::ivec2 &boundsMin, const glm::ivec2 &boundsMax)
{
    auto inBounds = [&](const glm::ivec2 &c)
    {
        return c.x >= boundsMin.x && c.y >= boundsMin.y && c.x <= boundsMax.x && c.y <= boundsMax.y;
    };

    if (!inBounds(start))
        return 0;

    int target = GetTileId(tm, start.x, start.y);
    Tile *startTile = GetTile(tm, start.x, start.y);
    uint8_t targetFlags = startTile ? startTile->flags : 0;
    if (target == id && (id < 0 || targetFlags == flags))
        return 0;

    auto matches = [&](const glm::ivec2 &c)
    {
        Tile *tile = GetTile(tm, c.x, c.y);
        int tileId = tile ? tile->id : -1;
        uint8_t tileFlags = tile ? tile->flags : 0;
        if (tileId != target)
            return false;
        return target < 0 || tileFlags == targetFlags;
    };

    std::vector<glm::ivec2> stack{start};
    int filled = 0;
    const int limit = 65536;

    while (!stack.empty() && filled < limit)
    {
        glm::ivec2 c = stack.back();
        stack.pop_back();
        if (!inBounds(c) || !matches(c))
            continue;

        SetTile(tm, c.x, c.y, id, flags);
        ++filled;

        stack.push_back({c.x + 1, c.y});
        stack.push_back({c.x - 1, c.y});
        stack.push_back({c.x, c.y + 1});
        stack.push_back({c.x, c.y - 1});
    }

    if (id < 0)
        PruneEmptyChunks(tm);
    return filled;
}

bool IsSolidAt(entt::registry &registry, const glm::vec2 &world)
{
    auto view = registry.view<TilemapComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto &tm = view.get<TilemapComponent>(entity);
        if (!tm.solid)
            continue;
        glm::ivec2 cell = WorldToTile(tm, view.get<TransformComponent>(entity), world);
        if (GetTileId(tm, cell.x, cell.y) >= 0)
            return true;
    }
    return false;
}

bool OverlapsSolid(entt::registry &registry, const glm::vec2 &min, const glm::vec2 &max)
{
    auto view = registry.view<TilemapComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto &tm = view.get<TilemapComponent>(entity);
        if (!tm.solid)
            continue;

        auto &transform = view.get<TransformComponent>(entity);
        glm::ivec2 a = WorldToTile(tm, transform, min);
        glm::ivec2 b = WorldToTile(tm, transform, max - glm::vec2(0.001f));
        for (int y = a.y; y <= b.y; ++y)
            for (int x = a.x; x <= b.x; ++x)
                if (GetTileId(tm, x, y) >= 0)
                    return true;
    }
    return false;
}
}

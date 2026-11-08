#include "LuaComponents.h"

#include <Engine/Rendering/TextureManager.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Scripting/LuaHelpers.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *TilemapMeta = "SIMP.Tilemap";

        int CheckCell(lua_State *L, int index)
        {
            return static_cast<int>(std::floor(luaL_checknumber(L, index)));
        }

        TransformComponent *TilemapTransform(lua_State *L, Entity *entity)
        {
            if (!entity->HasComponent<TransformComponent>())
                luaL_error(L, "tilemap entity has no transform");
            return &entity->GetComponent<TransformComponent>();
        }

        int TilemapGetTile(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            const int x = CheckCell(L, 2);
            const int y = CheckCell(L, 3);
            const Tile *tile = GetTile(*tilemap, x, y);
            const int id = tile ? tile->id : -1;
            const uint8_t flags = tile && tile->id >= 0 ? tile->flags : 0;
            lua_pushinteger(L, id);
            lua_pushboolean(L, (flags & TileFlipX) != 0);
            lua_pushboolean(L, (flags & TileFlipY) != 0);
            return 3;
        }

        int TilemapSetTile(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            const int x = CheckCell(L, 2);
            const int y = CheckCell(L, 3);
            const int id = static_cast<int>(luaL_checkinteger(L, 4));
            uint8_t flags = 0;
            if (lua_toboolean(L, 5))
                flags |= TileFlipX;
            if (lua_toboolean(L, 6))
                flags |= TileFlipY;
            SetTile(*tilemap, x, y, id, flags);
            return 0;
        }

        int TilemapEraseTile(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            EraseTile(*tilemap, CheckCell(L, 2), CheckCell(L, 3));
            return 0;
        }

        int TilemapFillRect(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            const int x1 = CheckCell(L, 2);
            const int y1 = CheckCell(L, 3);
            const int x2 = CheckCell(L, 4);
            const int y2 = CheckCell(L, 5);
            const int id = static_cast<int>(luaL_checkinteger(L, 6));
            const int minX = std::min(x1, x2), maxX = std::max(x1, x2);
            const int minY = std::min(y1, y2), maxY = std::max(y1, y2);
            if (static_cast<long long>(maxX - minX + 1) * (maxY - minY + 1) > 1000000)
                return luaL_error(L, "FillRect area is larger than one million tiles");
            for (int y = minY; y <= maxY; ++y)
                for (int x = minX; x <= maxX; ++x)
                {
                    if (id < 0)
                        EraseTile(*tilemap, x, y);
                    else
                        SetTile(*tilemap, x, y, id);
                }
            if (id < 0)
                PruneEmptyChunks(*tilemap);
            return 0;
        }

        int TilemapFloodFill(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            const glm::ivec2 start(CheckCell(L, 2), CheckCell(L, 3));
            const int id = static_cast<int>(luaL_checkinteger(L, 4));
            glm::ivec2 min = start, max = start;
            if (GetUsedBounds(*tilemap, min, max))
            {
                min = glm::min(min, start);
                max = glm::max(max, start);
            }
            lua_pushinteger(L, FloodFill(*tilemap, start, id, 0, min, max));
            return 1;
        }

        int TilemapClear(lua_State *L)
        {
            CheckComponent<TilemapComponent>(L, TilemapMeta)->chunks.clear();
            return 0;
        }

        int TilemapGetBounds(lua_State *L)
        {
            const TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            glm::ivec2 min(0), max(0);
            if (!GetUsedBounds(*tilemap, min, max))
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushinteger(L, min.x);
            lua_pushinteger(L, min.y);
            lua_pushinteger(L, max.x);
            lua_pushinteger(L, max.y);
            return 4;
        }

        int TilemapGetTileCount(lua_State *L)
        {
            lua_pushinteger(L, CountTiles(*CheckComponent<TilemapComponent>(L, TilemapMeta)));
            return 1;
        }

        int TilemapWorldToTile(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<TilemapComponent>(L, TilemapMeta);
            const TilemapComponent &tilemap = entity->GetComponent<TilemapComponent>();
            const TransformComponent *transform = TilemapTransform(L, entity);
            const glm::vec2 world(static_cast<float>(luaL_checknumber(L, 2)), static_cast<float>(luaL_checknumber(L, 3)));
            const glm::ivec2 cell = WorldToTile(tilemap, *transform, world);
            lua_pushinteger(L, cell.x);
            lua_pushinteger(L, cell.y);
            return 2;
        }

        int TilemapTileToWorld(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<TilemapComponent>(L, TilemapMeta);
            const TilemapComponent &tilemap = entity->GetComponent<TilemapComponent>();
            const TransformComponent *transform = TilemapTransform(L, entity);
            const glm::ivec2 cell(CheckCell(L, 2), CheckCell(L, 3));
            const glm::vec2 world = TileToWorld(tilemap, *transform, cell) + TilemapCellSize(tilemap, *transform) * 0.5f;
            lua_pushnumber(L, world.x);
            lua_pushnumber(L, world.y);
            return 2;
        }

        int TilemapGetTileAt(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<TilemapComponent>(L, TilemapMeta);
            TilemapComponent &tilemap = entity->GetComponent<TilemapComponent>();
            const TransformComponent *transform = TilemapTransform(L, entity);
            const glm::vec2 world(static_cast<float>(luaL_checknumber(L, 2)), static_cast<float>(luaL_checknumber(L, 3)));
            const glm::ivec2 cell = WorldToTile(tilemap, *transform, world);
            lua_pushinteger(L, GetTileId(tilemap, cell.x, cell.y));
            return 1;
        }

        int TilemapGetCellSize(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<TilemapComponent>(L, TilemapMeta);
            const TilemapComponent &tilemap = entity->GetComponent<TilemapComponent>();
            const glm::vec2 size = TilemapCellSize(tilemap, *TilemapTransform(L, entity));
            lua_pushnumber(L, size.x);
            lua_pushnumber(L, size.y);
            return 2;
        }

        int TilemapGetTileSize(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<TilemapComponent>(L, TilemapMeta)->tileSize);
            return 1;
        }

        int TilemapSetTileSize(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            tilemap->tileSize = std::max(1.0f, static_cast<float>(luaL_checknumber(L, 2)));
            return 0;
        }

        int TilemapIsSolid(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TilemapComponent>(L, TilemapMeta)->solid);
            return 1;
        }

        int TilemapSetSolid(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            tilemap->solid = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int TilemapIsVisible(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TilemapComponent>(L, TilemapMeta)->visible);
            return 1;
        }

        int TilemapSetVisible(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            tilemap->visible = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int TilemapGetTint(lua_State *L)
        {
            return PushRgba(L, CheckComponent<TilemapComponent>(L, TilemapMeta)->tint);
        }

        int TilemapSetTint(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            ReadRgba(L, 2, tilemap->tint);
            return 0;
        }

        int TilemapSetTileset(lua_State *L)
        {
            TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            size_t length = 0;
            const char *path = luaL_checklstring(L, 2, &length);
            const int tileWidth = static_cast<int>(luaL_checkinteger(L, 3));
            const int tileHeight = static_cast<int>(luaL_checkinteger(L, 4));
            const int margin = static_cast<int>(luaL_optinteger(L, 5, 0));
            const int spacing = static_cast<int>(luaL_optinteger(L, 6, 0));
            if (tileWidth <= 0 || tileHeight <= 0)
                return luaL_error(L, "tile width and height must be positive");

            bool loaded = false;
            {
                const std::string file(path, length);
                auto texture = TextureManager::Get().LoadTexture(file, file);
                if (texture)
                {
                    tilemap->tileset = std::make_shared<Tileset>(texture, tileWidth, tileHeight, margin, spacing);
                    loaded = true;
                }
            }
            lua_pushboolean(L, loaded);
            return 1;
        }

        int TilemapGetTilesetSize(lua_State *L)
        {
            const TilemapComponent *tilemap = CheckComponent<TilemapComponent>(L, TilemapMeta);
            if (!tilemap->tileset)
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushinteger(L, tilemap->tileset->GetColumns());
            lua_pushinteger(L, tilemap->tileset->GetRows());
            return 2;
        }

        const luaL_Reg TilemapMethods[] = {
            {"GetTile", TilemapGetTile},
            {"SetTile", TilemapSetTile},
            {"EraseTile", TilemapEraseTile},
            {"FillRect", TilemapFillRect},
            {"FloodFill", TilemapFloodFill},
            {"Clear", TilemapClear},
            {"GetBounds", TilemapGetBounds},
            {"GetTileCount", TilemapGetTileCount},
            {"WorldToTile", TilemapWorldToTile},
            {"TileToWorld", TilemapTileToWorld},
            {"GetTileAt", TilemapGetTileAt},
            {"GetCellSize", TilemapGetCellSize},
            {"GetTileSize", TilemapGetTileSize},
            {"SetTileSize", TilemapSetTileSize},
            {"IsSolid", TilemapIsSolid},
            {"SetSolid", TilemapSetSolid},
            {"IsVisible", TilemapIsVisible},
            {"SetVisible", TilemapSetVisible},
            {"GetTint", TilemapGetTint},
            {"SetTint", TilemapSetTint},
            {"SetTileset", TilemapSetTileset},
            {"GetTilesetSize", TilemapGetTilesetSize},
            {nullptr, nullptr}};
    }

    const ComponentBinding &TilemapBinding()
    {
        static const ComponentBinding binding{"Tilemap", TilemapMeta, TilemapMethods, &HasOf<TilemapComponent>, &AddOf<TilemapComponent>, &RemoveOf<TilemapComponent>};
        return binding;
    }
}

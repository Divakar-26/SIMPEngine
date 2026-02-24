#include "TilemapSystem.h"
#include <Engine/Scene/Component.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Math/Camera2D.h>

namespace SIMPEngine
{
    
    void TilemapSystem::Render(entt::registry &registry)
    {
        auto view = registry.view<TilemapComponent, TransformComponent>();

        for (auto entity : view)
        {
            auto &tilemap = view.get<TilemapComponent>(entity);
            auto &transform = view.get<TransformComponent>(entity);

            if (!tilemap.tileset)
                continue;

            auto texture = tilemap.tileset->GetTexture();
            if (!texture)
                continue;

            for (auto &[key, chunk] : tilemap.chunks)
            {
                int chunkX = (int)(key >> 32);
                int chunkY = (int)(key & 0xffffffff);

                for (int localY = 0; localY < CHUNK_SIZE; localY++)
                {
                    for (int localX = 0; localX < CHUNK_SIZE; localX++)
                    {
                        Tile &tile = chunk.tiles[localY * CHUNK_SIZE + localX];

                        if (tile.id < 0)
                            continue;

                        int tileX = chunkX * CHUNK_SIZE + localX;
                        int tileY = chunkY * CHUNK_SIZE + localY;

                        float worldX = transform.position.x + tileX * tilemap.tileSize;
                        float worldY = transform.position.y + tileY * tilemap.tileSize;

                        float centerX = worldX + tilemap.tileSize * 0.5f;
                        float centerY = worldY + tilemap.tileSize * 0.5f;

                        SDL_FRect src = tilemap.tileset->GetTileRect(tile.id);

                        Renderer::DrawTexture(
                            texture,
                            centerX,
                            centerY,
                            tilemap.tileSize,
                            tilemap.tileSize,
                            {255, 255, 255, 255},
                            0.0f,
                            transform.zIndex,
                            &src);
                    }
                }
            }
        }
    }
} // namespace SIMPEngine

#include "TilemapSystem.h"
#include <Engine/Scene/Component.h>
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
            {
                continue;
            }

            auto texture = tilemap.tileset->GetTexture();

            for (int y = 0; y < tilemap.height; y++)
            {
                for (int x = 0; x < tilemap.width; x++)
                {
                    int tileID = tilemap.tiles[y * tilemap.width + x];

                    if (tileID < 0)
                        continue;

                    float worldX = transform.position.x + x * tilemap.tileSize;
                    float worldY = transform.position.y + y * tilemap.tileSize;
                    float centerX = worldX + tilemap.tileSize * 0.5f;
                    float centerY = worldY + tilemap.tileSize * 0.5f;

                    SDL_FRect src = tilemap.tileset->GetTileRect(tileID);

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
} // namespace SIMPEngine

#include "TilemapSystem.h"
#include <Engine/Scene/Component.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Math/Camera2D.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    void TilemapSystem::VisibleWorldBounds(glm::vec2 &min, glm::vec2 &max)
    {
        glm::mat4 inv = glm::inverse(Renderer::GetAPI()->GetProjectionMatrix() * Renderer::GetViewMatrix());
        const glm::vec2 ndc[4] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
        for (int i = 0; i < 4; ++i)
        {
            glm::vec4 w = inv * glm::vec4(ndc[i], 0.0f, 1.0f);
            glm::vec2 p(w.x, w.y);
            min = i == 0 ? p : glm::min(min, p);
            max = i == 0 ? p : glm::max(max, p);
        }
    }

    void TilemapSystem::DrawTilemap(const TilemapComponent &tilemap, const TransformComponent &transform, const glm::vec2 &viewMin, const glm::vec2 &viewMax)
    {
        if (!tilemap.visible || !tilemap.tileset)
            return;

        auto texture = tilemap.tileset->GetTexture();
        if (!texture)
            return;

        glm::vec2 origin = TilemapOrigin(transform);
        glm::vec2 cell = TilemapCellSize(tilemap, transform);
        glm::ivec2 first(static_cast<int>(std::floor((viewMin.x - origin.x) / cell.x)) - 1,
                         static_cast<int>(std::floor((viewMin.y - origin.y) / cell.y)) - 1);
        glm::ivec2 last(static_cast<int>(std::ceil((viewMax.x - origin.x) / cell.x)) + 1,
                        static_cast<int>(std::ceil((viewMax.y - origin.y) / cell.y)) + 1);

        SpriteDesc desc;
        desc.TextureRef = texture;
        desc.Size = cell;
        desc.Z = transform.zIndex;
        desc.Tint = tilemap.tint;

        for (const auto &[key, chunk] : tilemap.chunks)
        {
            glm::ivec2 base = ChunkCoord(key) * CHUNK_SIZE;
            glm::ivec2 lo = glm::max(first - base, glm::ivec2(0));
            glm::ivec2 hi = glm::min(last - base, glm::ivec2(CHUNK_SIZE - 1));
            if (lo.x > hi.x || lo.y > hi.y)
                continue;

            for (int localY = lo.y; localY <= hi.y; localY++)
            {
                for (int localX = lo.x; localX <= hi.x; localX++)
                {
                    const Tile &tile = chunk.tiles[static_cast<size_t>(localY * CHUNK_SIZE + localX)];
                    if (tile.id < 0 || !tilemap.tileset->IsValid(tile.id))
                        continue;

                    SDL_FRect src = tilemap.tileset->GetTileRect(tile.id);
                    src.x += 0.01f;
                    src.y += 0.01f;
                    src.w -= 0.02f;
                    src.h -= 0.02f;

                    desc.Position = origin + (glm::vec2(base.x + localX, base.y + localY) + glm::vec2(0.5f)) * cell;
                    desc.SourceRect = &src;
                    desc.FlipX = (tile.flags & TileFlipX) != 0;
                    desc.FlipY = (tile.flags & TileFlipY) != 0;
                    Renderer::DrawSprite(desc);
                }
            }
        }
    }

    void TilemapSystem::Render(entt::registry &registry)
    {
        auto view = registry.view<TilemapComponent, TransformComponent>(entt::exclude<ShaderGroupMemberTag>);
        if (view.begin() == view.end())
            return;

        glm::vec2 viewMin, viewMax;
        VisibleWorldBounds(viewMin, viewMax);
        for (auto entity : view)
            DrawTilemap(view.get<TilemapComponent>(entity), view.get<TransformComponent>(entity), viewMin, viewMax);
    }
}

#pragma once

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Scene/TilemapUtils.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace Platformer
{
    constexpr float TileSize = 16.0f;
    constexpr float KillY = -48.0f;

    struct GameState
    {
        int Coins = 0;
        int TotalCoins = 0;
        int Hearts = 3;
        float Time = 0.0f;
        bool Won = false;
        bool GameOver = false;
        bool RestartRequested = false;
        float LevelWidth = 0.0f;
        float LevelHeight = 0.0f;
        glm::vec2 Spawn{0.0f};
    };

    inline GameState &State()
    {
        static GameState s_State;
        return s_State;
    }

    struct Box
    {
        glm::vec2 Min{0.0f};
        glm::vec2 Max{0.0f};

        bool Overlaps(const Box &o) const
        {
            return Min.x < o.Max.x && Max.x > o.Min.x && Min.y < o.Max.y && Max.y > o.Min.y;
        }
    };

    inline Box MakeBox(const glm::vec2 &center, const glm::vec2 &half)
    {
        return {center - half, center + half};
    }

    inline SDL_FRect Frame(int index, int columns, float width, float height)
    {
        return {(float)(index % columns) * width, (float)(index / columns) * height, width, height};
    }

    inline int TileCoord(float v)
    {
        return (int)std::floor(v / TileSize);
    }

    inline TilemapComponent *FindTilemap(SIMPEngine::Scene &scene, const char *name)
    {
        SIMPEngine::Entity e = scene.GetEntityByName(name);
        if (!e || !e.HasComponent<TilemapComponent>())
            return nullptr;
        return &e.GetComponent<TilemapComponent>();
    }

    inline bool HasTile(TilemapComponent *map, int tx, int ty)
    {
        if (!map)
            return false;
        Tile *tile = SIMPEngine::GetTile(*map, tx, ty);
        return tile && tile->id >= 0;
    }

    inline float Approach(float value, float target, float step)
    {
        if (value < target)
            return std::min(value + step, target);
        return std::max(value - step, target);
    }

    struct Body
    {
        glm::vec2 Half{5.0f, 8.0f};
        glm::vec2 Offset{0.0f};
        glm::vec2 Velocity{0.0f};
        bool OnGround = false;
        bool HitWall = false;
        bool DropThrough = false;

        Box BoxAt(const glm::vec2 &position) const
        {
            return MakeBox(position + Offset, Half);
        }

        float Bottom(const glm::vec2 &position) const
        {
            return position.y + Offset.y - Half.y;
        }
    };

    inline bool GroundBelow(SIMPEngine::Scene &scene, float x, float feetY)
    {
        int tx = TileCoord(x);
        int ty = TileCoord(feetY - 1.0f);
        return HasTile(FindTilemap(scene, "Terrain"), tx, ty) || HasTile(FindTilemap(scene, "Bridges"), tx, ty);
    }

    inline void MoveBody(SIMPEngine::Scene &scene, glm::vec2 &position, Body &body, float dt)
    {
        TilemapComponent *solid = FindTilemap(scene, "Terrain");
        TilemapComponent *oneWay = FindTilemap(scene, "Bridges");
        const float eps = 0.01f;

        body.HitWall = false;
        position.x += body.Velocity.x * dt;
        Box box = body.BoxAt(position);
        int y0 = TileCoord(box.Min.y + eps);
        int y1 = TileCoord(box.Max.y - eps);

        if (body.Velocity.x > 0.0f)
        {
            int tx = TileCoord(box.Max.x - eps);
            for (int ty = y0; ty <= y1; ++ty)
            {
                if (HasTile(solid, tx, ty))
                {
                    position.x = tx * TileSize - body.Half.x - body.Offset.x;
                    body.Velocity.x = 0.0f;
                    body.HitWall = true;
                    break;
                }
            }
        }
        else if (body.Velocity.x < 0.0f)
        {
            int tx = TileCoord(box.Min.x + eps);
            for (int ty = y0; ty <= y1; ++ty)
            {
                if (HasTile(solid, tx, ty))
                {
                    position.x = (tx + 1) * TileSize + body.Half.x - body.Offset.x;
                    body.Velocity.x = 0.0f;
                    body.HitWall = true;
                    break;
                }
            }
        }

        float previousBottom = body.Bottom(position);
        position.y += body.Velocity.y * dt;
        box = body.BoxAt(position);
        int x0 = TileCoord(box.Min.x + eps);
        int x1 = TileCoord(box.Max.x - eps);
        body.OnGround = false;

        if (body.Velocity.y <= 0.0f)
        {
            int ty = TileCoord(box.Min.y + eps);
            float top = (ty + 1) * TileSize;
            for (int tx = x0; tx <= x1; ++tx)
            {
                bool bridge = !body.DropThrough && HasTile(oneWay, tx, ty) && previousBottom >= top - 0.5f;
                if (HasTile(solid, tx, ty) || bridge)
                {
                    position.y = top + body.Half.y - body.Offset.y;
                    body.Velocity.y = 0.0f;
                    body.OnGround = true;
                    break;
                }
            }
        }
        else
        {
            int ty = TileCoord(box.Max.y - eps);
            for (int tx = x0; tx <= x1; ++tx)
            {
                if (HasTile(solid, tx, ty))
                {
                    position.y = ty * TileSize - body.Half.y - body.Offset.y;
                    body.Velocity.y = 0.0f;
                    break;
                }
            }
        }
    }

    inline void MarkForRemoval(SIMPEngine::Entity entity, float delay)
    {
        auto &registry = entity.GetScene()->GetRegistry();
        registry.emplace_or_replace<LifetimeComponent>(entity.GetHandle(), LifetimeComponent{delay});
    }
}

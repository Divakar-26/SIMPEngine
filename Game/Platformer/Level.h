#pragma once

#include "CameraAndHud.h"
#include "Common.h"
#include "Pickups.h"
#include "PlayerScript.h"

#include <Engine/Core/TimeStep.h>
#include <Engine/Layer/Layer.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Rendering/Tileset.h>
#include <Engine/Scene/SceneManager.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace Platformer
{
    inline const std::vector<std::string> &LevelMap()
    {
        static const std::vector<std::string> s_Map = {
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "........................................c.c.c............................................t.m.s...G",
            "........................................?.?.?...........c.............ccc.............c#############",
            ".......................................................===..........................c###############",
            ".......................................b...s...t...c.........c.........#...........#################",
            ".....................................############.===.......===..f.s...#...n.s..b.##################",
            ".........c.c.c..........c.c.c......##############...............####################################",
            ".m.P..t........b.s.f............n.###############...............####################################",
            "######################========###################...............####################################",
            "######################........###################...............####################################",
            "######################........###################...............####################################",
            "######################........###################...............####################################",
        };
        return s_Map;
    }

    inline std::shared_ptr<SIMPEngine::Texture> LoadSprite(const char *name)
    {
        std::string path = std::string("assets://platformer/sprites/") + name;
        return SIMPEngine::TextureManager::Get().LoadTexture(path, path);
    }

    inline SIMPEngine::Entity MakeSprite(SIMPEngine::Scene &scene, const char *name, const glm::vec2 &position, float z,
                                         const std::shared_ptr<SIMPEngine::Texture> &texture, float frameW, float frameH)
    {
        SIMPEngine::Entity entity = scene.CreateEntity(name);
        auto &tc = entity.GetComponent<TransformComponent>();
        tc.position = position;
        tc.zIndex = z;

        auto &sprite = entity.AddComponent<SpriteComponent>();
        sprite.texture = texture;
        sprite.width = frameW;
        sprite.height = frameH;
        sprite.useRegion = true;
        sprite.region = {0.0f, 0.0f, frameW, frameH};
        return entity;
    }

    inline void BuildLevel(SIMPEngine::Scene &scene)
    {
        using namespace SIMPEngine;

        const auto &map = LevelMap();
        const int height = (int)map.size();
        int width = 0;
        for (const auto &row : map)
            width = std::max(width, (int)row.size());

        auto cell = [&](int c, int r) -> char
        {
            if (r < 0 || r >= height || c < 0 || c >= (int)map[r].size())
                return '.';
            return map[r][c];
        };

        GameState &state = State();
        state = GameState{};
        state.LevelWidth = width * TileSize;
        state.LevelHeight = height * TileSize;

        scene.SetName("Knight Run");
        scene.SetBackgroundColor({0.46f, 0.71f, 0.89f, 1.0f});

        auto tileTexture = LoadSprite("world_tileset.png");
        auto knight = LoadSprite("knight.png");
        auto slimeGreen = LoadSprite("slime_green.png");
        auto slimePurple = LoadSprite("slime_purple.png");
        auto coin = LoadSprite("coin.png");
        if (!tileTexture || !knight || !slimeGreen || !slimePurple || !coin)
            return;
        auto tileset = std::make_shared<Tileset>(tileTexture, 16, 16);

        const char *layers[] = {"Decor", "Terrain", "Bridges"};
        const float layerZ[] = {-1.0f, 0.0f, 0.0f};
        entt::entity layerHandles[3];
        for (int i = 0; i < 3; ++i)
        {
            Entity layer = scene.CreateEntity(layers[i]);
            layer.GetComponent<TransformComponent>().zIndex = layerZ[i];
            auto &tm = layer.AddComponent<TilemapComponent>();
            tm.tileset = tileset;
            tm.tileSize = TileSize;
            layerHandles[i] = layer.GetHandle();
        }

        auto &registry = scene.GetRegistry();
        auto setTile = [&](int layer, int tx, int ty, int id)
        {
            SetTile(registry.get<TilemapComponent>(layerHandles[layer]), tx, ty, id);
        };

        std::vector<std::pair<char, glm::vec2>> spawns;
        for (int r = 0; r < height; ++r)
        {
            for (int c = 0; c < (int)map[r].size(); ++c)
            {
                char ch = map[r][c];
                int tx = c;
                int ty = height - 1 - r;
                glm::vec2 cellCenter(tx * TileSize + TileSize * 0.5f, ty * TileSize + TileSize * 0.5f);
                glm::vec2 feet(cellCenter.x, ty * TileSize);

                switch (ch)
                {
                case '#':
                    setTile(1, tx, ty, cell(c, r - 1) == '#' ? 16 : 0);
                    break;
                case '?':
                    setTile(1, tx, ty, 32);
                    break;
                case '=':
                    setTile(2, tx, ty, cell(c - 1, r) != '=' ? 9 : (cell(c + 1, r) != '=' ? 11 : 10));
                    break;
                case 't':
                    setTile(0, tx, ty, 80);
                    setTile(0, tx, ty + 1, 64);
                    setTile(0, tx, ty + 2, 48);
                    break;
                case 'b':
                    setTile(0, tx, ty, 65);
                    break;
                case 'f':
                    setTile(0, tx, ty, 97);
                    break;
                case 'm':
                    setTile(0, tx, ty, 87);
                    break;
                case 'n':
                    setTile(0, tx, ty, 88);
                    break;
                case 'G':
                    setTile(0, tx, ty, 72);
                    setTile(0, tx, ty + 1, 56);
                    spawns.emplace_back('G', glm::vec2(cellCenter.x, ty * TileSize + TileSize));
                    break;
                case 'P':
                    state.Spawn = feet + glm::vec2(0.0f, 12.0f);
                    break;
                case 'c':
                    spawns.emplace_back('c', cellCenter);
                    break;
                case 's':
                    spawns.emplace_back('s', feet + glm::vec2(0.0f, 12.0f));
                    break;
                default:
                    break;
                }
            }
        }

        int slimeIndex = 0;
        for (const auto &[kind, position] : spawns)
        {
            if (kind == 'c')
            {
                Entity e = MakeSprite(scene, "Coin", position, 1.0f, coin, 16.0f, 16.0f);
                registry.emplace<ScriptComponent>(e.GetHandle()).Bind<CoinScript>();
                state.TotalCoins++;
            }
            else if (kind == 's')
            {
                auto &texture = (slimeIndex++ % 2 == 0) ? slimeGreen : slimePurple;
                Entity e = MakeSprite(scene, "Slime", position, 2.0f, texture, 24.0f, 24.0f);
                registry.emplace<ScriptComponent>(e.GetHandle()).Bind<SlimeScript>();
            }
            else if (kind == 'G')
            {
                Entity goal = scene.CreateEntity("Goal");
                goal.GetComponent<TransformComponent>().position = position;
                auto &col = goal.AddComponent<CollisionComponent>();
                col.width = 12.0f;
                col.height = 32.0f;
            }
        }

        Entity player = MakeSprite(scene, "Player", state.Spawn, 3.0f, knight, 32.0f, 32.0f);
        registry.emplace<ScriptComponent>(player.GetHandle()).Bind<PlayerScript>();

        Entity camera = scene.CreateEntity("Camera");
        camera.GetComponent<TransformComponent>().position = state.Spawn;
        auto &cam = camera.AddComponent<CameraComponent>(0.2f, state.Spawn);
        cam.primary = true;
        cam.Camera.SetManualControl(false);
        registry.emplace<ScriptComponent>(camera.GetHandle()).Bind<CameraFollowScript>();

        Entity hud = scene.CreateEntity("HUD");
        registry.emplace<ScriptComponent>(hud.GetHandle()).Bind<HudScript>();

        scene.UpdateTransforms();
    }

    class PlatformerLayer : public SIMPEngine::Layer
    {
    public:
        explicit PlatformerLayer(std::shared_ptr<SIMPEngine::SceneManager> sceneManager)
            : Layer("PlatformerLayer"), m_SceneManager(std::move(sceneManager))
        {
        }

        void OnUpdate(SIMPEngine::TimeStep ts) override
        {
            (void)ts;
            if (!State().RestartRequested)
                return;

            State().RestartRequested = false;
            if (auto scene = m_SceneManager->GetActiveScene())
            {
                scene->Clear();
                BuildLevel(*scene);
            }
        }

    private:
        std::shared_ptr<SIMPEngine::SceneManager> m_SceneManager;
    };
}

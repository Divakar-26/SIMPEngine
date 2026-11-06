#pragma once

#include "Common.h"
#include "Pickups.h"

#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>

namespace Platformer
{
    class PlayerScript : public SIMPEngine::ScriptableEntity
    {
    protected:
        void OnCreate() override
        {
            m_Body.Half = {5.0f, 8.0f};
            m_Body.Offset = {0.0f, -4.0f};
            m_Invulnerable = 1.0f;

            GameState &state = State();
            SIMPEngine::Scene &scene = *GetEntity().GetScene();
            state.Coins = 0;
            state.Hearts = 3;
            state.Time = 0.0f;
            state.Won = false;
            state.GameOver = false;
            state.Spawn = GetComponent<TransformComponent>().position;
            state.TotalCoins = 0;
            for (auto handle : scene.GetRegistry().view<TagComponent>())
                if (scene.GetRegistry().get<TagComponent>(handle).Tag == "Coin")
                    state.TotalCoins++;

            if (TilemapComponent *terrain = FindTilemap(scene, "Terrain"))
            {
                int maxX = 0;
                for (const auto &[key, chunk] : terrain->chunks)
                {
                    int chunkX = (int)(key >> 32);
                    for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i)
                        if (chunk.tiles[i].id >= 0)
                            maxX = std::max(maxX, chunkX * CHUNK_SIZE + i % CHUNK_SIZE + 1);
                }
                state.LevelWidth = maxX * TileSize;
            }
        }

        void OnUpdate(float dt) override
        {
            using namespace SIMPEngine;

            dt = std::min(dt, 1.0f / 30.0f);
            GameState &state = State();
            auto &tc = GetComponent<TransformComponent>();
            Scene &scene = *GetEntity().GetScene();
            glm::vec2 position = tc.position;

            if (Input::IsKeyJustPressed(SIMPK_R))
                state.RestartRequested = true;

            m_Time += dt;
            m_Invulnerable = std::max(0.0f, m_Invulnerable - dt);
            m_HurtTimer = std::max(0.0f, m_HurtTimer - dt);
            if (!m_Dead && !state.Won)
                state.Time += dt;

            bool control = !m_Dead && !state.Won && m_HurtTimer <= 0.0f;
            float direction = 0.0f;
            if (control && (Input::IsKeyPressed(SIMPK_A) || Input::IsKeyPressed(SIMPK_LEFT)))
                direction -= 1.0f;
            if (control && (Input::IsKeyPressed(SIMPK_D) || Input::IsKeyPressed(SIMPK_RIGHT)))
                direction += 1.0f;
            if (direction != 0.0f)
                m_Facing = direction;

            float accel = m_HurtTimer > 0.0f ? 250.0f : (m_Body.OnGround ? 1400.0f : 900.0f);
            m_Body.Velocity.x = Approach(m_Body.Velocity.x, direction * RunSpeed, accel * dt);

            bool jumpPressed = Input::IsKeyJustPressed(SIMPK_SPACE) || Input::IsKeyJustPressed(SIMPK_W) || Input::IsKeyJustPressed(SIMPK_UP);
            bool jumpHeld = Input::IsKeyPressed(SIMPK_SPACE) || Input::IsKeyPressed(SIMPK_W) || Input::IsKeyPressed(SIMPK_UP);

            m_Coyote = m_Body.OnGround ? 0.1f : m_Coyote - dt;
            m_JumpBuffer = (control && jumpPressed) ? 0.12f : m_JumpBuffer - dt;
            if (m_JumpBuffer > 0.0f && m_Coyote > 0.0f)
            {
                m_Body.Velocity.y = JumpSpeed;
                m_JumpBuffer = 0.0f;
                m_Coyote = 0.0f;
            }

            float gravity = Gravity * ((m_Body.Velocity.y > 0.0f && jumpHeld) ? 1.0f : 1.8f);
            m_Body.Velocity.y = std::max(m_Body.Velocity.y - gravity * dt, -MaxFall);
            m_Body.DropThrough = control && (Input::IsKeyPressed(SIMPK_S) || Input::IsKeyPressed(SIMPK_DOWN));

            float previousBottom = m_Body.Bottom(position);
            MoveBody(scene, position, m_Body, dt);

            if (!m_Dead && position.y < KillY)
            {
                Hurt(position.x, true);
                if (!m_Dead)
                {
                    position = state.Spawn;
                    m_Body.Velocity = {0.0f, 0.0f};
                }
            }

            if (!m_Dead && !state.Won)
                Interact(scene, position, previousBottom);

            if (m_Dead && m_Time - m_DeathTime > 1.2f)
                state.GameOver = true;

            tc.position = position;
            Animate();
        }

    private:
        static constexpr float RunSpeed = 95.0f;
        static constexpr float JumpSpeed = 320.0f;
        static constexpr float Gravity = 900.0f;
        static constexpr float MaxFall = 380.0f;

        void Hurt(float fromX, bool fell)
        {
            GameState &state = State();
            if (!fell && m_Invulnerable > 0.0f)
                return;

            state.Hearts = std::max(0, state.Hearts - 1);
            m_Invulnerable = 1.2f;

            if (state.Hearts == 0)
            {
                m_Dead = true;
                m_DeathTime = m_Time;
                m_Body.Velocity = {0.0f, fell ? 0.0f : 160.0f};
                return;
            }

            if (!fell)
            {
                float away = GetComponent<TransformComponent>().position.x < fromX ? -1.0f : 1.0f;
                m_Body.Velocity = {away * 140.0f, 160.0f};
                m_HurtTimer = 0.35f;
            }
        }

        void Interact(SIMPEngine::Scene &scene, glm::vec2 &position, float previousBottom)
        {
            GameState &state = State();
            Box box = m_Body.BoxAt(position);
            auto &registry = scene.GetRegistry();

            for (auto handle : registry.view<ScriptComponent>())
            {
                auto *instance = registry.get<ScriptComponent>(handle).Instance.get();

                if (auto *coin = dynamic_cast<CoinScript *>(instance))
                {
                    if (!coin->IsCollected() && coin->GetBox().Overlaps(box))
                    {
                        coin->Collect();
                        state.Coins++;
                    }
                }
                else if (auto *slime = dynamic_cast<SlimeScript *>(instance))
                {
                    if (!slime->IsAlive())
                        continue;
                    Box enemy = slime->GetBox();
                    if (!enemy.Overlaps(box))
                        continue;

                    float enemyMid = (enemy.Min.y + enemy.Max.y) * 0.5f;
                    if (m_Body.Velocity.y < 0.0f && previousBottom >= enemyMid)
                    {
                        slime->Kill();
                        m_Body.Velocity.y = 220.0f;
                    }
                    else
                    {
                        Hurt((enemy.Min.x + enemy.Max.x) * 0.5f, false);
                    }
                }
            }

            SIMPEngine::Entity goal = scene.GetEntityByName("Goal");
            if (goal && goal.HasComponent<TransformComponent>())
            {
                Box goalBox = MakeBox(goal.GetComponent<TransformComponent>().position, {6.0f, 16.0f});
                if (goalBox.Overlaps(box))
                {
                    state.Won = true;
                    m_Body.Velocity.x = 0.0f;
                }
            }
        }

        void Animate()
        {
            auto &sprite = GetComponent<SpriteComponent>();
            int frame = 0;

            if (m_Dead)
                frame = 56 + std::min((int)((m_Time - m_DeathTime) * 8.0f), 3);
            else if (m_HurtTimer > 0.0f)
                frame = 48 + (int)(m_Time * 12.0f) % 4;
            else if (!m_Body.OnGround)
                frame = m_Body.Velocity.y > 0.0f ? 18 : 22;
            else if (std::abs(m_Body.Velocity.x) > 5.0f)
                frame = 16 + (int)(m_Time * 18.0f) % 16;
            else
                frame = (int)(m_Time * 7.0f) % 4;

            sprite.region = Frame(frame, 8, 32.0f, 32.0f);
            sprite.flipX = m_Facing < 0.0f;
            bool blink = m_Invulnerable > 0.0f && !m_Dead && (int)(m_Time * 20.0f) % 2 == 0;
            sprite.tint.a = blink ? 90 : 255;
        }

        Body m_Body;
        float m_Facing = 1.0f;
        float m_Time = 0.0f;
        float m_Coyote = 0.0f;
        float m_JumpBuffer = 0.0f;
        float m_Invulnerable = 0.0f;
        float m_HurtTimer = 0.0f;
        float m_DeathTime = 0.0f;
        bool m_Dead = false;
    };
}

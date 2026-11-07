#pragma once

#include "Common.h"

#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Particles/ParticlePresets.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Rendering/Renderer.h>

#include <SDL3/SDL_timer.h>

#include <cstdio>
#include <sstream>

namespace Mossveil
{
    inline bool AnyPressed(std::initializer_list<int> keys)
    {
        for (int key : keys)
            if (SIMPEngine::Input::IsKeyPressed(key))
                return true;
        return false;
    }

    inline bool AnyJustPressed(std::initializer_list<int> keys)
    {
        for (int key : keys)
            if (SIMPEngine::Input::IsKeyJustPressed(key))
                return true;
        return false;
    }

    inline void SpawnBurst(SIMPEngine::Scene &scene, const char *preset, glm::vec2 at, glm::vec4 start, glm::vec4 end, float scale = 1.0f, float z = 4.9f)
    {
        const auto *found = SIMPEngine::ParticlePresets::Find(preset);
        if (!found)
            return;
        auto entity = scene.CreateEntity("Burst");
        auto &transform = entity.GetComponent<TransformComponent>();
        transform.position = at;
        transform.zIndex = z;
        transform.scale = glm::vec2(scale);
        auto &emitter = entity.AddComponent<ParticleEmitterComponent>();
        emitter.effect = found->Effect;
        emitter.effect.Looping = false;
        emitter.effect.Prewarm = false;
        emitter.effect.StartColor = start;
        emitter.effect.MidColor = glm::mix(start, end, 0.5f);
        emitter.effect.EndColor = end;
        emitter.effect.Emissive = emitter.effect.Blend == SIMPEngine::BlendMode::Additive;
        emitter.state.Rng = static_cast<uint32_t>(SDL_GetTicks() * 2654435761u) | 1u;
        entity.AddComponent<LifetimeComponent>().remaining = 2.5f;
    }

    inline void ShakeCamera(SIMPEngine::Scene &scene, float amount)
    {
        auto camera = scene.GetEntityByName("Camera");
        if (camera && camera.HasComponent<CameraComponent>())
            camera.GetComponent<CameraComponent>().Shake(amount);
    }

    class CameraScript : public SIMPEngine::ScriptableEntity
    {
    protected:
        void OnCreate() override
        {
            auto &cc = GetComponent<CameraComponent>();
            cc.Camera.SetManualControl(false);
            cc.Camera.SetPosition(GetComponent<TransformComponent>().position);
        }
    };

    class PlayerScript : public SIMPEngine::ScriptableEntity
    {
    protected:
        void OnCreate() override
        {
            using namespace SIMPEngine;
            Scene &scene = *GetEntity().GetScene();
            auto &registry = scene.GetRegistry();
            for (auto entity : registry.view<TagComponent, TransformComponent, CollisionComponent>())
            {
                const std::string &tag = registry.get<TagComponent>(entity).Tag;
                Box box = ColliderBox(registry.get<TransformComponent>(entity), registry.get<CollisionComponent>(entity));
                if (tag == "Solid")
                    m_Solids.push_back(box);
                else if (tag == "OneWay")
                    m_OneWay.push_back(box);
                else if (tag == "Hazard")
                    m_Hazards.push_back(box);
            }

            auto trail = scene.GetEntityByName("Player Trail");
            m_Trail = trail ? trail.GetHandle() : entt::null;
            State().Checkpoint = GetComponent<TransformComponent>().position;
            State().PlayerDead = false;

            m_Sheets[0] = Tex(SheetPath(Sheets::Idle));
            m_Sheets[1] = Tex(SheetPath(Sheets::Walk));
            m_Sheets[2] = Tex(SheetPath(Sheets::Jump));
            m_Sheets[3] = Tex(SheetPath(Sheets::Dash));
        }

        void OnUpdate(float dt) override
        {
            using namespace SIMPEngine;
            dt = std::min(dt, 1.0f / 30.0f);
            GameState &state = State();
            Scene &scene = *GetEntity().GetScene();
            auto &transform = GetComponent<TransformComponent>();
            glm::vec2 position = transform.position;
            m_AnimTime += dt;

            if (m_Dead)
            {
                m_DeadTimer += dt;
                state.Fade = std::min(1.0f, m_DeadTimer / 0.35f);
                if (m_DeadTimer > 0.6f)
                {
                    position = state.Checkpoint;
                    m_Velocity = glm::vec2(0.0f);
                    m_Dead = false;
                    m_DashTime = 0.0f;
                    state.PlayerDead = false;
                    GetComponent<SpriteComponent>().tint.a = 255;
                }
                transform.position = position;
                transform.dirty = true;
                return;
            }

            bool control = !state.Exiting;
            float direction = 0.0f;
            if (control && AnyPressed({SIMPK_A, SIMPK_LEFT}))
                direction -= 1.0f;
            if (control && AnyPressed({SIMPK_D, SIMPK_RIGHT}))
                direction += 1.0f;
            if (direction != 0.0f && m_DashTime <= 0.0f)
                m_Facing = direction;

            bool jumpPressed = control && AnyJustPressed({SIMPK_SPACE, SIMPK_W, SIMPK_UP, SIMPK_Z});
            bool jumpHeld = control && AnyPressed({SIMPK_SPACE, SIMPK_W, SIMPK_UP, SIMPK_Z});
            bool dashPressed = control && AnyJustPressed({SIMPK_LSHIFT, SIMPK_RSHIFT, SIMPK_K, SIMPK_X});
            m_DropThrough = control && AnyPressed({SIMPK_S, SIMPK_DOWN});

            m_DashCooldown = std::max(0.0f, m_DashCooldown - dt);
            if (dashPressed && state.HasDash && m_DashReady && m_DashCooldown <= 0.0f)
            {
                m_DashTime = DashDuration;
                m_DashReady = false;
                m_DashCooldown = 0.38f;
                m_Velocity.y = 0.0f;
                ShakeCamera(scene, 0.18f);
            }

            if (m_DashTime > 0.0f)
            {
                m_DashTime -= dt;
                m_Velocity = {m_Facing * DashSpeed, 0.0f};
                if (m_DashTime <= 0.0f)
                    m_Velocity.x *= 0.4f;
            }
            else
            {
                float accel = m_OnGround ? 4200.0f : 2600.0f;
                m_Velocity.x = Approach(m_Velocity.x, direction * RunSpeed, accel * dt);

                m_Coyote = m_OnGround ? 0.1f : m_Coyote - dt;
                m_JumpBuffer = jumpPressed ? 0.13f : m_JumpBuffer - dt;
                if (m_JumpBuffer > 0.0f && m_Coyote > 0.0f)
                {
                    m_Velocity.y = JumpSpeed;
                    m_JumpBuffer = 0.0f;
                    m_Coyote = 0.0f;
                    m_OnGround = false;
                }

                float gravity = Gravity * ((m_Velocity.y > 0.0f && jumpHeld) ? 1.0f : 1.9f);
                m_Velocity.y = std::max(m_Velocity.y - gravity * dt, -MaxFall);
            }

            bool wasGrounded = m_OnGround;
            float fallSpeed = -m_Velocity.y;
            Move(position, dt);
            if (m_OnGround)
                m_DashReady = true;

            if (m_OnGround && !wasGrounded && fallSpeed > 650.0f)
            {
                glm::vec2 feet = position + glm::vec2(0.0f, Offset.y - Half.y + 6.0f);
                SpawnBurst(scene, "Dust Puff", feet, {0.55f, 0.65f, 0.6f, 0.45f}, {0.4f, 0.5f, 0.5f, 0.0f}, 1.4f, 5.1f);
                if (fallSpeed > 1350.0f)
                    ShakeCamera(scene, 0.3f);
            }

            if (m_Trail != entt::null && scene.GetRegistry().valid(m_Trail))
                scene.GetRegistry().get<ParticleEmitterComponent>(m_Trail).effect.Rate = m_DashTime > 0.0f ? 260.0f : 0.0f;

            Box body = BodyBox(position);
            Box hurt{body.Min + glm::vec2(6.0f, 4.0f), body.Max - glm::vec2(6.0f, 8.0f)};
            bool hit = position.y < state.BoundsMin.y + 120.0f;
            for (const Box &hazard : m_Hazards)
                hit = hit || hurt.Overlaps(hazard);
            if (hit)
                Die(scene, position);

            transform.position = position;
            transform.dirty = true;
            Animate(dt);
        }

    private:
        static constexpr float RunSpeed = 440.0f;
        static constexpr float JumpSpeed = 1180.0f;
        static constexpr float Gravity = 2900.0f;
        static constexpr float MaxFall = 1700.0f;
        static constexpr float DashSpeed = 1300.0f;
        static constexpr float DashDuration = 0.17f;
        static inline const glm::vec2 Half{22.0f, 46.0f};
        static inline const glm::vec2 Offset{0.0f, -6.0f};

        Box BodyBox(const glm::vec2 &position) const
        {
            glm::vec2 center = position + Offset;
            return {center - Half, center + Half};
        }

        void Move(glm::vec2 &position, float dt)
        {
            glm::vec2 delta = m_Velocity * dt;
            int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(delta.x), std::abs(delta.y)) / 18.0f)));
            glm::vec2 step = delta / static_cast<float>(steps);
            m_OnGround = false;

            for (int i = 0; i < steps; ++i)
            {
                position.x += step.x;
                Box box = BodyBox(position);
                for (const Box &solid : m_Solids)
                {
                    if (!box.Overlaps(solid))
                        continue;
                    if (step.x > 0.0f)
                        position.x = solid.Min.x - Half.x - Offset.x - 0.01f;
                    else if (step.x < 0.0f)
                        position.x = solid.Max.x + Half.x - Offset.x + 0.01f;
                    m_Velocity.x = 0.0f;
                    step.x = 0.0f;
                    box = BodyBox(position);
                }

                float previousBottom = BodyBox(position).Min.y;
                position.y += step.y;
                box = BodyBox(position);
                for (const Box &solid : m_Solids)
                {
                    if (!box.Overlaps(solid))
                        continue;
                    if (step.y <= 0.0f)
                    {
                        position.y = solid.Max.y + Half.y - Offset.y;
                        m_OnGround = true;
                    }
                    else
                    {
                        position.y = solid.Min.y - Half.y - Offset.y - 0.01f;
                    }
                    m_Velocity.y = 0.0f;
                    step.y = 0.0f;
                    box = BodyBox(position);
                }

                if (step.y <= 0.0f && !m_DropThrough)
                {
                    for (const Box &platform : m_OneWay)
                    {
                        if (box.Overlaps(platform) && previousBottom >= platform.Max.y - 2.0f)
                        {
                            position.y = platform.Max.y + Half.y - Offset.y;
                            m_Velocity.y = 0.0f;
                            step.y = 0.0f;
                            m_OnGround = true;
                            box = BodyBox(position);
                        }
                    }
                }
            }
        }

        void Die(SIMPEngine::Scene &scene, const glm::vec2 &position)
        {
            m_Dead = true;
            m_DeadTimer = 0.0f;
            m_Velocity = glm::vec2(0.0f);
            State().PlayerDead = true;
            GetComponent<SpriteComponent>().tint.a = 0;
            SpawnBurst(scene, "Explosion", position, {0.6f, 0.8f, 1.0f, 1.0f}, {0.2f, 0.3f, 0.8f, 0.0f}, 0.7f);
            ShakeCamera(scene, 0.55f);
        }

        void Animate(float dt)
        {
            const SheetInfo *sheet = &Sheets::Idle;
            int texture = 0;
            int frame = 0;
            if (m_DashTime > 0.0f)
            {
                sheet = &Sheets::Dash;
                texture = 3;
                frame = static_cast<int>((1.0f - m_DashTime / DashDuration) * (Sheets::Dash.Frames - 1));
            }
            else if (!m_OnGround)
            {
                sheet = &Sheets::Jump;
                texture = 2;
                if (m_Velocity.y > 500.0f)
                    frame = 1 + static_cast<int>(m_AnimTime * 12.0f) % 2;
                else if (m_Velocity.y > -250.0f)
                    frame = 3 + (m_Velocity.y < 0.0f ? 1 : 0);
                else
                    frame = 5 + static_cast<int>(m_AnimTime * 10.0f) % 3;
            }
            else if (std::abs(m_Velocity.x) > 40.0f)
            {
                sheet = &Sheets::Walk;
                texture = 1;
                m_WalkClock += dt * std::abs(m_Velocity.x) / RunSpeed;
                frame = static_cast<int>(m_WalkClock / 0.032f) % Sheets::Walk.Frames;
            }
            else
            {
                frame = static_cast<int>(m_AnimTime / 0.055f) % Sheets::Idle.Frames;
            }

            auto &sprite = GetComponent<SpriteComponent>();
            sprite.texture = m_Sheets[texture];
            sprite.useRegion = true;
            sprite.region = SheetFrame(*sheet, frame);
            sprite.flipX = m_Facing < 0.0f;
        }

        std::vector<Box> m_Solids;
        std::vector<Box> m_OneWay;
        std::vector<Box> m_Hazards;
        std::shared_ptr<SIMPEngine::Texture> m_Sheets[4];
        entt::entity m_Trail = entt::null;

        glm::vec2 m_Velocity{0.0f};
        float m_Facing = 1.0f;
        bool m_OnGround = false;
        bool m_DropThrough = false;
        float m_Coyote = 0.0f;
        float m_JumpBuffer = 0.0f;
        float m_DashTime = 0.0f;
        float m_DashCooldown = 0.0f;
        bool m_DashReady = true;
        bool m_Dead = false;
        float m_DeadTimer = 0.0f;
        float m_AnimTime = 0.0f;
        float m_WalkClock = 0.0f;
    };

    class DirectorScript : public SIMPEngine::ScriptableEntity
    {
    protected:
        void OnCreate() override
        {
            using namespace SIMPEngine;
            GameState &state = State();
            std::istringstream tag(GetComponent<TagComponent>().Tag);
            std::string word;
            int level = 1;
            uint32_t seed = 0;
            tag >> word >> level >> seed;
            state.Level = level;
            state.Seed = seed;
            state.HasDash = level >= 3;
            state.Wisps = 0;
            state.Fade = 1.0f;
            state.LevelTime = 0.0f;
            state.Exiting = false;
            state.PlayerDead = false;
            state.Message.clear();
            state.MessageTime = 0.0f;
            if (level == 1)
                state.RunWisps = 0;

            Scene &scene = *GetEntity().GetScene();
            auto &registry = scene.GetRegistry();
            for (auto entity : registry.view<TagComponent, TransformComponent>())
            {
                const std::string &name = registry.get<TagComponent>(entity).Tag;
                glm::vec2 at = registry.get<TransformComponent>(entity).position;
                if (name == "Wisp")
                    m_Wisps.push_back({entity, at, static_cast<float>(m_Wisps.size()) * 1.7f});
                else if (name == "Checkpoint")
                    m_Checkpoints.push_back({entity, at, false});
                else if (name == "Exit")
                    m_Exit = {entity, at};
                else if (name == "Dash Shrine")
                    m_Shrine = {entity, at};
                else if (name == "Player")
                    m_Player = entity;
                else if (name == "HUD Light")
                    m_HudLight = entity;
                else if (name == "Card Light")
                    m_CardLight = entity;
                else if (name == "Camera")
                    m_Camera = entity;
            }
            state.TotalWisps = static_cast<int>(m_Wisps.size());
            m_Font = Font::Load("assets://fonts/Inter-SemiBold.ttf");
        }

        void OnUpdate(float dt) override
        {
            using namespace SIMPEngine;
            GameState &state = State();
            Scene &scene = *GetEntity().GetScene();
            auto &registry = scene.GetRegistry();
            state.LevelTime += dt;
            state.MessageTime = std::max(0.0f, state.MessageTime - dt);

            if (Input::IsKeyJustPressed(SIMPK_1))
                Request(1, 0);
            else if (Input::IsKeyJustPressed(SIMPK_2))
                Request(2, 0);
            else if (Input::IsKeyJustPressed(SIMPK_3))
                Request(3, NewSeed());
            else if (Input::IsKeyJustPressed(SIMPK_R))
                Request(state.Level, state.Seed);
            else if (state.Level == 3 && Input::IsKeyJustPressed(SIMPK_N))
                Request(3, NewSeed());

            if (m_Ending)
            {
                state.Fade = 1.0f;
                if (Input::IsKeyJustPressed(SIMPK_RETURN) || Input::IsKeyJustPressed(SIMPK_SPACE))
                    Request(1, 0);
                UpdateLights(registry);
                return;
            }

            if (!state.PlayerDead && !state.Exiting)
                state.Fade = std::max(0.0f, state.Fade - dt * 1.3f);

            if (m_Player == entt::null || !registry.valid(m_Player))
                return;
            glm::vec2 player = registry.get<TransformComponent>(m_Player).position;

            for (auto &wisp : m_Wisps)
            {
                if (wisp.Collected || !registry.valid(wisp.Handle))
                    continue;
                auto &transform = registry.get<TransformComponent>(wisp.Handle);
                transform.position.y = wisp.Base.y + std::sin(state.LevelTime * 2.2f + wisp.Phase) * 12.0f;
                transform.position.x = wisp.Base.x + std::cos(state.LevelTime * 1.3f + wisp.Phase) * 5.0f;
                transform.dirty = true;
                if (glm::distance(player, transform.position) < 78.0f && !state.PlayerDead)
                {
                    wisp.Collected = true;
                    state.Wisps++;
                    state.RunWisps++;
                    SpawnBurst(scene, "Sparks", transform.position, {0.8f, 1.0f, 1.0f, 1.0f}, {0.3f, 0.8f, 1.0f, 0.0f}, 0.7f);
                    registry.emplace_or_replace<LifetimeComponent>(wisp.Handle, LifetimeComponent{0.0f});
                    if (state.Wisps == state.TotalWisps)
                        ShowMessage("Every wisp of this place answers you");
                }
            }

            for (auto &checkpoint : m_Checkpoints)
            {
                if (checkpoint.Active || !registry.valid(checkpoint.Handle))
                    continue;
                if (std::abs(player.x - checkpoint.At.x) < 90.0f && std::abs(player.y - checkpoint.At.y) < 220.0f)
                {
                    checkpoint.Active = true;
                    state.Checkpoint = {checkpoint.At.x, checkpoint.At.y - 20.0f};
                    registry.get<LightComponent>(checkpoint.Handle).intensity = 1.35f;
                    registry.get<GlowComponent>(checkpoint.Handle).intensity = 1.2f;
                    registry.get<SpriteComponent>(checkpoint.Handle).tint = {255, 255, 255, 255};
                    SpawnBurst(scene, "Heal", checkpoint.At + glm::vec2(0.0f, 40.0f), {0.6f, 0.85f, 1.0f, 1.0f}, {0.3f, 0.6f, 1.0f, 0.0f});
                    ShowMessage("A bluebloom wakes. You will return here");
                }
            }

            if (m_Shrine.Handle != entt::null && registry.valid(m_Shrine.Handle) && !state.HasDash &&
                glm::distance(player, m_Shrine.At) < 130.0f)
            {
                state.HasDash = true;
                SpawnBurst(scene, "Explosion", m_Shrine.At, {1.0f, 0.9f, 0.6f, 1.0f}, {0.4f, 1.0f, 0.8f, 0.0f}, 0.8f);
                registry.get<LightComponent>(m_Shrine.Handle).intensity = 0.5f;
                ShowMessage("The wind answers. Press SHIFT to dash");
                ShakeCamera(scene, 0.3f);
            }

            if (!state.Exiting && m_Exit.Handle != entt::null && glm::distance(player, m_Exit.At) < 130.0f)
            {
                state.Exiting = true;
                ShakeCamera(scene, 0.2f);
            }

            if (state.Exiting)
            {
                state.Fade = std::min(1.0f, state.Fade + dt * 1.1f);
                if (state.Fade >= 1.0f)
                {
                    if (state.Level < LevelCount)
                        Request(state.Level + 1, state.Level + 1 == 3 ? NewSeed() : 0);
                    else
                        m_Ending = true;
                }
            }

            UpdateLights(registry);
        }

        void OnRender() override
        {
            using namespace SIMPEngine;
            if (!m_Font)
                return;
            const GameState &state = State();
            glm::vec2 min, max;
            if (!View(min, max))
                return;
            glm::vec2 center = (min + max) * 0.5f;
            float h = max.y - min.y;
            float unit = h / 900.0f;

            Renderer::DrawQuad(center.x, center.y, (max.x - min.x) * 1.2f, h * 1.2f, 0.0f,
                               {0, 0, 0, static_cast<Uint8>(std::clamp(state.Fade, 0.0f, 1.0f) * 255.0f)}, true, 990.0f);

            if (m_Ending)
            {
                char line[96];
                Text("The light finds its way home.", center + glm::vec2(0.0f, 70.0f * unit), 46.0f * unit, {235, 240, 255, 255});
                std::snprintf(line, sizeof(line), "%d wisps gathered on your journey", state.RunWisps);
                Text(line, center + glm::vec2(0.0f, 0.0f), 26.0f * unit, {170, 220, 255, 255});
                Text("ENTER to walk the Mossway again    N for a new Shifting Deep", center - glm::vec2(0.0f, 60.0f * unit), 20.0f * unit, {200, 210, 230, 230});
                return;
            }

            char hud[96];
            std::snprintf(hud, sizeof(hud), "%d / %d", state.Wisps, state.TotalWisps);
            glm::vec2 corner(min.x + 40.0f * unit, max.y - 50.0f * unit);
            Renderer::DrawCircle(corner.x + 10.0f * unit, corner.y + 12.0f * unit, 9.0f * unit, {180, 240, 255, 255}, 1.0f, 995.0f);
            Text(hud, corner + glm::vec2(34.0f * unit, 0.0f), 28.0f * unit, {225, 240, 255, 255}, TextAlign::Left);
            std::snprintf(hud, sizeof(hud), "%s  %s", LevelNumeral(state.Level), LevelTitle(state.Level));
            Text(hud, corner - glm::vec2(0.0f, 34.0f * unit), 18.0f * unit, {170, 200, 210, 220}, TextAlign::Left);
            if (state.HasDash)
                Text("dash ready", corner - glm::vec2(0.0f, 60.0f * unit), 16.0f * unit, {255, 220, 160, 200}, TextAlign::Left);

            float card = CardAlpha();
            if (card > 0.01f)
            {
                Uint8 a = static_cast<Uint8>(card * 255.0f);
                glm::vec2 at = center + glm::vec2(0.0f, h * 0.22f);
                Text(LevelNumeral(state.Level), at + glm::vec2(0.0f, 70.0f * unit), 40.0f * unit, {170, 220, 255, a});
                Text(LevelTitle(state.Level), at, 58.0f * unit, {240, 245, 255, a});
                if (state.Level == 3)
                {
                    std::snprintf(hud, sizeof(hud), "carved anew from seed %u    N reshapes it", state.Seed);
                    Text(hud, at - glm::vec2(0.0f, 52.0f * unit), 20.0f * unit, {180, 210, 230, a});
                }
                else if (state.Level == 1)
                {
                    Text("A D or arrows to walk    SPACE to jump", at - glm::vec2(0.0f, 52.0f * unit), 20.0f * unit, {180, 210, 230, a});
                }
            }

            if (state.MessageTime > 0.0f)
            {
                float alpha = std::min(1.0f, state.MessageTime / 0.6f) * std::min(1.0f, (4.0f - state.MessageTime) / 0.4f);
                Text(state.Message.c_str(), {center.x, min.y + 90.0f * unit}, 26.0f * unit, {235, 240, 255, static_cast<Uint8>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)});
            }
        }

    private:
        struct WispInfo
        {
            entt::entity Handle;
            glm::vec2 Base;
            float Phase;
            bool Collected = false;
        };

        struct CheckpointInfo
        {
            entt::entity Handle;
            glm::vec2 At;
            bool Active;
        };

        struct Marker
        {
            entt::entity Handle = entt::null;
            glm::vec2 At{0.0f};
        };

        static uint32_t NewSeed()
        {
            uint32_t x = static_cast<uint32_t>(SDL_GetTicksNS() & 0xFFFFFFFFu);
            x ^= x >> 16;
            x *= 0x7feb352du;
            x ^= x >> 15;
            return (x % 89999u) + 10000u;
        }

        static void Request(int level, uint32_t seed)
        {
            State().PendingLevel = level;
            State().PendingSeed = seed;
        }

        static void ShowMessage(const char *text)
        {
            State().Message = text;
            State().MessageTime = 4.0f;
        }

        float CardAlpha() const
        {
            float t = State().LevelTime;
            float in = std::clamp((t - 0.5f) / 0.9f, 0.0f, 1.0f);
            float out = std::clamp((4.6f - t) / 1.0f, 0.0f, 1.0f);
            return in * out;
        }

        bool View(glm::vec2 &min, glm::vec2 &max)
        {
            auto &registry = GetEntity().GetScene()->GetRegistry();
            if (m_Camera == entt::null || !registry.valid(m_Camera))
                return false;
            float left, right, top, bottom;
            registry.get<CameraComponent>(m_Camera).Camera.GetVisibleWorldBounds(left, right, top, bottom);
            min = {std::min(left, right), std::min(top, bottom)};
            max = {std::max(left, right), std::max(top, bottom)};
            return max.x > min.x && max.y > min.y;
        }

        void UpdateLights(entt::registry &registry)
        {
            glm::vec2 min, max;
            if (!View(min, max))
                return;
            float unit = (max.y - min.y) / 900.0f;
            const GameState &state = State();

            if (m_HudLight != entt::null && registry.valid(m_HudLight))
            {
                auto &transform = registry.get<TransformComponent>(m_HudLight);
                transform.position = {min.x + 115.0f * unit, max.y - 62.0f * unit};
                transform.dirty = true;
                auto &light = registry.get<LightComponent>(m_HudLight);
                light.radius = 220.0f * unit;
                light.falloff = 1.5f;
                light.intensity = m_Ending ? 0.0f : 3.2f;
            }
            if (m_CardLight != entt::null && registry.valid(m_CardLight))
            {
                glm::vec2 center = (min + max) * 0.5f;
                bool message = state.MessageTime > 0.0f;
                float card = CardAlpha();
                auto &transform = registry.get<TransformComponent>(m_CardLight);
                transform.position = m_Ending ? center : (card > 0.01f ? center + glm::vec2(0.0f, (max.y - min.y) * 0.22f) : glm::vec2(center.x, min.y + 90.0f * unit));
                transform.dirty = true;
                auto &light = registry.get<LightComponent>(m_CardLight);
                light.radius = (m_Ending ? 900.0f : (card > 0.01f ? 620.0f : 480.0f)) * unit;
                light.falloff = 1.2f;
                light.intensity = m_Ending ? 3.0f : (card > 0.01f ? card * 3.2f : (message ? 2.8f : 0.0f));
            }
        }

        void Text(const char *text, glm::vec2 at, float size, SDL_Color color, SIMPEngine::TextAlign align = SIMPEngine::TextAlign::Center)
        {
            SIMPEngine::Renderer::DrawText(text, at.x + size * 0.06f, at.y - size * 0.06f, size, {0, 0, 0, static_cast<Uint8>(color.a * 0.6f)}, align, 0.0f, 994.0f, m_Font.get());
            SIMPEngine::Renderer::DrawText(text, at.x, at.y, size, color, align, 0.0f, 995.0f, m_Font.get());
        }

        std::vector<WispInfo> m_Wisps;
        std::vector<CheckpointInfo> m_Checkpoints;
        Marker m_Exit;
        Marker m_Shrine;
        entt::entity m_Player = entt::null;
        entt::entity m_HudLight = entt::null;
        entt::entity m_CardLight = entt::null;
        entt::entity m_Camera = entt::null;
        std::shared_ptr<SIMPEngine::Font> m_Font;
        bool m_Ending = false;
    };
}

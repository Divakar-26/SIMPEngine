#include "PhysicsSystem.h"
#include "CharacterControllerSystem.h"

#include <Engine/Core/Profiler/EngineProfiler.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/TilemapUtils.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        float MoveTowards(float current, float target, float maxDelta)
        {
            if (std::abs(target - current) <= maxDelta)
                return target;
            return current + (target > current ? maxDelta : -maxDelta);
        }

        bool AnyPressed(std::initializer_list<int> keys)
        {
            for (int key : keys)
                if (Input::IsKeyPressed(key))
                    return true;
            return false;
        }

        bool AnyJustPressed(std::initializer_list<int> keys)
        {
            for (int key : keys)
                if (Input::IsKeyJustPressed(key))
                    return true;
            return false;
        }

        bool AnyJustReleased(std::initializer_list<int> keys)
        {
            for (int key : keys)
                if (Input::IsKeyJustReleased(key))
                    return true;
            return false;
        }

        struct PendingEvent
        {
            entt::entity Entity;
            ScriptEvent Type;
            float Value;
        };
    }

    void CharacterControllerSystem::Update(Scene &scene, float dt)
    {
        PROFILE_SCOPE("CharacterControllerSystem::Update");
        entt::registry &registry = scene.GetRegistry();
        m_Order.clear();
        for (auto entity : registry.view<CharacterControllerComponent, TransformComponent>())
            m_Order.push_back(entity);
        if (m_Order.empty())
            return;
        dt = std::min(dt, 1.0f / 20.0f);

        m_Solids.clear();
        for (auto entity : registry.view<CollisionComponent, TransformComponent>())
        {
            if (registry.any_of<CharacterControllerComponent, TriggerComponent>(entity))
                continue;
            const auto &collision = registry.get<CollisionComponent>(entity);
            const WorldTransform2D world = DecomposeWorld(registry.get<TransformComponent>(entity));
            Solid solid;
            solid.Min = world.Position + glm::vec2(collision.offsetX, collision.offsetY);
            solid.Max = solid.Min + glm::vec2(collision.width, collision.height);
            if (const auto *tag = registry.try_get<TagComponent>(entity))
                solid.Tag = &tag->Tag;
            m_Solids.push_back(solid);
        }

        std::vector<PendingEvent> events;
        for (entt::entity entity : m_Order)
        {
            auto &c = registry.get<CharacterControllerComponent>(entity);
            auto &transform = registry.get<TransformComponent>(entity);
            const glm::vec2 half = glm::abs(c.size) * 0.5f;
            const bool platformer = c.mode == ControllerMode::Platformer;

            auto isOneWay = [&](const Solid &s)
            { return platformer && !c.oneWayTag.empty() && s.Tag && *s.Tag == c.oneWayTag; };

            auto blocked = [&](const glm::vec2 &position, bool vertical, bool movingDown, float previousBottom)
            {
                const glm::vec2 min = position + c.offset - half + glm::vec2(0.01f);
                const glm::vec2 max = position + c.offset + half - glm::vec2(0.01f);
                for (const Solid &s : m_Solids)
                {
                    if (!(min.x < s.Max.x && max.x > s.Min.x && min.y < s.Max.y && max.y > s.Min.y))
                        continue;
                    const bool oneWay = isOneWay(s);
                    if (!oneWay && !c.solidTag.empty() && (!s.Tag || *s.Tag != c.solidTag))
                        continue;
                    if (oneWay)
                    {
                        if (vertical && movingDown && previousBottom >= s.Max.y - 0.5f)
                            return true;
                        continue;
                    }
                    return true;
                }
                return c.collideTilemaps && OverlapsSolid(registry, min, max);
            };

            auto moveAxis = [&](glm::vec2 &position, float delta, int axis)
            {
                if (delta == 0.0f)
                    return false;
                const float stepLength = std::max(1.0f, std::min({4.0f, half.x, half.y}));
                const int steps = std::max(1, static_cast<int>(std::ceil(std::abs(delta) / stepLength)));
                const float step = delta / static_cast<float>(steps);
                for (int i = 0; i < steps; ++i)
                {
                    glm::vec2 next = position;
                    next[axis] += step;
                    const float previousBottom = position.y + c.offset.y - half.y;
                    if (blocked(next, axis == 1, axis == 1 && step < 0.0f, previousBottom))
                    {
                        float lo = 0.0f, hi = 1.0f;
                        for (int k = 0; k < 6; ++k)
                        {
                            const float mid = (lo + hi) * 0.5f;
                            glm::vec2 probe = position;
                            probe[axis] += step * mid;
                            if (blocked(probe, axis == 1, axis == 1 && step < 0.0f, previousBottom))
                                hi = mid;
                            else
                                lo = mid;
                        }
                        position[axis] += step * lo;
                        return true;
                    }
                    position = next;
                }
                return false;
            };

            float ix = 0.0f, iy = 0.0f;
            bool jumpPressed = false, jumpReleased = false;
            if (c.playerInput)
            {
                if (InputActions::Has("MoveX"))
                {
                    ix = InputActions::GetAxis("MoveX");
                    iy = InputActions::GetAxis("MoveY");
                }
                else
                {
                    ix = (AnyPressed({SIMPK_D, SIMPK_RIGHT}) ? 1.0f : 0.0f) - (AnyPressed({SIMPK_A, SIMPK_LEFT}) ? 1.0f : 0.0f);
                    iy = (AnyPressed({SIMPK_W, SIMPK_UP}) ? 1.0f : 0.0f) - (AnyPressed({SIMPK_S, SIMPK_DOWN}) ? 1.0f : 0.0f);
                }
                if (platformer)
                {
                    if (InputActions::Has("Jump"))
                    {
                        jumpPressed = InputActions::IsJustPressed("Jump");
                        jumpReleased = InputActions::IsJustReleased("Jump");
                    }
                    else
                    {
                        jumpPressed = AnyJustPressed({SIMPK_SPACE, SIMPK_W, SIMPK_UP});
                        jumpReleased = AnyJustReleased({SIMPK_SPACE, SIMPK_W, SIMPK_UP});
                    }
                }
            }
            ix = std::clamp(ix + c.scriptInput.x, -1.0f, 1.0f);
            iy = std::clamp(iy + c.scriptInput.y, -1.0f, 1.0f);
            jumpPressed = jumpPressed || c.scriptJump;
            jumpReleased = jumpReleased || c.scriptRelease;
            c.scriptJump = false;
            c.scriptRelease = false;

            c.justLanded = c.justJumped = c.hitWall = c.hitCeiling = false;
            const bool wasGrounded = c.grounded;
            glm::vec2 position = transform.position;

            if (platformer)
            {
                float rate = ix != 0.0f ? c.acceleration : c.deceleration;
                if (!c.grounded)
                    rate *= c.airControl;
                c.velocity.x = MoveTowards(c.velocity.x, ix * c.moveSpeed, rate * dt);

                if (c.grounded)
                {
                    c.coyoteLeft = c.coyoteTime;
                    c.jumpsUsed = 0;
                }
                else
                    c.coyoteLeft -= dt;
                c.bufferLeft = jumpPressed ? c.jumpBuffer : c.bufferLeft - dt;

                if (c.bufferLeft > 0.0f)
                {
                    bool jump = false;
                    if (c.coyoteLeft > 0.0f && c.jumpsUsed == 0)
                    {
                        c.jumpsUsed = 1;
                        jump = true;
                    }
                    else if (c.maxJumps > 1 && c.jumpsUsed < c.maxJumps)
                    {
                        c.jumpsUsed = std::max(c.jumpsUsed, 1) + 1;
                        jump = true;
                    }
                    if (jump)
                    {
                        c.velocity.y = c.jumpSpeed;
                        c.bufferLeft = 0.0f;
                        c.coyoteLeft = 0.0f;
                        c.grounded = false;
                        c.justJumped = true;
                        events.push_back({entity, ScriptEvent::Jumped, 0.0f});
                    }
                }
                if (jumpReleased && c.velocity.y > 0.0f)
                    c.velocity.y *= c.jumpCut;
                c.velocity.y = std::max(c.velocity.y - c.gravity * dt, -c.maxFallSpeed);

                if (moveAxis(position, c.velocity.x * dt, 0))
                {
                    c.velocity.x = 0.0f;
                    c.hitWall = true;
                }
                const float fallSpeed = -c.velocity.y;
                if (moveAxis(position, c.velocity.y * dt, 1))
                {
                    if (c.velocity.y < 0.0f)
                        c.grounded = true;
                    else
                        c.hitCeiling = true;
                    c.velocity.y = 0.0f;
                }
                else if (c.velocity.y != 0.0f)
                    c.grounded = false;

                if (c.grounded && !wasGrounded)
                {
                    c.justLanded = true;
                    events.push_back({entity, ScriptEvent::Landed, std::max(fallSpeed, 0.0f)});
                }
            }
            else
            {
                glm::vec2 input(ix, iy);
                if (glm::length(input) > 1.0f)
                    input = glm::normalize(input);
                const glm::vec2 target = input * c.moveSpeed;
                const float rate = (input.x != 0.0f || input.y != 0.0f ? c.acceleration : c.deceleration) * dt;
                const glm::vec2 diff = target - c.velocity;
                const float distance = glm::length(diff);
                c.velocity = distance <= rate ? target : c.velocity + diff / distance * rate;

                if (moveAxis(position, c.velocity.x * dt, 0))
                {
                    c.velocity.x = 0.0f;
                    c.hitWall = true;
                }
                if (moveAxis(position, c.velocity.y * dt, 1))
                {
                    c.velocity.y = 0.0f;
                    c.hitWall = true;
                }
                c.grounded = false;
            }

            if (position != transform.position)
            {
                transform.position = position;
                transform.dirty = true;
                PhysicsSystem::SyncBodyToTransform(registry, entity);
            }

            if (c.velocity.x > 5.0f)
                c.facing = 1;
            else if (c.velocity.x < -5.0f)
                c.facing = -1;
            if (c.flipSprite)
                if (auto *sprite = registry.try_get<SpriteComponent>(entity))
                    sprite->flipX = c.facing < 0;
        }

        for (const PendingEvent &e : events)
            if (registry.valid(e.Entity))
                scene.SendEvent(e.Entity, e.Type, entt::null, e.Value);
    }
}

#pragma once

#include "Common.h"

namespace Platformer
{
    class CoinScript : public SIMPEngine::ScriptableEntity
    {
    public:
        bool IsCollected() const { return m_Collected; }

        Box GetBox()
        {
            return MakeBox(GetComponent<TransformComponent>().position, {5.0f, 5.0f});
        }

        void Collect()
        {
            if (m_Collected)
                return;
            m_Collected = true;
            GetComponent<SpriteComponent>().tint.a = 0;
            MarkForRemoval(GetEntity(), 0.0f);
        }

    protected:
        void OnCreate() override
        {
            m_Time = GetComponent<TransformComponent>().position.x * 0.013f;
        }

        void OnUpdate(float dt) override
        {
            m_Time += dt;
            auto &sprite = GetComponent<SpriteComponent>();
            sprite.region = Frame((int)(m_Time * 10.0f) % 12, 12, 16.0f, 16.0f);
        }

    private:
        bool m_Collected = false;
        float m_Time = 0.0f;
    };

    class SlimeScript : public SIMPEngine::ScriptableEntity
    {
    public:
        bool IsAlive() const { return !m_Dead; }

        Box GetBox()
        {
            return m_Body.BoxAt(GetComponent<TransformComponent>().position);
        }

        void Kill()
        {
            if (m_Dead)
                return;
            m_Dead = true;
            m_Time = 0.0f;
            MarkForRemoval(GetEntity(), 0.4f);
        }

    protected:
        void OnCreate() override
        {
            m_Body.Half = {6.0f, 5.0f};
            m_Body.Offset = {0.0f, -7.0f};
        }

        void OnUpdate(float dt) override
        {
            dt = std::min(dt, 1.0f / 30.0f);
            m_Time += dt;
            auto &sprite = GetComponent<SpriteComponent>();

            if (m_Dead)
            {
                sprite.region = Frame(8 + std::min((int)(m_Time * 10.0f), 3), 4, 24.0f, 24.0f);
                return;
            }

            auto &tc = GetComponent<TransformComponent>();
            SIMPEngine::Scene &scene = *GetEntity().GetScene();
            glm::vec2 position = tc.position;

            m_Body.Velocity.x = m_Direction * 28.0f;
            m_Body.Velocity.y = std::max(m_Body.Velocity.y - 900.0f * dt, -380.0f);
            MoveBody(scene, position, m_Body, dt);

            float frontX = position.x + m_Direction * (m_Body.Half.x + 1.0f);
            if (m_Body.HitWall || (m_Body.OnGround && !GroundBelow(scene, frontX, m_Body.Bottom(position))))
                m_Direction = -m_Direction;

            tc.position = position;
            sprite.region = Frame((int)(m_Time * 8.0f) % 8, 4, 24.0f, 24.0f);
            sprite.flipX = m_Direction > 0.0f;
        }

    private:
        Body m_Body;
        float m_Direction = -1.0f;
        float m_Time = 0.0f;
        bool m_Dead = false;
    };
}

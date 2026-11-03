#pragma once
#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/PCH.h>
#include <vector>
#include <random>
#include <cmath>
#include <array>

class ConstellationScript : public SIMPEngine::ScriptableEntity
{
public:
    struct Dot
    {
        float x, y;
        float vx, vy;
        SDL_Color color;
        float radius;
    };

    static constexpr float CONNECT_DIST = 180.0f;
    static constexpr float DOT_SPEED    = 300.0f;
    static constexpr float DOT_RADIUS   = 8.0f;
    static constexpr float LINE_WIDTH   = 0.8f;
    static constexpr float AA           = 0.5f;
    static constexpr float WORLD_W      = 960.0f;
    static constexpr float WORLD_H      = 540.0f;
    static constexpr int   SPAWN_COUNT  = 5;

    std::vector<Dot> m_Dots;
    std::mt19937     m_Rng{42};

    std::array<SDL_Color, 6> m_Palette = {{
        {100, 220, 255, 255},
        { 80, 200, 120, 255},
        {200, 100, 255, 255},
        {255, 200,  60, 255},
        {255,  80,  80, 255},
        {255, 255, 255, 255},
    }};
    static constexpr int PALETTE_SIZE = 6;

    Dot MakeDot(float x, float y)
    {
        std::uniform_real_distribution<float> rv(-1.0f, 1.0f);
        std::uniform_int_distribution<int>    rc(0, PALETTE_SIZE - 1);
        Dot d;
        d.x      = x;
        d.y      = y;
        d.vx     = rv(m_Rng) * DOT_SPEED;
        d.vy     = rv(m_Rng) * DOT_SPEED;
        d.radius = rv(m_Rng) * DOT_RADIUS + 3;
        d.color  = m_Palette[rc(m_Rng)];
        return d;
    }

    void OnCreate() override
    {
        std::uniform_real_distribution<float> rx(-WORLD_W, WORLD_W);
        std::uniform_real_distribution<float> ry(-WORLD_H, WORLD_H);

        for (int i = 0; i < 80; i++)
            m_Dots.push_back(MakeDot(rx(m_Rng), ry(m_Rng)));
    }

    void OnUpdate(float dt) override
    {
        for (auto& d : m_Dots)
        {
            d.x += d.vx * dt;
            d.y += d.vy * dt;

            if (d.x < -WORLD_W) { d.x = -WORLD_W; d.vx *= -1; }
            if (d.x >  WORLD_W) { d.x =  WORLD_W; d.vx *= -1; }
            if (d.y < -WORLD_H) { d.y = -WORLD_H; d.vy *= -1; }
            if (d.y >  WORLD_H) { d.y =  WORLD_H; d.vy *= -1; }
        }

        if (SIMPEngine::Input::IsMouseButtonJustPressed(SIMPEngine::SIMP_Keys::SIMPK_MOUSE_LEFT) && IsMouseInViewport())
        {
            glm::vec2 world = GetMouseWorldPosition();

            std::uniform_real_distribution<float> scatter(-30.0f, 30.0f);
            for (int i = 0; i < SPAWN_COUNT; i++)
                m_Dots.push_back(MakeDot(
                    world.x + scatter(m_Rng),
                    world.y + scatter(m_Rng)));
        }
    }

    void OnRender() override
    {
        int count = (int)m_Dots.size();

        for (auto& d : m_Dots)
        {
            SIMPEngine::Renderer::DrawCircle(
                d.x, d.y, d.radius, d.color, AA);
        }

        for (int i = 0; i < count; i++)
        {
            for (int j = i + 1; j < count; j++)
            {
                float dx   = m_Dots[j].x - m_Dots[i].x;
                float dy   = m_Dots[j].y - m_Dots[i].y;
                float dist = std::sqrt(dx*dx + dy*dy);

                if (dist < CONNECT_DIST)
                {
                    float t = 1.0f - (dist / CONNECT_DIST);
                    SDL_Color lc = {200, 200, 220, (Uint8)(t * 160.0f)};
                    SIMPEngine::Renderer::DrawCapsuleLine(
                        m_Dots[i].x, m_Dots[i].y,
                        m_Dots[j].x, m_Dots[j].y,
                        LINE_WIDTH, lc, AA);
                }
            }
        }
    }
};

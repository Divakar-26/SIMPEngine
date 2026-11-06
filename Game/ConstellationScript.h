#pragma once

#include <Engine/Scene/ScriptableEntity.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>
#include <Engine/Rendering/Renderer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

class ConstellationScript : public SIMPEngine::ScriptableEntity
{
public:
    struct Dot
    {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float radius = 4.0f;
        float pulse = 0.0f;
        float pulseSpeed = 1.0f;
        SDL_Color color{255, 255, 255, 255};
    };

    struct Star
    {
        float x = 0.0f;
        float y = 0.0f;
        float radius = 1.0f;
        float phase = 0.0f;
        float frequency = 1.0f;
    };

    static constexpr float WORLD_W = 960.0f;
    static constexpr float WORLD_H = 540.0f;
    static constexpr float CONNECT_DIST = 180.0f;
    static constexpr float CONNECT_DIST_SQ = CONNECT_DIST * CONNECT_DIST;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float TWO_PI = PI * 2.0f;

    static constexpr std::size_t INITIAL_DOTS = 96;
    static constexpr std::size_t MAX_DOTS = 220;
    static constexpr std::size_t SPAWN_COUNT = 8;
    static constexpr std::size_t STAR_COUNT = 110;

    // Each spatial cell is one connection radius wide. A dot only needs to
    // inspect its own cell and the eight adjacent cells when finding links.
    static constexpr int GRID_COLUMNS = 11;
    static constexpr int GRID_ROWS = 6;
    static constexpr float GRID_CELL_SIZE = CONNECT_DIST;

    static constexpr std::array<SDL_Color, 6> PALETTE = {{
        {104, 224, 255, 255},
        {106, 178, 255, 255},
        {169, 145, 255, 255},
        {111, 235, 201, 255},
        {255, 190, 116, 255},
        {224, 236, 255, 255}
    }};

    std::vector<Dot> m_Dots;
    std::array<Star, STAR_COUNT> m_Stars{};
    std::array<std::vector<std::size_t>, GRID_COLUMNS * GRID_ROWS> m_SpatialGrid{};
    std::mt19937 m_Rng{std::random_device{}()};

    float m_Time = 0.0f;
    std::size_t m_ConnectionCount = 0;
    bool m_Paused = false;
    bool m_HasMousePosition = false;
    glm::vec2 m_MouseWorld{0.0f};

    void OnCreate() override
    {
        m_Dots.clear();
        m_Dots.reserve(MAX_DOTS);

        for (auto& cell : m_SpatialGrid)
            cell.reserve(12);

        InitializeStars();
        SeedDots(INITIAL_DOTS);
    }

    void OnUpdate(float dt) override
    {
        const float safeDt = std::clamp(dt, 0.0f, 0.05f);
        m_Time += safeDt;

        HandleInput();
        UpdateStars(safeDt);

        if (!m_Paused)
            UpdateDots(safeDt);
    }

    void OnRender() override
    {
        RenderBackdrop();
        RenderConnections();
        RenderDots();
        RenderCursor();
        RenderOverlay();
    }

private:
    static constexpr float DOT_SPEED_MIN = 55.0f;
    static constexpr float DOT_SPEED_MAX = 145.0f;
    static constexpr float DOT_RADIUS_MIN = 2.8f;
    static constexpr float DOT_RADIUS_MAX = 5.8f;
    static constexpr float MOUSE_REMOVE_RADIUS = 58.0f;
    static constexpr float MOUSE_REMOVE_RADIUS_SQ = MOUSE_REMOVE_RADIUS * MOUSE_REMOVE_RADIUS;

    void InitializeStars()
    {
        std::uniform_real_distribution<float> xDist(-WORLD_W, WORLD_W);
        std::uniform_real_distribution<float> yDist(-WORLD_H, WORLD_H);
        std::uniform_real_distribution<float> radiusDist(0.55f, 1.45f);
        std::uniform_real_distribution<float> phaseDist(0.0f, TWO_PI);
        std::uniform_real_distribution<float> frequencyDist(0.35f, 1.4f);

        for (Star& star : m_Stars)
        {
            star.x = xDist(m_Rng);
            star.y = yDist(m_Rng);
            star.radius = radiusDist(m_Rng);
            star.phase = phaseDist(m_Rng);
            star.frequency = frequencyDist(m_Rng);
        }
    }

    Dot MakeDot(float x, float y)
    {
        std::uniform_real_distribution<float> angleDist(0.0f, TWO_PI);
        std::uniform_real_distribution<float> speedDist(DOT_SPEED_MIN, DOT_SPEED_MAX);
        std::uniform_real_distribution<float> radiusDist(DOT_RADIUS_MIN, DOT_RADIUS_MAX);
        std::uniform_real_distribution<float> phaseDist(0.0f, TWO_PI);
        std::uniform_real_distribution<float> pulseSpeedDist(0.8f, 2.1f);
        std::uniform_int_distribution<std::size_t> colorDist(0, PALETTE.size() - 1);

        const float angle = angleDist(m_Rng);
        const float speed = speedDist(m_Rng);

        Dot dot;
        dot.x = std::clamp(x, -WORLD_W + DOT_RADIUS_MAX, WORLD_W - DOT_RADIUS_MAX);
        dot.y = std::clamp(y, -WORLD_H + DOT_RADIUS_MAX, WORLD_H - DOT_RADIUS_MAX);
        dot.vx = std::cos(angle) * speed;
        dot.vy = std::sin(angle) * speed;
        dot.radius = radiusDist(m_Rng);
        dot.pulse = phaseDist(m_Rng);
        dot.pulseSpeed = pulseSpeedDist(m_Rng);
        dot.color = PALETTE[colorDist(m_Rng)];
        return dot;
    }

    void SeedDots(std::size_t count)
    {
        m_Dots.clear();
        count = std::min(count, MAX_DOTS);

        std::uniform_real_distribution<float> xDist(-WORLD_W + 24.0f, WORLD_W - 24.0f);
        std::uniform_real_distribution<float> yDist(-WORLD_H + 24.0f, WORLD_H - 24.0f);

        for (std::size_t i = 0; i < count; ++i)
            m_Dots.push_back(MakeDot(xDist(m_Rng), yDist(m_Rng)));
    }

    void SpawnBurst(const glm::vec2& position)
    {
        // Keep the simulation bounded even when the user repeatedly clicks.
        if (m_Dots.size() + SPAWN_COUNT > MAX_DOTS)
        {
            const std::size_t excess = m_Dots.size() + SPAWN_COUNT - MAX_DOTS;
            m_Dots.erase(m_Dots.begin(), m_Dots.begin() + static_cast<std::ptrdiff_t>(excess));
        }

        std::uniform_real_distribution<float> scatter(-38.0f, 38.0f);
        for (std::size_t i = 0; i < SPAWN_COUNT; ++i)
        {
            m_Dots.push_back(MakeDot(
                position.x + scatter(m_Rng),
                position.y + scatter(m_Rng)));
        }
    }

    void RemoveNearestDot(const glm::vec2& position)
    {
        std::size_t nearestIndex = m_Dots.size();
        float nearestDistanceSq = MOUSE_REMOVE_RADIUS_SQ;

        for (std::size_t i = 0; i < m_Dots.size(); ++i)
        {
            const float dx = m_Dots[i].x - position.x;
            const float dy = m_Dots[i].y - position.y;
            const float distanceSq = dx * dx + dy * dy;

            if (distanceSq < nearestDistanceSq)
            {
                nearestDistanceSq = distanceSq;
                nearestIndex = i;
            }
        }

        if (nearestIndex < m_Dots.size())
            m_Dots.erase(m_Dots.begin() + static_cast<std::ptrdiff_t>(nearestIndex));
    }

    void HandleInput()
    {
        using SIMPEngine::Input;

        if (Input::IsKeyJustPressed(SIMPEngine::SIMPK_SPACE))
            m_Paused = !m_Paused;

        if (Input::IsKeyJustPressed(SIMPEngine::SIMPK_R))
            SeedDots(INITIAL_DOTS);

        if (Input::IsKeyJustPressed(SIMPEngine::SIMPK_BACKSPACE))
            m_Dots.clear();

        m_HasMousePosition = IsMouseInViewport();
        if (!m_HasMousePosition)
            return;

        m_MouseWorld = GetMouseWorldPosition();

        if (Input::IsMouseButtonJustPressed(SIMPEngine::SIMPK_MOUSE_LEFT))
            SpawnBurst(m_MouseWorld);
        else if (Input::IsMouseButtonJustPressed(SIMPEngine::SIMPK_MOUSE_RIGHT))
            RemoveNearestDot(m_MouseWorld);
    }

    void UpdateStars(float dt)
    {
        // The star field remains alive while the particle simulation is paused.
        for (Star& star : m_Stars)
            star.phase += dt * star.frequency;
    }

    void UpdateDots(float dt)
    {
        for (Dot& dot : m_Dots)
        {
            dot.x += dot.vx * dt;
            dot.y += dot.vy * dt;
            dot.pulse += dot.pulseSpeed * dt;

            const float xLimit = WORLD_W - dot.radius;
            const float yLimit = WORLD_H - dot.radius;

            if (dot.x < -xLimit)
            {
                dot.x = -xLimit;
                dot.vx = std::abs(dot.vx);
            }
            else if (dot.x > xLimit)
            {
                dot.x = xLimit;
                dot.vx = -std::abs(dot.vx);
            }

            if (dot.y < -yLimit)
            {
                dot.y = -yLimit;
                dot.vy = std::abs(dot.vy);
            }
            else if (dot.y > yLimit)
            {
                dot.y = yLimit;
                dot.vy = -std::abs(dot.vy);
            }
        }
    }

    static int GetGridX(float x)
    {
        const float normalized = (x + WORLD_W) / GRID_CELL_SIZE;
        return std::clamp(static_cast<int>(normalized), 0, GRID_COLUMNS - 1);
    }

    static int GetGridY(float y)
    {
        const float normalized = (y + WORLD_H) / GRID_CELL_SIZE;
        return std::clamp(static_cast<int>(normalized), 0, GRID_ROWS - 1);
    }

    void BuildSpatialGrid()
    {
        for (auto& cell : m_SpatialGrid)
            cell.clear();

        for (std::size_t i = 0; i < m_Dots.size(); ++i)
        {
            const Dot& dot = m_Dots[i];
            const int x = GetGridX(dot.x);
            const int y = GetGridY(dot.y);
            const std::size_t cellIndex = static_cast<std::size_t>(y * GRID_COLUMNS + x);
            m_SpatialGrid[cellIndex].push_back(i);
        }
    }

    void RenderBackdrop()
    {
        using SIMPEngine::Renderer;

        Renderer::SetBlendMode(SIMPEngine::BlendMode::Alpha);
        Renderer::DrawQuad(
            0.0f, 0.0f,
            WORLD_W * 2.0f, WORLD_H * 2.0f,
            0.0f,
            {7, 12, 25, 255}, true, -100.0f);

        const SDL_Color gridColor{46, 73, 105, 34};
        for (float x = -WORLD_W + 120.0f; x < WORLD_W; x += 120.0f)
            Renderer::DrawLine(x, -WORLD_H, x, WORLD_H, 0.7f, gridColor, -90.0f);

        for (float y = -WORLD_H + 120.0f; y < WORLD_H; y += 120.0f)
            Renderer::DrawLine(-WORLD_W, y, WORLD_W, y, 0.7f, gridColor, -90.0f);

        Renderer::SetBlendMode(SIMPEngine::BlendMode::Additive);
        for (const Star& star : m_Stars)
        {
            const float twinkle = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(star.phase));
            const Uint8 alpha = static_cast<Uint8>(18.0f + twinkle * 58.0f);
            Renderer::DrawCircle(
                star.x, star.y, star.radius,
                {134, 181, 255, alpha}, 0.8f, -80.0f);
        }
        Renderer::SetBlendMode(SIMPEngine::BlendMode::Alpha);

        const SDL_Color borderColor{74, 124, 171, 100};
        Renderer::DrawLine(-WORLD_W, -WORLD_H, WORLD_W, -WORLD_H, 1.2f, borderColor, -70.0f);
        Renderer::DrawLine(-WORLD_W, WORLD_H, WORLD_W, WORLD_H, 1.2f, borderColor, -70.0f);
        Renderer::DrawLine(-WORLD_W, -WORLD_H, -WORLD_W, WORLD_H, 1.2f, borderColor, -70.0f);
        Renderer::DrawLine(WORLD_W, -WORLD_H, WORLD_W, WORLD_H, 1.2f, borderColor, -70.0f);
    }

    void RenderConnections()
    {
        using SIMPEngine::Renderer;
        BuildSpatialGrid();
        m_ConnectionCount = 0;

        for (std::size_t i = 0; i < m_Dots.size(); ++i)
        {
            const Dot& a = m_Dots[i];
            const int gridX = GetGridX(a.x);
            const int gridY = GetGridY(a.y);

            for (int offsetY = -1; offsetY <= 1; ++offsetY)
            {
                const int neighborY = gridY + offsetY;
                if (neighborY < 0 || neighborY >= GRID_ROWS)
                    continue;

                for (int offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const int neighborX = gridX + offsetX;
                    if (neighborX < 0 || neighborX >= GRID_COLUMNS)
                        continue;

                    const std::size_t cellIndex = static_cast<std::size_t>(neighborY * GRID_COLUMNS + neighborX);
                    for (const std::size_t j : m_SpatialGrid[cellIndex])
                    {
                        if (j <= i)
                            continue;

                        const Dot& b = m_Dots[j];
                        const float dx = b.x - a.x;
                        const float dy = b.y - a.y;
                        const float distanceSq = dx * dx + dy * dy;
                        if (distanceSq >= CONNECT_DIST_SQ)
                            continue;

                        const float distance = std::sqrt(distanceSq);
                        const float proximity = 1.0f - distance / CONNECT_DIST;
                        const float strength = proximity * proximity;
                        const auto mix = [](Uint8 lhs, Uint8 rhs) -> Uint8
                        {
                            return static_cast<Uint8>((static_cast<unsigned int>(lhs) +
                                                       static_cast<unsigned int>(rhs)) / 2U);
                        };

                        const SDL_Color lineColor{
                            mix(a.color.r, b.color.r),
                            mix(a.color.g, b.color.g),
                            mix(a.color.b, b.color.b),
                            static_cast<Uint8>(16.0f + strength * 116.0f)
                        };

                        Renderer::DrawCapsuleLine(
                            a.x, a.y, b.x, b.y,
                            0.55f + strength * 0.55f,
                            lineColor, 0.9f, -10.0f);
                        ++m_ConnectionCount;
                    }
                }
            }
        }
    }

    void RenderDots()
    {
        using SIMPEngine::Renderer;
        const SIMPEngine::BlendMode previousBlend = Renderer::GetBlendMode();

        Renderer::SetBlendMode(SIMPEngine::BlendMode::Additive);
        for (const Dot& dot : m_Dots)
        {
            const float pulse = 0.68f + 0.32f * (0.5f + 0.5f * std::sin(dot.pulse));
            const Uint8 outerAlpha = static_cast<Uint8>(16.0f + pulse * 18.0f);
            const Uint8 innerAlpha = static_cast<Uint8>(24.0f + pulse * 34.0f);

            Renderer::DrawCircle(
                dot.x, dot.y, dot.radius * 3.4f,
                {dot.color.r, dot.color.g, dot.color.b, outerAlpha}, 1.2f, 0.0f);
            Renderer::DrawCircle(
                dot.x, dot.y, dot.radius * 1.9f,
                {dot.color.r, dot.color.g, dot.color.b, innerAlpha}, 0.9f, 0.5f);
        }
        Renderer::SetBlendMode(SIMPEngine::BlendMode::Alpha);

        for (const Dot& dot : m_Dots)
        {
            Renderer::DrawCircle(dot.x, dot.y, dot.radius, dot.color, 0.75f, 1.0f);
            Renderer::DrawCircle(
                dot.x - dot.radius * 0.22f,
                dot.y + dot.radius * 0.22f,
                dot.radius * 0.24f,
                {255, 255, 255, 220}, 0.65f, 1.5f);
        }

        Renderer::SetBlendMode(previousBlend);
    }

    void RenderCursor()
    {
        if (!m_HasMousePosition)
            return;

        using SIMPEngine::Renderer;
        const float pulse = 0.5f + 0.5f * std::sin(m_Time * 3.2f);
        const float radius = 17.0f + pulse * 3.0f;

        Renderer::DrawRing(
            m_MouseWorld.x, m_MouseWorld.y, radius, 1.1f,
            {124, 221, 255, 175}, 0.8f, 4.0f);
        Renderer::DrawCircle(
            m_MouseWorld.x, m_MouseWorld.y, 2.2f,
            {220, 246, 255, 190}, 0.7f, 4.1f);
    }

    void RenderOverlay()
    {
        using SIMPEngine::Renderer;
        const SDL_Color titleColor{216, 237, 255, 245};
        const SDL_Color mutedColor{124, 158, 194, 210};
        const SDL_Color accentColor{113, 220, 255, 235};

        Renderer::DrawText(
            "CONSTELLATION", -914.0f, 493.0f, 23.0f,
            titleColor, SIMPEngine::TextAlign::Left, 0.0f, 12.0f);
        Renderer::DrawText(
            "INTERACTIVE PARTICLE NETWORK", -912.0f, 468.0f, 10.0f,
            mutedColor, SIMPEngine::TextAlign::Left, 0.0f, 12.0f);

        Renderer::DrawLine(
            -914.0f, 451.0f, -620.0f, 451.0f,
            1.0f, {68, 137, 183, 135}, 12.0f);

        const std::string stats =
            "NODES  " + std::to_string(m_Dots.size()) +
            "     CONNECTIONS  " + std::to_string(m_ConnectionCount);
        Renderer::DrawText(
            stats, -914.0f, -486.0f, 13.0f,
            accentColor, SIMPEngine::TextAlign::Left, 0.0f, 12.0f);
        Renderer::DrawText(
            "LEFT CLICK  SPAWN     RIGHT CLICK  REMOVE     SPACE  PAUSE     R  RESET     BACKSPACE  CLEAR",
            -914.0f, -512.0f, 9.0f,
            mutedColor, SIMPEngine::TextAlign::Left, 0.0f, 12.0f);

        Renderer::DrawText(
            m_Paused ? "PAUSED" : "LIVE",
            914.0f, 493.0f, 13.0f,
            m_Paused ? SDL_Color{255, 194, 120, 240} : SDL_Color{116, 238, 197, 240},
            SIMPEngine::TextAlign::Right, 0.0f, 12.0f);
    }
};
#pragma once

#include "Common.h"

#include <Engine/Rendering/Font.h>
#include <Engine/Rendering/Renderer.h>

#include <cstdio>
#include <memory>

namespace Platformer
{
    struct ViewRect
    {
        glm::vec2 Center{0.0f};
        glm::vec2 Half{0.0f};
    };

    inline ViewRect CameraView(SIMPEngine::Scene &scene)
    {
        ViewRect view;
        SIMPEngine::Entity camera = scene.GetEntityByName("Camera");
        if (!camera || !camera.HasComponent<CameraComponent>())
            return view;

        auto &cam = camera.GetComponent<CameraComponent>().Camera;
        auto [w, h] = cam.GetViewportSize();
        float aspect = (w > 0 && h > 0) ? (float)w / (float)h : 16.0f / 9.0f;
        view.Center = cam.GetPosition();
        view.Half.y = 540.0f * cam.GetZoom();
        view.Half.x = view.Half.y * aspect;
        return view;
    }

    class CameraFollowScript : public SIMPEngine::ScriptableEntity
    {
    protected:
        void OnCreate() override
        {
            GetComponent<CameraComponent>().Camera.SetManualControl(false);
            m_Position = GetComponent<TransformComponent>().position;
        }

        void OnUpdate(float dt) override
        {
            SIMPEngine::Scene &scene = *GetEntity().GetScene();
            auto &camera = GetComponent<CameraComponent>().Camera;
            auto [w, h] = camera.GetViewportSize();

            float pixelsPerUnit = h > 0 ? std::max(1.0f, std::round((float)h / 220.0f)) : 4.0f;
            if (h > 0)
                camera.SetZoom((float)h / (1080.0f * pixelsPerUnit));

            SIMPEngine::Entity player = scene.GetEntityByName("Player");
            glm::vec2 target = m_Position;
            if (player && player.HasComponent<TransformComponent>())
                target = player.GetComponent<TransformComponent>().position + glm::vec2(0.0f, 16.0f);

            m_Position += (target - m_Position) * std::min(1.0f, dt * 6.0f);

            ViewRect view = CameraView(scene);
            const GameState &state = State();
            glm::vec2 clamped = m_Position;
            if (state.LevelWidth > view.Half.x * 2.0f)
                clamped.x = std::clamp(clamped.x, view.Half.x, state.LevelWidth - view.Half.x);
            clamped.y = std::max(clamped.y, view.Half.y);

            clamped = glm::round(clamped * pixelsPerUnit) / pixelsPerUnit;
            GetComponent<TransformComponent>().position = clamped;
        }

    private:
        glm::vec2 m_Position{0.0f};
    };

    class HudScript : public SIMPEngine::ScriptableEntity
    {
    protected:
        void OnCreate() override
        {
            m_Font = SIMPEngine::Font::Load("assets://platformer/fonts/PixelOperator8.ttf");
        }

        void OnUpdate(float dt) override
        {
            m_Time += dt;
        }

        void OnRender() override
        {
            if (!m_Font)
                return;

            using SIMPEngine::TextAlign;
            const GameState &state = State();
            ViewRect view = CameraView(*GetEntity().GetScene());
            glm::vec2 topLeft = view.Center + glm::vec2(-view.Half.x + 6.0f, view.Half.y - 5.0f);
            glm::vec2 topRight = view.Center + glm::vec2(view.Half.x - 6.0f, view.Half.y - 5.0f);

            char text[96];
            std::snprintf(text, sizeof(text), "COINS %d/%d", state.Coins, state.TotalCoins);
            Label(text, topLeft, 8.0f, TextAlign::Left, {255, 222, 89, 255});

            std::snprintf(text, sizeof(text), "LIVES %d   TIME %.1f", state.Hearts, state.Time);
            Label(text, topRight, 8.0f, TextAlign::Right, {255, 255, 255, 255});

            if (m_Time < 5.0f && !state.Won && !state.GameOver)
                Label("ARROWS / A D MOVE   SPACE JUMP", view.Center + glm::vec2(0.0f, view.Half.y - 22.0f), 8.0f, TextAlign::Center, {255, 255, 255, 230});

            if (state.Won)
            {
                Label("LEVEL CLEAR", view.Center + glm::vec2(0.0f, 24.0f), 16.0f, TextAlign::Center, {255, 222, 89, 255});
                std::snprintf(text, sizeof(text), "COINS %d/%d   TIME %.1f", state.Coins, state.TotalCoins, state.Time);
                Label(text, view.Center + glm::vec2(0.0f, 2.0f), 8.0f, TextAlign::Center, {255, 255, 255, 255});
                Label("PRESS R TO PLAY AGAIN", view.Center + glm::vec2(0.0f, -12.0f), 8.0f, TextAlign::Center, {255, 255, 255, 255});
            }
            else if (state.GameOver)
            {
                Label("GAME OVER", view.Center + glm::vec2(0.0f, 24.0f), 16.0f, TextAlign::Center, {255, 110, 110, 255});
                Label("PRESS R TO TRY AGAIN", view.Center + glm::vec2(0.0f, 2.0f), 8.0f, TextAlign::Center, {255, 255, 255, 255});
            }
        }

    private:
        void Label(const char *text, const glm::vec2 &at, float size, SIMPEngine::TextAlign align, SDL_Color color)
        {
            SIMPEngine::Renderer::DrawText(text, at.x + 1.0f, at.y - 1.0f, size, {20, 20, 30, 200}, align, 0.0f, 999.0f, m_Font.get());
            SIMPEngine::Renderer::DrawText(text, at.x, at.y, size, color, align, 0.0f, 1000.0f, m_Font.get());
        }

        std::shared_ptr<SIMPEngine::Font> m_Font;
        float m_Time = 0.0f;
    };
}

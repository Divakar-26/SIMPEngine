#pragma once

#include <glm/glm.hpp>

#include <SDL3/SDL_events.h>

#include <vector>

namespace SIMPEngine
{
    struct TouchPoint
    {
        SDL_FingerID Id = 0;
        glm::vec2 Position{0.0f};
        bool Pressed = false;
        bool Released = false;
    };

    class TouchControls
    {
    public:
        static void SetEnabled(bool enabled);
        static bool IsEnabled();
        static void SetVisible(bool visible);
        static bool IsVisible();
        static void SetFloatingStick(bool floating);
        static bool IsFloatingStick();
        static void SetScale(float scale);
        static void SetSafeArea(float windowWidth, float windowHeight, float x, float y, float width, float height);
        static bool TakePointerPark();
        static float TakePinchWheel();

        static void BeginFrame();
        static bool HandleEvent(const SDL_Event &event, float windowWidth, float windowHeight, glm::vec2 &mousePoint, bool &mouseDown, bool &mouseUp);
        static void Update();
        static void Render();
        static void Reset();

        static glm::vec2 GetStick();
        static bool IsJumpDown();
        static bool IsInteractDown();

        static std::vector<TouchPoint> GetTouches();
        static bool GetPinch(float &scale, glm::vec2 &center, glm::vec2 &pan);
    };
}

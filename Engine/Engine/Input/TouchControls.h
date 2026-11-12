#pragma once

#include <glm/glm.hpp>

#include <SDL3/SDL_events.h>

namespace SIMPEngine
{
    class TouchControls
    {
    public:
        static void SetEnabled(bool enabled);
        static bool IsEnabled();
        static void SetVisible(bool visible);
        static bool IsVisible();

        static bool HandleEvent(const SDL_Event &event, float windowWidth, float windowHeight, glm::vec2 &mousePoint, bool &mouseDown, bool &mouseUp);
        static void Update();
        static void Render();
        static void Reset();

        static glm::vec2 GetStick();
        static bool IsJumpDown();
        static bool IsInteractDown();
    };
}

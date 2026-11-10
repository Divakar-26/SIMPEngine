#pragma once

#include <glm/glm.hpp>
#include <unordered_set>
#include <utility>

namespace SIMPEngine{
    class Input{
        public:

            static bool IsKeyPressed(int keyCode);
            static bool IsKeyJustPressed(int keyCode);
            static bool IsMouseButtonPressed(int button);
            static bool IsMouseButtonJustPressed(int button);
            static bool IsKeyJustReleased(int keyCode);
            static bool IsMouseButtonJustReleased(int button);

            static std::pair<int,int> GetMousePosition();
            static glm::vec2 GetMousePositionF();

            static void SetViewport(float x, float y, float width, float height);
            static void ClearViewport();
            static glm::vec2 GetMouseViewportPosition();
            static glm::vec2 GetViewportSize();
            static bool IsMouseInViewport();

            static void SetEnabled(bool enabled);
            static bool IsEnabled();

            static void OnKeyPressed(int keyCode);
            static void OnKeyReleased(int keyCode);
            static void OnMouseButtonPressed(int button);
            static void OnMouseButtonReleased(int button);
            static void OnMouseMoved(float x, float y);
            static void OnMouseWheel(float y);
            static float GetMouseWheel();
            static void BeginFrame();

            static void ResetAllKeys();
        private:

            static std::unordered_set<int> s_KeysPressed;
            static std::unordered_set<int> s_KeysJustPressed;
            static std::unordered_set<int> s_MouseButtonsPressed;
            static std::unordered_set<int> s_MouseButtonsJustPressed;
            static std::unordered_set<int> s_KeysJustReleased;
            static std::unordered_set<int> s_MouseButtonsJustReleased;
            static float s_MouseX, s_MouseY;
            static float s_MouseWheel;

            static bool s_Enabled;
            static bool s_HasViewport;
            static glm::vec4 s_Viewport;
    };
}

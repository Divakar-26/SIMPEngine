#pragma once

#include<unordered_set>

namespace SIMPEngine{
    class Input{
        public:
            static bool IsKeyPressed(int keyCode);
            static bool IsMouseButtonPressed(int button);
            static std::pair<int,int> GetMousePosition();

            static void OnKeyPressed(int keyCode);
            static void OnKeyReleased(int keyCode);
            static void OnMouseButtonPressed(int button);
            static void OnMouseButtonReleased(int button);
            static void OnMouseMoved(int x, int);
        private:
            static std::unordered_set<int> s_KeysPressed;
            static std::unordered_set<int> s_MouseButtonsPressed;
            static int s_MouseX, s_MouseY;
    };
}
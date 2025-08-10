#include "Engine/Input.h"
#include "Engine/Log.h"

namespace SIMPEngine
{
    std::unordered_set<int> Input::s_KeysPressed;
    std::unordered_set<int> Input::s_MouseButtonsPressed;
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;

    bool Input::IsKeyPressed(int keycode)
    {
        bool pressed = s_KeysPressed.find(keycode) != s_KeysPressed.end();
        // CORE_INFO("Checking key {} pressed? {}", keycode, pressed);
        return s_KeysPressed.find(keycode) != s_KeysPressed.end();
    }

    bool Input::IsMouseButtonPressed(int button)
    {
        return s_MouseButtonsPressed.find(button) != s_MouseButtonsPressed.end();
    }

    std::pair<int, int> Input::GetMousePosition()
    {
        return {s_MouseX, s_MouseY};
    }

    void Input::OnKeyPressed(int keycode)
    {
        s_KeysPressed.insert(keycode);
        // CORE_INFO("Key pressed: {}", keycode);
    }

    void Input::OnKeyReleased(int keycode)
    {
        s_KeysPressed.erase(keycode);
    }

    void Input::OnMouseButtonPressed(int button)
    {
        s_MouseButtonsPressed.insert(button);
    }

    void Input::OnMouseButtonReleased(int button)
    {
        s_MouseButtonsPressed.erase(button);
    }

    void Input::OnMouseMoved(int x, int y)
    {
        s_MouseX = x;
        s_MouseY = y;
    }
} // namespace SIMPEngine

#include <Engine/Input/Input.h>
#include <Engine/Core/Log.h>

namespace SIMPEngine
{
    std::unordered_set<int> Input::s_KeysPressed;

    std::unordered_set<int> Input::s_MouseButtonsPressed;
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;
    float Input::s_MouseWheel = 0.0f;

    bool Input::IsKeyPressed(int keycode)
    {
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

    void Input::ResetAllKeys()
    {
        s_KeysPressed.clear();
        s_MouseButtonsPressed.clear();
        s_MouseWheel = 0;
    }

    float Input::GetMouseWheel()
    {
        const float delta = s_MouseWheel;
        s_MouseWheel = 0.0f;
        return delta;
    }

    void Input::OnMouseWheel(float y)
    {
        s_MouseWheel += y;
    }

    void Input::BeginFrame()
    {
        s_MouseWheel = 0.0f;
    }
}

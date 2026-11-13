#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Core/Log.h>

#include <cmath>

namespace SIMPEngine
{
    std::unordered_set<int> Input::s_KeysPressed;
    std::unordered_set<int> Input::s_VirtualKeys;
    std::unordered_set<int> Input::s_KeysJustPressed;

    std::unordered_set<int> Input::s_MouseButtonsPressed;
    std::unordered_set<int> Input::s_MouseButtonsJustPressed;
    std::unordered_set<int> Input::s_KeysJustReleased;
    std::unordered_set<int> Input::s_MouseButtonsJustReleased;
    float Input::s_MouseX = 0.0f;
    float Input::s_MouseY = 0.0f;
    float Input::s_MouseWheel = 0.0f;
    std::unordered_set<int> Input::s_PadButtonsPressed;
    std::unordered_set<int> Input::s_PadButtonsJustPressed;
    std::unordered_set<int> Input::s_PadButtonsJustReleased;
    float Input::s_PadAxes[8] = {};
    int Input::s_PadCount = 0;
    float Input::s_PadDeadzone = 0.2f;

    bool Input::s_Enabled = true;
    bool Input::s_HasViewport = false;
    glm::vec4 Input::s_Viewport{0.0f};

    bool Input::IsKeyPressed(int keycode)
    {
        return s_Enabled && (s_KeysPressed.count(keycode) != 0 || s_VirtualKeys.count(keycode) != 0);
    }

    bool Input::IsPhysicalKeyPressed(int keycode)
    {
        return s_Enabled && s_KeysPressed.count(keycode) != 0;
    }

    void Input::SetVirtualKey(int keycode, bool down)
    {
        const bool physical = s_KeysPressed.count(keycode) != 0;
        if (down)
        {
            if (s_VirtualKeys.insert(keycode).second && !physical)
                s_KeysJustPressed.insert(keycode);
        }
        else if (s_VirtualKeys.erase(keycode) && !physical)
            s_KeysJustReleased.insert(keycode);
    }

    bool Input::IsKeyJustPressed(int keycode)
    {
        return s_Enabled && s_KeysJustPressed.find(keycode) != s_KeysJustPressed.end();
    }

    bool Input::IsMouseButtonPressed(int button)
    {
        return s_Enabled && s_MouseButtonsPressed.find(button) != s_MouseButtonsPressed.end();
    }

    bool Input::IsMouseButtonJustPressed(int button)
    {
        return s_Enabled && s_MouseButtonsJustPressed.find(button) != s_MouseButtonsJustPressed.end();
    }

    bool Input::IsKeyJustReleased(int keycode)
    {
        return s_Enabled && s_KeysJustReleased.find(keycode) != s_KeysJustReleased.end();
    }

    bool Input::IsMouseButtonJustReleased(int button)
    {
        return s_Enabled && s_MouseButtonsJustReleased.find(button) != s_MouseButtonsJustReleased.end();
    }

    std::pair<int, int> Input::GetMousePosition()
    {
        return {static_cast<int>(std::floor(s_MouseX)), static_cast<int>(std::floor(s_MouseY))};
    }

    glm::vec2 Input::GetMousePositionF()
    {
        return {s_MouseX, s_MouseY};
    }

    void Input::SetViewport(float x, float y, float width, float height)
    {
        s_HasViewport = true;
        s_Viewport = {x, y, width, height};
    }

    void Input::ClearViewport()
    {
        s_HasViewport = false;
    }

    glm::vec2 Input::GetMouseViewportPosition()
    {
        if (!s_HasViewport)
            return {s_MouseX, s_MouseY};
        return {s_MouseX - s_Viewport.x, s_MouseY - s_Viewport.y};
    }

    glm::vec2 Input::WindowToViewport(const glm::vec2 &windowPosition)
    {
        if (!s_HasViewport)
            return windowPosition;
        return {windowPosition.x - s_Viewport.x, windowPosition.y - s_Viewport.y};
    }

    glm::vec2 Input::GetViewportSize()
    {
        if (!s_HasViewport)
            return {static_cast<float>(Renderer::m_WindowWidth), static_cast<float>(Renderer::m_WindowHeight)};
        return {s_Viewport.z, s_Viewport.w};
    }

    bool Input::IsMouseInViewport()
    {
        glm::vec2 p = GetMouseViewportPosition();
        glm::vec2 size = GetViewportSize();
        return p.x >= 0.0f && p.y >= 0.0f && p.x < size.x && p.y < size.y;
    }

    void Input::SetEnabled(bool enabled)
    {
        if (s_Enabled && !enabled)
            ResetAllKeys();
        s_Enabled = enabled;
    }

    bool Input::IsEnabled()
    {
        return s_Enabled;
    }

    void Input::OnKeyPressed(int keycode)
    {
        if (s_KeysPressed.insert(keycode).second && !s_VirtualKeys.count(keycode))
            s_KeysJustPressed.insert(keycode);
    }

    void Input::OnKeyReleased(int keycode)
    {
        if (s_KeysPressed.erase(keycode) && !s_VirtualKeys.count(keycode))
            s_KeysJustReleased.insert(keycode);
    }

    void Input::OnMouseButtonPressed(int button)
    {
        if (s_MouseButtonsPressed.insert(button).second)
            s_MouseButtonsJustPressed.insert(button);
    }

    void Input::OnMouseButtonReleased(int button)
    {
        if (s_MouseButtonsPressed.erase(button))
            s_MouseButtonsJustReleased.insert(button);
    }

    void Input::OnMouseMoved(float x, float y)
    {
        s_MouseX = x;
        s_MouseY = y;
    }

    bool Input::IsGamepadButtonPressed(int button)
    {
        return s_Enabled && s_PadButtonsPressed.find(button) != s_PadButtonsPressed.end();
    }

    bool Input::IsGamepadButtonJustPressed(int button)
    {
        return s_Enabled && s_PadButtonsJustPressed.find(button) != s_PadButtonsJustPressed.end();
    }

    bool Input::IsGamepadButtonJustReleased(int button)
    {
        return s_Enabled && s_PadButtonsJustReleased.find(button) != s_PadButtonsJustReleased.end();
    }

    float Input::GetGamepadAxis(int axis)
    {
        if (!s_Enabled || axis < 0 || axis >= 8)
            return 0.0f;
        const float raw = s_PadAxes[axis];
        const float magnitude = std::abs(raw);
        if (magnitude <= s_PadDeadzone)
            return 0.0f;
        const float scaled = (magnitude - s_PadDeadzone) / (1.0f - s_PadDeadzone);
        return raw < 0.0f ? -scaled : scaled;
    }

    bool Input::IsGamepadConnected()
    {
        return s_PadCount > 0;
    }

    int Input::GetGamepadCount()
    {
        return s_PadCount;
    }

    float Input::GetGamepadDeadzone()
    {
        return s_PadDeadzone;
    }

    void Input::SetGamepadDeadzone(float deadzone)
    {
        s_PadDeadzone = deadzone < 0.0f ? 0.0f : (deadzone > 0.95f ? 0.95f : deadzone);
    }

    void Input::OnGamepadButton(int button, bool down)
    {
        if (down)
        {
            if (s_PadButtonsPressed.insert(button).second)
                s_PadButtonsJustPressed.insert(button);
        }
        else if (s_PadButtonsPressed.erase(button))
            s_PadButtonsJustReleased.insert(button);
    }

    void Input::OnGamepadAxis(int axis, float value)
    {
        if (axis >= 0 && axis < 8)
            s_PadAxes[axis] = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
    }

    void Input::OnGamepadConnected(bool connected)
    {
        if (connected)
            ++s_PadCount;
        else if (s_PadCount > 0)
            --s_PadCount;
        if (s_PadCount == 0)
        {
            s_PadButtonsPressed.clear();
            s_PadButtonsJustPressed.clear();
            s_PadButtonsJustReleased.clear();
            for (float &axis : s_PadAxes)
                axis = 0.0f;
        }
    }

    const std::unordered_set<int> &Input::GetGamepadButtonsJustPressed()
    {
        return s_PadButtonsJustPressed;
    }

    const std::unordered_set<int> &Input::GetKeysJustPressed()
    {
        return s_KeysJustPressed;
    }

    const std::unordered_set<int> &Input::GetMouseButtonsJustPressed()
    {
        return s_MouseButtonsJustPressed;
    }

    void Input::ResetAllKeys()
    {
        s_PadButtonsPressed.clear();
        s_PadButtonsJustPressed.clear();
        s_PadButtonsJustReleased.clear();
        for (float &axis : s_PadAxes)
            axis = 0.0f;
        s_KeysPressed.clear();
        s_VirtualKeys.clear();
        s_KeysJustPressed.clear();
        s_MouseButtonsPressed.clear();
        s_MouseButtonsJustPressed.clear();
        s_KeysJustReleased.clear();
        s_MouseButtonsJustReleased.clear();
        s_MouseWheel = 0.0f;
    }

    float Input::GetMouseWheel()
    {
        if (!s_Enabled)
            return 0.0f;
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
        s_KeysJustPressed.clear();
        s_MouseButtonsJustPressed.clear();
        s_KeysJustReleased.clear();
        s_MouseButtonsJustReleased.clear();
        s_PadButtonsJustPressed.clear();
        s_PadButtonsJustReleased.clear();
    }
}

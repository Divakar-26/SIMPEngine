#include <Engine/Input/Input.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Core/Log.h>

namespace SIMPEngine
{
    std::unordered_set<int> Input::s_KeysPressed;
    std::unordered_set<int> Input::s_KeysJustPressed;

    std::unordered_set<int> Input::s_MouseButtonsPressed;
    std::unordered_set<int> Input::s_MouseButtonsJustPressed;
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;
    float Input::s_MouseWheel = 0.0f;

    bool Input::s_Enabled = true;
    bool Input::s_HasViewport = false;
    glm::vec4 Input::s_Viewport{0.0f};

    bool Input::IsKeyPressed(int keycode)
    {
        return s_Enabled && s_KeysPressed.find(keycode) != s_KeysPressed.end();
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

    std::pair<int, int> Input::GetMousePosition()
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
            return {static_cast<float>(s_MouseX), static_cast<float>(s_MouseY)};
        return {static_cast<float>(s_MouseX) - s_Viewport.x, static_cast<float>(s_MouseY) - s_Viewport.y};
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
        if (s_KeysPressed.insert(keycode).second)
            s_KeysJustPressed.insert(keycode);
    }

    void Input::OnKeyReleased(int keycode)
    {
        s_KeysPressed.erase(keycode);
    }

    void Input::OnMouseButtonPressed(int button)
    {
        if (s_MouseButtonsPressed.insert(button).second)
            s_MouseButtonsJustPressed.insert(button);
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
        s_KeysJustPressed.clear();
        s_MouseButtonsPressed.clear();
        s_MouseButtonsJustPressed.clear();
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
    }
}

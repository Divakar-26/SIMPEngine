#include <Engine/Input/TouchControls.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Rendering/Renderer.h>

#include <SDL3/SDL_keycode.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <unordered_map>

namespace SIMPEngine
{
    namespace
    {
        enum class Role
        {
            Stick,
            Jump,
            Interact,
            Pointer,
            Extra
        };

        struct Finger
        {
            Role Owner = Role::Extra;
            glm::vec2 Position{0.0f};
            unsigned Order = 0;
            bool Pressed = false;
        };

        bool s_Enabled = false;
        bool s_Visible = true;
        std::unordered_map<SDL_FingerID, Finger> s_Fingers;
        glm::vec2 s_Stick{0.0f};
        bool s_Jump = false;
        bool s_Interact = false;
        glm::vec2 s_StickOrigin{-1.0f};
        glm::vec2 s_StickPoint{0.0f};
        bool s_Injected = false;
        SDL_FingerID s_Pointer = 0;
        bool s_HasPointer = false;
        bool s_Floating = false;
        float s_Scale = 1.0f;
        glm::vec2 s_Window{0.0f};
        unsigned s_Counter = 0;
        std::vector<TouchPoint> s_Released;
        bool s_HasPinch = false;
        float s_PinchScale = 1.0f;
        glm::vec2 s_PinchCenter{0.0f};
        glm::vec2 s_PinchPan{0.0f};
        float s_PrevDistance = 0.0f;
        glm::vec2 s_PrevCenter{0.0f};
        SDL_FingerID s_PinchA = 0;
        SDL_FingerID s_PinchB = 0;
        bool s_PinchActive = false;
        glm::vec4 s_SafeInsets{0.0f};
        bool s_ParkPending = false;
        float s_PinchWheel = 0.0f;
        std::vector<int> s_HeldKeys;

        bool Active()
        {
            return s_Enabled && s_Visible && !Input::IsGamepadConnected();
        }

        void SetHeldKeys(const std::vector<int> &keys)
        {
            for (int key : s_HeldKeys)
                if (std::find(keys.begin(), keys.end(), key) == keys.end())
                    Input::SetVirtualKey(key, false);
            for (int key : keys)
                if (std::find(s_HeldKeys.begin(), s_HeldKeys.end(), key) == s_HeldKeys.end())
                    Input::SetVirtualKey(key, true);
            s_HeldKeys = keys;
        }

        struct Layout
        {
            float Width = 0.0f;
            float Height = 0.0f;
            float Unit = 0.0f;
            glm::vec2 StickCenter;
            float StickRadius = 0.0f;
            glm::vec2 JumpCenter;
            glm::vec2 InteractCenter;
            float ButtonRadius = 0.0f;
        };

        Layout MakeLayout(float width, float height)
        {
            const float dp = std::max(s_Scale, 0.5f);
            Layout layout;
            layout.Width = width;
            layout.Height = height;
            layout.Unit = std::min(width, height);
            layout.StickRadius = std::clamp(layout.Unit * 0.17f, 52.0f * dp, std::max(52.0f * dp, 92.0f * dp));
            layout.ButtonRadius = std::clamp(layout.Unit * 0.095f, 30.0f * dp, std::max(30.0f * dp, 52.0f * dp));
            const float margin = std::max(layout.Unit * 0.04f, 14.0f * dp);
            const float left = margin + s_SafeInsets.x;
            const float right = margin + s_SafeInsets.z;
            const float bottom = margin + s_SafeInsets.w;
            layout.StickCenter = {left + layout.StickRadius * 1.25f, height - bottom - layout.StickRadius * 1.25f};
            const float br = layout.ButtonRadius;
            layout.JumpCenter = {width - right - br * 1.1f, height - bottom - br * 2.4f};
            layout.InteractCenter = {width - right - br * 3.7f, height - bottom - br * 1.3f};
            return layout;
        }

        bool Inside(const glm::vec2 &point, const glm::vec2 &center, float radius)
        {
            return glm::length(point - center) <= radius;
        }

        std::vector<std::pair<SDL_FingerID, Finger>> FreeFingers()
        {
            std::vector<std::pair<SDL_FingerID, Finger>> free;
            for (const auto &[id, finger] : s_Fingers)
                if (finger.Owner == Role::Pointer || finger.Owner == Role::Extra)
                    free.emplace_back(id, finger);
            std::sort(free.begin(), free.end(), [](const auto &a, const auto &b) { return a.second.Order < b.second.Order; });
            return free;
        }

        void Recompute(const Layout &layout)
        {
            s_Stick = {0.0f, 0.0f};
            s_Jump = false;
            s_Interact = false;
            for (const auto &[id, finger] : s_Fingers)
            {
                if (finger.Owner == Role::Stick)
                {
                    glm::vec2 delta = finger.Position - s_StickOrigin;
                    const float length = glm::length(delta);
                    const float radius = layout.StickRadius;
                    if (length > 0.0f)
                    {
                        const float amount = std::min(length / radius, 1.0f);
                        s_Stick = {delta.x / length * amount, -delta.y / length * amount};
                    }
                    s_StickPoint = s_StickOrigin + (length > radius ? delta / length * radius : delta);
                }
                else if (finger.Owner == Role::Jump)
                    s_Jump = true;
                else if (finger.Owner == Role::Interact)
                    s_Interact = true;
            }
        }
    }

    void TouchControls::SetEnabled(bool enabled)
    {
        if (enabled && !s_Enabled)
            s_ParkPending = true;
        s_Enabled = enabled;
        if (!enabled)
            Reset();
    }

    void TouchControls::SetSafeArea(float windowWidth, float windowHeight, float x, float y, float width, float height)
    {
        if (width <= 0.0f || height <= 0.0f)
        {
            s_SafeInsets = glm::vec4(0.0f);
            return;
        }
        s_SafeInsets = {std::max(x, 0.0f), std::max(y, 0.0f), std::max(windowWidth - (x + width), 0.0f), std::max(windowHeight - (y + height), 0.0f)};
    }

    bool TouchControls::TakePointerPark()
    {
        const bool park = s_ParkPending;
        s_ParkPending = false;
        return park && s_Enabled;
    }

    float TouchControls::TakePinchWheel()
    {
        const float wheel = s_PinchWheel;
        s_PinchWheel = 0.0f;
        return wheel;
    }

    bool TouchControls::IsEnabled()
    {
        return s_Enabled;
    }

    void TouchControls::SetVisible(bool visible)
    {
        s_Visible = visible;
    }

    bool TouchControls::IsVisible()
    {
        return s_Visible;
    }

    glm::vec2 TouchControls::GetStick()
    {
        return s_Stick;
    }

    bool TouchControls::IsJumpDown()
    {
        return s_Jump;
    }

    bool TouchControls::IsInteractDown()
    {
        return s_Interact;
    }

    void TouchControls::SetFloatingStick(bool floating)
    {
        s_Floating = floating;
    }

    bool TouchControls::IsFloatingStick()
    {
        return s_Floating;
    }

    void TouchControls::SetScale(float scale)
    {
        s_Scale = scale > 0.0f ? scale : 1.0f;
    }

    void TouchControls::BeginFrame()
    {
        s_Released.clear();
        for (auto &[id, finger] : s_Fingers)
            finger.Pressed = false;
    }

    std::vector<TouchPoint> TouchControls::GetTouches()
    {
        std::vector<TouchPoint> touches;
        for (const auto &[id, finger] : FreeFingers())
            touches.push_back({id, finger.Position, finger.Pressed, false});
        for (const TouchPoint &point : s_Released)
            touches.push_back(point);
        return touches;
    }

    bool TouchControls::GetPinch(float &scale, glm::vec2 &center, glm::vec2 &pan)
    {
        scale = s_PinchScale;
        center = s_PinchCenter;
        pan = s_PinchPan;
        return s_HasPinch;
    }

    void TouchControls::Reset()
    {
        s_Fingers.clear();
        s_Released.clear();
        s_Stick = {0.0f, 0.0f};
        s_Jump = false;
        s_Interact = false;
        s_HasPointer = false;
        s_HasPinch = false;
        s_PinchActive = false;
        s_PinchWheel = 0.0f;
        SetHeldKeys({});
        if (s_Injected)
        {
            InputActions::Inject("MoveX", 0.0f);
            InputActions::Inject("MoveY", 0.0f);
            InputActions::Inject("Jump", 0.0f);
            InputActions::Inject("Interact", 0.0f);
            s_Injected = false;
        }
    }

    bool TouchControls::HandleEvent(const SDL_Event &event, float windowWidth, float windowHeight, glm::vec2 &mousePoint, bool &mouseDown, bool &mouseUp)
    {
        mouseDown = mouseUp = false;
        if (event.type != SDL_EVENT_FINGER_DOWN && event.type != SDL_EVENT_FINGER_MOTION && event.type != SDL_EVENT_FINGER_UP && event.type != SDL_EVENT_FINGER_CANCELED)
            return false;

        s_Window = {windowWidth, windowHeight};
        const glm::vec2 point(event.tfinger.x * windowWidth, event.tfinger.y * windowHeight);
        const SDL_FingerID id = event.tfinger.fingerID;
        const Layout layout = MakeLayout(windowWidth, windowHeight);
        const bool show = Active();

        if (event.type == SDL_EVENT_FINGER_DOWN)
        {
            Finger finger;
            finger.Position = point;
            finger.Order = ++s_Counter;
            finger.Pressed = true;
            const bool stickBusy = std::any_of(s_Fingers.begin(), s_Fingers.end(), [](const auto &entry) { return entry.second.Owner == Role::Stick; });
            const float grab = 1.6f;
            if (show && !stickBusy && Inside(point, layout.StickCenter, layout.StickRadius * grab))
            {
                finger.Owner = Role::Stick;
                s_StickOrigin = layout.StickCenter;
                s_StickPoint = layout.StickCenter;
            }
            else if (show && Inside(point, layout.JumpCenter, layout.ButtonRadius * grab))
                finger.Owner = Role::Jump;
            else if (show && Inside(point, layout.InteractCenter, layout.ButtonRadius * grab))
                finger.Owner = Role::Interact;
            else if (show && s_Floating && !stickBusy && point.x < windowWidth * 0.45f && point.y > windowHeight * 0.3f)
            {
                const float margin = layout.StickRadius;
                finger.Owner = Role::Stick;
                s_StickOrigin = {std::clamp(point.x, margin, windowWidth - margin), std::clamp(point.y, margin, windowHeight - margin)};
                s_StickPoint = s_StickOrigin;
            }
            else if (!s_HasPointer)
            {
                finger.Owner = Role::Pointer;
                s_HasPointer = true;
                s_Pointer = id;
                mousePoint = point;
                mouseDown = true;
            }
            else
                finger.Owner = Role::Extra;
            s_Fingers[id] = finger;
            Recompute(layout);
            return true;
        }

        auto it = s_Fingers.find(id);
        if (it == s_Fingers.end())
            return true;

        if (event.type == SDL_EVENT_FINGER_MOTION)
        {
            it->second.Position = point;
            if (it->second.Owner == Role::Pointer)
                mousePoint = point;
        }
        else
        {
            const glm::vec2 releasePoint = it->second.Position;
            if (it->second.Owner == Role::Pointer)
            {
                mousePoint = releasePoint;
                mouseUp = true;
                s_HasPointer = false;
                s_ParkPending = true;
            }
            if (it->second.Owner == Role::Pointer || it->second.Owner == Role::Extra)
                s_Released.push_back({id, releasePoint, false, true});
            s_Fingers.erase(it);
        }
        Recompute(layout);
        return true;
    }

    void TouchControls::Update()
    {
        s_HasPinch = false;
        s_PinchScale = 1.0f;
        s_PinchPan = {0.0f, 0.0f};
        const auto free = FreeFingers();
        if (free.size() >= 2)
        {
            const SDL_FingerID a = free[0].first;
            const SDL_FingerID b = free[1].first;
            const glm::vec2 pa = free[0].second.Position;
            const glm::vec2 pb = free[1].second.Position;
            const float distance = glm::length(pb - pa);
            const glm::vec2 center = (pa + pb) * 0.5f;
            if (s_PinchActive && s_PinchA == a && s_PinchB == b && s_PrevDistance > 1.0f)
            {
                s_HasPinch = true;
                s_PinchScale = distance / s_PrevDistance;
                s_PinchPan = center - s_PrevCenter;
                s_PinchWheel += std::log(std::max(s_PinchScale, 0.01f)) / std::log(1.15f);
            }
            s_PinchCenter = center;
            s_PinchA = a;
            s_PinchB = b;
            s_PinchActive = true;
            s_PrevDistance = distance;
            s_PrevCenter = center;
        }
        else
            s_PinchActive = false;

        if (!s_Enabled)
            return;
        std::vector<int> keys;
        if (s_Stick.x > 0.5f)
            keys.insert(keys.end(), {SDLK_RIGHT, SDLK_D});
        else if (s_Stick.x < -0.5f)
            keys.insert(keys.end(), {SDLK_LEFT, SDLK_A});
        if (s_Stick.y > 0.5f)
            keys.insert(keys.end(), {SDLK_UP, SDLK_W});
        else if (s_Stick.y < -0.5f)
            keys.insert(keys.end(), {SDLK_DOWN, SDLK_S});
        if (s_Jump)
            keys.push_back(SDLK_SPACE);
        if (s_Interact)
            keys.push_back(SDLK_E);
        SetHeldKeys(keys);
        InputActions::Inject("MoveX", s_Stick.x);
        InputActions::Inject("MoveY", s_Stick.y);
        InputActions::Inject("Jump", s_Jump ? 1.0f : 0.0f);
        InputActions::Inject("Interact", s_Interact ? 1.0f : 0.0f);
        s_Injected = true;
    }

    void TouchControls::Render()
    {
        if (!Active())
            return;

        const glm::vec2 viewport = Renderer::GetViewportSize();
        if (viewport.x < 16.0f || viewport.y < 16.0f)
            return;
        const glm::vec2 window = (s_Window.x > 0.0f && s_Window.y > 0.0f) ? s_Window : viewport;
        const Layout layout = MakeLayout(window.x, window.y);
        const glm::vec2 k = viewport / window;
        const float ks = std::min(k.x, k.y);

        const glm::mat4 previousView = Renderer::GetViewMatrix();
        const glm::mat4 previousProjection = Renderer::GetAPI()->GetProjectionMatrix();
        Renderer::Flush();
        Renderer::SetViewMatrix(glm::mat4(1.0f));
        Renderer::SetProjectionMatrix(glm::ortho(0.0f, viewport.x, viewport.y, 0.0f, -1000.0f, 1000.0f));

        const bool stickHeld = std::any_of(s_Fingers.begin(), s_Fingers.end(), [](const auto &entry) { return entry.second.Owner == Role::Stick; });
        const glm::vec2 base = (stickHeld ? s_StickOrigin : layout.StickCenter) * k;
        const float stickRadius = layout.StickRadius * ks;
        const float z = 900.0f;
        Renderer::DrawCircle(base.x, base.y, stickRadius, {255, 255, 255, 34}, 1.0f, z);
        Renderer::DrawRing(base.x, base.y, stickRadius, 3.0f, {255, 255, 255, 90}, 1.0f, z + 0.01f);
        const glm::vec2 knob = base + glm::vec2(s_Stick.x, -s_Stick.y) * stickRadius;
        Renderer::DrawCircle(knob.x, knob.y, stickRadius * 0.42f, {255, 255, 255, s_Stick != glm::vec2(0.0f) ? static_cast<Uint8>(150) : static_cast<Uint8>(90)}, 1.0f, z + 0.02f);

        const float buttonRadius = layout.ButtonRadius * ks;
        auto button = [&](const glm::vec2 &center, bool down, SDL_Color color)
        {
            const glm::vec2 c = center * k;
            Renderer::DrawCircle(c.x, c.y, buttonRadius, {color.r, color.g, color.b, static_cast<Uint8>(down ? 150 : 60)}, 1.0f, z);
            Renderer::DrawRing(c.x, c.y, buttonRadius, 3.0f, {255, 255, 255, static_cast<Uint8>(down ? 200 : 110)}, 1.0f, z + 0.01f);
        };
        button(layout.JumpCenter, s_Jump, {120, 200, 255, 255});
        button(layout.InteractCenter, s_Interact, {255, 210, 110, 255});

        Renderer::Flush();
        Renderer::SetViewMatrix(previousView);
        Renderer::SetProjectionMatrix(previousProjection);
    }
}

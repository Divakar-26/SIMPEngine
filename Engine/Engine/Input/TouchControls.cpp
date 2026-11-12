#include <Engine/Input/TouchControls.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Rendering/Renderer.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
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
            Pointer
        };

        struct Finger
        {
            Role Owner = Role::Pointer;
            glm::vec2 Position{0.0f};
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
            Layout layout;
            layout.Width = width;
            layout.Height = height;
            layout.Unit = std::min(width, height);
            layout.StickRadius = layout.Unit * 0.17f;
            layout.StickCenter = {layout.Unit * 0.24f, height - layout.Unit * 0.26f};
            layout.ButtonRadius = layout.Unit * 0.095f;
            layout.JumpCenter = {width - layout.Unit * 0.2f, height - layout.Unit * 0.24f};
            layout.InteractCenter = {width - layout.Unit * 0.42f, height - layout.Unit * 0.14f};
            return layout;
        }

        bool Inside(const glm::vec2 &point, const glm::vec2 &center, float radius)
        {
            return glm::length(point - center) <= radius;
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
        s_Enabled = enabled;
        if (!enabled)
            Reset();
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

    void TouchControls::Reset()
    {
        s_Fingers.clear();
        s_Stick = {0.0f, 0.0f};
        s_Jump = false;
        s_Interact = false;
        s_HasPointer = false;
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

        const glm::vec2 point(event.tfinger.x * windowWidth, event.tfinger.y * windowHeight);
        const SDL_FingerID id = event.tfinger.fingerID;
        const Layout layout = MakeLayout(windowWidth, windowHeight);
        const bool show = s_Enabled;

        if (event.type == SDL_EVENT_FINGER_DOWN)
        {
            Finger finger;
            finger.Position = point;
            if (show && Inside(point, layout.StickCenter, layout.StickRadius * 1.5f))
            {
                finger.Owner = Role::Stick;
                s_StickOrigin = layout.StickCenter;
                s_StickPoint = layout.StickCenter;
            }
            else if (show && Inside(point, layout.JumpCenter, layout.ButtonRadius * 1.5f))
                finger.Owner = Role::Jump;
            else if (show && Inside(point, layout.InteractCenter, layout.ButtonRadius * 1.5f))
                finger.Owner = Role::Interact;
            else if (!s_HasPointer)
            {
                finger.Owner = Role::Pointer;
                s_HasPointer = true;
                s_Pointer = id;
                mousePoint = point;
                mouseDown = true;
            }
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
            if (it->second.Owner == Role::Pointer)
            {
                mousePoint = point;
                mouseUp = true;
                s_HasPointer = false;
            }
            s_Fingers.erase(it);
        }
        Recompute(layout);
        return true;
    }

    void TouchControls::Update()
    {
        if (!s_Enabled)
            return;
        InputActions::Inject("MoveX", s_Stick.x);
        InputActions::Inject("MoveY", s_Stick.y);
        InputActions::Inject("Jump", s_Jump ? 1.0f : 0.0f);
        InputActions::Inject("Interact", s_Interact ? 1.0f : 0.0f);
        s_Injected = true;
    }

    void TouchControls::Render()
    {
        if (!s_Enabled || !s_Visible)
            return;

        const glm::vec2 viewport = Renderer::GetViewportSize();
        const float width = viewport.x;
        const float height = viewport.y;
        if (width < 16.0f || height < 16.0f)
            return;
        const Layout layout = MakeLayout(width, height);

        const glm::mat4 previousView = Renderer::GetViewMatrix();
        const glm::mat4 previousProjection = Renderer::GetAPI()->GetProjectionMatrix();
        Renderer::Flush();
        Renderer::SetViewMatrix(glm::mat4(1.0f));
        Renderer::SetProjectionMatrix(glm::ortho(0.0f, width, height, 0.0f, -1000.0f, 1000.0f));

        const float z = 900.0f;
        Renderer::DrawCircle(layout.StickCenter.x, layout.StickCenter.y, layout.StickRadius, {255, 255, 255, 34}, 1.0f, z);
        Renderer::DrawRing(layout.StickCenter.x, layout.StickCenter.y, layout.StickRadius, 3.0f, {255, 255, 255, 90}, 1.0f, z + 0.01f);
        const glm::vec2 knob = layout.StickCenter + glm::vec2(s_Stick.x, -s_Stick.y) * layout.StickRadius;
        Renderer::DrawCircle(knob.x, knob.y, layout.StickRadius * 0.42f, {255, 255, 255, s_Stick != glm::vec2(0.0f) ? static_cast<Uint8>(150) : static_cast<Uint8>(90)}, 1.0f, z + 0.02f);

        auto button = [&](const glm::vec2 &center, bool down, SDL_Color color)
        {
            Renderer::DrawCircle(center.x, center.y, layout.ButtonRadius, {color.r, color.g, color.b, static_cast<Uint8>(down ? 150 : 60)}, 1.0f, z);
            Renderer::DrawRing(center.x, center.y, layout.ButtonRadius, 3.0f, {255, 255, 255, static_cast<Uint8>(down ? 200 : 110)}, 1.0f, z + 0.01f);
        };
        button(layout.JumpCenter, s_Jump, {120, 200, 255, 255});
        button(layout.InteractCenter, s_Interact, {255, 210, 110, 255});

        Renderer::Flush();
        Renderer::SetViewMatrix(previousView);
        Renderer::SetProjectionMatrix(previousProjection);
    }
}

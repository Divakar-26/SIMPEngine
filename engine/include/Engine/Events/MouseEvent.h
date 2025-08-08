#pragma once
#include "Event.h"

namespace SIMPEngine {

    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}

        static EventType StaticType() { return EventType::MouseMoved; }
        EventType GetEventType() const override { return StaticType(); }
        const char* GetName() const override { return "MouseMoved"; }

        float GetX() const { return m_MouseX; }
        float GetY() const { return m_MouseY; }

    private:
        float m_MouseX, m_MouseY;
    };

    class MouseScrolledEvent : public Event {
    public:
        MouseScrolledEvent(float xOffset, float yOffset)
            : m_XOffset(xOffset), m_YOffset(yOffset) {}

        static EventType StaticType() { return EventType::MouseScrolled; }
        EventType GetEventType() const override { return StaticType(); }
        const char* GetName() const override { return "MouseScrolled"; }

        float GetXOffset() const { return m_XOffset; }
        float GetYOffset() const { return m_YOffset; }

    private:
        float m_XOffset, m_YOffset;
    };

    class MouseButtonEvent : public Event {
    public:
        int GetMouseButton() const { return m_Button; }
    protected:
        MouseButtonEvent(int button) : m_Button(button) {}
        int m_Button;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent {
    public:
        MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

        static EventType StaticType() { return EventType::MouseButtonPressed; }
        EventType GetEventType() const override { return StaticType(); }
        const char* GetName() const override { return "MouseButtonPressed"; }
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent {
    public:
        MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

        static EventType StaticType() { return EventType::MouseButtonReleased; }
        EventType GetEventType() const override { return StaticType(); }
        const char* GetName() const override { return "MouseButtonReleased"; }
    };

} // namespace SIMPEngine

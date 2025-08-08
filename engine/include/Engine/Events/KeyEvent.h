#pragma once
#include "Event.h"

namespace SIMPEngine
{
    class KeyEvent : public Event
    {
    public:
        int GetKeyCode() const { return m_KeyCode; }

    protected:
        KeyEvent(int keyCode) : m_KeyCode(keyCode) {}
        int m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(int keycode, bool repeat) : KeyEvent(keycode), m_Repeat(repeat) {}

        static EventType StaticType() { return EventType::KeyPressed; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "KeyPressed"; }

    private:
        bool m_Repeat;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}

        static EventType StaticType() { return EventType::KeyReleased; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "KeyReleased"; }
    };

    class KeyTypedEvent : public KeyEvent
    {
        KeyTypedEvent(int keycode) : KeyEvent(keycode) {}

        static EventType StaticType() { return EventType::KeyTyped; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "KeyTyped"; }
    };
}
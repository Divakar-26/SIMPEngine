#pragma once
#include <Engine/Events/Event.h>

namespace SIMPEngine
{
    //make a parent class KeyEvent which will hold the keycode 
    class KeyEvent : public Event
    {
    public:
        int GetKeyCode() const { return m_KeyCode; }

    protected:
        KeyEvent(int keyCode) : m_KeyCode(keyCode) {}
        int m_KeyCode;
    };

    // inherit it from KeyEvent
    class KeyPressedEvent : public KeyEvent
    {
    public:

        //repeat represent if the key is repeating or not
        KeyPressedEvent(int keycode, bool repeat) : KeyEvent(keycode), m_Repeat(repeat) {}


        static EventType StaticType() { return EventType::KeyPressed; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "KeyPressed"; }
        
        // for the logger
        std::string ToString() const override
        {
            return std::string(GetName()) + ": " + std::to_string(m_KeyCode) +
                   (m_Repeat ? " (repeat)" : "");
        }

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

        std::string ToString() const override
        {
            return std::string(GetName()) + ": " + std::to_string(m_KeyCode);
        }
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(int keycode) : KeyEvent(keycode) {}

        static EventType StaticType() { return EventType::KeyTyped; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "KeyTyped"; }

        std::string ToString() const override
        {
            return std::string(GetName()) + ": " + std::to_string(m_KeyCode);
        }
    };
}
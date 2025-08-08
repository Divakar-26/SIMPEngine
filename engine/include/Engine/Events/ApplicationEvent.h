#pragma once
#include "Event.h"

namespace SIMPEngine
{
    class WindowCloseEvent : public Event
    {
    public:
        static EventType StaticType() { return EventType::WindowClose; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "WindowClose"; }

        std::string ToString() const override
        {
            return std::string(GetName());
        }
    };

    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(int w, int h) : m_Width(w), m_Height(h) {}
        static EventType StaticType() { return EventType::WindowResize; }
        EventType GetEventType() const override { return StaticType(); }
        const char *GetName() const override { return "WindowResize"; }

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }


        std::string ToString() const override
        {
            return std::string(GetName()) + ": " + std::to_string(m_Width) + ", " + std::to_string(m_Height);
        }

    private:
        int m_Width, m_Height;
    };
}
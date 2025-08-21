#pragma once
#include "Event.h"

namespace SIMPEngine
{
    // make a WindowCloseEvent class and inherit it from the parent class Event
    class WindowCloseEvent : public Event
    {
    public:

        //returns the Event name from ENUM
        static EventType StaticType() { return EventType::WindowClose; }

        //return the Name
        EventType GetEventType() const override { return StaticType(); }

        //return the name of the event
        const char *GetName() const override { return "WindowClose"; }

        //for the logger
        std::string ToString() const override
        {
            return std::string(GetName());
        }
    };

    //same as WindowCloseEvent but this time it takes the width and height in the constructor, these values represet the window size
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
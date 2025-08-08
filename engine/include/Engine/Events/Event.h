#pragma once

#include <string>
#include <iostream>

namespace SIMPEngine
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowResize,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled
    };

    class Event
    {
    public:
        virtual EventType GetEventType() const = 0;
        virtual const char *GetName() const = 0;
        virtual std::string ToString() const { return GetName(); }

        bool Handled = false;
    };
} // namespcae SIMPEngine
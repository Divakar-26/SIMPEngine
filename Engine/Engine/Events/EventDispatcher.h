#pragma once
#include <Engine/Events/Event.h>

namespace SIMPEngine
{
    class EventDispatcher
    {
        Event &m_Event;

    public:
        EventDispatcher(Event &e) : m_Event(e) {}

        template <typename T, typename F>
        bool Dispatch(const F &func)
        {
            if (m_Event.GetEventType() == T::StaticType())
            {
                m_Event.Handled = func(static_cast<T &>(m_Event));
                return true;
            }
            return false;
        }
    };
} // namespace SIMPEngine

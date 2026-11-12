#pragma once

namespace SIMPEngine
{
    enum class ScriptEvent
    {
        FixedUpdate,
        TriggerEnter,
        TriggerStay,
        TriggerExit,
        Collision,
        Timer,
        Damaged,
        Death,
        PathPoint,
        PathEnd,
        Landed,
        Jumped,
        Grabbed,
        Dropped,
        Clicked,
        HoverEnter,
        HoverExit,
        Count
    };
}

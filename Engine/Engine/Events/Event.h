#pragma once

#include <string>
#include <iostream>

namespace SIMPEngine
{
    //this is base calss for Events, it defines events like WindowClose, KeyPressed, Etc.
    //using this we can make our event system and hook it with any backend like SDL or GLFW
    enum class EventType
    {
        None = 0, // No event
        WindowClose, // Window Close event
        WindowResize, // Windoe Resize Event
        KeyPressed, //When any keyboard key is pressed then this event will be fired
        KeyReleased, //When any keyboard key is released then this event will be fired
        KeyTyped, // For text input (maybe)
        MouseButtonPressed, // Self explanotary
        MouseButtonReleased, // 
        MouseMoved, // if mouse is moved then this event will fired 
        MouseScrolled // you know the drill
    };

    class Event
    {
    public:
        //all the following methos are virtual so that the class which will be inherint this class will have to define thier own respective methods

        //this returns the Event types
        virtual EventType GetEventType() const = 0;

        //this returns the eventtype's name for example "WindowClose" in string "MouseMoved" etc
        virtual const char *GetName() const = 0;

        //this also return the same thing in string for the logger to use, by defualt it the name of the event, but we can change it like "Window Reised 1280 x 720"
        virtual std::string ToString() const { return GetName(); }

        //this is to make sure the event is not passed to any other layers. if it is handled true then the event will end there.
        bool Handled = false;
    };
} // namespcae SIMPEngine
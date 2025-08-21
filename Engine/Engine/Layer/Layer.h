#pragma once

#include<string>
#include<iostream>

#include"Events/Event.h"
#include"TimeStep.h"
#include<SDL3/SDL_events.h>



namespace SIMPEngine{

    // making all the game , UI, HUD, Pause play will be very hard. so we will seperate all this using layers, each layer will have thier own Render, update, Event function. This is really helpful because we can seperate the responsibilies. Example -> renderingLayer will handel rendering
    // UIlayer -> this will handle the UI of the editor
    // we can stop event at any layer but doing e.handled = true. this will prevent other layer to get the event like. if i press WASD in editor but the game will be not affected because UI layer is always at the top, it will handle the WASD so that lower level do not get them.
    
    // this is base class for Layer
    class Layer{
        public:
        
        Layer(const std::string& name= "Layer") : m_DebugName(name) {}
        virtual ~Layer() = default;

        //all the functions are virtual so that it can be defined as per the layer

        
        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(class TimeStep ts) {}
        virtual void OnRender() {}
        virtual void OnEvent(Event &e) {}
        virtual void OnSDLEvent(SDL_Event &e) {}

        const std::string & GetName() const {return m_DebugName;}

        
        protected:

            //this will be helpful in Editor
            std::string m_DebugName;
    };
}
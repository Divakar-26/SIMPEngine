#pragma once

#include<string>
#include<iostream>
#include"Events/Event.h"
#include"TimeStep.h"
#include<SDL3/SDL_events.h>



namespace SIMPEngine{
    class Layer{
        public:
        Layer(const std::string& name= "Layer") : m_DebugName(name) {}
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(class TimeStep ts) {}
        virtual void OnRender() {}
        virtual void OnEvent(Event &e) {}
        virtual void OnSDLEvent(SDL_Event &e) {}

        const std::string & GetName() const {return m_DebugName;}

        
        protected:
            std::string m_DebugName;
    };
}
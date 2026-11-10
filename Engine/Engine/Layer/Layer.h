#pragma once

#include<string>
#include<iostream>

#include<Engine/Events/Event.h>
#include<Engine/Core/TimeStep.h>
#include<SDL3/SDL_events.h>

namespace SIMPEngine{
    class Layer{
        public:

        Layer(const std::string& name= "Layer") : m_DebugName(name) {}
        virtual ~Layer() = default;

        virtual void OnAttach() {}

        virtual void OnDetach() {}

        virtual void OnUpdate(class TimeStep) {}

        virtual void OnRender() {}

        virtual void OnEvent(Event &) {}

        virtual void OnSDLEvent(SDL_Event &) {}

        const std::string & GetName() const {return m_DebugName;}

        protected:

            std::string m_DebugName;
    };
}

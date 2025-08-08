#pragma once

#include<string>
#include<iostream>
#include"Engine/Events/Event.h"

namespace SIMPEngine{
    class Layer{
        public:
        Layer(const std::string& name= "Layer") : m_DebugName(name) {}
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate() {}
        virtual void OnEvent(Event &e) {}

        const std::string & GetName() const {return m_DebugName;}

        
        protected:
            std::string m_DebugName;
    };
}
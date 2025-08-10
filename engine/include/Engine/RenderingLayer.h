#pragma once

#include "Engine/Layer.h"
#include "Engine/Renderer.h"
#include "Engine/Events/Event.h"
#include"Engine/TimeStep.h"
#include <SDL3/SDL_pixels.h> // for SDL_Color

namespace SIMPEngine
{
    class RenderingLayer : public Layer
    {
    public:
        RenderingLayer() 
            : Layer("RenderingLayer") {}
        ~RenderingLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(class TimeStep ts) override;
        void OnRender() override;
        void OnEvent(Event &e) override;
    };
}

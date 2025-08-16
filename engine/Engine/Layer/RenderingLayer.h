#pragma once

#include "Layer/Layer.h"
#include "Rendering/Renderer.h"
#include "Events/Event.h"
#include "TimeStep.h"
#include <SDL3/SDL_pixels.h>

#include "Math/Camera2D.h"
#include "Scene/Scene.h"

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

        Camera2D &GetCamera() { return m_Camera; }
        const Camera2D &GetCamera() const { return m_Camera; }

    private:
        Camera2D m_Camera;
        Scene m_Scene; 
    };
}

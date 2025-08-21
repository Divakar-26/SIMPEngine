#pragma once

#include "Layer/Layer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Animation.h"
#include "Events/Event.h"
#include "TimeStep.h"
#include <SDL3/SDL_pixels.h>

#include "Math/Camera2D.h"
#include "Scene/Scene.h"

#include <queue>
#include <functional>

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

        Camera2D &GetCamera() { return m_Scene.GetActiveCamera(); }

        Scene &GetScene() { return m_Scene; }
        const Scene &GetScene() const { return m_Scene; }

        using SceneCommand = std::function<void(Scene &)>;


        std::pair<int, int> GetViewportSize() { return m_Scene.GetMainCamera().GetViewportSize(); }


    private:
        Scene m_Scene;

        std::queue<SceneCommand> m_CommandQueue;
        Animation *anim = nullptr;
    };
}

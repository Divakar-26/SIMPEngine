#pragma once

#include "Layer/Layer.h"
#include "Rendering/Renderer.h"
#include "Rendering/Animation.h"
#include "Events/Event.h"
#include "Core/TimeStep.h"
#include <SDL3/SDL_pixels.h>

#include "Math/Camera2D.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include <queue>
#include <functional>

namespace SIMPEngine
{
    class RenderingLayer : public Layer
    {
    public:
        RenderingLayer(std::shared_ptr<SceneManager> sceneManager)
            : Layer("RenderingLayer"), m_SceneManager(sceneManager) {}

        ~RenderingLayer() override = default;

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(class TimeStep ts) override;
        void OnRender() override;
        void OnEvent(Event &e) override;

        Camera2D &GetCamera() { return m_SceneManager->GetActiveScene()->GetActiveCamera(); }

        Scene &GetScene() { return *m_SceneManager->GetActiveScene(); }
        const Scene &GetScene() const { return *m_SceneManager->GetActiveScene(); }

        std::pair<int, int> GetViewportSize()
        {
            return m_SceneManager->GetActiveScene()->GetMainCamera().GetViewportSize();
        }

    private:
        Scene m_Scene;
        std::shared_ptr<SceneManager> m_SceneManager;

        Animation *anim = nullptr;
    };
}

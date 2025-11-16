#pragma once

#include <Engine/Layer/Layer.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/Animation.h>
#include <Engine/Events/Event.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Math/Camera2D.h>

#include <SDL3/SDL_pixels.h>
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

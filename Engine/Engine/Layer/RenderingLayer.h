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

#include <string>
#include <utility>

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

        void SetRenderToBackbuffer(bool enabled) { m_RenderToBackbuffer = enabled; }
        void SetSimulationEnabled(bool enabled) { m_SimulationEnabled = enabled; }
        void SetReloadScene(const std::string &path) { m_ReloadScenePath = path; }
        bool IsSimulationEnabled() const { return m_SimulationEnabled; }
        void Step(float dt);

        bool HasFault() const { return !m_Fault.empty(); }
        std::string TakeFault() { return std::exchange(m_Fault, std::string()); }

    private:
        void UpdateScene(Scene &scene, float dt);
        void RecordFault(const char *stage, const char *what);

        std::string m_Fault;
        std::string m_LastFault;
        std::shared_ptr<SceneManager> m_SceneManager;
        bool m_SimulationEnabled = true;
        bool m_RenderToBackbuffer = true;
        std::string m_ReloadScenePath;
    };
}

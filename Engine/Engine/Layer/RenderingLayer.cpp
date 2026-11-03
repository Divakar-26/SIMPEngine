#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/Loaders/TiledLoader.h>
#include <Engine/Core/EngineDimensions.h>
#include <Engine/PCH.h>

#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Events/MouseEvent.h"
#include "Input/Input.h"
#include "Input/SIMP_Keys.h"

namespace SIMPEngine
{
    class WindowResizeEvent;

    void RenderingLayer::OnAttach()
    {
        srand(time(NULL));
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(TimeStep ts)
    {
        PROFILE_SCOPE("RenderingLayer/Update");

        auto scene = m_SceneManager->GetActiveScene();

        if (!scene)
            return;

        if (Input::IsKeyPressed(SIMPK_F1))
        {
            SceneSerializer s(scene.get());
            scene->Clear();
            s.Deserialize("assets://scenes/Level1.yaml");
        }

        if (m_SimulationEnabled)
            scene->OnUpdate(ts.GetSeconds());
    }

    void RenderingLayer::Step(float dt)
    {
        auto scene = m_SceneManager->GetActiveScene();
        if (scene)
            scene->OnUpdate(dt);
    }

    void RenderingLayer::OnRender()
    {
        PROFILE_SCOPE("RenderingLayer/Render");
        if (!m_RenderToBackbuffer)
            return;

        auto scene = m_SceneManager->GetActiveScene();
        if (!scene)
            return;

        auto &camera = scene->GetActiveCamera();
        auto [cw, ch] = camera.GetViewportSize();
        if (cw != Renderer::m_WindowWidth || ch != Renderer::m_WindowHeight)
            camera.SetViewportSize((float)Renderer::m_WindowWidth, (float)Renderer::m_WindowHeight);

        const glm::vec4 &bg = scene->GetBackgroundColor();
        Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
        Renderer::Clear();

        scene->OnRender();
        Renderer::Flush();
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &ev)
                                                     { return false; });

        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent &ev)
                                               {
        int width = ev.GetWidth();
        int height = ev.GetHeight();

        Renderer::GetAPI()->ResizeViewport(width, height);
        EngineDimensions::SetDimensions(width, height);

        auto &camera = m_SceneManager->GetActiveScene()->GetActiveCamera();
        camera.SetViewportSize(width, height);

        CORE_INFO("Viewport resized: {}x{}", width, height);
        return false; });
    }
}

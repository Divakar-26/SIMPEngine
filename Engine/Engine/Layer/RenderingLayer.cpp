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
        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
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

        scene->OnUpdate(ts.GetSeconds());
    }

    void RenderingLayer::OnRender()
    {
        PROFILE_SCOPE("RenderingLayer/Render");
        auto scene = m_SceneManager->GetActiveScene();

        if (scene)
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

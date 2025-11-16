#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/PCH.h>

float kl = 0.0f;

namespace SIMPEngine
{
    void RenderingLayer::OnAttach()
    {

        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        srand(time(NULL));

        int w = SIMPEngine::Renderer::m_WindowWidth;
        int h = SIMPEngine::Renderer::m_WindowHeight;

        // making scene
        auto scene1 = std::make_shared<SIMPEngine::Scene>("Level1");
        m_SceneManager->AddScene("Level1", scene1);
        m_SceneManager->SetActiveScene("Level1");


    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(class TimeStep ts)
    {
        auto e = m_SceneManager->GetActiveScene()->GetEntityByName("Player");
        
    }

    void RenderingLayer::OnRender()
    {
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
        
        // Update all primary cameras with new viewport size
        auto scene = m_SceneManager->GetActiveScene();
        if (scene)
        {
            auto view = scene->GetRegistry().view<CameraComponent>();
            for (auto entity : view)
            {
                auto &camComp = view.get<CameraComponent>(entity);
                if (camComp.primary)
                {
                    // camComp.Camera.SetViewportSize(width, height);
                }
            }
        }

        CORE_INFO("Viewport resized: {}x{}", width, height);
        return false; });
    }

}

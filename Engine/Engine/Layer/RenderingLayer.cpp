#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/PCH.h>

float kl = 0.0f;
float roatation = 0.0f;

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

    void RenderingLayer::OnUpdate(TimeStep ts)
    {
        auto camera = m_SceneManager->GetActiveScene()->GetActiveCamera();

        glm::vec2 rawMouse(Input::GetMousePosition().first,
                           Input::GetMousePosition().second);

        int windowW = Renderer::m_WindowWidth;
        int windowH = Renderer::m_WindowHeight;

        // Convert raw pixels → your centered coordinate system
        glm::vec2 screen;
        screen.x = rawMouse.x - windowW * 0.5f;
        screen.y = (windowH * 0.5f) - rawMouse.y;

        // Now convert screen → world
        glm::vec2 world = camera.ScreenToWorld(screen);
        if (Input::IsMouseButtonPressed(1))
        {
            std::cout << "Mouse World = " << world.x << " " << world.y << std::endl;
        }
    }

    void RenderingLayer::OnRender()
    {
        auto scene = m_SceneManager->GetActiveScene();

        if (scene)
            scene->OnRender();

        Renderer::DrawQuad(0, 0, 200, 200);
        Renderer::DrawQuad(0, 0, 1280, 720, 0.0, {255, 0, 0, 255});

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

        CORE_INFO("Viewport resized: {}x{}", width, height);
        return false; });
    }

}

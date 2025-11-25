#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/PCH.h>

float kl = 0.0f;
float roatation = 0.0f;

namespace SIMPEngine
{
    void RenderingLayer::OnAttach()
    {   
        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        srand(time(NULL));

        // auto scene = std::make_shared<SIMPEngine::Scene>("Level1");
        // m_SceneManager->AddScene("Level1", scene);
        // m_SceneManager->SetActiveScene("Level1");

        // // Spawn an example entity
        // Entity entity = scene->CreateEntity("TestQuad");
        // auto &transform = entity.GetComponent<TransformComponent>();
        // transform.position = {0.0f, 0.0f}; // Centered
        // transform.scale = {1.0f, 1.0f};
        // transform.rotation = 0.0f;
        // transform.zIndex = 0.0f;

        // auto &render = entity.AddComponent<RenderComponent>();
        // render.width = 200.0f;
        // render.height = 200.0f;
        // render.color = {0, 255, 0, 255}; // Green

        // auto & camera = entity.AddComponent<CameraComponent>();
        // camera.primary = true;
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(TimeStep ts)
    {
        // auto &camera = m_SceneManager->GetActiveScene()->GetActiveCamera();


        // glm::vec2 rawMouse(Input::GetMousePosition().first,
        //                    Input::GetMousePosition().second);

        // int windowW = Renderer::m_WindowWidth;
        // int windowH = Renderer::m_WindowHeight;

        // // Convert raw pixels → your centered coordinate system
        // glm::vec2 screen;
        // screen.x = rawMouse.x - windowW * 0.5f;
        // screen.y = (windowH * 0.5f) - rawMouse.y;



        auto scene = m_SceneManager->GetActiveScene();
        if(scene){
            scene->OnUpdate(ts.GetSeconds());
        }

        SceneSerializer s(scene.get());

        if(Input::IsKeyPressed(SIMPK_F1)){
            scene->Clear();
            s.Deserialize("assets://scenes/Level1.yaml");
        }

        // glm::vec2 world = camera.ScreenToWorld(screen);
        // if (Input::IsMouseButtonPressed(1))
        // {
        //     std::cout << "Mouse World = " << world.x << " " << world.y << std::endl;
        // }
        // if (Input::IsKeyPressed(SIMPK_LEFT))
        // {
        //     camera.Move({-200.0f * ts.GetSeconds(), 0});
        //     std::cout << "Camera moved" << std::endl;
        // }

        // Entity entity = scene->GetEntityByName("TestQuad");
        // auto & t = entity.GetComponent<TransformComponent>();
        // t.position.x += 200.0 * ts.GetSeconds();

        // Renderer::SetViewMatrix(camera.GetViewMatrix());
    }

    void RenderingLayer::OnRender()
    {
        auto scene = m_SceneManager->GetActiveScene();

        if (scene)
            scene->OnRender();

        // // Renderer::DrawQuad(0, 0, 200, 200);
        // Renderer::DrawQuad(0, 0, 1280, 720, 0.0, {255, 0, 0, 255});

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

        // auto &camera = m_SceneManager->GetActiveScene()->GetActiveCamera();
        // camera.SetViewportSize(width, height);

        CORE_INFO("Viewport resized: {}x{}", width, height);
        return false; });
    }

}

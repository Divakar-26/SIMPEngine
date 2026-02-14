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

        auto scene = m_SceneManager->GetActiveScene();
        for (int i = 0; i < 100000; i++)
        {
            Entity e = scene->CreateEntity("test");

            auto &t =
                e.GetComponent<TransformComponent>();

            t.position.x = 100;
            t.position.y = 100;

            auto &r = e.AddComponent<RenderComponent>();

            r.width = 40;
            r.height = 40;
            r.color = {255, 0, 0, 255};
            static int count = 0;
            CORE_ERROR("{}", count++);
        }

        // auto scene = std::make_shared<SIMPEngine::Scene>("Level1");
        // m_SceneManager->AddScene("Level1", scene);
        // m_SceneManager->SetActiveScene("Level1");
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
        if (scene)
        {
            scene->OnUpdate(ts.GetSeconds());
        }

        SceneSerializer s(scene.get());

        if (Input::IsKeyPressed(SIMPK_F1))
        {
            scene->Clear();
            s.Deserialize("assets://scenes/Level1.yaml");
        }

        // glm::vec2 world = camera.ScreenToWorld(screen);
        if (Input::IsMouseButtonPressed(1))
        {
            auto pos = Input::GetMousePosition();
            glm::vec2 cor = {pos.first, pos.second};
            auto world = scene->GetActiveCamera().ScreenToWorld(cor);
            std::cout << world.x << " " << world.y << std::endl;

            for (int i = 0; i < 1000; i++)
            {
                Entity e = scene->CreateEntity("test");

                auto &t =
                    e.GetComponent<TransformComponent>();

                t.position.x = world.x;
                t.position.y = world.y;

                auto &r = e.AddComponent<RenderComponent>();

                r.width = 40;
                r.height = 40;
                r.color = {255, 0, 0, 255};
                static int count = 0;
                CORE_ERROR("{}", count++);
            }
            // auto & p = e.AddComponent<PhysicsComponent>();

            // p.body = new AccelEngine::RigidBody();
            // p.body->shapeType = AccelEngine::ShapeType::AABB;
            // p.body->aabb.halfSize = {20, 20};
            // p.body->position = {world.x, world.y};
            // p.body->rotation = 0.0;
            // p.body->inverseMass = 1;
            // p.body->restitution = 1;
            // p.body->calculateInertia();

            // scene->physicsWorld.addBody(p.body);
            // scene->bodies.push_back(p.body);
        }

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

        int width = Renderer::m_WindowWidth;
        int height = Renderer::m_WindowHeight;

        // CORE_ERROR("{} {}", width, height);
        Renderer::DrawQuad(0, 0, width, height, 0.0f, {255, 255, 255, 255}, false, 0);

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

        auto &camera = m_SceneManager->GetActiveScene()->GetActiveCamera();
        camera.SetViewportSize(width, height);

        CORE_INFO("Viewport resized: {}x{}", width, height);
        return false; });
    }

}

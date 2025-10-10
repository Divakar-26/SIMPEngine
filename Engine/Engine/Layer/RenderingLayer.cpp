#include "PCH.h"

#include "Layer/RenderingLayer.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

float kl = 0.0f;

namespace SIMPEngine
{
    void RenderingLayer::OnAttach()
    {

        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        srand(time(NULL));

        int w = SIMPEngine::Renderer::m_WindowWidth;
        int h = SIMPEngine::Renderer::m_WindowHeight;

        auto scene1 = std::make_shared<SIMPEngine::Scene>("Level1");
        m_SceneManager->AddScene("Level1", scene1);
        m_SceneManager->SetActiveScene("Level1");

        Entity e = m_SceneManager->GetActiveScene()->CreateEntity("Player");
        auto &transform = e.GetComponent<TransformComponent>();
        transform.position = {0.0f, 0.0f};
        transform.scale = {1.0f, 1.0f};
        transform.zIndex = 0.0f;

        auto &render = e.AddComponent<RenderComponent>();
        render.width = 100.0f;
        render.height = 100.0f;
        render.color = SDL_Color{255, 0, 0, 255};

        auto &physics = e.AddComponent<PhysicsComponent>();

        auto &camera = e.AddComponent<CameraComponent>(1.0f, glm::vec2(w / 2.0f, h / 2.0f));
        camera.primary = false;

        auto &vel = e.AddComponent<VelocityComponent>();
        vel.vx = 100.0f;

        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position.Set(transform.position.x, transform.position.y);
        physics.body = m_SceneManager->GetActiveScene()->physicsSystem.world.CreateBody(&bodyDef);

        b2PolygonShape boxShape;
        boxShape.SetAsBox(render.width / 2.0f, render.height / 2.0f);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &boxShape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.3f;
        physics.body->CreateFixture(&fixtureDef);

        // entity two
        Entity bottomWall = m_SceneManager->GetActiveScene()->CreateEntity("WALL");
        auto &transform2 = bottomWall.GetComponent<TransformComponent>();

        transform2.position = {0.0f, 500.0f};
        transform2.rotation = 0.0f;
        transform2.scale = {1.0f, 1.0f};
        transform2.zIndex = 0.0f;

        auto &render3 = bottomWall.AddComponent<RenderComponent>();
        render3.width = 1920.0f;
        render3.height = 100.0f;
        render3.color = SDL_Color{0, 0, 255, 255};

        auto & physics2 = bottomWall.AddComponent<PhysicsComponent>();

        b2BodyDef wallShape;
        wallShape.type = b2_staticBody;
        wallShape.position.Set(transform2.position.x, transform2.position.y);
        physics2.body = m_SceneManager->GetActiveScene()->physicsSystem.world.CreateBody(&wallShape);

        b2PolygonShape wallPShape;
        wallPShape.SetAsBox(render3.width / 2.0f, render3.height / 2.0f);

        b2FixtureDef wallFix;
        wallFix.shape = &wallPShape;
        wallFix.density = 1.0f;
        wallFix.friction = 0.3f;
        physics2.body->CreateFixture(&wallFix);

        Entity e2 = m_SceneManager->GetActiveScene()->CreateEntity("Player2");
        auto &t1 = e2.GetComponent<TransformComponent>();
        t1.position = {200.0f, 500.0f};
        t1.scale = {1.0f, 1.0f};
        t1.zIndex = 0.0f;

        auto &render2 = e2.AddComponent<RenderComponent>();
        render2.width = 50.0f;
        render2.height = 50.0f;
        render2.color = SDL_Color{0, 255, 0, 255};

        CORE_INFO("WIDTH : {}, HEIGHT : {}", camera.Camera.GetPosition().x, camera.Camera.GetPosition().y);
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(class TimeStep ts)
    {
        auto e = m_SceneManager->GetActiveScene()->GetEntityByName("Player");
        if(Input::IsKeyPressed(SIMPK_RIGHT)){
            e.GetComponent<TransformComponent>().position.x += 5.0f;
            CORE_INFO("{}", e.GetComponent<TransformComponent>().position.x);
        }

        auto scene = m_SceneManager->GetActiveScene();
        if (scene)
            scene->OnUpdate(ts.GetSeconds());

        kl += ts.GetSeconds() * 90.0f;
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
                camComp.Camera.SetViewportSize(width, height);
            }
        }
    }

    CORE_INFO("Viewport resized: {}x{}", width, height);
    return false; });
    }

}
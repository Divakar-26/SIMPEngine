#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/TilemapUtils.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/Loaders/TiledLoader.h>
#include <Engine/Core/ProfilingUtils.h>
#include <Engine/Core/EngineDimensions.h>
#include <Engine/PCH.h>

namespace SIMPEngine
{
    std::shared_ptr<Texture> m_TestGrass;

    void RenderingLayer::OnAttach()
    {
        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        srand(time(NULL));

        auto scene = m_SceneManager->GetActiveScene();
        // for (int i = 0; i < 100000; i++)
        // {
        //     Entity e = scene->CreateEntity("test");

        //     auto &t =
        //         e.GetComponent<TransformComponent>();

        //     t.position.x = 100;
        //     t.position.y = 100;

        //     auto &r = e.AddComponent<RenderComponent>();

        //     r.width = 40;
        //     r.height = 40;
        //     r.color = {255, 0, 0, 255};
        //     static int count = 0;
        //     CORE_ERROR("{}", count++);
        // }

        // ==================== HARDCODED TEST CODE ====================
        // Entity parent = scene->CreateEntity("parent");
        // Entity child = scene->CreateEntity("child");

        // scene->SetParent(child, parent);

        // child.GetComponent<TransformComponent>().position = {50, 60};
        // auto &cRC = child.AddComponent<RenderComponent>();
        // cRC.color = {0, 0, 255, 255};
        // cRC.width = 100;
        // cRC.height = 100;

        // auto &rc = parent.AddComponent<RenderComponent>();
        // rc.color = {255, 255, 0, 255};
        // rc.width = 100;
        // rc.height = 100;

        // auto &tc = parent.GetComponent<TransformComponent>();
        // tc.position = {300, -300};

        // auto &vc = parent.AddComponent<VelocityComponent>();
        // vc.vx = 100;

        // auto &life = parent.AddComponent<LifetimeComponent>();
        // life.remaining = 2.0f;

        // Entity map = scene->CreateEntity("Tilemap");

        // auto size = Renderer::GetViewportSize();

        // auto &tm = map.AddComponent<TilemapComponent>();
        // auto &tr = map.GetComponent<TransformComponent>();

        m_TestGrass = TextureManager::Get().LoadTexture(
            "assets://textures/Grass.png",
            "assets://textures/Grass.png");

        // tm.tileset = std::make_shared<Tileset>(texture, 16, 16);
        // tm.tileSize = 64.0f;

        // // Create tiles in negative and positive space
        // SetTile(tm, 0, 0, 1);
        // SetTile(tm, -1, 0, 1);
        // // SetTile(tm, 0,1,2);
        // // SetTile(tm, -1,0,2);
        // // SetTile(tm, -1,-1,1);
        // // auto scene = std::make_shared<SIMPEngine::Scene>("Level1");
        // // m_SceneManager->AddScene("Level1", scene);
        // // m_SceneManager->SetActiveScene("Level1");
        // ============================================================
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(TimeStep ts)
    {
        // glm::vec2 rawMouse(Input::GetMousePosition().first,
        //                    Input::GetMousePosition().second);

        // int windowW = Renderer::m_WindowWidth;
        // int windowH = Renderer::m_WindowHeight;

        // // Convert raw pixels → your centered coordinate system
        // glm::vec2 screen;
        // screen.x = rawMouse.x - windowW * 0.5f;
        // screen.y = (windowH * 0.5f) - rawMouse.y;

        auto scene = m_SceneManager->GetActiveScene();

        SceneSerializer s(scene.get());

        if (Input::IsKeyPressed(SIMPK_F1))
        {
            scene->Clear();
            s.Deserialize("assets://scenes/Level1.yaml");
        }

        // glm::vec2 world = camera.ScreenToWorld(screen);
        // if (Input::IsMouseButtonPressed(1))
        // {
        //     auto scene = m_SceneManager->GetActiveScene();

        //     auto pos = Input::GetMousePosition();
        //     glm::vec2 world = scene->GetActiveCamera()
        //                           .ScreenToWorld({pos.first, pos.second});

        //     Entity e = scene->BuildEntity("Box")
        //                    .At(world.x, world.y)
        //                    .With<RenderComponent>();

        //     auto &rc = e.GetComponent<RenderComponent>();
        //     rc.width = 64;
        //     rc.height = 64;
        //     float r = rand() % 255;
        //     float g = rand() % 255;
        //     float b = rand() % 255;
        //     rc.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
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
        if (scene)
        {
            scene->OnUpdate(ts.GetSeconds());
        }
    }

    void RenderingLayer::OnRender()
    {
        PROFILE_RENDER_SYSTEM();

        auto scene = m_SceneManager->GetActiveScene();

        if (scene)
            scene->OnRender();

        int width = Renderer::m_WindowWidth;
        int height = Renderer::m_WindowHeight;

        // CORE_ERROR("{} {}", width, height);
        Renderer::DrawQuad(0, 0, width, height, 0.0f, {255, 255, 255, 255}, false, 0);

        if (m_TestGrass)
        {
            Renderer::DrawTexture(
                m_TestGrass,
                100,
                100,
                128,
                128, SDL_Color{255, 0,0,255}, 0, 1);
        }

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

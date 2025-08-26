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

        auto myTexture = TextureManager::Get().GetTexture("man");

        Entity entity1 = m_Scene.CreateEntity("RedBox");
        entity1.GetComponent<TransformComponent>().position.x = 500.0f;
        entity1.GetComponent<TransformComponent>().position.y = 500.0f;
        entity1.AddComponent<TagComponent>("RedBox");
        entity1.AddComponent<SpriteComponent>(myTexture, 100.0f, 100.0f);
        auto &cam = entity1.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f, 0.0f));
        // cam.primary = true;

        auto &col = entity1.AddComponent<CollisionComponent>();
        col.width = 100.0f;
        col.height = 100.0f;

        auto &vel = entity1.AddComponent<VelocityComponent>();
        vel.vx = 0.0f;
        vel.vy = 0.0f;


        std::vector<Sprite> frames;
        auto it = TextureManager::Get().GetTexture("coin");

        for(int i = 0; i < 12; i++){
            
            frames.emplace_back(Sprite(it, SDL_FRect{(float)(i * 16.0f), 0.0f, 16.0f, 16.0f }));
        }

        // frames.push_back(new Sprite(it->GetSDLTexture(), {0,0,16,16}));
        anim = new Animation(frames, 0.2f, true);
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(class TimeStep ts)
    {
        while (!m_CommandQueue.empty())
        {
            auto command = m_CommandQueue.front();
            command(m_Scene);
            m_CommandQueue.pop();
        }

        m_Scene.OnUpdate(ts.GetSeconds());

        auto &vel = m_Scene.GetEntityByName("RedBox").GetComponent<VelocityComponent>();

        if(m_Scene.GetRegistry().any_of<VelocityComponent>(m_Scene.GetEntityByName("RedBox"))){

            int sprint;
            if(Input::IsKeyPressed(SDLK_LSHIFT)){
                sprint = 1000.0f;
            }
            if(!Input::IsKeyPressed(SDLK_LSHIFT)){
                sprint = 0.0f;
            }

            if (Input::IsKeyPressed(SDLK_UP))
                vel.vy = -500.0f + -sprint;
            else if (Input::IsKeyPressed(SDLK_DOWN))
                vel.vy = 500.0f + sprint;
            else
                vel.vy = 0.0f;

            if (Input::IsKeyPressed(SDLK_LEFT))
                vel.vx = -500.0f + -sprint;
            else if (Input::IsKeyPressed(SDLK_RIGHT))
                vel.vx = 500.0f + sprint;
            else
                vel.vx = 0.0f;
        }

        kl += ts.GetSeconds() * 90.0f;
        anim->Update(ts.GetSeconds());
    }

    void RenderingLayer::OnRender()
    {
        m_Scene.OnRender();
        anim->Draw(500,500,300,300,SDL_Color{255,255,255,255});

        // auto cointex = TextureManager::Get().GetTexture("coin");

        // SDL_FRect rect;
        // rect.x = 0;
        // rect.y = 0;
        // rect.w = 16;
        // rect.h = 16;

        // Renderer::DrawTexture(cointex->GetSDLTexture(), 100, 100, 100, 100, SDL_Color{255, 255, 255, 255}, 45.0f, &rect);

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
    // m_Camera.SetViewportSize(width, height);

    CORE_INFO("Viewport resized: {}x{}", width, height);
    return false;
        return false; });
    }

}

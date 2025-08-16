#include "Layer/RenderingLayer.h"

#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Log.h"
#include "Input/Input.h"
#include <SDL3/SDL_keycode.h>

#include "Rendering/TextureManager.h"
#include "Rendering/Sprite.h"
#include "Rendering/Animation.h"
#include "Rendering/SpriteAtlas.h"
#include "Scene/Entity.h"

#include <iostream>


float kl = 0.0f;

namespace SIMPEngine
{
    void RenderingLayer::OnAttach()
    {

        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        srand(time(NULL));

        Entity entity1 = m_Scene.CreateEntity("RedBox");
        entity1.AddComponent<RenderComponent>(100.0f, 100.0f, SDL_Color{255, 0, 0, 255});
        entity1.GetComponent<TransformComponent>().x = 500.0f;
        entity1.GetComponent<TransformComponent>().y = 500.0f;
        entity1.AddComponent<TagComponent>("RedBox");

        auto &col = entity1.AddComponent<CollisionComponent>();
        col.width = 100.0f;
        col.height = 100.0f;

        auto &vel = entity1.AddComponent<VelocityComponent>();
        vel.vx = 0.0f;
        vel.vy = 0.0f;

        Entity entity2 = m_Scene.CreateEntity("BlueBox");
        entity2.AddComponent<RenderComponent>(100.0f, 100.0f, SDL_Color{0, 0, 255, 255});
        entity2.GetComponent<TransformComponent>().x = 700.0f;
        entity2.GetComponent<TransformComponent>().y = 700.0f;
        entity2.AddComponent<TagComponent>("BlueBox");

        auto &col2 = entity2.AddComponent<CollisionComponent>();
        col2.width = 100.0f;
        col2.height = 100.0f;
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(class TimeStep ts)
    {
        m_Scene.OnUpdate(ts.GetSeconds());
        
        m_Camera.Update(ts.GetSeconds());
        
        auto &vel = m_Scene.GetEntityByName("RedBox").GetComponent<VelocityComponent>();

        if (Input::IsKeyPressed(SDLK_UP))
            vel.vy = -100.0f;
        else if (Input::IsKeyPressed(SDLK_DOWN))
            vel.vy = 100.0f;
        else
            vel.vy = 0.0f;

        if (Input::IsKeyPressed(SDLK_LEFT))
            vel.vx = -100.0f;
        else if (Input::IsKeyPressed(SDLK_RIGHT))
            vel.vx = 100.0f;
        else
            vel.vx = 0.0f;

        Renderer::SetViewMatrix(m_Camera.GetViewMatrix());
        
        kl += ts.GetSeconds() * 90.0f;
    }

    void RenderingLayer::OnRender()
    {
        m_Scene.OnRender();
        
        Renderer::DrawQuad(50, 50, 200, 150, SDL_Color{255, 0, 0, 255});

        auto cointex = TextureManager::Get().GetTexture("coin");
        SDL_FRect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = 16;
        rect.h = 16;
        Renderer::DrawTexture(cointex->GetSDLTexture(), 100 + kl, 100, 100, 100, SDL_Color{255, 255, 255, 255}, 45.0f, &rect);
        // Renderer::Present();
        Renderer::Flush();
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        m_Camera.OnEvent(e);
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &ev)
                                                     { return false; });
    }

}

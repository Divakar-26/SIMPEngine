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
    }

    void RenderingLayer::OnDetach()
    {
    }

    void RenderingLayer::OnUpdate(class TimeStep ts)
    {
        if (Input::IsKeyPressed(SDLK_A))
        {
            auto pos = m_Camera.GetPosition();
            m_Camera.SetPosition({pos.x - 100.0f * ts.GetSeconds(), pos.y});
        }
        if (Input::IsKeyPressed(SDLK_D))
        {
            auto pos = m_Camera.GetPosition();
            m_Camera.SetPosition({pos.x + 100.0f * ts.GetSeconds(), pos.y});
        }
        if (Input::IsKeyPressed(SDLK_W))
        {
            auto pos = m_Camera.GetPosition();
            m_Camera.SetPosition({pos.x, pos.y - 100.0f * ts.GetSeconds()});
        }
        if (Input::IsKeyPressed(SDLK_S))
        {
            auto pos = m_Camera.GetPosition();
            m_Camera.SetPosition({pos.x, pos.y + 100.0f * ts.GetSeconds()});
        }
        float zoomSpeed = 1.0f;
        if (Input::IsKeyPressed(SDLK_Q))
        {
            float zoom = m_Camera.GetZoom();
            zoom -= zoomSpeed * ts.GetSeconds();
            if (zoom < 0.1f)
                zoom = 0.1f;
            m_Camera.SetZoom(zoom);
        }
        if (Input::IsKeyPressed(SDLK_E))
        {
            float zoom = m_Camera.GetZoom();
            zoom += zoomSpeed * ts.GetSeconds();
            if (zoom > 5.0f)
                zoom = 5.0f;
            m_Camera.SetZoom(zoom);
        }

        if(Input::IsKeyPressed(SDLK_UP)){
            m_Scene.GetEntityByName("RedBox").GetComponent<TransformComponent>().y -= ts.GetSeconds() * 100.0f;
        }
        if(Input::IsKeyPressed(SDLK_DOWN)){
            m_Scene.GetEntityByName("RedBox").GetComponent<TransformComponent>().y += ts.GetSeconds() * 100.0f;
        }
        if(Input::IsKeyPressed(SDLK_LEFT)){
            m_Scene.GetEntityByName("RedBox").GetComponent<TransformComponent>().x -= ts.GetSeconds() * 100.0f;
        }
        if(Input::IsKeyPressed(SDLK_RIGHT)){
            m_Scene.GetEntityByName("RedBox").GetComponent<TransformComponent>().x += ts.GetSeconds() * 100.0f;
        }

        Renderer::SetViewMatrix(m_Camera.GetViewMatrix());
        m_Scene.OnUpdate(ts.GetSeconds());
    }

    void RenderingLayer::OnRender()
    {
        // Set viewport to desired area, e.g., center 800x600 in a 1280x720 window:

        Renderer::Clear();

        m_Scene.OnRender();
        Renderer::DrawQuad(50, 50, 200, 150, SDL_Color{255, 0, 0, 255});

        auto cointex = TextureManager::Get().GetTexture("coin");
        SDL_FRect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = 16;
        rect.h = 16;
        Renderer::DrawTexture(cointex->GetSDLTexture(), 100, 100, 100, 100, SDL_Color{255, 255, 255, 255}, 45.0f, &rect);
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &ev)
                                                     { return false; });
    }

}

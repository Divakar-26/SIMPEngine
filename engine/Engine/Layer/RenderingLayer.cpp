#include "Layer/RenderingLayer.h"

#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"
#include "Events/ApplicationEvent.h"
#include "Events/EventDispatcher.h"
#include "Log.h"
#include "Input/Input.h"
#include <SDL3/SDL_keycode.h>

#include"Rendering/TextureManager.h"

#include <iostream>

float i = 1.0f;
float angle = 0.0f;

namespace SIMPEngine
{
    void RenderingLayer::OnAttach()
    {
        // Set up anything needed before rendering starts
        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    }

    void RenderingLayer::OnDetach()
    {
        // Clean up if needed
    }

    void RenderingLayer::OnUpdate(class TimeStep ts)
    {
        i += 50.0 * ts.GetSeconds();

        if (Input::IsMouseButtonPressed(SDL_BUTTON_LEFT))
        {
            i -= 50.0 * ts.GetSeconds();
        }
        else if (Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT))
        {
            auto cor = Input::GetMousePosition();
            i = cor.first - 50;
        }

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
            m_Camera.SetPosition({pos.x, pos.y  - 100.0f * ts.GetSeconds()});
        }
        if (Input::IsKeyPressed(SDLK_S))
        {
            auto pos = m_Camera.GetPosition();
            m_Camera.SetPosition({pos.x , pos.y + 100.0f * ts.GetSeconds()});
        }
        float zoomSpeed = 1.0f;
        if (Input::IsKeyPressed(SDLK_Q))
        {
            float zoom = m_Camera.GetZoom();
            zoom -= zoomSpeed * ts.GetSeconds();
            if (zoom < 0.1f)
                zoom = 0.1f; // clamp minimum zoom
            m_Camera.SetZoom(zoom);
        }
        if (Input::IsKeyPressed(SDLK_E))
        {
            float zoom = m_Camera.GetZoom();
            zoom += zoomSpeed * ts.GetSeconds();
            if (zoom > 5.0f)
                zoom = 5.0f; // clamp maximum zoom
            m_Camera.SetZoom(zoom);
        }

        Renderer::SetViewMatrix(m_Camera.GetViewMatrix());

        angle += 1.0f;
    }

    void RenderingLayer::OnRender()
    {
        // Set viewport to desired area, e.g., center 800x600 in a 1280x720 window:

        Renderer::Clear();

        // Draw your game scene here
        Renderer::DrawQuad(50 + i, 50, 200, 150, SDL_Color{255, 0, 0, 255});
        Renderer::DrawQuad(300, 100, 100, 100, SDL_Color{0, 255, 0, 255});
        Renderer::DrawQuad(450, 200, 150, 150, SDL_Color{0, 0, 255, 0});
        Renderer::DrawQuad(0, 0, 36, 54, SDL_Color{255, 255, 255, 255});
        Renderer::DrawLine(0, 0, 800, 600, SDL_Color{0, 0, 0, 255});

        Renderer::DrawCircle(300, 200, 200, SDL_Color{255, 255, 0, 255});

        auto tex = TextureManager::Get().LoadTexture("player", "player.png", Renderer::GetSDLRenderer());
        Renderer::DrawTexture(tex->GetSDLTexture(), 100,100,tex->GetWidth(), tex->GetHeight(), {255,255,255,255}, angle);

        auto tex2 = TextureManager::Get().GetTexture("circle");
        Renderer::DrawTexture(tex2->GetSDLTexture(), 300,300, tex->GetWidth(), tex->GetWidth(), {0,255,0,255}, 0);
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &ev)
                                                     { return false; });
    }

}

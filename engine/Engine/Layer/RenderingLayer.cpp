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

#include <iostream>

float i = 1.0f;
float angle = 0.0f;
int id = 0;

float x, y;

float frameDuration = 0.1;
bool loop = true;

SIMPEngine::Animation *coinA = nullptr;

namespace SIMPEngine
{
    void RenderingLayer::OnAttach()
    {
        // Set up anything needed before rendering starts
        Renderer::SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        srand(time(NULL));

        int w = 90;
        int h = 90;
        int numFrame = 9;
        std::vector<Sprite> frames;
        auto cointex = TextureManager::Get().GetTexture("walk");
        for (int i = 0; i < numFrame; i++)
        {
            Sprite frame(cointex);
            SDL_FRect temp = {(float)i * w, 0, (float)w, (float)h};
            frame.SetSourceRect(temp);
            frames.push_back(frame);
        }

        coinA = new Animation(cointex, frames, frameDuration, loop);
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
        else if (Input::IsKeyPressed(SDLK_H))
        {
            id += 1;
            if (id > 12)
            {
                id = 0;
                CORE_WARN("I increased {}", id * 16);
            }
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

        angle += 90.0f * ts.GetSeconds();

        id += 2 * ts.GetSeconds();

        coinA->Update(ts.GetSeconds());

        x += 20.0f * ts.GetSeconds();
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
        Renderer::DrawTexture(tex->GetSDLTexture(), 100, 100, tex->GetWidth(), tex->GetHeight(), {255, 255, 255, 255}, angle);

        auto tex2 = TextureManager::Get().GetTexture("circle");
        Renderer::DrawTexture(tex2->GetSDLTexture(), 300, 300, tex->GetWidth(), tex->GetWidth(), {0, 255, 0, 255}, 0);

        auto coinTex = TextureManager::Get().GetTexture("coin");

        coinA->Draw(500 + x, 400 + y, 200, 200, SDL_Color{255, 255, 255, 255}, 0.0f);
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &ev)
                                                     { return false; });
    }

}

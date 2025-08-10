#include "Engine/RenderingLayer.h"

#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/EventDispatcher.h"
#include "Engine/Log.h"

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

    void RenderingLayer::OnUpdate()
    {
        // Clear screen
        Renderer::Clear();

        // Draw some quads
        Renderer::DrawQuad(50, 50, 200, 150, SDL_Color{255, 0, 0, 255});
        Renderer::DrawQuad(300, 100, 100, 100, SDL_Color{0, 255, 0, 255});
        Renderer::DrawQuad(450, 200, 150, 150, SDL_Color{0, 0, 255, 255});

        Renderer::DrawQuad(0, 0, 36, 54, SDL_Color{255, 255, 255, 255});

        // Present happens in Application or Renderer::Present()
    }

    void RenderingLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &ev){
            return false;

        });
    }

}

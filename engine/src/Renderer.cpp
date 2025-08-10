// Renderer.cpp
#include "Engine/Renderer.h"

namespace SIMPEngine {

    RenderingAPI* Renderer::s_RenderingAPI = nullptr;

    void Renderer::Init(RenderingAPI* api, SDL_Renderer* sdlRenderer) {
        s_RenderingAPI = api;
        s_RenderingAPI->Init(sdlRenderer);
    }

    void Renderer::SetClearColor(float r, float g, float b, float a) {
        s_RenderingAPI->SetClearColor(r, g, b, a);
    }

    void Renderer::Clear() {
        s_RenderingAPI->Clear();
    }

    void Renderer::DrawQuad(float x, float y, float width, float height, SDL_Color color) {
        s_RenderingAPI->DrawQuad(x, y, width, height, color);
    }

    void Renderer::Present() {
        s_RenderingAPI->Present();
    }

}
